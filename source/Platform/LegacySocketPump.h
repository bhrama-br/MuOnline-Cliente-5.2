#pragma once

// Substituto de WSAAsyncSelect fora do Windows.
//
// O Winsock entrega eventos de socket como MENSAGEM DE JANELA: WSAAsyncSelect
// registra o socket e o window proc recebe FD_READ / FD_WRITE / FD_CLOSE. Nem o
// navegador nem o Android tem window proc, e o cliente nao pode simplesmente
// bloquear em recv() -- ele tem um laco de quadro para manter.
//
// Aqui os mesmos eventos sao produzidos por polling: uma chamada por quadro faz
// select() com tempo zero e entrega os eventos ao mesmo tratador. Os codigos
// entregues sao os do Winsock, entao a tabela de despacho do cliente vale sem
// alteracao.
//
// Nao existe no Windows: la o Winsock de verdade continua fazendo o trabalho.

#if !defined(_WIN32)

namespace Platform
{
    // Recebe (evento, erro) com os mesmos valores que o lParam do Windows
    // carregaria: FD_READ / FD_WRITE / FD_CLOSE e um codigo de erro.
    typedef void (*LegacySocketEventHandler)(int evento, int erro);

    // Passa a acompanhar `socket`. Um socket <= 0 desliga o acompanhamento.
    void RegisterLegacySocket(int socket, LegacySocketEventHandler tratador);

    // Chamar uma vez por quadro. Nao bloqueia.
    void PumpLegacySocket();

    // Faz FD_WRITE ser entregue de novo.
    //
    // O Winsock entrega FD_WRITE uma vez e o rearma quando um send falha com
    // WSAEWOULDBLOCK; quem chama isto sao os pontos de envio do cliente, no mesmo
    // momento em que enfileiram os dados. Sem o rearme, o que entra na fila nunca
    // sai.
    void RearmLegacySocketWrite();

    // Descritor do socket registrado, ou -1. Existe para o rastreador de `fclose`
    // poder reconhecer quando alguem fecha, por acidente, o descritor da conexao.
    int LegacySocketFd();
}

#endif  // !_WIN32
