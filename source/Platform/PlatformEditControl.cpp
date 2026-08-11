// Controle EDIT sintetico, para os campos de texto da UI fora do Windows.
//
// POR QUE EXISTE: CUITextInputBox nao guarda nem desenha texto por conta propria.
// Ela cria um controle EDIT NATIVO do Windows
//
//     m_hEditWnd = CreateWindowW(L"edit", ...);          // UIControls.cpp
//
// pede que ELE se pinte numa DIB de memoria
//
//     CallWindowProcW(m_hOldProc, m_hEditWnd, WM_PAINT, (WPARAM)m_hMemDC, 0);
//
// e so entao converte essa DIB em textura. Fora do Windows nao existe janela filha:
// CreateWindowW devolvia NULL e, com isso, os campos de Conta e Senha da tela de
// login apareciam vazios e nao aceitavam digitacao -- nao havia onde o caractere
// pousar.
//
// O QUE ESTE ARQUIVO FAZ: implementa o minimo do EDIT que CUITextInputBox usa --
// texto, limite, mascara de senha, caret, selecao, visibilidade -- e atende
// WM_ERASEBKGND/WM_PAINT desenhando na DIB pelo rasterizador de PlatformText.cpp.
// CUITextInputBox continua INTACTA, o que evita duplicar a logica de layout,
// medicao e upload de textura que ela ja tem.
//
// O teclado entra por Platform::EnviarCaractereDeTexto/EnviarTeclaDeTexto, que
// despacham WM_CHAR/WM_KEYDOWN pelo proc INSTALADO PELO CLIENTE (EditWndProc, via
// SetWindowLongW(GWL_WNDPROC)) -- exatamente o caminho do Windows. Assim os filtros
// do cliente (somente numeros, somente serial, Enter que confirma, Tab que troca de
// campo) continuam valendo sem alteracao.

#if !defined(_WIN32)

#include "WindowsCompat.h"

#include <set>
#include <string>

namespace
{
    // Estado de um EDIT sintetico. O HWND devolvido ao cliente e o endereco desta
    // estrutura.
    struct EditControl
    {
        std::wstring text;
        size_t       selectionStart;   // [inicioSelecao, fimSelecao) esta selecionado
        size_t       selectionEnd;      // e tambem e onde fica o caret
        int          limit;          // 0 = sem limite
        DWORD        estilo;
        bool         visivel;
        WNDPROC      proc;            // proc corrente (EditWndProc depois do subclass)
        LONG         userData;  // GWL_USERDATA: o CUITextInputBox dono
        int          width;
        int          height;
        HDC          lastDc;        // DC do ultimo WM_PAINT, usado por GetCaretPos
    };

    std::set<EditControl*>& Registro()
    {
        static std::set<EditControl*> registro;
        return registro;
    }

    HWND g_focusedField = NULL;

    EditControl* Edicao(HWND window)
    {
        if (window == NULL) return NULL;
        EditControl* control = (EditControl*)window;
        std::set<EditControl*>& registro = Registro();
        return registro.find(control) == registro.end() ? NULL : control;
    }

    bool ENomeDeClasseEdit(LPCWSTR classe)
    {
        if (classe == NULL) return false;
        // Comparacao sem diferenciar caixa, como faz o registro de classes do Win32.
        static const wchar_t target[] = { 'e', 'd', 'i', 't', 0 };
        for (int i = 0; i < 4; ++i)
        {
            wchar_t character = classe[i];
            if (character >= 'A' && character <= 'Z') character = (wchar_t)(character + 32);
            if (character != target[i]) return false;
        }
        return classe[4] == 0;
    }

    void NormalizeSelection(EditControl& control)
    {
        const size_t size = control.text.size();
        if (control.selectionStart > size) control.selectionStart = size;
        if (control.selectionEnd > size)    control.selectionEnd = size;
        if (control.selectionStart > control.selectionEnd)
        {
            const size_t troca = control.selectionStart;
            control.selectionStart = control.selectionEnd;
            control.selectionEnd = troca;
        }
    }

