"""Ponte WebSocket <-> TCP para o cliente Web.

POR QUE E NECESSARIA: o navegador nao abre socket TCP cru. O Emscripten traduz
sockets POSIX em WebSocket, e o servidor de MU fala TCP puro -- entao sem um
tradutor no meio o `connect()` do cliente sempre falha ("Connect Error").

COMO O EMSCRIPTEN ESCOLHE O DESTINO: quando `Module.websocket.url` fica no padrao
('ws://'), ele o trata como PREFIXO e monta `ws://<host>:<porta>` a partir do
endereco passado ao connect(). Ou seja, a ponte tem de escutar no MESMO host:porta
que o cliente pediu -- nao da para apontar todos os sockets para uma URL fixa, porque
nesse caso o destino pretendido se perde.

Como o servidor real ja ocupa a porta, a ponte escuta num host DIFERENTE
(127.0.0.2, um apelido de loopback) e repassa para o servidor de verdade. O
MainInfo.ini do build Web aponta para 127.0.0.2; o do PC continua com o IP real.

Uso:
    python ws_tcp_bridge.py <host_ponte> <host_servidor> escuta[:destino] ...

Exemplo, com o servidor local em 44405 e a ponte em 44406:

    python ws_tcp_bridge.py 127.0.0.1 192.168.1.253 44406:44405

Escutar numa porta diferente da de destino e o jeito de conviver com o servidor
real: ele ja ocupa a porta dele, e no Windows nao existe apelido de loopback para
desempatar por host (bind em 127.0.0.2 volta PermissionError).

Aceita varias portas porque, depois do ConnectServer, o cliente conecta no servidor
de jogo em outra porta -- essas vem da lista de servidores, entao precisam ser
mapeadas tambem para o jogo ir alem da tela de login.
"""
import asyncio
import sys
import time

import websockets


async def bombear_ws_para_tcp(ws, escritor):
    try:
        async for mensagem in ws:
            if isinstance(mensagem, str):
                mensagem = mensagem.encode("latin-1")
            print("  %s cliente -> servidor: %d bytes  %s"
                  % (time.strftime("%H:%M:%S"), len(mensagem), mensagem[:12].hex()), flush=True)
            escritor.write(mensagem)
            await escritor.drain()
        # ATENCAO: sair do `async for` NAO prova que o cliente fechou.
        #
        # A versao anterior deste log afirmava isso, e estava enganando: a outra bomba
        # (tcp->ws) fecha o WebSocket no seu `finally`, e nesse caso o `async for` aqui
        # tambem termina normalmente. Ou seja, "lado CLIENTE encerrou" aparecia mesmo
        # quando quem encerrou fomos nos.
        #
        # O que decide e a ORDEM: quem terminou primeiro. Por isso as duas bombas
        # registram o proprio fim, com marca de tempo.
        # close_rcvd / close_sent dizem QUEM iniciou o fechamento -- e essa e a unica
        # forma de sair do circulo: as duas bombas terminam no mesmo segundo, e o
        # codigo 1000 aparece tanto se o cliente fechou quanto se fomos nos.
        print("  [ws->tcp] terminou: codigo=%s recebido_do_cliente=%s enviado_por_nos=%s t=%s"
              % (ws.close_code, getattr(ws, "close_rcvd", None),
                 getattr(ws, "close_sent", None), time.strftime("%H:%M:%S")), flush=True)
    except Exception as erro:
        print("  [ws->tcp] terminou por erro: %s: %s t=%s"
              % (type(erro).__name__, erro, time.strftime("%H:%M:%S")), flush=True)
    finally:
        try:
            escritor.close()
        except Exception:
            pass


def tamanho_do_pacote(quadro):
    """Tamanho declarado do pacote MU no inicio de `quadro`, ou 0 se incompleto."""
    if not quadro:
        return 0
    tipo = quadro[0]
    if tipo in (0xC1, 0xC3):
        if len(quadro) < 2:
            return 0
        return quadro[1]
    if tipo in (0xC2, 0xC4):
        if len(quadro) < 3:
            return 0
        return (quadro[1] << 8) | quadro[2]
    return -1   # cabecalho desconhecido: nao da para andar no fluxo


