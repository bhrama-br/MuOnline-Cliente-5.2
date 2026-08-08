#!/usr/bin/env python3
"""Le um RenderPerformance_v*.csv e imprime a reparticao do frame.

O CSV tem ~101 colunas. Ler isso a mao e como ler o frame a olho nu -- foi assim
que sete rodadas de otimizacao foram gastas dentro de 24% do frame. Esta
ferramenta faz tres coisas que a planilha nao faz sozinha:

1. Agrupa por (mundo, cena, configuracao de flags) e tira a media das amostras. A
   configuracao INTEIRA entra na chave: uma flag de fora faz capturas de regimes
   diferentes cairem na mesma media, e o proprio valor da flag vira uma media sem
   sentido.
2. VALIDA as identidades do CSV antes de imprimir. Se `cpu_us` nao for a soma das
   suas fases, ou o frame nao fechar, a reparticao esta errada e qualquer conclusao
   tirada dela tambem.
3. Mostra os PICOS, nao so as medias. Um mergulho de 1 segundo (a Twisting Slash
   derrubando o FPS) e diluido ate sumir numa media de 120 frames.

Uso:
    python read_render_csv.py Client/RenderPerformance_v16.csv
    python read_render_csv.py Client/RenderPerformance_v16.csv --world 0
"""

import argparse
import csv
import sys
from collections import defaultdict

# Fases que somam cpu_us. Uma coluna nova esquecida aqui aparece como divergencia,
# que e o comportamento certo: falha ruidosa, nao silenciosa.
CPU_PHASES = [
    "us_terrain", "us_objects", "us_characters", "us_effects", "us_sprites",
    "us_simulation", "us_select", "us_setup_gl", "us_frustum", "us_misc",
    "us_water", "us_ui", "us_framebegin", "us_unmeasured",
]

# Fases que, somadas a cpu_us, dao frame_total_us.
OUTSIDE_PHASES = ["us_overlay", "us_present", "us_protocol", "us_pump",
                  "us_limiter", "us_frame_gap"]

# Detalhe: ANINHAM dentro da coluna-pai e nao entram em soma nenhuma.
DETAIL = {
    "us_characters": ["us_char_pose", "us_char_shadow", "us_char_parts"],
    "us_simulation": ["us_sim_ui", "us_sim_objects", "us_sim_chars",
                      "us_sim_effects", "us_sim_rest"],
}

# Toda coluna que MUDA o resultado tem que estar aqui.
FLAGS = ["backend", "gpu_skinning", "instancing", "transform_cache", "batching",
         "mesh_cache", "cpu_matrices", "render_scale", "vsync", "fps_limit",
         "wheel_trail_cap"]


def read_num(row, column, default=0.0):
    """Le uma coluna numerica. Ausente vira o default: um CSV de geracao anterior
    nao tem as colunas novas, e recusar o arquivo inteiro por isso seria inutil."""
    value = row.get(column)
    if value is None or value == "":
        return default
    try:
        return float(value)
    except ValueError:
        return default


def config_key(row):
    return tuple(row.get(f, "?") for f in FLAGS)


def mean(values):
    return sum(values) / len(values) if values else 0.0


def validate(rows):
    """Confere as identidades do CSV. Retorna lista de problemas."""
    problems = []

    phase_sum = sum(mean([read_num(r, c) for r in rows]) for c in CPU_PHASES)
    cpu = mean([read_num(r, "cpu_us_avg") for r in rows])
    if cpu > 0:
        error = abs(phase_sum - cpu) / cpu
        if error > 0.02:
            problems.append(
                "soma das fases (%.0f us) difere de cpu_us (%.0f us) em %.1f%% "
                "-- a reparticao de cpu_us nao fecha" % (phase_sum, cpu, error * 100))

    frame = mean([read_num(r, "frame_total_us") for r in rows])
    outside_sum = sum(mean([read_num(r, c) for r in rows]) for c in OUTSIDE_PHASES)
    if frame > 0 and any(r.get("us_present") for r in rows):
        error = abs((cpu + outside_sum) - frame) / frame
        if error > 0.02:
            problems.append(
                "cpu_us + fases externas (%.0f us) difere de frame_total_us (%.0f us) "
                "em %.1f%%" % (cpu + outside_sum, frame, error * 100))

    for parent, children in DETAIL.items():
        if not any(r.get(children[0]) for r in rows):
            continue
        parent_total = mean([read_num(r, parent) for r in rows])
        child_sum = sum(mean([read_num(r, c) for r in rows]) for c in children)
        if parent_total > 0 and abs(child_sum - parent_total) / parent_total > 0.02:
            problems.append(
                "detalhe de %s nao fecha: filhas somam %.0f us, pai tem %.0f us"
                % (parent, child_sum, parent_total))

    nesting = mean([read_num(r, "phase_nesting") for r in rows])
    if nesting > 0.005:
        problems.append(
            "phase_nesting = %.2f por frame: DUAS FASES SE ANINHARAM. A reparticao "
            "de cpu_us conta tempo duas vezes e NAO e confiavel." % nesting)

    return problems