    // Apaga o trecho selecionado, se houver. Devolve true quando apagou algo.
    bool DeleteSelection(EditControl& control)
    {
        NormalizeSelection(control);
        if (control.selectionStart == control.selectionEnd) return false;
        control.text.erase(control.selectionStart,
                             control.selectionEnd - control.selectionStart);
        control.selectionEnd = control.selectionStart;
        return true;
    }

    void ColocarCaret(EditControl& control, size_t position)
    {
        if (position > control.text.size()) position = control.text.size();
        control.selectionStart = position;
        control.selectionEnd = position;
    }

    // O que o texto MOSTRA: a propria cadeia, ou asteriscos num campo de senha.
    //
    // Um asterisco por caractere, e nao o glifo redondo dos controles novos, porque
    // CUITextInputBox::Render mede a largura com uma cadeia de '*' -- se o desenho
    // usasse outro glifo, o retangulo enviado a textura nao bateria com o pintado.
    std::wstring TextoVisivel(const EditControl& control)
    {
        if ((control.estilo & ES_PASSWORD) == 0) return control.text;
        return std::wstring(control.text.size(), L'*');
    }

    LRESULT DefaultEditProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
}

namespace Platform
{
    void LimparSuperficieDoDc(HDC dcHandle);   // PlatformText.cpp
}

namespace
{
    // O "proc padrao" do EDIT: e o que CallWindowProcW(m_hOldProc, ...) alcanca
    // depois de EditWndProc filtrar a mensagem. Toda a mudanca de estado do controle
    // acontece aqui, como no Windows.
    LRESULT DefaultEditProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        EditControl* found = Edicao(window);
        if (found == NULL) return 0;
        EditControl& control = *found;

        switch (message)
        {
        case WM_CHAR:
        {
            const wchar_t character = (wchar_t)wParam;
            if (character == VK_BACK)
            {
                if (!DeleteSelection(control) && control.selectionEnd > 0)
                {
                    control.text.erase(control.selectionEnd - 1, 1);
                    ColocarCaret(control, control.selectionEnd - 1);
                }
                return 0;
            }
            // Enter, Tab e Escape sao decididos por EditWndProc (confirmar, trocar de
            // campo, fechar) e nao entram no texto. Multilinha e a excecao do Enter.
            if (character == VK_RETURN)
            {
                if ((control.estilo & ES_MULTILINE) == 0) return 0;
                DeleteSelection(control);
                control.text.insert(control.selectionEnd, 1, L'\n');
                ColocarCaret(control, control.selectionEnd + 1);
                return 0;
            }
            if (character == VK_TAB || character == VK_ESCAPE) return 0;
            if (character < 32) return 0;
            DeleteSelection(control);
            if (control.limit > 0 && (int)control.text.size() >= control.limit)
                return 0;
            control.text.insert(control.selectionEnd, 1, character);
            ColocarCaret(control, control.selectionEnd + 1);
            return 0;
        }

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_LEFT:
                NormalizeSelection(control);
                ColocarCaret(control, control.selectionEnd > 0 ? control.selectionEnd - 1 : 0);
                break;
            case VK_RIGHT:
                NormalizeSelection(control);
                ColocarCaret(control, control.selectionEnd + 1);
                break;
            case VK_HOME:
                ColocarCaret(control, 0);
                break;
            case VK_END:
                ColocarCaret(control, control.text.size());
                break;
            case VK_DELETE:
                if (!DeleteSelection(control) && control.selectionEnd < control.text.size())
                    control.text.erase(control.selectionEnd, 1);
                break;
            default:
                break;
            }
            return 0;

        case EM_SETLIMITTEXT:
            control.limit = (int)wParam;
            return 0;

        case EM_GETSEL:
            return (LRESULT)((control.selectionEnd & 0xFFFF) << 16 |
                             (control.selectionStart & 0xFFFF));

