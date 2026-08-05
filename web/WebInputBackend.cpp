#include <atomic>
#include <cstring>
#include <emscripten/html5.h>
#include "PlatformInput.h"

namespace
{
    class WebInputBackend : public Platform::IInputBackend
    {
    public:
        WebInputBackend() : m_x(0), m_y(0), m_left(false), m_right(false), m_middle(false) {}
        bool BindWindow(Platform::NativeWindowHandle) override { return true; }
        void ReadPointer(Platform::PointerSnapshot& pointer) override
        {
            pointer.x = m_x.load(); pointer.y = m_y.load();
            pointer.leftButtonDown = m_left.load();
            pointer.rightButtonDown = m_right.load();
            pointer.middleButtonDown = m_middle.load();
        }
        unsigned int GetDoubleClickTimeMilliseconds() const override { return 500; }
        void Set(long x, long y, unsigned short botoes)
        {
            m_x = x; m_y = y;
            // Bits de `buttons` do DOM: 1 = esquerdo, 2 = direito, 4 = meio.
            m_left   = (botoes & 1) != 0;
            m_right  = (botoes & 2) != 0;
            m_middle = (botoes & 4) != 0;
            SyncLegacyButtonKeys();
        }
        void SetButton(long x, long y, int eventType, unsigned short button, unsigned short buttons)
        {
            m_x = x; m_y = y;

            // Some browsers report a transient zero `buttons` mask on the
            // down/up callbacks.  The legacy UI needs the exact edge, so use
            // the event type and button as the authoritative state there.
            if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN)
            {
                if (button == 0) m_left = true;
                else if (button == 1) m_middle = true;
                else if (button == 2) m_right = true;
            }
            else if (eventType == EMSCRIPTEN_EVENT_MOUSEUP)
            {
                if (button == 0) m_left = false;
                else if (button == 1) m_middle = false;
                else if (button == 2) m_right = false;
            }
            else
            {
                Set(x, y, buttons);
                return;
            }
            SyncLegacyButtonKeys();
        }
    private:
        void SyncLegacyButtonKeys()
        {
            // New UI controls read mouse buttons through IsPress(VK_*BUTTON),
            // which ultimately uses this legacy asynchronous-key table.
            Platform::DefinirTeclaLegada(0x01, m_left.load());
            Platform::DefinirTeclaLegada(0x02, m_right.load());
            Platform::DefinirTeclaLegada(0x04, m_middle.load());
        }
        std::atomic<long> m_x, m_y;
        std::atomic<bool> m_left, m_right, m_middle;
    };

    WebInputBackend g_backend;
    std::atomic<bool> g_webChatOpenRequested(false);
    EM_BOOL OnMouse(int eventType, const EmscriptenMouseEvent* event, void*)
    {
        // targetX/targetY, nao canvasX/canvasY: estes ultimos estao deprecados no
        // Emscripten e NAO sao mais preenchidos — ficam sempre em zero, o que
        // fazia o ponteiro parecer parado no canto e a camera nunca girar.
        g_backend.SetButton(event->targetX, event->targetY, eventType,
                            event->button, event->buttons);
        return EM_TRUE;
    }

    // Codigos virtuais do Win32 que o controle EDIT sintetico entende. Os nomes vem
    // de KeyboardEvent.key, que e estavel entre navegadores e ja resolve layout,
    // Shift e acentos -- ao contrario de keyCode, que esta obsoleto e e por teclado
    // fisico.
    const unsigned int kVkBack   = 0x08;
    const unsigned int kVkTab    = 0x09;
    const unsigned int kVkReturn = 0x0D;
    const unsigned int kVkEscape = 0x1B;
    const unsigned int kVkEnd    = 0x23;
    const unsigned int kVkHome   = 0x24;
    const unsigned int kVkLeft   = 0x25;
    const unsigned int kVkUp     = 0x26;
    const unsigned int kVkRight  = 0x27;
    const unsigned int kVkDown   = 0x28;
    const unsigned int kVkDelete = 0x2E;

    // Decodifica UTF-8. O `key` de um caractere imprimivel tem exatamente um
    // codepoint; qualquer coisa maior e um nome de tecla ("Enter", "ArrowLeft").
    // Devolve 0 quando nao e um unico codepoint.
    unsigned int CodepointUnico(const char* texto)
    {
        if (texto == NULL || texto[0] == '\0') return 0;
        const unsigned char* bytes = (const unsigned char*)texto;
        unsigned int codigo = 0;
        int comprimento = 0;
        if (bytes[0] < 0x80)             { codigo = bytes[0];        comprimento = 1; }
        else if ((bytes[0] & 0xE0) == 0xC0) { codigo = bytes[0] & 0x1F; comprimento = 2; }
        else if ((bytes[0] & 0xF0) == 0xE0) { codigo = bytes[0] & 0x0F; comprimento = 3; }
        else if ((bytes[0] & 0xF8) == 0xF0) { codigo = bytes[0] & 0x07; comprimento = 4; }
        else return 0;
        for (int i = 1; i < comprimento; ++i)
        {
            if ((bytes[i] & 0xC0) != 0x80) return 0;
            codigo = (codigo << 6) | (bytes[i] & 0x3F);
        }
        return bytes[comprimento] == '\0' ? codigo : 0;
    }

    // Codigo virtual do Win32 a partir do evento do DOM.
    //
    // `keyCode` do DOM coincide com o VK do Windows na faixa que o cliente usa: 8/9/13
    // (backspace, tab, enter), 16-18 (shift, ctrl, alt), 27 (esc), 32 (espaco), 33-40
    // (pgup/pgdn/end/home/setas), 45/46 (insert, delete), 48-57 (digitos), 65-90
    // (letras), 112-123 (F1-F12). Fora dessa faixa nao importa: o cliente nao consulta.
    unsigned int CodigoVirtual(const EmscriptenKeyboardEvent* evento)
    {
        if (evento->keyCode > 0 && evento->keyCode < 256) return evento->keyCode;
        if (evento->which > 0 && evento->which < 256) return (unsigned int)evento->which;
        // Modern browsers may leave the deprecated numeric fields empty.  Enter
        // is a gameplay key (it opens the chat before a text field has focus),
        // so resolve it from the stable DOM key name as well.
        if (strcmp(evento->key, "Enter") == 0) return kVkReturn;
        return 0;
    }

    EM_BOOL OnKeyUp(int, const EmscriptenKeyboardEvent* event, void*)
    {
        const unsigned int virtualDom = CodigoVirtual(event);
        if (virtualDom != 0) Platform::DefinirTeclaLegada((int)virtualDom, false);
        return Platform::HaFocoDeTexto() ? EM_TRUE : EM_FALSE;
    }

    // Ao perder o foco da janela nao chegam mais eventos de soltar: sem limpar, uma
    // tecla ficaria "presa" para sempre (o cliente andaria sozinho, por exemplo).
    EM_BOOL OnBlur(int, const EmscriptenFocusEvent*, void*)
    {
        Platform::LimparTecladoLegado();
        return EM_FALSE;
    }

    EM_BOOL OnKeyDown(int, const EmscriptenKeyboardEvent* event, void*)
    {
        // A tabela de teclas do JOGO e alimentada SEMPRE, mesmo com campo de texto em
        // foco -- e o que o Windows faz, porque GetAsyncKeyState e global. Quem separa
        // os dois mundos e o cliente: CInput::IsKeyDown devolve false em modo de edicao.
        const unsigned int virtualDom = CodigoVirtual(event);
        if (virtualDom != 0) Platform::DefinirTeclaLegada((int)virtualDom, true);

        // Opening chat must work before a native-like text control has focus.
        // Queue it for the game loop instead of relying on the browser callback
        // timing relative to the legacy asynchronous-key scan.
        if (virtualDom == kVkReturn && !Platform::HaFocoDeTexto())
            g_webChatOpenRequested = true;

        // Sem campo de texto em foco a tecla nao e do EDIT: deixar o navegador em paz
        // preserva F5, F12 e os atalhos do usuario.
        if (!Platform::HaFocoDeTexto()) return EM_FALSE;

        const char* nome = event->key;
        // Ctrl+tecla nao vira caractere. O cliente trata Ctrl+C/V/X pelo proprio
        // filtro, mas colar de verdade depende da area de transferencia do sistema,
        // que ainda nao existe fora do Windows (OpenClipboard e stub).
        if (event->ctrlKey || event->altKey || event->metaKey) return EM_FALSE;

        unsigned int virtual_ = 0;
        if      (strcmp(nome, "Backspace")  == 0) virtual_ = kVkBack;
        else if (strcmp(nome, "Tab")        == 0) virtual_ = kVkTab;
        else if (strcmp(nome, "Enter")      == 0) virtual_ = kVkReturn;
        else if (strcmp(nome, "Escape")     == 0) virtual_ = kVkEscape;

        // Backspace, Tab, Enter e Escape chegam ao EDIT como WM_CHAR, e nao WM_KEYDOWN:
        // e assim que o Windows entrega (TranslateMessage converte), e EditWndProc
        // depende disso -- e no case WM_CHAR que ele decide confirmar o login (Enter),
        // trocar de campo (Tab) e permitir apagar num campo de so numeros.
        if (virtual_ != 0)
        {
            Platform::EnviarCaractereDeTexto(virtual_);
            return EM_TRUE;
        }

        if      (strcmp(nome, "Delete")     == 0) virtual_ = kVkDelete;
        else if (strcmp(nome, "Home")       == 0) virtual_ = kVkHome;
        else if (strcmp(nome, "End")        == 0) virtual_ = kVkEnd;
        else if (strcmp(nome, "ArrowLeft")  == 0) virtual_ = kVkLeft;
        else if (strcmp(nome, "ArrowRight") == 0) virtual_ = kVkRight;
        else if (strcmp(nome, "ArrowUp")    == 0) virtual_ = kVkUp;
        else if (strcmp(nome, "ArrowDown")  == 0) virtual_ = kVkDown;
        if (virtual_ != 0)
        {
            Platform::EnviarTeclaDeTexto(virtual_);
            return EM_TRUE;
        }

        const unsigned int codigo = CodepointUnico(nome);
        if (codigo >= 32)
        {
            Platform::EnviarCaractereDeTexto(codigo);
            return EM_TRUE;
        }
        // Teclas de funcao e modificadoras ("F5", "Shift"): nao sao texto.
        return EM_FALSE;
    }
}