def notes(rows):
    """Observacoes que NAO sao falha de integridade. A divergencia fps/fps_period e
    esperada -- e o motivo da segunda coluna existir. Tratar isso como erro faria a
    ferramenta reprovar toda captura, e um aviso que sempre aparece nao e lido."""
    out = []
    fps = mean([read_num(r, "fps") for r in rows])
    fps_period = mean([read_num(r, "fps_period") for r in rows])
    if fps_period > 0 and fps > 0 and (fps - fps_period) / fps_period > 0.03:
        out.append("a coluna fps (%.1f) supera fps_period (%.1f) em %.1f%% -- "
                   "esperado, compare por fps_period"
                   % (fps, fps_period, (fps - fps_period) / fps_period * 100))
    return out


def print_group(config, rows):
    world = rows[0].get("world", "?")
    scene = rows[0].get("scene", "?")
    frame = mean([read_num(r, "frame_total_us") for r in rows])
    cpu = mean([read_num(r, "cpu_us_avg") for r in rows])
    fps_period = mean([read_num(r, "fps_period") for r in rows])
    fps_column = mean([read_num(r, "fps") for r in rows])

    print("=" * 78)
    print("mundo %s  cena %s  |  %d amostra(s) de 120 frames" % (world, scene, len(rows)))
    print("flags: " + "  ".join("%s=%s" % (f, v) for f, v in zip(FLAGS, config)))
    print("-" * 78)
    print("frame_total  %8.0f us" % frame, end="")
    if fps_period > 0:
        print("   fps_period %6.1f   (coluna fps: %.1f)" % (fps_period, fps_column))
    else:
        print("   fps %6.1f  (sem fps_period: CSV de geracao anterior)" % fps_column)
    print("cpu_us       %8.0f us   %5.1f%% do frame" % (cpu, 100 * cpu / frame if frame else 0))

    # Picos. Media de 120 frames nao mostra mergulho transitorio, e mergulho
    # transitorio e o que o jogador relata.
    frame_max = mean([read_num(r, "frame_total_us_max") for r in rows])
    fps_worst = mean([read_num(r, "fps_min") for r in rows])
    if frame_max > 0:
        print()
        print("PIOR FRAME  %8.0f us   fps_min %6.1f   (%.1fx o frame medio)"
              % (frame_max, fps_worst, frame_max / frame if frame else 0))
        for column, label in (("us_present_max", "present"), ("us_effects_max", "effects")):
            value = mean([read_num(r, column) for r in rows])
            if value > 0:
                print("  pico de %-8s %8.0f us" % (label, value))
    alive = mean([read_num(r, "effects_live_avg") for r in rows])
    alive_max = mean([read_num(r, "effects_live_max") for r in rows])
    if alive_max > 0:
        print("  efeitos vivos: media %.1f, pico %.0f de 200 slots%s"
              % (alive, alive_max, "  <-- POOL SATURADO" if alive_max >= 195 else ""))
    # Rastros da Twisting Slash: cada um custa um modelo de ARMA animado por frame,
    # nao um sprite. Por isso tem contador proprio.
    trails = mean([read_num(r, "wheel_trails_avg") for r in rows])
    trails_max = mean([read_num(r, "wheel_trails_max") for r in rows])
    cap = mean([read_num(r, "wheel_trail_cap", -1) for r in rows])
    if trails_max > 0 or cap >= 0:
        print("  rastros Twisting Slash: media %.1f, pico %.0f%s"
              % (trails, trails_max,
                 "   (-wheeltrail=%d ATIVO)" % int(cap) if cap >= 0 else ""))
    print()

    blocks = [(c, mean([read_num(r, c) for r in rows])) for c in CPU_PHASES + OUTSIDE_PHASES]
    blocks = [(c, v) for c, v in blocks if v > 0.5]
    blocks.sort(key=lambda x: -x[1])

    print("%-20s %10s %8s   %s" % ("bloco", "us/frame", "% frame", ""))
    for name, value in blocks:
        pct = 100 * value / frame if frame else 0
        bar = "#" * min(int(pct / 2), 50)
        print("%-20s %10.0f %7.1f%%   %s" % (name, value, pct, bar))

    for parent, children in DETAIL.items():
        if not any(r.get(children[0]) for r in rows):
            continue
        parent_total = mean([read_num(r, parent) for r in rows])
        if parent_total < 0.5:
            continue
        print()
        print("  detalhe de %s (aninhado, nao soma no frame):" % parent)
        detail_rows = sorted(((c, mean([read_num(r, c) for r in rows])) for c in children),
                             key=lambda x: -x[1])
        for name, value in detail_rows:
            pct = 100 * value / parent_total if parent_total else 0
            print("  %-20s %8.0f us  %5.1f%% de %s" % (name, value, pct, parent))
    print()


