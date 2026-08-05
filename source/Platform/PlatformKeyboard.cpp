// Estado de teclado do jogo, fora do Windows.
//
// POR QUE EXISTE: todo o teclado do cliente passa por UM ponto,
// `SEASON3B::CNewKeyInput::ScanAsyncKeyState` (NewUICommon.cpp:439), que varre as 256
// teclas com `GetAsyncKeyState` e deriva os estados NONE/PRESS/REPEAT/RELEASE. Em cima
// disso ficam `SEASON3B::IsPress`/`IsRepeat`, `CInput::IsKeyDown` e, portanto, TODOS os
// atalhos: Enter para entrar no jogo na cena de personagem, ESC, F1-F12, teclas de
// habilidade, modificadores de movimento.
//
// Fora do Windows `GetAsyncKeyState` era um stub que devolvia 0, então nada disso
// funcionava -- e o sintoma era mudo: a tecla simplesmente não fazia nada, sem erro.
// Este arquivo guarda o estado real das teclas e faz `GetAsyncKeyState` respondê-lo.
//
// Quem alimenta são os backends de plataforma (web/WebInputBackend.cpp e o Android),
// chamando Platform::DefinirTeclaLegada nos eventos de tecla.

#if !defined(_WIN32)

#include "WindowsCompat.h"
#include "PlatformInput.h"

// Enter tem um caminho PROPRIO, alem da tabela.
//
// `ScanAsyncKeyState` termina com:
//
//   if (IsPress(VK_RETURN) && IsEnterPressed() == false)
//       m_pInputInfo[VK_RETURN].byKeyState = KEY_NONE;
//   SetEnterPressed(false);
//
// ou seja: o PRESS do Enter e CANCELADO se `g_bEnterPressed` nao tiver sido marcado
// desde a varredura anterior. No Windows quem marca e o window proc, uma vez por
// pressionamento real. Sem reproduzir isso, o Enter nunca chega a valer -- mesmo com a
// tabela correta.
extern bool g_bEnterPressed;

namespace
{
    // 0 = solta, 1 = pressionada. Indexada pelo codigo virtual do Win32.
    unsigned char g_teclas[256] = { 0 };
}

namespace Platform
{
    void DefinirTeclaLegada(int codigoVirtual, bool pressionada)
    {
        if (codigoVirtual < 0 || codigoVirtual > 255) return;
        g_teclas[codigoVirtual] = pressionada ? 1 : 0;

        if (pressionada && codigoVirtual == VK_RETURN)
            g_bEnterPressed = true;
    }

    void LimparTecladoLegado()
    {
        for (int i = 0; i < 256; ++i) g_teclas[i] = 0;
    }

    bool TeclaLegadaPressionada(int codigoVirtual)
    {
        if (codigoVirtual < 0 || codigoVirtual > 255) return false;
        return g_teclas[codigoVirtual] != 0;
    }
}

// O cliente testa `HIBYTE(GetAsyncKeyState(k)) & 0x80`, entao o bit alto e o que conta.
SHORT GetAsyncKeyState(int codigoVirtual)
{
    return Platform::TeclaLegadaPressionada(codigoVirtual) ? (SHORT)0x8000 : (SHORT)0;
}

// Sem estado de alternancia (Caps/Num Lock): o cliente so consulta o bit alto.
SHORT GetKeyState(int codigoVirtual)
{
    return GetAsyncKeyState(codigoVirtual);
}

#endif  // !_WIN32
