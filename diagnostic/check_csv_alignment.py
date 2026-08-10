#!/usr/bin/env python3
"""Confere que header, formato e argumentos do CSV de render tem a mesma largura.

POR QUE ISSO EXISTE: `CaptureRenderStatsCsv` escreve a linha com um `fprintf` de mais de
120 especificadores, e o header e uma constante SEPARADA. Se as duas larguras divergirem, o
arquivo sai com nome de coluna em cima de valor de outra -- e nenhum leitor de CSV reclama.
Foi assim que a geracao v15 se corrompeu: `vsync` lia 232 e `fps_limit` lia 714.

Pior que a divergencia header/formato e a divergencia formato/argumentos: passar menos
argumentos do que o formato pede e comportamento INDEFINIDO em C. Com /W0 (o nivel deste
projeto) o compilador nao avisa.

Esta checagem foi rodada a mao seis vezes em 2026-08-10, uma por coluna acrescentada, e
pegou dois desalinhamentos antes de virarem captura. Automatizar remove a dependencia de
alguem lembrar.

Uso:
    python check_csv_alignment.py
"""

import os
import re
import sys

SOURCE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "..", "source", "ZzzScene.cpp")

# `resolution` e impressa como "%dx%d": uma coluna, dois argumentos. Qualquer campo do
# formato com mais de um especificador precisa estar aqui, senao a conta nao fecha.
MULTI_SPEC_COLUMNS = {"resolution": 2}

# Campos literais do formato, sem especificador. `frames` e escrito como 120 fixo.
LITERAL_FIELDS = {"120"}

SPEC = re.compile(r"%[-#0-9.l]*[a-zA-Z]")
C_NEWLINE = chr(92) + "n"


def read_source():
    with open(SOURCE, encoding="utf-8", errors="replace") as handle:
        return handle.read()


def string_literal_parts(text):
    """Concatena os pedacos de uma string literal C partida em varias linhas."""
    return "".join(re.findall(r'"(.*?)"', text, re.S))


def extract_header(source):
    match = re.search(r"kRenderCsvHeader\s*=\s*(.*?);", source, re.S)
    if match is None:
        return None
    header = string_literal_parts(match.group(1))
    if header.endswith(C_NEWLINE):
        header = header[: -len(C_NEWLINE)]
    return header.split(",")


def extract_format_and_args(source):
    start = source.find('fprintf(file, "')
    if start < 0:
        return None, None
    depth = 0
    index = start + len("fprintf")
    while index < len(source):
        char = source[index]
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                break
        index += 1
    call = source[start : index + 1]

    match = re.match(r'fprintf\(file,\s*(".*?")\s*,\s*', call, re.S)
    if match is None:
        return None, None
    fmt = string_literal_parts(match.group(1))
    if fmt.endswith(C_NEWLINE):
        fmt = fmt[: -len(C_NEWLINE)]

    # Argumentos no nivel zero de parenteses/colchetes. Comentario de linha e removido
    # antes: ha `//` dentro da lista.
    body = re.sub(r"//.*", "", call[match.end() : -1])
    depth = 0
    args = []
    current = ""
    for char in body:
        if char in "([":
            depth += 1
        elif char in ")]":
            depth -= 1
        if char == "," and depth == 0:
            args.append(current.strip())
            current = ""
        else:
            current += char
    if current.strip():
        args.append(current.strip())
    return fmt, args


def main():
    source = read_source()

    columns = extract_header(source)
    fmt, args = extract_format_and_args(source)
    if columns is None or fmt is None:
        print("nao encontrei kRenderCsvHeader ou o fprintf em ZzzScene.cpp.")
        print("Se o codigo foi reorganizado, esta checagem precisa acompanhar.")
        return 2

    fields = fmt.split(",")

    problems = []
    if len(columns) != len(fields):
        problems.append("header tem %d coluna(s) e o formato tem %d campo(s)"
                        % (len(columns), len(fields)))

    # Especificadores esperados: um por campo, mais o extra das colunas multi-spec,
    # menos os campos literais.
    expected_args = 0
    for column, field in zip(columns, fields):
        if field in LITERAL_FIELDS:
            continue
        expected_args += MULTI_SPEC_COLUMNS.get(column, 1)
    if len(fields) != len(columns):
        expected_args = None   # sem alinhamento nao ha o que esperar

    if expected_args is not None and len(args) != expected_args:
        problems.append("o formato pede %d argumento(s) e o fprintf passa %d"
                        % (expected_args, len(args)))

    # Campo sem especificador que nao esta na lista de literais: quase sempre virgula
    # esquecida ou coluna colada.
    for column, field in zip(columns, fields):
        if "%" not in field and field not in LITERAL_FIELDS:
            problems.append("campo do formato para a coluna '%s' nao tem especificador: %r"
                            % (column, field))

    print("header      %3d colunas" % len(columns))
    print("formato     %3d campos, %d especificador(es)" % (len(fields), len(SPEC.findall(fmt))))
    print("argumentos  %3d" % len(args))
    if expected_args is not None:
        print("esperados   %3d  (campos - literais + multi-spec)" % expected_args)
    print()

    if problems:
        for problem in problems:
            print("FALHA: %s" % problem)
        print()
        print("Corrija ANTES de capturar. Largura divergente escreve valor sob nome de outra")
        print("coluna e nenhum leitor de CSV reclama -- foi assim que a v15 se corrompeu.")
        print("Argumento faltando e comportamento indefinido, e com /W0 nao ha aviso.")
        return 2

    print("header, formato e argumentos alinhados.")
    print("Isto NAO confere se cada argumento e a coluna certa -- so a largura. Para a")
    print("ordem, leia as ultimas linhas do fprintf contra o fim do header.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
