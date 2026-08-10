#!/usr/bin/env python3
"""Confere se as UV de chrome do shader batem com as da CPU, sem rodar o jogo.

POR QUE ISSO EXISTE: o caminho de GPU e o legado tem de produzir a MESMA UV, senao a
armadura de todo player fica errada. `-gpuskinning=compare` detecta isso alternando os
dois por frame, mas exige o jogo aberto e olho na tela. Este script cobre a classe de
erro mais provavel -- sinal trocado, fator errado, termo esquecido na traducao -- em um
segundo, e sem depender de ninguem estar disponivel para olhar.

O que ele NAO cobre: estado de GL (blend, depth, textura ligada). Isso continua sendo
trabalho do `compare`.

Os efeitos 1, 2 e 3 entram como CONTROLE: eles estao em producao ha rodadas e sabemos
que fecham. Se o script reprovar um deles, o errado e o script.

Uso:
    python check_chrome_uv.py
"""

import math
import re
import sys

CPU_SOURCE = "../source/ZzzBMD.cpp"
GLSL_SOURCE = "../source/Platform/GlslLegacyRenderAdapter.cpp"


def wave(t):
    """`float wave = static_cast<long>(WorldTime) % 10000 * 0.0001f;` (ZzzBMD.cpp)"""
    return int(t) % 10000 * 0.0001


def wave2(t):
    """`float Wave2 = (int)WorldTime % 5000 * 0.00024f - 0.4f;` (ZzzBMD.cpp)"""
    return int(t) % 5000 * 0.00024 - 0.4


def wave_time(t):
    """`float waveTime = floor(uWorldTime) * 0.0001;` (shader)

    NAO tem o modulo 10000 que a CPU aplica. A diferenca e um INTEIRO, e sob GL_REPEAT
    isso nao muda o pixel amostrado -- e a razao por que o efeito 1 sempre fechou."""
    return math.floor(t) * 0.0001


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


# --- CPU: copiado de ZzzBMD.cpp, cadeia de UV de RenderMesh -------------------
def cpu_effect(effect, n, t):
    L = (math.cos(t * 0.001), math.sin(t * 0.002), 1.0)
    if effect == 1:      # RENDER_CHROME
        return (n[2] * 0.5 + wave(t), n[1] * 0.5 + wave(t) * 2.0)
    if effect == 2:      # RENDER_CHROME2
        return ((n[2] + n[0]) * 0.8 + wave2(t) * 2.0,
                (n[1] + n[0]) * 1.0 + wave2(t) * 3.0)
    if effect == 3:      # RENDER_METAL, cai no `else` da cadeia
        return (n[2] * 0.5 + 0.2, n[1] * 0.5 + 0.5)
    if effect == 5:      # RENDER_CHROME4
        u = dot(n, L)
        v = 1.0 - dot(n, L)
        v -= n[2] * 0.5 + wave(t) * 3.0
        u += n[1] * 0.5 + L[1] * 3.0
        return (u, v)
    if effect == 6:      # RENDER_CHROME6
        c = (n[2] + n[0]) * 0.8 + wave2(t) * 2.0
        return (c, c)
    raise ValueError(effect)


# --- Shader: copiado do vertex shader em GlslLegacyRenderAdapter.cpp ----------
def glsl_effect(effect, n, t):
    if effect == 1:
        return (n[2] * 0.5 + wave_time(t), n[1] * 0.5 + wave_time(t) * 2.0)
    if effect == 2:
        w2 = math.fmod(math.floor(t), 5000.0) * 0.00024 - 0.4
        return ((n[2] + n[0]) * 0.8 + w2 * 2.0, (n[1] + n[0]) + w2 * 3.0)
    if effect == 3:
        return (n[2] * 0.5 + 0.2, n[1] * 0.5 + 0.5)
    if effect == 5:
        L = (math.cos(t * 0.001), math.sin(t * 0.002), 1.0)
        d = dot(n, L)
        return (d + n[1] * 0.5 + L[1] * 3.0,
                (1.0 - d) - (n[2] * 0.5 + wave_time(t) * 3.0))
    if effect == 6:
        w6 = math.fmod(math.floor(t), 5000.0) * 0.00024 - 0.4
        c = (n[2] + n[0]) * 0.8 + w6 * 2.0
        return (c, c)
    raise ValueError(effect)