def main():
    ap = argparse.ArgumentParser(description="Le a reparticao do frame de um RenderPerformance CSV")
    ap.add_argument("csv", help="caminho do RenderPerformance_v*.csv")
    ap.add_argument("--world", help="filtra por world")
    args = ap.parse_args()

    try:
        with open(args.csv, newline="", encoding="utf-8", errors="replace") as fh:
            raw_rows = [line.rstrip("\r\n") for line in fh if line.strip()]
    except OSError as error:
        print("nao consegui abrir %s: %s" % (args.csv, error), file=sys.stderr)
        return 1

    if not raw_rows:
        print("CSV vazio", file=sys.stderr)
        return 1

    # Checagem de largura ANTES de qualquer leitura. Um arquivo com linhas de
    # larguras diferentes foi escrito por builds com conjuntos de colunas
    # diferentes; csv.DictReader nao reclama, joga o excedente numa chave None e
    # desalinha tudo em silencio. Foi assim que o v15 passou a reportar vsync=232.
    widths = {}
    for i, line in enumerate(raw_rows):
        widths.setdefault(line.count(",") + 1, []).append(i + 1)
    if len(widths) > 1:
        print("ERRO: o arquivo mistura linhas de larguras diferentes.", file=sys.stderr)
        for width in sorted(widths):
            nums = widths[width]
            sample = ", ".join(str(n) for n in nums[:6]) + ("..." if len(nums) > 6 else "")
            print("  %3d campos: %d linha(s) (linhas %s)" % (width, len(nums), sample),
                  file=sys.stderr)
        print("", file=sys.stderr)
        print("Causa: builds com conjuntos de colunas diferentes gravaram no mesmo",
              file=sys.stderr)
        print("arquivo. Nenhuma leitura desse arquivo e valida -- as colunas estao",
              file=sys.stderr)
        print("deslocadas. O build atual rotaciona o arquivo quando o header difere,",
              file=sys.stderr)
        print("entao isso nao volta a acontecer. Apague este arquivo e capture de novo.",
              file=sys.stderr)
        return 2

    rows = list(csv.DictReader(raw_rows))
    if not rows:
        print("CSV so tem header, nenhuma amostra", file=sys.stderr)
        return 1

    if args.world:
        rows = [r for r in rows if r.get("world") == args.world]
        if not rows:
            print("nenhuma amostra para world=%s" % args.world, file=sys.stderr)
            return 1

    print("%s: %d amostras, %d colunas" % (args.csv, len(rows), len(rows[0])))
    print()

    groups = defaultdict(list)
    for row in rows:
        groups[(row.get("world"), row.get("scene")) + config_key(row)].append(row)

    total_problems = 0
    for key in sorted(groups, key=lambda k: -mean([read_num(r, "frame_total_us") for r in groups[k]])):
        group_rows = groups[key]
        # Valida ANTES de imprimir. Mostrar a reparticao primeiro e a ressalva depois
        # convida a ler o numero e ignorar o aviso -- o habito que custou sete rodadas.
        problems = validate(group_rows)
        if problems:
            total_problems += len(problems)
            print("=" * 78)
            print("mundo %s cena %s: INSTRUMENTO NAO FECHA A CONTA"
                  % (group_rows[0].get("world", "?"), group_rows[0].get("scene", "?")))
            for p in problems:
                print("   - %s" % p)
            print()
        print_group(key[2:], group_rows)
        for n in notes(group_rows):
            print("  nota: %s" % n)
            print()

    if total_problems:
        print("=" * 78)
        print("%d problema(s) de integridade. Corrija o instrumento ANTES de tirar "
              "conclusao -- este projeto ja gastou sete rodadas confiando em numero "
              "que nao fechava." % total_problems)
        return 2

    print("=" * 78)
    print("todas as identidades do CSV fecham.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