async def reescrever_endereco_do_jogo(pacote, host_servidor, host_ponte):
    """Troca o endereco anunciado no pacote 0xF4/0x03 pelo endereco da ponte.

    POR QUE: ao escolher o servidor, o ConnectServer responde com o IP e a PORTA do
    servidor de jogo, e o cliente conecta lá. No navegador esse connect vira
    `ws://<ip>:<porta>` -- endereco onde nao existe ponte, e onde o servidor real ja
    ocupa a porta. Sem reescrever, o login para na selecao de servidor.

    Como a ponte ve todo o trafego, ela troca o endereco por um dela e sobe o repasse
    correspondente na hora. O cliente nao percebe diferenca.

    Layout (PRECEIVE_SERVER_ADDRESS, WSclient.h:1092):
        [0]      0xC1
        [1]      tamanho (0x16 = 22)
        [2]      HeadCode  = 0xF4
        [3]      SubCode   = 0x03
        [4:19]   IP, ASCII de 15 bytes com preenchimento nulo
        [19]     ALINHAMENTO -- a struct NAO e empacotada, e `WORD Port` depois de
                 `BYTE IP[15]` cai em deslocamento impar (19), entao o compilador
                 insere um byte. E o que faz o tamanho ser 22 e nao 21.
        [20:22]  Porta, WORD na ordem do host

    O byte de alinhamento foi a causa de um erro concreto: lendo a porta em [19:21] a
    ponte anunciava 23919 (padding + byte baixo), abria repasse para uma porta que nao
    existe, e o cliente conectava num numero que nao correspondia ao anunciado. A porta
    real e 55901.
    """
    if len(pacote) < 22 or pacote[0] != 0xC1 or pacote[2] != 0xF4 or pacote[3] != 0x03:
        return pacote

    ip_alvo = pacote[4:19].split(b"\x00", 1)[0].decode("latin-1")
    porta_alvo = pacote[20] | (pacote[21] << 8)

    # O servidor pode anunciar um IP que so faz sentido de fora; o destino real e
    # sempre a maquina do servidor, entao o host da linha de comando manda.
    porta_ponte = await garantir_repasse(host_servidor, porta_alvo, host_ponte)

    novo_ip = host_ponte.encode("latin-1")[:15].ljust(15, b"\x00")
    novo = bytearray(pacote)
    novo[4:19] = novo_ip
    novo[20] = porta_ponte & 0xFF
    novo[21] = (porta_ponte >> 8) & 0xFF
    print("  reescrito servidor de jogo: %s:%d  ->  %s:%d"
          % (ip_alvo, porta_alvo, host_ponte, porta_ponte), flush=True)
    return bytes(novo)


async def bombear_tcp_para_ws(leitor, ws, host_servidor, host_ponte):
    pendente = b""
    try:
        while True:
            dados = await leitor.read(65536)
            if not dados:
                print("  [tcp->ws] terminou: SERVIDOR fechou a conexao t=%s"
                      % (time.strftime("%H:%M:%S"),), flush=True)
                break
            print("  %s servidor -> cliente: %d bytes  %s"
                  % (time.strftime("%H:%M:%S"), len(dados), dados[:12].hex()), flush=True)

            # Acumula porque um pacote pode chegar partido entre dois reads, e a
            # reescrita precisa do pacote inteiro. O que nao estiver completo espera.
            pendente += dados
            saida = b""
            while pendente:
                tamanho = tamanho_do_pacote(pendente)
                if tamanho < 0:
                    # Cabecalho que nao reconhecemos: repassa o resto sem mexer, em
                    # vez de descartar. Melhor um pacote nao reescrito do que perder
                    # o sincronismo do fluxo.
                    saida += pendente
                    pendente = b""
                    break
                if tamanho == 0 or len(pendente) < tamanho:
                    break   # incompleto: espera o proximo read
                pacote = pendente[:tamanho]
                pendente = pendente[tamanho:]
                saida += await reescrever_endereco_do_jogo(pacote, host_servidor, host_ponte)

            if saida:
                await ws.send(saida)
    except Exception as erro:
        # Faltava registrar ESTE caminho, e a falta criava ambiguidade: se esta bomba
        # morre por excecao, o `finally` abaixo fecha o WebSocket, e a outra bomba
        # termina "normalmente" com codigo 1000 -- parecendo que o cliente fechou.
        print("  [tcp->ws] terminou por erro: %s: %s t=%s"
              % (type(erro).__name__, erro, time.strftime("%H:%M:%S")), flush=True)
    finally:
        try:
            await ws.close()
        except Exception:
            pass