def normals():
    """Varredura de normais unitarias plausiveis, mais os eixos e casos degenerados."""
    out = [(1, 0, 0), (0, 1, 0), (0, 0, 1), (-1, 0, 0), (0, -1, 0), (0, 0, -1)]
    for i in range(9):
        theta = i * math.pi / 8
        for j in range(9):
            phi = j * math.pi / 4
            out.append((math.sin(theta) * math.cos(phi),
                        math.sin(theta) * math.sin(phi),
                        math.cos(theta)))
    return out


def times():
    """WorldTime em ms. Inclui o instante da virada do modulo 10000, que e onde a
    diferenca entre `wave` e `waveTime` fica maxima."""
    return [0.0, 1.0, 137.0, 4999.0, 5000.0, 9999.0, 10000.0, 10001.0,
            123456.0, 3600000.0]


def source_declares(path, needle):
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            return needle in fh.read()
    except OSError:
        return None


def main():
    # Guarda contra a armadilha obvia: um script que so compara as proprias copias das
    # formulas nao prova nada se o arquivo tiver mudado. Isto nao e verificacao formal,
    # e um alarme -- se a linha do shader deixar de existir, a copia aqui esta velha.
    marcadores = [
        (GLSL_SOURCE, "materialEffect == 5"),
        (GLSL_SOURCE, "materialEffect == 6"),
        (CPU_SOURCE, "RENDER_CHROME4) == RENDER_CHROME4"),
        (CPU_SOURCE, "% 10000 * 0.0001f"),
    ]
    for path, needle in marcadores:
        found = source_declares(path, needle)
        if found is None:
            print("AVISO: nao consegui abrir %s -- rode de dentro de diagnostic/" % path)
        elif not found:
            print("AVISO: '%s' nao aparece em %s. A copia deste script pode estar velha."
                  % (needle, path))

    # Tolerancia frouxa de proposito: a GPU calcula em float32 e a CPU em float32 com
    # ordem de operacoes diferente. Erro de traducao produz divergencia de ordem 0.1+,
    # nao de 1e-5.
    tolerance = 1e-4
    problems = 0
    for effect, label, modular in ((1, "CHROME  (controle)", True),
                                   (2, "CHROME2 (controle)", True),
                                   (3, "METAL   (controle)", False),
                                   (5, "CHROME4", True),
                                   (6, "CHROME6", False)):
        worst = 0.0
        worst_case = None
        for n in normals():
            for t in times():
                c = cpu_effect(effect, n, t)
                g = glsl_effect(effect, n, t)
                for axis in (0, 1):
                    delta = abs(c[axis] - g[axis])
                    if modular:
                        # GL_REPEAT amostra modulo 1.0: diferenca inteira e invisivel.
                        # E a unica folga que este script concede, e ela e a razao de
                        # `waveTime` poder ignorar o modulo 10000 da CPU.
                        delta = abs(delta - round(delta))
                    if delta > worst:
                        worst = delta
                        worst_case = (n, t, c, g)
        status = "OK " if worst <= tolerance else "FALHA"
        if worst > tolerance:
            problems += 1
        print("%s  efeito %d  %-18s  pior divergencia %.2e%s"
              % (status, effect, label, worst,
                 "   (modulo 1.0)" if modular else ""))
        if worst > tolerance and worst_case is not None:
            n, t, c, g = worst_case
            print("        normal (%.3f, %.3f, %.3f)  t=%.0f" % (n[0], n[1], n[2], t))
            print("        cpu  (%.6f, %.6f)" % c)
            print("        glsl (%.6f, %.6f)" % g)

    print()
    if problems:
        print("%d efeito(s) divergem. Corrija a traducao ANTES de medir FPS: uma UV "
              "errada aparece em toda armadura da multidao." % problems)
        return 2
    print("as UV do shader batem com as da CPU dentro de %.0e." % tolerance)
    print("Isto NAO cobre estado de GL (blend, depth, textura). Para isso, "
          "-gpuskinning=compare.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
