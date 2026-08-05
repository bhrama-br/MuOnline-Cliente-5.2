#pragma once

#include "PlatformTypes.h"

// This header deliberately exposes no Win32 types. Platform backends translate
// native events into this legacy-compatible pointer snapshot.
namespace Platform
{
    struct PointerSnapshot
    {
        long x;
        long y;
        bool leftButtonDown;
        bool rightButtonDown;
        bool middleButtonDown;
    };

    class IInputBackend
    {
    public:
        virtual ~IInputBackend() {}
        virtual bool BindWindow(NativeWindowHandle window) = 0;
        virtual void ReadPointer(PointerSnapshot& pointer) = 0;
        virtual unsigned int GetDoubleClickTimeMilliseconds() const = 0;
    };

    // Teclado -> campo de texto em foco.
    //
    // Implementadas em Platform/PlatformEditControl.cpp, sobre o controle EDIT
    // sintetico que substitui o EDIT nativo do Windows em CUITextInputBox. Os
    // backends de plataforma chamam estas funcoes ao receber teclas; quando devolvem
    // false nao ha campo em foco e a tecla pertence ao jogo.
    bool HaFocoDeTexto();
    bool EnviarCaractereDeTexto(unsigned int codigo);       // equivale a WM_CHAR
    bool EnviarTeclaDeTexto(unsigned int codigoVirtual);    // equivale a WM_KEYDOWN

    // Teclado -> jogo. Implementadas em Platform/PlatformKeyboard.cpp; alimentam a
    // tabela que GetAsyncKeyState responde, e com ela SEASON3B::IsPress/IsRepeat e
    // CInput::IsKeyDown -- ou seja, todos os atalhos do cliente.
    void DefinirTeclaLegada(int codigoVirtual, bool pressionada);
    void LimparTecladoLegado();
    bool TeclaLegadaPressionada(int codigoVirtual);

    IInputBackend& GetInputBackend();
    void SetInputBackend(IInputBackend* backend);
    void SetAndroidPointerState(long x, long y, bool pressed);
    void InitializeWebInput();
    void GetWebPointerState(long& x, long& y);
    // One-shot request raised by the browser's Enter key while no text field
    // owns focus.  The game loop consumes it on its normal update thread.
    bool ConsumeWebChatOpenRequest();
}
