#!/usr/bin/env python3
"""Confere que todo leitor dos arrays de transformacao materializa antes de ler.

POR QUE ISSO EXISTE: `-statictransformcache=on` adia o laco por vertice de
`BMD::TransformVertices`. A corretude do cache depende de UMA propriedade:

    ninguem le VertexTransform / NormalTransform / IntensityTransform sem que
    EnsureVerticesTransformed() tenha rodado para aquele modelo.

Sao arrays GLOBAIS (ZzzBMD.h:424), compartilhados por todos os modelos -- o motor so
consegue ter os vertices de um modelo materializados por vez. Um leitor novo sem a guarda
nao quebra a compilacao, nao quebra o caminho antigo (com a flag off o laco roda ansioso e
os arrays estao sempre cheios) e NAO aparece em teste de FPS. Ele quebra so com a flag
ligada, so no modelo errado, e provavelmente so em um mapa especifico -- a classe de bug
mais caro de achar.

A auditoria manual de 2026-08-10 passou: 4 leitores externos (PhysicsManager x3,
GM_Raklion, GMNewTown x2) e 8 funcoes em ZzzBMD.cpp, todos com a guarda. Este script existe
para que a proxima adicao nao dependa de alguem lembrar.

Uso:
    python check_transform_cache_readers.py
"""

import os
import re
import sys

ARRAYS = ("VertexTransform", "NormalTransform", "IntensityTransform")
GUARD = "EnsureVerticesTransformed"

# `TransformVertices` PRODUZ os arrays; exigir a guarda nela seria recursao.
# `EnsureVerticesTransformed` e a propria guarda.
PRODUCERS = ("BMD::TransformVertices", "BMD::EnsureVerticesTransformed")

# Declaracoes `extern` e a definicao dos arrays nao sao leitura.
DECLARATION = re.compile(r"^\s*extern\s|^\s*(vec3_t|float)\s+\w+\[MAX_MESH\]")

SOURCE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "source")

FUNCTION_START = re.compile(r"^[A-Za-z_][A-Za-z0-9_:<>\*&\s]*\([^;]*$")


def enclosing_functions(lines):
    """Mapa linha -> assinatura da funcao de nivel zero que a contem.

    Heuristica deliberada: assinatura comecando na coluna 0. Ela erra em macro e em
    definicao dentro de classe, e por isso o script AVISA em vez de reprovar quando nao
    consegue atribuir dono a uma leitura -- um falso positivo silencioso seria pior."""
    starts = []
    for number, line in enumerate(lines, 1):
        if FUNCTION_START.match(line) and not line.lstrip().startswith("//"):
            starts.append((number, line.strip()))
    return starts


def owner_of(starts, number):
    found = None
    for start, signature in starts:
        if start <= number:
            found = (start, signature)
        else:
            break
    return found


def audit(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        lines = handle.read().split("\n")
    starts = enclosing_functions(lines)

    readers = {}
    guarded = set()
    orphans = []
    for number, line in enumerate(lines, 1):
        if DECLARATION.match(line):
            continue
        stripped = line.lstrip()
        if stripped.startswith("//"):
            continue
        if any(("%s[" % name) in line for name in ARRAYS):
            owner = owner_of(starts, number)
            if owner is None:
                orphans.append(number)
            else:
                readers.setdefault(owner, []).append(number)
        if GUARD + "()" in line and "void BMD::" not in line:
            owner = owner_of(starts, number)
            if owner is not None:
                guarded.add(owner)
    return readers, guarded, orphans


def main():
    problems = 0
    warnings = 0
    total_readers = 0

    files = []
    for root, _dirs, names in os.walk(SOURCE_DIR):
        for name in names:
            if name.endswith((".cpp", ".h")):
                files.append(os.path.join(root, name))
    files.sort()

    for path in files:
        readers, guarded, orphans = audit(path)
        if not readers and not orphans:
            continue
        relative = os.path.relpath(path, os.path.join(SOURCE_DIR, ".."))
        print("%s" % relative)
        for owner in sorted(readers, key=lambda item: item[0]):
            start, signature = owner
            count = len(readers[owner])
            total_readers += count
            producer = any(name in signature for name in PRODUCERS)
            if producer:
                status = "OK  (produz)"
            elif owner in guarded:
                status = "OK "
            else:
                status = "FALHA"
                problems += 1
            print("  %-5s linha %-6d %2d leitura(s)  %s"
                  % (status, start, count, signature[:58]))
        for number in orphans:
            warnings += 1
            print("  AVISO linha %-6d leitura fora de funcao reconhecida -- confira a mao"
                  % number)

    print()
    print("%d leitura(s) em %d arquivo(s)." % (total_readers, len(
        [p for p in files if audit(p)[0]])))
    if warnings:
        print("%d aviso(s): o reconhecedor de funcao nao atribuiu dono. Nao e reprovacao, "
              "mas confira." % warnings)
    if problems:
        print()
        print("%d funcao(oes) leem os arrays de transformacao SEM chamar %s()." % (problems, GUARD))
        print("Com -statictransformcache=on elas podem ler dado de OUTRO modelo, ou de "
              "frame anterior. Adicione a chamada antes da primeira leitura.")
        return 2
    print("todo leitor materializa antes de ler: o cache de transformacao esta seguro "
          "por este criterio.")
    print("Isto NAO cobre a ordem entre modelos no mesmo frame -- para isso, "
          "-statictransformcache=compare.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
