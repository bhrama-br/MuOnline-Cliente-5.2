#include "LegacySocketPump.h"

#if !defined(_WIN32)

#include "WinsockCompat.h"

#include <sys/select.h>
#include <errno.h>
#include <stdio.h>
#if defined(__EMSCRIPTEN__)
#  include <emscripten.h>   // EM_ASM_INT, em SituacaoDeLeitura
#endif

namespace
{
    int g_socket = -1;
    Platform::LegacySocketEventHandler g_tratador = 0;

    // O Windows entrega FD_WRITE UMA vez, quando o socket fica gravavel, e so
    // volta a entregar depois de um send() que retorne WSAEWOULDBLOCK. Um socket
    // conectado e quase sempre gravavel, entao repetir a cada quadro faria o
    // cliente chamar FDWriteSend sem parar. Este sinalizador reproduz o disparo
    // unico.
    bool g_escritaEntregue = false;
    bool g_fechadoEntregue = false;
}

#if defined(__EMSCRIPTEN__)
namespace
{
    // Situacao de leitura do socket, perguntada ao proprio Emscripten.
    //
    // POR QUE NAO BASTA O `select`: o `poll` do libsockfs devolve POLLIN quando a fila
    // tem mensagem OU quando o peer esta fechando/fechado OU quando nao ha peer. Ou
    // seja, "legivel" nao quer dizer "ha dados" -- e o cliente reagia a cada uma dessas
    // acordadas chamando recv, que voltava vazio. O resultado era um laco de leitura
    // vazia (o `nRecv devolveu 1` repetido, milhares de vezes por minuto) alem de
    // confundir fim de fluxo com nada-a-ler.
    //
    // Devolve: >0 mensagens na fila; 0 fila vazia com WebSocket aberto (acordada
    // espuria); -1 WebSocket fechando/fechado (fim de fluxo de verdade); -2 sem socket.
    int SituacaoDeLeitura(int fd)
    {
        return EM_ASM_INT({
            var stream = FS.streams[$0];
            var sock = stream && stream.node && stream.node.sock;
            if (!sock) return -2;
            var queue = sock.recv_queue ? sock.recv_queue.length : 0;
            if (queue > 0) return queue;
            var key = sock.daddr + ":" + sock.dport;
            var peer = sock.peers ? sock.peers[key] : undefined;
            if (!peer || !peer.socket) return -1;
            var estado = peer.socket.readyState;
            if (estado === 2 || estado === 3) return -1;   // CLOSING ou CLOSED
            return 0;
        }, fd);
    }
}
#endif

namespace Platform
{
    void RegisterLegacySocket(int socket, LegacySocketEventHandler tratador)
    {
        g_socket = socket;
        g_tratador = tratador;
        g_escritaEntregue = false;
        g_fechadoEntregue = false;
    }

    int LegacySocketFd()
    {
        return g_socket;
    }

    void RearmLegacySocketWrite()
    {
        // Reproduz o rearme do Winsock.
        //
        // FD_WRITE e entregue UMA vez, e o Windows volta a entrega-lo quando um send
        // falha com WSAEWOULDBLOCK -- e assim que o cliente sabe que pode esvaziar a
        // fila de envio. Sem rearmar, um pacote que caiu na fila fica lá para sempre.
        //
        // Foi o que aconteceu no fluxo de login: o pedido de endereco do servidor de
        // jogo (0xF4/0x03) caiu no caminho "would block", foi enfileirado, e nenhum
        // FD_WRITE novo apareceu para envia-lo. O cliente parecia ignorar o clique --
        // sem erro, sem nada na rede.
        g_escritaEntregue = false;
    }

    void PumpLegacySocket()
    {
        if (g_socket < 0 || g_tratador == 0) return;

        // NAO sondar o descritor com `send` de tamanho zero, nem com fcntl.
        //
        // Ficou registrado porque as duas tentativas custaram tempo: no Emscripten um
        // send de 0 byte NAO e inofensivo -- envia um quadro WebSocket VAZIO de verdade
        // (visivel na ponte como "cliente -> servidor: 0 bytes"), ou seja, a sonda mexia
        // no que estava medindo. E `fcntl(fd, F_GETFD)` e o oposto: nunca falha aqui,
        // entao o silencio dele nao provava nada.
        //
        // Quem decide agora e SituacaoDeLeitura, que pergunta o estado ao proprio
        // libsockfs sem tocar no fluxo.

        fd_set leitura, escrita, excecao;
        FD_ZERO(&leitura);
        FD_ZERO(&escrita);
        FD_ZERO(&excecao);
        FD_SET(g_socket, &leitura);
        FD_SET(g_socket, &excecao);
        if (!g_escritaEntregue) FD_SET(g_socket, &escrita);

        struct timeval semEspera;
        semEspera.tv_sec = 0;
        semEspera.tv_usec = 0;

        const int ready = select(g_socket + 1, &leitura, &escrita, &excecao, &semEspera);
        if (ready <= 0) return;

        if (FD_ISSET(g_socket, &escrita))
        {
            g_escritaEntregue = true;
            g_tratador(FD_WRITE, 0);
        }

        if (FD_ISSET(g_socket, &leitura))
        {
            // SEM MSG_PEEK.
            //
            // A versao anterior espiava 1 byte para distinguir "ha dados" de "a outra
            // ponta fechou", que e o que o Windows resolve com FD_READ vs FD_CLOSE.
            // Isso NAO funciona no Emscripten: a implementacao de socket dele declara
            // `recvmsg(sock, length)` -- sem parametro de flags --, entao MSG_PEEK e
            // silenciosamente ignorado e a espiada CONSOME o byte.
            //
            // O efeito era destrutivo e mudo: o 0xC1 que abre todo pacote do MU
            // desaparecia, o cliente nunca reconhecia um pacote valido e ficava em
            // CurrentProtocolState 0 para sempre -- com a conexao TCP viva e visivel
            // no netstat, o que fazia parecer problema de servidor.
            //
            // Quem le agora e so o cliente, uma vez.
#if defined(__EMSCRIPTEN__)
            // FD_READ so quando ha mensagem de fato; fim de fluxo vira FD_CLOSE.
            // Ver SituacaoDeLeitura sobre por que "legivel" nao basta.
            const int situacao = SituacaoDeLeitura(g_socket);
            if (situacao > 0)
            {
                g_tratador(FD_READ, 0);
            }
            else if (situacao == -1)
            {
                if (!g_fechadoEntregue)
                {
                    g_fechadoEntregue = true;
                    fprintf(stderr, "[socket] fim de fluxo detectado (WebSocket fechado)\n");
                    g_tratador(FD_CLOSE, 0);
                }
            }
            // situacao == 0: acordada espuria. Nao entregar nada -- era exatamente
            // isso que produzia o laco de leitura vazia.
#else
            g_tratador(FD_READ, 0);
#endif
        }

        if (FD_ISSET(g_socket, &excecao) && !g_fechadoEntregue)
        {
            g_fechadoEntregue = true;
            g_tratador(FD_CLOSE, errno);
        }
    }
}

#endif  // !_WIN32