        case EM_SETSEL:
            // Convencao do EDIT: fim -1 quer dizer "ate o final". Inicio negativo o
            // cliente usa para "nao selecionar, caret no fim" (GiveFocus(FALSE) manda
            // (-2, -1)).
            if ((LONG_PTR)wParam < 0)
            {
                ColocarCaret(control, control.text.size());
            }
            else
            {
                control.selectionStart = (size_t)wParam;
                control.selectionEnd = (lParam < 0) ? control.text.size() : (size_t)lParam;
                NormalizeSelection(control);
            }
            return 0;

        case EM_REPLACESEL:
        {
            DeleteSelection(control);
            const wchar_t* novo = (const wchar_t*)lParam;
            if (novo != NULL)
            {
                std::wstring text(novo);
                if (control.limit > 0 &&
                    (int)(control.text.size() + text.size()) > control.limit)
                    text.resize((size_t)control.limit - control.text.size());
                control.text.insert(control.selectionEnd, text);
                ColocarCaret(control, control.selectionEnd + text.size());
            }
            return 0;
        }

        case EM_GETLINECOUNT:
        {
            LRESULT lines = 1;
            for (size_t i = 0; i < control.text.size(); ++i)
                if (control.text[i] == L'\n') ++lines;
            return lines;
        }

        case WM_ERASEBKGND:
            // O DC vem em wParam: CUITextInputBox::Render passa a sua DIB de memoria.
            Platform::LimparSuperficieDoDc((HDC)wParam);
            return 1;

        case WM_PAINT:
        {
            HDC dc = (HDC)wParam;
            control.lastDc = dc;
            const std::wstring visivel = TextoVisivel(control);
            if (visivel.empty()) return 0;
            if ((control.estilo & ES_MULTILINE) == 0)
            {
                // Origem da DIB: e de lá que Render recorta e envia para a textura,
                // colocando o resultado no canto do campo.
                TextOutW(dc, 0, 0, visivel.c_str(), (int)visivel.size());
            }
            else
            {
                SIZE lineHeight = { 0, 0 };
                GetTextExtentPoint32W(dc, L"Q", 1, &lineHeight);
                if (lineHeight.cy <= 0) lineHeight.cy = 12;
                int y = 0;
                size_t start = 0;
                while (start <= visivel.size())
                {
                    size_t fim = visivel.find(L'\n', start);
                    if (fim == std::wstring::npos) fim = visivel.size();
                    if (fim > start)
                        TextOutW(dc, 0, y, visivel.c_str() + start, (int)(fim - start));
                    y += (int)lineHeight.cy;
                    if (fim == visivel.size()) break;
                    start = fim + 1;
                }
            }
            return 0;
        }

        case WM_SETFONT:
            // A fonte do desenho e a do DC, que CUITextInputBox::SetFont ja seleciona
            // com SelectObject(m_hMemDC, hFont). Nada a guardar aqui.
            return 0;

        default:
            return 0;
        }
    }

    // Despacha para o proc corrente da janela, como SendMessage faz no Win32: depois
    // do subclass isso e EditWndProc, que filtra e depois chama o proc padrao.
    LRESULT DespacharParaEdicao(EditControl& control, HWND window,
                                UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (control.proc != NULL)
            return control.proc(window, message, wParam, lParam);
        return DefaultEditProc(window, message, wParam, lParam);
    }
}

// ---- Superficie Win32 esperada pelo cliente ---------------------------------

HWND CreateWindowW(LPCWSTR classe, LPCWSTR text, DWORD estilo,
                   int, int, int width, int height,
                   HWND, HMENU, HINSTANCE, LPVOID)
{
    if (!ENomeDeClasseEdit(classe))
    {
        PLATFORM_STUB_ONCE("CreateWindowW (apenas a classe EDIT e emulada)");
        return NULL;
    }
    EditControl* control = new EditControl();
    control->selectionStart = 0;
    control->selectionEnd = 0;
    control->limit = 0;
    control->estilo = estilo;
    // WS_VISIBLE no estilo, mas CUITextInputBox::Init chama ShowWindow(SW_HIDE) logo
    // depois e revela o campo por SetState. Respeitar o estilo aqui deixa o campo
    // coerente caso alguem crie um EDIT sem passar por SetState.
    control->visivel = (estilo & WS_VISIBLE) != 0;
    control->proc = NULL;
    control->userData = 0;
    control->width = width;
    control->height = height;
    control->lastDc = NULL;
    if (text != NULL) control->text.assign(text);
    Registro().insert(control);
    return (HWND)control;
}