# Repasses ativos: porta de destino no servidor -> porta escutada pela ponte.
# Existe para nao abrir dois listeners para o mesmo destino quando o cliente
# reconecta, e para reutilizar a porta ja anunciada.
g_repasses = {}
g_host_ponte = "127.0.0.1"


async def garantir_repasse(host_servidor, porta_destino, host_ponte):
    """Devolve a porta local que repassa para <host_servidor>:<porta_destino>."""
    if porta_destino in g_repasses:
        return g_repasses[porta_destino]

    # Porta 0 deixa o sistema escolher uma livre: evita adivinhar e evita colidir
    # com o servidor real, que pode estar nesta mesma maquina.
    servidor = await websockets.serve(
        criar_manipulador(host_servidor, porta_destino, porta_destino),
        host_ponte, 0,
        subprotocols=["binary"],
        max_size=None,
    )
    porta_escuta = servidor.sockets[0].getsockname()[1]
    g_repasses[porta_destino] = porta_escuta
    print("escutando ws://%s:%d  ->  tcp://%s:%d  (sob demanda)"
          % (host_ponte, porta_escuta, host_servidor, porta_destino), flush=True)
    return porta_escuta


def criar_manipulador(host_servidor, porta_escuta, porta_destino):
    async def manipulador(ws):
        try:
            leitor, escritor = await asyncio.open_connection(host_servidor, porta_destino)
        except Exception as erro:
            print("  %d: nao conectou em %s:%d (%s)"
                  % (porta_escuta, host_servidor, porta_destino, erro), flush=True)
            await ws.close()
            return

        print("  %d: cliente conectado -> %s:%d"
              % (porta_escuta, host_servidor, porta_destino), flush=True)
        await asyncio.gather(
            bombear_ws_para_tcp(ws, escritor),
            bombear_tcp_para_ws(leitor, ws, host_servidor, g_host_ponte),
        )
        print("  %d: sessao encerrada" % porta_escuta, flush=True)

    return manipulador


async def principal():
    if len(sys.argv) < 4:
        print(__doc__)
        raise SystemExit(2)

    global g_host_ponte
    host_ponte = sys.argv[1]
    host_servidor = sys.argv[2]
    g_host_ponte = host_ponte

    # Cada argumento e "escuta:destino" ou so "porta" (escuta == destino).
    # Portas distintas existem porque o servidor real ja ocupa a dele, e no Windows
    # nao da para escutar num apelido de loopback para desempatar por host.
    pares = []
    for arg in sys.argv[3:]:
        if ":" in arg:
            escuta, destino = arg.split(":", 1)
            pares.append((int(escuta), int(destino)))
        else:
            pares.append((int(arg), int(arg)))

    servidores = []
    for porta_escuta, porta_destino in pares:
        # subprotocols=['binary']: e o que o Emscripten pede. Sem aceitar, o
        # navegador recusa a conexao no aperto de mao.
        servidor = await websockets.serve(
            criar_manipulador(host_servidor, porta_escuta, porta_destino),
            host_ponte, porta_escuta,
            subprotocols=["binary"],
            max_size=None,
            # ping_interval=None: SEM keepalive de WebSocket.
            #
            # O padrao da biblioteca e pingar a cada 20 s e derrubar se o pong nao vier
            # em 20 s. O cliente aqui e um jogo em wasm que trava a thread principal por
            # dezenas de segundos (carga sincrona de assets, quadros de vários segundos),
            # e uma queda por keepalive apareceria como "o servidor resetou" -- que foi
            # exatamente o sintoma perseguido.
            #
            # Nao ha o que o keepalive proteja aqui: a ponte roda em rede local e a
            # sessao dura minutos.
            ping_interval=None,
        )
        servidores.append(servidor)
        g_repasses[porta_destino] = porta_escuta
        print("escutando ws://%s:%d  ->  tcp://%s:%d"
              % (host_ponte, porta_escuta, host_servidor, porta_destino), flush=True)

    await asyncio.Future()   # roda ate ser interrompida


asyncio.run(principal())
