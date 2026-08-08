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
    struct ControleDeEdicao
    {
        std::wstring texto;
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

    std::set<ControleDeEdicao*>& Registro()
    {
        static std::set<ControleDeEdicao*> registro;
        return registro;
    }

    HWND g_foco = NULL;

    ControleDeEdicao* Edicao(HWND window)
    {
        if (window == NULL) return NULL;
        ControleDeEdicao* controle = (ControleDeEdicao*)window;
        std::set<ControleDeEdicao*>& registro = Registro();
        return registro.find(controle) == registro.end() ? NULL : controle;
    }

    bool ENomeDeClasseEdit(LPCWSTR classe)
    {
        if (classe == NULL) return false;
        // Comparacao sem diferenciar caixa, como faz o registro de classes do Win32.
        static const wchar_t target[] = { 'e', 'd', 'i', 't', 0 };
        for (int i = 0; i < 4; ++i)
        {
            wchar_t caractere = classe[i];
            if (caractere >= 'A' && caractere <= 'Z') caractere = (wchar_t)(caractere + 32);
            if (caractere != target[i]) return false;
        }
        return classe[4] == 0;
    }

    void NormalizeSelection(ControleDeEdicao& controle)
    {
        const size_t size = controle.texto.size();
        if (controle.selectionStart > size) controle.selectionStart = size;
        if (controle.selectionEnd > size)    controle.selectionEnd = size;
        if (controle.selectionStart > controle.selectionEnd)
        {
            const size_t troca = controle.selectionStart;
            controle.selectionStart = controle.selectionEnd;
            controle.selectionEnd = troca;
        }
    }

    // Apaga o trecho selecionado, se houver. Devolve true quando apagou algo.
    bool DeleteSelection(ControleDeEdicao& controle)
    {
        NormalizeSelection(controle);
        if (controle.selectionStart == controle.selectionEnd) return false;
        controle.texto.erase(controle.selectionStart,
                             controle.selectionEnd - controle.selectionStart);
        controle.selectionEnd = controle.selectionStart;
        return true;
    }

    void ColocarCaret(ControleDeEdicao& controle, size_t position)
    {
        if (position > controle.texto.size()) position = controle.texto.size();
        controle.selectionStart = position;
        controle.selectionEnd = position;
    }

    // O que o texto MOSTRA: a propria cadeia, ou asteriscos num campo de senha.
    //
    // Um asterisco por caractere, e nao o glifo redondo dos controles novos, porque
    // CUITextInputBox::Render mede a largura com uma cadeia de '*' -- se o desenho
    // usasse outro glifo, o retangulo enviado a textura nao bateria com o pintado.
    std::wstring TextoVisivel(const ControleDeEdicao& controle)
    {
        if ((controle.estilo & ES_PASSWORD) == 0) return controle.texto;
        return std::wstring(controle.texto.size(), L'*');
    }

    LRESULT ProcPadraoDeEdicao(HWND window, UINT mensagem, WPARAM wParam, LPARAM lParam);
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
    LRESULT ProcPadraoDeEdicao(HWND window, UINT mensagem, WPARAM wParam, LPARAM lParam)
    {
        ControleDeEdicao* achado = Edicao(window);
        if (achado == NULL) return 0;
        ControleDeEdicao& controle = *achado;

        switch (mensagem)
        {
        case WM_CHAR:
        {
            const wchar_t caractere = (wchar_t)wParam;
            if (caractere == VK_BACK)
            {
                if (!DeleteSelection(controle) && controle.selectionEnd > 0)
                {
                    controle.texto.erase(controle.selectionEnd - 1, 1);
                    ColocarCaret(controle, controle.selectionEnd - 1);
                }
                return 0;
            }
            // Enter, Tab e Escape sao decididos por EditWndProc (confirmar, trocar de
            // campo, fechar) e nao entram no texto. Multilinha e a excecao do Enter.
            if (caractere == VK_RETURN)
            {
                if ((controle.estilo & ES_MULTILINE) == 0) return 0;
                DeleteSelection(controle);
                controle.texto.insert(controle.selectionEnd, 1, L'\n');
                ColocarCaret(controle, controle.selectionEnd + 1);
                return 0;
            }
            if (caractere == VK_TAB || caractere == VK_ESCAPE) return 0;
            if (caractere < 32) return 0;
            DeleteSelection(controle);
            if (controle.limit > 0 && (int)controle.texto.size() >= controle.limit)
                return 0;
            controle.texto.insert(controle.selectionEnd, 1, caractere);
            ColocarCaret(controle, controle.selectionEnd + 1);
            return 0;
        }

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_LEFT:
                NormalizeSelection(controle);
                ColocarCaret(controle, controle.selectionEnd > 0 ? controle.selectionEnd - 1 : 0);
                break;
            case VK_RIGHT:
                NormalizeSelection(controle);
                ColocarCaret(controle, controle.selectionEnd + 1);
                break;
            case VK_HOME:
                ColocarCaret(controle, 0);
                break;
            case VK_END:
                ColocarCaret(controle, controle.texto.size());
                break;
            case VK_DELETE:
                if (!DeleteSelection(controle) && controle.selectionEnd < controle.texto.size())
                    controle.texto.erase(controle.selectionEnd, 1);
                break;
            default:
                break;
            }
            return 0;

        case EM_SETLIMITTEXT:
            controle.limit = (int)wParam;
            return 0;

        case EM_GETSEL:
            return (LRESULT)((controle.selectionEnd & 0xFFFF) << 16 |
                             (controle.selectionStart & 0xFFFF));

        case EM_SETSEL:
            // Convencao do EDIT: fim -1 quer dizer "ate o final". Inicio negativo o
            // cliente usa para "nao selecionar, caret no fim" (GiveFocus(FALSE) manda
            // (-2, -1)).
            if ((LONG_PTR)wParam < 0)
            {
                ColocarCaret(controle, controle.texto.size());
            }
            else
            {
                controle.selectionStart = (size_t)wParam;
                controle.selectionEnd = (lParam < 0) ? controle.texto.size() : (size_t)lParam;
                NormalizeSelection(controle);
            }
            return 0;

        case EM_REPLACESEL:
        {
            DeleteSelection(controle);
            const wchar_t* novo = (const wchar_t*)lParam;
            if (novo != NULL)
            {
                std::wstring texto(novo);
                if (controle.limit > 0 &&
                    (int)(controle.texto.size() + texto.size()) > controle.limit)
                    texto.resize((size_t)controle.limit - controle.texto.size());
                controle.texto.insert(controle.selectionEnd, texto);
                ColocarCaret(controle, controle.selectionEnd + texto.size());
            }
            return 0;
        }

        case EM_GETLINECOUNT:
        {
            LRESULT lines = 1;
            for (size_t i = 0; i < controle.texto.size(); ++i)
                if (controle.texto[i] == L'\n') ++lines;
            return lines;
        }

        case WM_ERASEBKGND:
            // O DC vem em wParam: CUITextInputBox::Render passa a sua DIB de memoria.
            Platform::LimparSuperficieDoDc((HDC)wParam);
            return 1;

        case WM_PAINT:
        {
            HDC dc = (HDC)wParam;
            controle.lastDc = dc;
            const std::wstring visivel = TextoVisivel(controle);
            if (visivel.empty()) return 0;
            if ((controle.estilo & ES_MULTILINE) == 0)
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
    LRESULT DespacharParaEdicao(ControleDeEdicao& controle, HWND window,
                                UINT mensagem, WPARAM wParam, LPARAM lParam)
    {
        if (controle.proc != NULL)
            return controle.proc(window, mensagem, wParam, lParam);
        return ProcPadraoDeEdicao(window, mensagem, wParam, lParam);
    }
}

// ---- Superficie Win32 esperada pelo cliente ---------------------------------

HWND CreateWindowW(LPCWSTR classe, LPCWSTR texto, DWORD estilo,
                   int, int, int width, int height,
                   HWND, HMENU, HINSTANCE, LPVOID)
{
    if (!ENomeDeClasseEdit(classe))
    {
        PLATFORM_STUB_ONCE("CreateWindowW (apenas a classe EDIT e emulada)");
        return NULL;
    }
    ControleDeEdicao* controle = new ControleDeEdicao();
    controle->selectionStart = 0;
    controle->selectionEnd = 0;
    controle->limit = 0;
    controle->estilo = estilo;
    // WS_VISIBLE no estilo, mas CUITextInputBox::Init chama ShowWindow(SW_HIDE) logo
    // depois e revela o campo por SetState. Respeitar o estilo aqui deixa o campo
    // coerente caso alguem crie um EDIT sem passar por SetState.
    controle->visivel = (estilo & WS_VISIBLE) != 0;
    controle->proc = NULL;
    controle->userData = 0;
    controle->width = width;
    controle->height = height;
    controle->lastDc = NULL;
    if (texto != NULL) controle->texto.assign(texto);
    Registro().insert(controle);
    return (HWND)controle;
}

BOOL DestroyWindow(HWND window)
{
    ControleDeEdicao* controle = Edicao(window);
    if (controle == NULL) return FALSE;
    if (g_foco == window) g_foco = NULL;
    Registro().erase(controle);
    delete controle;
    return TRUE;
}

BOOL ShowWindow(HWND window, int comando)
{
    ControleDeEdicao* controle = Edicao(window);
    if (controle == NULL) return FALSE;
    const BOOL previous = controle->visivel ? TRUE : FALSE;
    controle->visivel = (comando != SW_HIDE);
    if (!controle->visivel && g_foco == window) g_foco = NULL;
    return previous;
}

BOOL IsWindowVisible(HWND window)
{
    ControleDeEdicao* controle = Edicao(window);
    return (controle != NULL && controle->visivel) ? TRUE : FALSE;
}

BOOL SetWindowPos(HWND window, HWND, int, int, int width, int height, UINT sinalizadores)
{
    ControleDeEdicao* controle = Edicao(window);
    if (controle == NULL) return FALSE;
    if ((sinalizadores & SWP_NOSIZE) == 0)
    {
        controle->width = width;
        controle->height = height;
    }
    return TRUE;
}

LONG SetWindowLongW(HWND window, int index, LONG value)
{
    ControleDeEdicao* controle = Edicao(window);
    if (controle == NULL) return 0;
    if (index == GWL_WNDPROC)
    {
        // O "proc antigo" devolvido e o proc padrao do EDIT. E dele que
        // CUITextInputBox guarda o ponteiro em m_hOldProc e para quem EditWndProc
        // reenvia toda mensagem que nao filtrou -- o subclass do Win32, igual.
        WNDPROC previous = (controle->proc != NULL) ? controle->proc : &ProcPadraoDeEdicao;
        controle->proc = (WNDPROC)value;
        return (LONG)previous;
    }
    if (index == GWL_USERDATA)
    {
        const LONG previous = controle->userData;
        controle->userData = value;
        return previous;
    }
    if (index == GWL_STYLE)
    {
        const LONG previous = (LONG)controle->estilo;
        controle->estilo = (DWORD)value;
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
    ControleDeEdicao* controle = Edicao(window);
    if (controle == NULL) return 0;
    if (index == GWL_WNDPROC)  return (LONG)controle->proc;
    if (index == GWL_USERDATA) return controle->userData;
    if (index == GWL_STYLE)    return (LONG)controle->estilo;
    return 0;
}

LONG GetWindowLongA(HWND window, int index)
{
    return GetWindowLongW(window, index);
}

LRESULT CallWindowProcW(WNDPROC proc, HWND window, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    if (proc == NULL) return 0;
    return proc(window, mensagem, wParam, lParam);
}

LRESULT CallWindowProcA(WNDPROC proc, HWND window, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProcW(proc, window, mensagem, wParam, lParam);
}

LRESULT SendMessageW(HWND window, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    ControleDeEdicao* controle = Edicao(window);
    if (controle == NULL)
    {
        PLATFORM_STUB_ONCE("SendMessageW (janela sem equivalente fora do Windows)");
        return 0;
    }
    return DespacharParaEdicao(*controle, window, mensagem, wParam, lParam);
}

LRESULT SendMessage(HWND window, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    return SendMessageW(window, mensagem, wParam, lParam);
}

// Sem fila de mensagens: postar e entregar na hora. Para os usos do cliente
// (EM_SETSEL depois de dar foco) o efeito e o mesmo.
BOOL PostMessageW(HWND window, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    if (Edicao(window) == NULL) return FALSE;
    SendMessageW(window, mensagem, wParam, lParam);
    return TRUE;
}

BOOL PostMessageA(HWND window, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    return PostMessageW(window, mensagem, wParam, lParam);
}

BOOL PostMessage(HWND window, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    return PostMessageW(window, mensagem, wParam, lParam);
}

int GetWindowTextW(HWND window, LPWSTR destination, int size)
{
    if (destination == NULL || size <= 0) return 0;
    destination[0] = 0;
    ControleDeEdicao* controle = Edicao(window);
    if (controle == NULL) return 0;
    // O texto devolvido e o REAL, nunca a mascara: e por aqui que a senha digitada
    // chega ao pacote de login.
    int copiados = 0;
    for (; copiados < size - 1 && copiados < (int)controle->texto.size(); ++copiados)
        destination[copiados] = controle->texto[copiados];
    destination[copiados] = 0;
    return copiados;
}

int GetWindowText(HWND window, LPSTR destination, int size)
{
    if (destination == NULL || size <= 0) return 0;
    destination[0] = '\0';
    ControleDeEdicao* controle = Edicao(window);
    if (controle == NULL) return 0;
    int copiados = 0;
    for (; copiados < size - 1 && copiados < (int)controle->texto.size(); ++copiados)
    {
        const wchar_t caractere = controle->texto[copiados];
        destination[copiados] = (caractere < 256) ? (char)caractere : '?';
    }
    destination[copiados] = '\0';
    return copiados;
}

int GetWindowTextA(HWND window, LPSTR destination, int size)
{
    return GetWindowText(window, destination, size);
}

BOOL SetWindowTextW(HWND window, LPCWSTR texto)
{
    ControleDeEdicao* controle = Edicao(window);
    if (controle == NULL) return FALSE;
    controle->texto.assign(texto != NULL ? texto : L"");
    if (controle->limit > 0 && (int)controle->texto.size() > controle->limit)
        controle->texto.resize((size_t)controle->limit);
    ColocarCaret(*controle, controle->texto.size());
    return TRUE;
}

BOOL SetWindowTextA(HWND window, LPCSTR texto)
{
    std::wstring largo;
    if (texto != NULL)
        for (int i = 0; texto[i] != '\0'; ++i)
            largo.push_back((wchar_t)(unsigned char)texto[i]);
    return SetWindowTextW(window, largo.c_str());
}

BOOL GetCaretPos(LPPOINT ponto)
{
    if (ponto == NULL) return FALSE;
    ponto->x = 0;
    ponto->y = 0;
    ControleDeEdicao* controle = Edicao(g_foco);
    if (controle == NULL) return FALSE;
    // A largura do caret e a do texto que vem ANTES dele, medida com a mesma fonte
    // usada no desenho -- por isso o DC guardado no ultimo WM_PAINT.
    if (controle->lastDc != NULL)
    {
        NormalizeSelection(*controle);
        const std::wstring visivel = TextoVisivel(*controle);
        SIZE medida = { 0, 0 };
        if (controle->selectionEnd > 0)
            GetTextExtentPoint32W(controle->lastDc, visivel.c_str(),
                                  (int)controle->selectionEnd, &medida);
        ponto->x = medida.cx;
    }
    return TRUE;
}

HWND SetFocus(HWND window)
{
    const HWND previous = g_foco;
    // Foco em janela que nao e EDIT (a janela principal) apenas limpa o foco de
    // texto: e o que faz o teclado voltar a ser do jogo.
    g_foco = (Edicao(window) != NULL) ? window : NULL;
    return previous;
}

HWND GetFocus()
{
    return g_foco;
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
    bool HaFocoDeTexto()
    {
        return Edicao(g_foco) != NULL;
    }

    bool EnviarCaractereDeTexto(unsigned int codigo)
    {
        ControleDeEdicao* controle = Edicao(g_foco);
        if (controle == NULL)
        {
            return false;
        }
        DespacharParaEdicao(*controle, g_foco, WM_CHAR, (WPARAM)codigo, 0);
        return true;
    }

    bool EnviarTeclaDeTexto(unsigned int codigoVirtual)
    {
        ControleDeEdicao* controle = Edicao(g_foco);
        if (controle == NULL) return false;
        DespacharParaEdicao(*controle, g_foco, WM_KEYDOWN, (WPARAM)codigoVirtual, 0);
        return true;
    }
}

#endif  // !_WIN32