BOOL DestroyWindow(HWND window)
{
    EditControl* control = Edicao(window);
    if (control == NULL) return FALSE;
    if (g_focusedField == window) g_focusedField = NULL;
    Registro().erase(control);
    delete control;
    return TRUE;
}

BOOL ShowWindow(HWND window, int comando)
{
    EditControl* control = Edicao(window);
    if (control == NULL) return FALSE;
    const BOOL previous = control->visivel ? TRUE : FALSE;
    control->visivel = (comando != SW_HIDE);
    if (!control->visivel && g_focusedField == window) g_focusedField = NULL;
    return previous;
}

BOOL IsWindowVisible(HWND window)
{
    EditControl* control = Edicao(window);
    return (control != NULL && control->visivel) ? TRUE : FALSE;
}

BOOL SetWindowPos(HWND window, HWND, int, int, int width, int height, UINT sinalizadores)
{
    EditControl* control = Edicao(window);
    if (control == NULL) return FALSE;
    if ((sinalizadores & SWP_NOSIZE) == 0)
    {
        control->width = width;
        control->height = height;
    }
    return TRUE;
}

LONG SetWindowLongW(HWND window, int index, LONG value)
{
    EditControl* control = Edicao(window);
    if (control == NULL) return 0;
    if (index == GWL_WNDPROC)
    {
        // O "proc antigo" devolvido e o proc padrao do EDIT. E dele que
        // CUITextInputBox guarda o ponteiro em m_hOldProc e para quem EditWndProc
        // reenvia toda mensagem que nao filtrou -- o subclass do Win32, igual.
        WNDPROC previous = (control->proc != NULL) ? control->proc : &DefaultEditProc;
        control->proc = (WNDPROC)value;
        return (LONG)previous;
    }
    if (index == GWL_USERDATA)
    {
        const LONG previous = control->userData;
        control->userData = value;
        return previous;
    }
    if (index == GWL_STYLE)
    {
        const LONG previous = (LONG)control->estilo;
        control->estilo = (DWORD)value;
        return previous;
    }
    return 0;
}

LONG SetWindowLongA(HWND window, int index, LONG value)
{
    return SetWindowLongW(window, index, value);
}

LONG GetWindowLongW(HWND window, int index)
{
    EditControl* control = Edicao(window);
    if (control == NULL) return 0;
    if (index == GWL_WNDPROC)  return (LONG)control->proc;
    if (index == GWL_USERDATA) return control->userData;
    if (index == GWL_STYLE)    return (LONG)control->estilo;
    return 0;
}

LONG GetWindowLongA(HWND window, int index)
{
    return GetWindowLongW(window, index);
}

LRESULT CallWindowProcW(WNDPROC proc, HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (proc == NULL) return 0;
    return proc(window, message, wParam, lParam);
}

LRESULT CallWindowProcA(WNDPROC proc, HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProcW(proc, window, message, wParam, lParam);
}

LRESULT SendMessageW(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    EditControl* control = Edicao(window);
    if (control == NULL)
    {
        PLATFORM_STUB_ONCE("SendMessageW (janela sem equivalente fora do Windows)");
        return 0;
    }
    return DespacharParaEdicao(*control, window, message, wParam, lParam);
}

LRESULT SendMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    return SendMessageW(window, message, wParam, lParam);
}

// Sem fila de mensagens: postar e entregar na hora. Para os usos do cliente
// (EM_SETSEL depois de dar foco) o efeito e o mesmo.
BOOL PostMessageW(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (Edicao(window) == NULL) return FALSE;
    SendMessageW(window, message, wParam, lParam);
    return TRUE;
}

BOOL PostMessageA(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    return PostMessageW(window, message, wParam, lParam);
}

BOOL PostMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    return PostMessageW(window, message, wParam, lParam);
}

