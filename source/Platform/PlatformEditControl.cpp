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
        size_t       inicioSelecao;   // [inicioSelecao, fimSelecao) esta selecionado
        size_t       fimSelecao;      // e tambem e onde fica o caret
        int          limite;          // 0 = sem limite
        DWORD        estilo;
        bool         visivel;
        WNDPROC      proc;            // proc corrente (EditWndProc depois do subclass)
        LONG         dadosDoUsuario;  // GWL_USERDATA: o CUITextInputBox dono
        int          largura;
        int          altura;
        HDC          ultimoDc;        // DC do ultimo WM_PAINT, usado por GetCaretPos
    };

    std::set<ControleDeEdicao*>& Registro()
    {
        static std::set<ControleDeEdicao*> registro;
        return registro;
    }

    HWND g_foco = NULL;

    ControleDeEdicao* Edicao(HWND janela)
    {
        if (janela == NULL) return NULL;
        ControleDeEdicao* controle = (ControleDeEdicao*)janela;
        std::set<ControleDeEdicao*>& registro = Registro();
        return registro.find(controle) == registro.end() ? NULL : controle;
    }

    bool ENomeDeClasseEdit(LPCWSTR classe)
    {
        if (classe == NULL) return false;
        // Comparacao sem diferenciar caixa, como faz o registro de classes do Win32.
        static const wchar_t alvo[] = { 'e', 'd', 'i', 't', 0 };
        for (int i = 0; i < 4; ++i)
        {
            wchar_t caractere = classe[i];
            if (caractere >= 'A' && caractere <= 'Z') caractere = (wchar_t)(caractere + 32);
            if (caractere != alvo[i]) return false;
        }
        return classe[4] == 0;
    }

    void NormalizarSelecao(ControleDeEdicao& controle)
    {
        const size_t tamanho = controle.texto.size();
        if (controle.inicioSelecao > tamanho) controle.inicioSelecao = tamanho;
        if (controle.fimSelecao > tamanho)    controle.fimSelecao = tamanho;
        if (controle.inicioSelecao > controle.fimSelecao)
        {
            const size_t troca = controle.inicioSelecao;
            controle.inicioSelecao = controle.fimSelecao;
            controle.fimSelecao = troca;
        }
    }

    // Apaga o trecho selecionado, se houver. Devolve true quando apagou algo.
    bool ApagarSelecao(ControleDeEdicao& controle)
    {
        NormalizarSelecao(controle);
        if (controle.inicioSelecao == controle.fimSelecao) return false;
        controle.texto.erase(controle.inicioSelecao,
                             controle.fimSelecao - controle.inicioSelecao);
        controle.fimSelecao = controle.inicioSelecao;
        return true;
    }

    void ColocarCaret(ControleDeEdicao& controle, size_t posicao)
    {
        if (posicao > controle.texto.size()) posicao = controle.texto.size();
        controle.inicioSelecao = posicao;
        controle.fimSelecao = posicao;
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

    LRESULT ProcPadraoDeEdicao(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam);
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
    LRESULT ProcPadraoDeEdicao(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam)
    {
        ControleDeEdicao* achado = Edicao(janela);
        if (achado == NULL) return 0;
        ControleDeEdicao& controle = *achado;

        switch (mensagem)
        {
        case WM_CHAR:
        {
            const wchar_t caractere = (wchar_t)wParam;
            if (caractere == VK_BACK)
            {
                if (!ApagarSelecao(controle) && controle.fimSelecao > 0)
                {
                    controle.texto.erase(controle.fimSelecao - 1, 1);
                    ColocarCaret(controle, controle.fimSelecao - 1);
                }
                return 0;
            }
            // Enter, Tab e Escape sao decididos por EditWndProc (confirmar, trocar de
            // campo, fechar) e nao entram no texto. Multilinha e a excecao do Enter.
            if (caractere == VK_RETURN)
            {
                if ((controle.estilo & ES_MULTILINE) == 0) return 0;
                ApagarSelecao(controle);
                controle.texto.insert(controle.fimSelecao, 1, L'\n');
                ColocarCaret(controle, controle.fimSelecao + 1);
                return 0;
            }
            if (caractere == VK_TAB || caractere == VK_ESCAPE) return 0;
            if (caractere < 32) return 0;
            ApagarSelecao(controle);
            if (controle.limite > 0 && (int)controle.texto.size() >= controle.limite)
                return 0;
            controle.texto.insert(controle.fimSelecao, 1, caractere);
            ColocarCaret(controle, controle.fimSelecao + 1);
            return 0;
        }

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_LEFT:
                NormalizarSelecao(controle);
                ColocarCaret(controle, controle.fimSelecao > 0 ? controle.fimSelecao - 1 : 0);
                break;
            case VK_RIGHT:
                NormalizarSelecao(controle);
                ColocarCaret(controle, controle.fimSelecao + 1);
                break;
            case VK_HOME:
                ColocarCaret(controle, 0);
                break;
            case VK_END:
                ColocarCaret(controle, controle.texto.size());
                break;
            case VK_DELETE:
                if (!ApagarSelecao(controle) && controle.fimSelecao < controle.texto.size())
                    controle.texto.erase(controle.fimSelecao, 1);
                break;
            default:
                break;
            }
            return 0;

        case EM_SETLIMITTEXT:
            controle.limite = (int)wParam;
            return 0;

        case EM_GETSEL:
            return (LRESULT)((controle.fimSelecao & 0xFFFF) << 16 |
                             (controle.inicioSelecao & 0xFFFF));

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
                controle.inicioSelecao = (size_t)wParam;
                controle.fimSelecao = (lParam < 0) ? controle.texto.size() : (size_t)lParam;
                NormalizarSelecao(controle);
            }
            return 0;

        case EM_REPLACESEL:
        {
            ApagarSelecao(controle);
            const wchar_t* novo = (const wchar_t*)lParam;
            if (novo != NULL)
            {
                std::wstring texto(novo);
                if (controle.limite > 0 &&
                    (int)(controle.texto.size() + texto.size()) > controle.limite)
                    texto.resize((size_t)controle.limite - controle.texto.size());
                controle.texto.insert(controle.fimSelecao, texto);
                ColocarCaret(controle, controle.fimSelecao + texto.size());
            }
            return 0;
        }

        case EM_GETLINECOUNT:
        {
            LRESULT linhas = 1;
            for (size_t i = 0; i < controle.texto.size(); ++i)
                if (controle.texto[i] == L'\n') ++linhas;
            return linhas;
        }

        case WM_ERASEBKGND:
            // O DC vem em wParam: CUITextInputBox::Render passa a sua DIB de memoria.
            Platform::LimparSuperficieDoDc((HDC)wParam);
            return 1;

        case WM_PAINT:
        {
            HDC dc = (HDC)wParam;
            controle.ultimoDc = dc;
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
                SIZE alturaDeLinha = { 0, 0 };
                GetTextExtentPoint32W(dc, L"Q", 1, &alturaDeLinha);
                if (alturaDeLinha.cy <= 0) alturaDeLinha.cy = 12;
                int y = 0;
                size_t inicio = 0;
                while (inicio <= visivel.size())
                {
                    size_t fim = visivel.find(L'\n', inicio);
                    if (fim == std::wstring::npos) fim = visivel.size();
                    if (fim > inicio)
                        TextOutW(dc, 0, y, visivel.c_str() + inicio, (int)(fim - inicio));
                    y += (int)alturaDeLinha.cy;
                    if (fim == visivel.size()) break;
                    inicio = fim + 1;
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
    LRESULT DespacharParaEdicao(ControleDeEdicao& controle, HWND janela,
                                UINT mensagem, WPARAM wParam, LPARAM lParam)
    {
        if (controle.proc != NULL)
            return controle.proc(janela, mensagem, wParam, lParam);
        return ProcPadraoDeEdicao(janela, mensagem, wParam, lParam);
    }
}

// ---- Superficie Win32 esperada pelo cliente ---------------------------------

HWND CreateWindowW(LPCWSTR classe, LPCWSTR texto, DWORD estilo,
                   int, int, int largura, int altura,
                   HWND, HMENU, HINSTANCE, LPVOID)
{
    if (!ENomeDeClasseEdit(classe))
    {
        PLATFORM_STUB_ONCE("CreateWindowW (apenas a classe EDIT e emulada)");
        return NULL;
    }
    ControleDeEdicao* controle = new ControleDeEdicao();
    controle->inicioSelecao = 0;
    controle->fimSelecao = 0;
    controle->limite = 0;
    controle->estilo = estilo;
    // WS_VISIBLE no estilo, mas CUITextInputBox::Init chama ShowWindow(SW_HIDE) logo
    // depois e revela o campo por SetState. Respeitar o estilo aqui deixa o campo
    // coerente caso alguem crie um EDIT sem passar por SetState.
    controle->visivel = (estilo & WS_VISIBLE) != 0;
    controle->proc = NULL;
    controle->dadosDoUsuario = 0;
    controle->largura = largura;
    controle->altura = altura;
    controle->ultimoDc = NULL;
    if (texto != NULL) controle->texto.assign(texto);
    Registro().insert(controle);
    return (HWND)controle;
}

BOOL DestroyWindow(HWND janela)
{
    ControleDeEdicao* controle = Edicao(janela);
    if (controle == NULL) return FALSE;
    if (g_foco == janela) g_foco = NULL;
    Registro().erase(controle);
    delete controle;
    return TRUE;
}

BOOL ShowWindow(HWND janela, int comando)
{
    ControleDeEdicao* controle = Edicao(janela);
    if (controle == NULL) return FALSE;
    const BOOL anterior = controle->visivel ? TRUE : FALSE;
    controle->visivel = (comando != SW_HIDE);
    if (!controle->visivel && g_foco == janela) g_foco = NULL;
    return anterior;
}

BOOL IsWindowVisible(HWND janela)
{
    ControleDeEdicao* controle = Edicao(janela);
    return (controle != NULL && controle->visivel) ? TRUE : FALSE;
}

BOOL SetWindowPos(HWND janela, HWND, int, int, int largura, int altura, UINT sinalizadores)
{
    ControleDeEdicao* controle = Edicao(janela);
    if (controle == NULL) return FALSE;
    if ((sinalizadores & SWP_NOSIZE) == 0)
    {
        controle->largura = largura;
        controle->altura = altura;
    }
    return TRUE;
}

LONG SetWindowLongW(HWND janela, int indice, LONG valor)
{
    ControleDeEdicao* controle = Edicao(janela);
    if (controle == NULL) return 0;
    if (indice == GWL_WNDPROC)
    {
        // O "proc antigo" devolvido e o proc padrao do EDIT. E dele que
        // CUITextInputBox guarda o ponteiro em m_hOldProc e para quem EditWndProc
        // reenvia toda mensagem que nao filtrou -- o subclass do Win32, igual.
        WNDPROC anterior = (controle->proc != NULL) ? controle->proc : &ProcPadraoDeEdicao;
        controle->proc = (WNDPROC)valor;
        return (LONG)anterior;
    }
    if (indice == GWL_USERDATA)
    {
        const LONG anterior = controle->dadosDoUsuario;
        controle->dadosDoUsuario = valor;
        return anterior;
    }
    if (indice == GWL_STYLE)
    {
        const LONG anterior = (LONG)controle->estilo;
        controle->estilo = (DWORD)valor;
        return anterior;
    }
    return 0;
}

LONG SetWindowLongA(HWND janela, int indice, LONG valor)
{
    return SetWindowLongW(janela, indice, valor);
}

LONG GetWindowLongW(HWND janela, int indice)
{
    ControleDeEdicao* controle = Edicao(janela);
    if (controle == NULL) return 0;
    if (indice == GWL_WNDPROC)  return (LONG)controle->proc;
    if (indice == GWL_USERDATA) return controle->dadosDoUsuario;
    if (indice == GWL_STYLE)    return (LONG)controle->estilo;
    return 0;
}

LONG GetWindowLongA(HWND janela, int indice)
{
    return GetWindowLongW(janela, indice);
}

LRESULT CallWindowProcW(WNDPROC proc, HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    if (proc == NULL) return 0;
    return proc(janela, mensagem, wParam, lParam);
}

LRESULT CallWindowProcA(WNDPROC proc, HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProcW(proc, janela, mensagem, wParam, lParam);
}

LRESULT SendMessageW(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    ControleDeEdicao* controle = Edicao(janela);
    if (controle == NULL)
    {
        PLATFORM_STUB_ONCE("SendMessageW (janela sem equivalente fora do Windows)");
        return 0;
    }
    return DespacharParaEdicao(*controle, janela, mensagem, wParam, lParam);
}

LRESULT SendMessage(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    return SendMessageW(janela, mensagem, wParam, lParam);
}

// Sem fila de mensagens: postar e entregar na hora. Para os usos do cliente
// (EM_SETSEL depois de dar foco) o efeito e o mesmo.
BOOL PostMessageW(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    if (Edicao(janela) == NULL) return FALSE;
    SendMessageW(janela, mensagem, wParam, lParam);
    return TRUE;
}

BOOL PostMessageA(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    return PostMessageW(janela, mensagem, wParam, lParam);
}

BOOL PostMessage(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam)
{
    return PostMessageW(janela, mensagem, wParam, lParam);
}

int GetWindowTextW(HWND janela, LPWSTR destino, int tamanho)
{
    if (destino == NULL || tamanho <= 0) return 0;
    destino[0] = 0;
    ControleDeEdicao* controle = Edicao(janela);
    if (controle == NULL) return 0;
    // O texto devolvido e o REAL, nunca a mascara: e por aqui que a senha digitada
    // chega ao pacote de login.
    int copiados = 0;
    for (; copiados < tamanho - 1 && copiados < (int)controle->texto.size(); ++copiados)
        destino[copiados] = controle->texto[copiados];
    destino[copiados] = 0;
    return copiados;
}

int GetWindowText(HWND janela, LPSTR destino, int tamanho)
{
    if (destino == NULL || tamanho <= 0) return 0;
    destino[0] = '\0';
    ControleDeEdicao* controle = Edicao(janela);
    if (controle == NULL) return 0;
    int copiados = 0;
    for (; copiados < tamanho - 1 && copiados < (int)controle->texto.size(); ++copiados)
    {
        const wchar_t caractere = controle->texto[copiados];
        destino[copiados] = (caractere < 256) ? (char)caractere : '?';
    }
    destino[copiados] = '\0';
    return copiados;
}

int GetWindowTextA(HWND janela, LPSTR destino, int tamanho)
{
    return GetWindowText(janela, destino, tamanho);
}

BOOL SetWindowTextW(HWND janela, LPCWSTR texto)
{
    ControleDeEdicao* controle = Edicao(janela);
    if (controle == NULL) return FALSE;
    controle->texto.assign(texto != NULL ? texto : L"");
    if (controle->limite > 0 && (int)controle->texto.size() > controle->limite)
        controle->texto.resize((size_t)controle->limite);
    ColocarCaret(*controle, controle->texto.size());
    return TRUE;
}

BOOL SetWindowTextA(HWND janela, LPCSTR texto)
{
    std::wstring largo;
    if (texto != NULL)
        for (int i = 0; texto[i] != '\0'; ++i)
            largo.push_back((wchar_t)(unsigned char)texto[i]);
    return SetWindowTextW(janela, largo.c_str());
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
    if (controle->ultimoDc != NULL)
    {
        NormalizarSelecao(*controle);
        const std::wstring visivel = TextoVisivel(*controle);
        SIZE medida = { 0, 0 };
        if (controle->fimSelecao > 0)
            GetTextExtentPoint32W(controle->ultimoDc, visivel.c_str(),
                                  (int)controle->fimSelecao, &medida);
        ponto->x = medida.cx;
    }
    return TRUE;
}

HWND SetFocus(HWND janela)
{
    const HWND anterior = g_foco;
    // Foco em janela que nao e EDIT (a janela principal) apenas limpa o foco de
    // texto: e o que faz o teclado voltar a ser do jogo.
    g_foco = (Edicao(janela) != NULL) ? janela : NULL;
    return anterior;
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
