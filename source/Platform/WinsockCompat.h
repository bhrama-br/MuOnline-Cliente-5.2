#pragma once

// Winsock sobre sockets POSIX, para WSctlc.cpp compilar fora do Windows.
//
// A maior parte do Winsock E BSD sockets com outros nomes: socket, connect,
// send, recv, htons e sockaddr_in sao identicos. O que muda de verdade:
//
//  - `SOCKET` e um handle sem sinal no Windows e um `int` no POSIX, e o valor
//    invalido e -1 em vez de ~0. O codigo compara com INVALID_SOCKET, entao os
//    dois precisam concordar.
//  - `closesocket` e `close`.
//  - `WSAGetLastError` e `errno`, e WSAEWOULDBLOCK e EAGAIN/EWOULDBLOCK.
//  - `ioctlsocket(FIONBIO)` vira fcntl(O_NONBLOCK), que e mais confiavel: no
//    Emscripten ioctl sobre socket nao esta implementado.
//  - `WSAAsyncSelect` NAO tem equivalente: e notificacao por mensagem de janela.
//    Ver Platform/LegacySocketPump.h, que faz a mesma entrega por polling.
//
// No Windows este cabecalho e vazio: o build do PC continua usando o Winsock de
// verdade.

#if !defined(_WIN32)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

typedef int SOCKET;

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR   (-1)
#endif

typedef struct sockaddr* LPSOCKADDR;

// Codigos de erro. O legado compara com estes nomes.
#ifndef WSAEWOULDBLOCK
#define WSAEWOULDBLOCK  EWOULDBLOCK
#define WSAECONNRESET   ECONNRESET
#define WSAECONNABORTED ECONNABORTED
#define WSAENETDOWN     ENETDOWN
#define WSAECONNREFUSED ECONNREFUSED
#define WSAETIMEDOUT    ETIMEDOUT
#define WSAEHOSTUNREACH EHOSTUNREACH
#endif

// WSAGetLastError ja vem de WindowsCompat.h (tambem `return errno`), entao nao e
// redefinida aqui.

inline int closesocket(SOCKET s) { return ::close(s); }

// "Ainda nao da para enviar" contra "erro de verdade", no envio.
//
// O cliente trata SOCKET_ERROR no send comparando com WSAEWOULDBLOCK: se for isso,
// enfileira e reenvia quando chegar FD_WRITE; qualquer outro erro fecha a conexao.
//
// No Emscripten o `connect()` e ASSINCRONO -- ele volta com sucesso e o WebSocket
// ainda esta abrindo. O cliente manda o primeiro pacote imediatamente e o send
// falha com ENOTCONN, que nao e WSAEWOULDBLOCK: o resultado era
//
//   [Send Packet Error] WSAGetLastError() != WSAEWOULDBLOCK
//   [Socket Closed][Clear PacketQueue]
//
// logo apos conectar, com o cliente parado em CurrentProtocolState 0 -- ou seja, a
// conexao TCP existia (visivel no netstat) e o cliente a derrubava sozinho.
//
// ENOTCONN/EINPROGRESS/EALREADY aqui significam exatamente o que WSAEWOULDBLOCK
// significa no Windows: tente mais tarde. A fila e o reenvio por FD_WRITE que o
// cliente ja tem cobrem o resto.
inline bool LegacySendWouldBlock(int erro)
{
    return erro == EWOULDBLOCK || erro == EAGAIN
        || erro == ENOTCONN || erro == EINPROGRESS || erro == EALREADY;
}

// Eventos de socket. Tambem definidos em WindowsCompat.h, mas este cabecalho e
// usado por LegacySocketPump.cpp, que nao inclui aquele.
#ifndef FD_READ
#define FD_READ    0x01
#define FD_WRITE   0x02
#define FD_CONNECT 0x10
#define FD_CLOSE   0x20
#endif

// Encerramento parcial da conexao.
#ifndef SD_RECEIVE
#define SD_RECEIVE SHUT_RD
#define SD_SEND    SHUT_WR
#define SD_BOTH    SHUT_RDWR
#endif

// `struct linger` existe no POSIX; o legado usa o nome em maiusculas.
typedef struct linger LINGER;

#ifndef CopyMemory
#define CopyMemory(destination, source, size) memcpy((destination), (source), (size))
#endif

// WSAStartup/WSACleanup inicializam a DLL do Winsock. No POSIX nao ha nada para
// iniciar; devolver sucesso mantem o fluxo do chamador intacto.
typedef struct
{
    unsigned short wVersion;
    unsigned short wHighVersion;
    char           szDescription[257];
    char           szSystemStatus[129];
    unsigned short iMaxSockets;
    unsigned short iMaxUdpDg;
    char*          lpVendorInfo;
} WSADATA;

#ifndef MAKEWORD
#define MAKEWORD(a, b) ((unsigned short)(((unsigned char)(a)) | (((unsigned short)((unsigned char)(b))) << 8)))
#endif

inline int WSAStartup(unsigned short versao, WSADATA* data)
{
    if (data != 0)
    {
        memset(data, 0, sizeof(*data));
        data->wVersion = versao;
        data->wHighVersion = versao;
    }
    return 0;
}

inline int WSACleanup() { return 0; }

// Modo nao-bloqueante. O unico uso de ioctlsocket no cliente e FIONBIO.
#ifndef FIONBIO
#define FIONBIO 0x8004667E
#endif

inline int ioctlsocket(SOCKET s, long comando, unsigned long* argumento)
{
    if (comando != (long)FIONBIO || argumento == 0) return SOCKET_ERROR;

    const int sinalizadores = fcntl(s, F_GETFL, 0);
    if (sinalizadores < 0) return SOCKET_ERROR;

    const int novo = (*argumento != 0) ? (sinalizadores | O_NONBLOCK)
                                       : (sinalizadores & ~O_NONBLOCK);
    return (fcntl(s, F_SETFL, novo) < 0) ? SOCKET_ERROR : 0;
}

// Extracao do evento/erro que o Windows empacota no lParam da mensagem. A bomba
// de polling entrega os mesmos valores, entao as tabelas de despacho do cliente
// continuam valendo sem alteracao.
#ifndef WSAGETSELECTEVENT
#define WSAGETSELECTEVENT(l) ((int)((l) & 0xFFFF))
#define WSAGETSELECTERROR(l) ((int)(((l) >> 16) & 0xFFFF))
#endif

#endif  // !_WIN32