int GetWindowTextW(HWND window, LPWSTR destination, int size)
{
    if (destination == NULL || size <= 0) return 0;
    destination[0] = 0;
    EditControl* control = Edicao(window);
    if (control == NULL) return 0;
    // O texto devolvido e o REAL, nunca a mascara: e por aqui que a senha digitada
    // chega ao pacote de login.
    int copied = 0;
    for (; copied < size - 1 && copied < (int)control->text.size(); ++copied)
        destination[copied] = control->text[copied];
    destination[copied] = 0;
    return copied;
}

int GetWindowText(HWND window, LPSTR destination, int size)
{
    if (destination == NULL || size <= 0) return 0;
    destination[0] = '\0';
    EditControl* control = Edicao(window);
    if (control == NULL) return 0;
    int copied = 0;
    for (; copied < size - 1 && copied < (int)control->text.size(); ++copied)
    {
        const wchar_t character = control->text[copied];
        destination[copied] = (character < 256) ? (char)character : '?';
    }
    destination[copied] = '\0';
    return copied;
}

int GetWindowTextA(HWND window, LPSTR destination, int size)
{
    return GetWindowText(window, destination, size);
}

BOOL SetWindowTextW(HWND window, LPCWSTR text)
{
    EditControl* control = Edicao(window);
    if (control == NULL) return FALSE;
    control->text.assign(text != NULL ? text : L"");
    if (control->limit > 0 && (int)control->text.size() > control->limit)
        control->text.resize((size_t)control->limit);
    ColocarCaret(*control, control->text.size());
    return TRUE;
}

BOOL SetWindowTextA(HWND window, LPCSTR text)
{
    std::wstring largo;
    if (text != NULL)
        for (int i = 0; text[i] != '\0'; ++i)
            largo.push_back((wchar_t)(unsigned char)text[i]);
    return SetWindowTextW(window, largo.c_str());
}

BOOL GetCaretPos(LPPOINT ponto)
{
    if (ponto == NULL) return FALSE;
    ponto->x = 0;
    ponto->y = 0;
    EditControl* control = Edicao(g_focusedField);
    if (control == NULL) return FALSE;
    // A largura do caret e a do texto que vem ANTES dele, medida com a mesma fonte
    // usada no desenho -- por isso o DC guardado no ultimo WM_PAINT.
    if (control->lastDc != NULL)
    {
        NormalizeSelection(*control);
        const std::wstring visivel = TextoVisivel(*control);
        SIZE medida = { 0, 0 };
        if (control->selectionEnd > 0)
            GetTextExtentPoint32W(control->lastDc, visivel.c_str(),
                                  (int)control->selectionEnd, &medida);
        ponto->x = medida.cx;
    }
    return TRUE;
}

HWND SetFocus(HWND window)
{
    const HWND previous = g_focusedField;
    // Foco em janela que nao e EDIT (a janela principal) apenas limpa o foco de
    // texto: e o que faz o teclado voltar a ser do jogo.
    g_focusedField = (Edicao(window) != NULL) ? window : NULL;
    return previous;
}

HWND GetFocus()
{
    return g_focusedField;
}

int GetScrollPos(HWND, int)
{
    return 0;
}

int SetScrollPos(HWND, int, int, BOOL)
{
    return 0;
}

// ---- Entrada de teclado ----------------------------------------------------

namespace Platform
{
    bool HasTextFieldFocus()
    {
        return Edicao(g_focusedField) != NULL;
    }

    bool SendTextCharacter(unsigned int codigo)
    {
        EditControl* control = Edicao(g_focusedField);
        if (control == NULL)
        {
            return false;
        }
        DespacharParaEdicao(*control, g_focusedField, WM_CHAR, (WPARAM)codigo, 0);
        return true;
    }

    bool SendTextKey(unsigned int codigoVirtual)
    {
        EditControl* control = Edicao(g_focusedField);
        if (control == NULL) return false;
        DespacharParaEdicao(*control, g_focusedField, WM_KEYDOWN, (WPARAM)codigoVirtual, 0);
        return true;
    }
}

#endif  // !_WIN32
