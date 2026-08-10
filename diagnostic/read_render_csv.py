#!/usr/bin/env python3
"""Le um RenderPerformance_v*.csv e imprime a reparticao do frame.

O CSV tem ~117 colunas. Ler isso a mao e como ler o frame a olho nu -- foi assim
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
    python read_render_csv.py Client/RenderPerformance_v17.csv
    python read_render_csv.py Client/RenderPerformance_v17.csv --world 0

Le tambem os arquivos das geracoes anteriores: coluna ausente vira zero e a secao
correspondente nao e impressa.
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

# Quarto nivel: us_char_transform aninha dentro de us_char_parts, que ja e detalhe de
# us_characters. NAO entra em DETAIL -- lá ele quebraria a soma das filhas com o pai.
# Ele existe porque us_char_parts, sendo subtracao, apontava o bloco dominante da
# multidao sem dizer o que tinha dentro.
SUBDETAIL = {"us_char_parts": ["us_char_mesh", "us_char_link", "us_char_transform"]}

# Aninhamento REAL, e nao o que o nome sugere. A captura de 2026-08-10 saiu com
# us_char_draw em 147% de us_char_mesh porque RenderPartObjectEffect tem dois
# chamadores e so um esta dentro de RenderPartObject. A afirmacao de aninhamento
# estava num comentario e nao era verificada -- agora e.
NESTING = {
    "us_char_mesh": "us_char_parts",
    "us_char_link": "us_char_parts",
    "us_char_transform": "us_char_parts",
    "us_char_draw": "us_char_mesh",
    "us_char_link_draw": "us_char_link",
}

# Toda coluna que MUDA o resultado tem que estar aqui. crowd_spawn entra porque uma
# captura com multidao sintetica e outra sem sao regimes diferentes: misturar as duas
# na mesma media produz um numero que nao corresponde a nenhuma execucao.
FLAGS = ["backend", "gpu_skinning", "instancing", "transform_cache", "batching",
         "mesh_cache", "cpu_matrices", "render_scale", "vsync", "fps_limit",
         "wheel_trail_cap", "crowd_lod", "crowd_spawn", "crowd_spawn_monsters",
         "crowd_spawn_npcs", "crowd_max_full"]

# Composicao da multidao. Player, monstro e NPC nao custam o mesmo -- o player tem
# ~15 malhas de equipamento e sombra propria, o monstro tem uma malha de corpo --
# entao um mesmo chars_visible com composicoes diferentes e carga diferente.
KINDS = ["chars_players_avg", "chars_monsters_avg", "chars_npcs_avg",
         "chars_other_avg"]


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

    # Identidades dos contadores de multidao. As duas sao exatas por construcao
    # (CrowdLod::CountCharacter incrementa visible OU culled, e sempre um nivel), o
    # que as torna uteis: se uma delas nao fechar, o laco de personagens contou o
    # mesmo slot duas vezes ou deixou de contar -- e a distribuicao de LOD nao serve.
    if any(r.get("chars_live_avg") for r in rows):
        live = mean([read_num(r, "chars_live_avg") for r in rows])
        visible = mean([read_num(r, "chars_visible_avg") for r in rows])
        culled = mean([read_num(r, "chars_culled_frustum_avg") for r in rows])
        if live > 0 and abs((visible + culled) - live) / live > 0.02:
            problems.append(
                "chars_visible + chars_culled_frustum (%.1f) difere de chars_live "
                "(%.1f): o laco de personagens nao contou cada slot uma vez"
                % (visible + culled, live))
        levels = sum(mean([read_num(r, "chars_lod%d_avg" % i) for r in rows])
                     for i in range(4))
        if live > 0 and abs(levels - live) / live > 0.02:
            problems.append(
                "chars_lod0..3 somam %.1f, mas chars_live e %.1f: a distribuicao de "
                "LOD nao cobre a multidao inteira" % (levels, live))
        if any(r.get(KINDS[0]) for r in rows):
            kinds = sum(mean([read_num(r, c) for r in rows]) for c in KINDS)
            if live > 0 and abs(kinds - live) / live > 0.02:
                problems.append(
                    "player+monstro+NPC+outro somam %.1f, mas chars_live e %.1f: a "
                    "composicao nao cobre a multidao inteira" % (kinds, live))

    # Cada coluna cronometrada tem de caber no pai DECLARADO em NESTING. Passar do pai
    # significa que o intervalo esta sendo contado em dois lugares -- e foi assim que o
    # us_char_draw de 2026-08-10 se denunciou (147% de us_char_mesh). Estas colunas nao
    # se somam entre si: mesh contem draw, e contem transform no regime diferido.
    for child, parent in NESTING.items():
        if not any(r.get(child) for r in rows):
            continue
        parent_total = mean([read_num(r, parent) for r in rows])
        child_total = mean([read_num(r, child) for r in rows])
        if parent_total > 0.5 and child_total > parent_total * 1.02:
            problems.append(
                "%s (%.0f us) excede %s (%.0f us): o tempo esta contado duas vezes"
                % (child, child_total, parent, parent_total))

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
    # Multidao. O custo por personagem so faz sentido dividido pelos VISIVEIS: os
    # fora do frustum nao desenham nada e diluiriam a conta.
    if any(r.get("chars_live_avg") for r in rows):
        live = mean([read_num(r, "chars_live_avg") for r in rows])
        visible = mean([read_num(r, "chars_visible_avg") for r in rows])
        visible_max = mean([read_num(r, "chars_visible_max") for r in rows])
        culled = mean([read_num(r, "chars_culled_frustum_avg") for r in rows])
        beyond = mean([read_num(r, "chars_beyond_far_avg") for r in rows])
        forced = mean([read_num(r, "chars_lod_forced_avg") for r in rows])
        spawn = mean([read_num(r, "crowd_spawn") for r in rows])
        spawn_mon = mean([read_num(r, "crowd_spawn_monsters") for r in rows])
        spawn_npc = mean([read_num(r, "crowd_spawn_npcs") for r in rows])
        pedido = []
        if spawn > 0:     pedido.append("player=%d" % int(spawn))
        if spawn_mon > 0: pedido.append("monstro=%d" % int(spawn_mon))
        if spawn_npc > 0: pedido.append("npc=%d" % int(spawn_npc))
        print()
        print("MULTIDAO   vivos %.1f   visiveis %.1f (pico %.0f)   fora do frustum %.1f%s"
              % (live, visible, visible_max, culled,
                 "   [RIG SINTETICO: %s]" % ", ".join(pedido) if pedido else ""))
        if any(r.get(KINDS[0]) for r in rows):
            players, monsters, npcs, other = [mean([read_num(r, c) for r in rows])
                                              for c in KINDS]
            print("  composicao: %.1f player   %.1f monstro   %.1f NPC   %.1f outro"
                  % (players, monsters, npcs, other))
            # Pedir e conseguir sao coisas diferentes: sem monstro vivo no mapa o rig
            # nao tem tipo para clonar e nao cria nada. Sem este aviso, a coluna de
            # pedido pareceria a carga aplicada.
            for nome, pedidoN, real in (("monstro", spawn_mon, monsters),
                                        ("NPC", spawn_npc, npcs)):
                if pedidoN > 0 and real < pedidoN * 0.5:
                    print("  AVISO: pediu %d %s e o mapa tem %.1f -- o rig clona os tipos "
                          "presentes; sem nenhum vivo, nao cria" % (int(pedidoN), nome, real))
        if beyond > 0.05:
            print("  %.1f personagem(ns) alem de CameraViewFar AINDA sao desenhados: o "
                  "teste de visibilidade e 2D e nao tem plano far" % beyond)
        if forced > 0.05:
            print("  %.1f isento(s) de LOD (heroi, alvo, party, mapa PvP)" % forced)
        levels = [mean([read_num(r, "chars_lod%d_avg" % i) for r in rows]) for i in range(4)]
        if visible > 0:
            print("  niveis: L0 %.1f   L1 %.1f   L2 %.1f   L3 %.1f" % tuple(levels))
            if levels[0] >= live - 0.05:
                print("  todos em L0: nenhum corte de LOD teria efeito nesta captura")
        poses = mean([read_num(r, "char_poses_avg") for r in rows])
        meshes = mean([read_num(r, "char_part_meshes_avg") for r in rows])
        shadows = mean([read_num(r, "char_shadows_avg") for r in rows])
        characters_us = mean([read_num(r, "us_characters") for r in rows])
        print("  por frame: %.1f poses   %.1f malhas   %.1f sombras" % (poses, meshes, shadows))
        if visible > 0:
            print("  por visivel: %.1f malhas   %.1f us de us_characters"
                  % (meshes / visible, characters_us / visible))

        # Coletor de instancias: a pergunta e se REORDENAR a emissao renderia lote maior,
        # ou se as malhas reordenaveis vem uma-a-uma e nao ha lote possivel. A regra de
        # decisao fica aqui, e nao na cabeca de quem le a planilha.
        if any(r.get("char_batch_breaks_avg") for r in rows):
            accum = mean([read_num(r, "char_batch_accum_avg") for r in rows])
            breaks = mean([read_num(r, "char_batch_breaks_avg") for r in rows])
            run_max = mean([read_num(r, "char_batch_run_max") for r in rows])
            run = accum / breaks if breaks > 0 else 0.0
            print("  coletor de instancias: %.1f acumuladas, %.1f quebras"
                  " -> sequencia media %.2f, pico %.0f" % (accum, breaks, run, run_max))
            instancing = rows[0].get("instancing", "?")
            if instancing != "on":
                print("    (instancing=%s: nada acumula por construcao, medida invalida)"
                      % instancing)
            elif accum < 1.0:
                print("    quase nada e reordenavel -> o problema e MATERIAL, nao ordem de"
                      " emissao")
            elif run < 3.0:
                print("    sequencia curta -> a ordem de emissao e o problema: malha")
                print("    reordenavel vem intercalada com compositora e o balde nunca cresce")
            else:
                print("    sequencia longa -> o balde ja cresce sozinho; o gargalo esta em"
                      " outro lugar")
            # A politica de reordenacao NAO precisa ser inventada: RenderQueue::Execute
            # (Platform/RenderPipeline.cpp:103) ja ordena os opacos por shader/blend/depth/
            # textura com stable_sort e funde os adjacentes com PodeFundir, preservando a
            # ordem de emissao dos TRANSPARENTES via `sequence` -- que e exatamente a
            # ressalva de blend. O caminho de malha estatica (DrawStaticMesh) nao tem essa
            # politica; dar a mesma a ele e o conserto, e nao um mecanismo novo.
            if breaks > 0 and run < 3.0:
                print("    ver RenderPipeline.cpp:103 -- a fila de opacos ja ordena e funde"
                      " com a politica certa")
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
            # Um nivel mais fundo. `us_char_mesh` e o tempo dentro de RenderPartObject
            # e CONTEM o transform quando ele e diferido; o que sobra do pai sao os
            # extras por personagem. Sem essa divisao o proximo corte seria palpite.
            children = SUBDETAIL.get(name) or []
            present = [c for c in children if any(r.get(c) for r in rows)]
            if present:
                meshes = mean([read_num(r, "char_part_meshes_avg") for r in rows])
                chars = mean([read_num(r, "chars_visible_avg") for r in rows])
                for child in present:
                    child_value = mean([read_num(r, child) for r in rows])
                    extra = ""
                    if child == "us_char_mesh" and meshes > 0.5:
                        extra = "   (%.0f us por malha)" % (child_value / meshes)
                    elif child in ("us_char_link",) and chars > 0.5:
                        extra = "   (%.0f us por personagem)" % (child_value / chars)
                    print("    %-18s %8.0f us  %5.1f%% de %s%s"
                          % (child, child_value,
                             100 * child_value / value if value else 0, name, extra))
                    # Dentro de RenderLinkObject: desenho da arma contra o preparo
                    # (PlayAnimation + Animation + Transform da matriz de osso).
                    if child == "us_char_link" and any(r.get("us_char_link_draw") for r in rows):
                        ld = mean([read_num(r, "us_char_link_draw") for r in rows])
                        print("      %-16s %8.0f us  %5.1f%% de us_char_link"
                              % ("us_char_link_draw", ld,
                                 100 * ld / child_value if child_value else 0))
                        print("      %-16s %8.0f us  %5.1f%% de us_char_link"
                              % ("preparo (ossos)", child_value - ld,
                                 100 * (child_value - ld) / child_value if child_value else 0))
                    # Dentro de RenderPartObject: desenho contra o setup em volta.
                    if child == "us_char_mesh" and any(r.get("us_char_draw") for r in rows):
                        draw = mean([read_num(r, "us_char_draw") for r in rows])
                        print("      %-16s %8.0f us  %5.1f%% de us_char_mesh"
                              % ("us_char_draw", draw,
                                 100 * draw / child_value if child_value else 0))
                        print("      %-16s %8.0f us  %5.1f%% de us_char_mesh"
                              % ("setup+ossos", child_value - draw,
                                 100 * (child_value - draw) / child_value if child_value else 0))
                # O resto do bloco, por subtracao das duas irmas medidas. Agora que
                # RenderLinkObject tem coluna, o que sobra aqui e de fato o miudo por
                # personagem: nome, barra de vida, marca de guild, pet, ganchos de mapa.
                measured = sum(mean([read_num(r, c) for r in rows])
                               for c in ("us_char_mesh", "us_char_link") if c in present)
                if measured > 0:
                    rest = value - measured
                    print("    %-18s %8.0f us  %5.1f%% de %s%s"
                          % ("miudo por char", rest,
                             100 * rest / value if value else 0, name,
                             "   (%.0f us por personagem)" % (rest / chars) if chars > 0.5 else ""))
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