namespace Platform
{
    IInputBackend& GetInputBackend() { return g_backend; }
    void SetInputBackend(IInputBackend*) {}
    void InitializeWebInput()
    {
        // mousedown/mouseup, e nao so mousemove.
        //
        // Antes so o movimento era registrado, e o estado do botao vinha do
        // `event->buttons` DESSE movimento -- ou seja, um clique sem arrastar nunca
        // chegava ao cliente. O sintoma era mudo: a cena respondia ao mouse (a camera
        // girava), mas nada era clicavel, e o HUD do cliente mostrava
        // "MousePos : 512 384" parado no centro. Foi o que travou o fluxo de login
        // depois de a lista de servidores aparecer: dava para VER "Valhalla" e nao
        // dava para escolher.
        emscripten_set_mousemove_callback("#canvas", NULL, EM_TRUE, OnMouse);
        emscripten_set_mousedown_callback("#canvas", NULL, EM_TRUE, OnMouse);
        emscripten_set_mouseup_callback("#canvas", NULL, EM_TRUE, OnMouse);
        // O canvas recebe foco em todo clique (shell.html). Registrar nele evita que
        // o Chrome entregue Enter ao elemento da pagina que tinha foco antes do jogo.
        // Tambem impede a mesma tecla de passar duas vezes por canvas e window.
        emscripten_set_keydown_callback("#canvas", NULL, EM_TRUE, OnKeyDown);
        emscripten_set_keyup_callback("#canvas", NULL, EM_TRUE, OnKeyUp);
        emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, OnBlur);
    }
    void GetWebPointerState(long& x, long& y) { PointerSnapshot pointer; g_backend.ReadPointer(pointer); x = pointer.x; y = pointer.y; }
    bool ConsumeWebChatOpenRequest()
    {
        return g_webChatOpenRequested.exchange(false);
    }
}
