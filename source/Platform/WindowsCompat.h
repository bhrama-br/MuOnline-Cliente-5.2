#pragma once

// Compatibilidade minima para compilar os modulos de jogo fora do Windows
// (Emscripten/WebGL2 e Android/GLES3). No Windows este cabecalho nao faz nada:
// os tipos reais vem de <windows.h>.
//
// O objetivo nao e emular a API Win32, e sim permitir que o codigo compartilhado
// (matematica, formatos, logica de cena) compile enquanto os subsistemas ainda
// dependentes de Win32 sao migrados um a um. Qualquer funcao aqui que apenas
// devolva um valor neutro esta marcada com TODO e deve ser substituida por uma
// implementacao da camada Platform.

#ifdef _WIN32

// No Windows nada e redefinido; <windows.h> continua sendo a fonte da verdade.

#else

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <strings.h>
#include <cerrno>
// mkdir, para CreateDirectory.
#include <sys/stat.h>
#include <sys/types.h>

// Sockets POSIX: o WinSock do legado e mapeado nome a nome, sem emulacao.
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

// Diagnostico de stub. Varias funcoes deste cabecalho nao tem implementacao real
// e devolvem valor neutro — metricas de texto zeradas, config.ini sempre default,
// registro sempre ausente. Sem aviso, isso vira comportamento errado silencioso,
// que e o pior modo de falha possivel numa migracao.
//
// Cada stub reporta uma vez, na primeira chamada, para que o console mostre
// exatamente quais subsistemas o build esta fingindo ter.
#define PLATFORM_STUB_ONCE(name)                                              \
    do {                                                                      \
        static bool s_reported = false;                                        \
        if (!s_reported) {                                                     \
            s_reported = true;                                                 \
            fprintf(stderr, "[Platform] NAO IMPLEMENTADO: %s\n", (name));      \
        }                                                                      \
    } while (0)

typedef int                  BOOL;
typedef unsigned char        BYTE;
typedef unsigned short       WORD;
typedef unsigned int         DWORD;
typedef long                 LONG;
typedef unsigned int         UINT;
typedef int                  INT;
typedef float                FLOAT;
typedef char                 CHAR;
typedef unsigned char        UCHAR;
typedef short                SHORT;
typedef unsigned short       USHORT;
typedef long long            LONGLONG;
typedef unsigned long long   ULONGLONG;
typedef void*                LPVOID;
typedef const void*          LPCVOID;
typedef char*                LPSTR;
typedef const char*          LPCSTR;
typedef char                 TCHAR;
typedef char*                LPTSTR;
typedef const char*          LPCTSTR;
typedef unsigned char*       PBYTE;
typedef DWORD*               LPDWORD;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// Handles opacos: o codigo compartilhado apenas os armazena e compara.
typedef void* HANDLE;
typedef void* HWND;
typedef void* HDC;
typedef void* HGLRC;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HFONT;
typedef void* HBITMAP;
typedef void* HCURSOR;

#ifndef NULL
#define NULL 0
#endif

typedef void                 VOID;
typedef void*                PVOID;

// Extensao do MSVC usada em _define.h como "unsigned __int64". Precisa ser macro,
// nao typedef: "unsigned" nao pode ser combinado com um nome de typedef.
// QWORD nao e definido aqui de proposito, para nao divergir daquele typedef.
#ifndef _MSC_VER
#define __int64 long long
#endif

typedef wchar_t              WCHAR;
typedef wchar_t*             LPWSTR;
typedef const wchar_t*       LPCWSTR;

#ifndef CP_ACP
#define CP_ACP   0
#endif
#ifndef CP_UTF8
#define CP_UTF8  65001
#endif
typedef intptr_t             LRESULT;
typedef uintptr_t            WPARAM;
typedef intptr_t             LPARAM;
typedef intptr_t             INT_PTR;
typedef uintptr_t            UINT_PTR;

typedef struct tagPOINT { LONG x; LONG y; } POINT, *LPPOINT;
typedef struct tagRECT { LONG left; LONG top; LONG right; LONG bottom; } RECT, *LPRECT;
typedef struct tagSIZE { LONG cx; LONG cy; } SIZE, *LPSIZE;

// Anotacoes SAL usadas em assinaturas herdadas do SDK do Windows.
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif

// Nome CRT da Microsoft, ausente fora do Windows.
#define itoa(value, buffer, radix) (sprintf((buffer), "%d", (int)(value)), (buffer))

// WinSock -> POSIX. Os nomes mudam, a semantica e a mesma.
typedef int SOCKET;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
#ifndef WSAEWOULDBLOCK
#define WSAEWOULDBLOCK EWOULDBLOCK
#endif
inline int WSAGetLastError() { return errno; }

#define ZeroMemory(destination, length) memset((destination), 0, (length))
#define __forceinline inline

#ifndef LOBYTE
#define LOBYTE(w) ((BYTE)((w) & 0xff))
#endif
#ifndef HIBYTE
#define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xff))
#endif
#ifndef LOWORD
#define LOWORD(l) ((WORD)((l) & 0xffff))
#endif
#ifndef HIWORD
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xffff))
#endif
#ifndef SW_SHOW
#define SW_SHOW 5
#endif

typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

// Estrutura de I/O sobreposta do Win32; so aparece em assinaturas nao usadas
// pelos alvos portateis.
typedef struct _OVERLAPPED OVERLAPPED, *LPOVERLAPPED;

// min/max: no Windows vem como macro de <windows.h> e o codigo legado usa a forma
// sem qualificacao. Aqui sao funcoes de verdade, para nao colidir com <algorithm>.
// Dois parametros de tipo: o legado mistura int/float/DWORD nas chamadas.
template <typename A, typename B>
inline auto min(const A& a, const B& b) -> decltype(a < b ? a : b) { return a < b ? a : b; }
template <typename A, typename B>
inline auto max(const A& a, const B& b) -> decltype(a > b ? a : b) { return a > b ? a : b; }

typedef long HRESULT;
#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif
#ifndef S_OK
#define S_OK ((HRESULT)0)
#endif

// TODO(Platform): caixas de mensagem. Sem gerenciador de janelas, MessageBox nao
// exibe nada e devolve IDOK; erros fatais precisam de um canal proprio.
#ifndef MB_OK
#define MB_OK           0x00000000
#define MB_YESNO        0x00000004
#define MB_ICONERROR    0x00000010
#define MB_ICONWARNING  0x00000030
#define MB_ICONQUESTION 0x00000020
#endif
#ifndef IDOK
#define IDOK    1
#define IDCANCEL 2
#define IDYES   6
#define IDNO    7
#endif
inline int MessageBoxA(HWND, LPCSTR text, LPCSTR, UINT)
{ fprintf(stderr, "[Platform] MessageBox suprimida: %s\n", text ? text : "");
  return IDOK; }
#define MessageBox MessageBoxA

typedef unsigned char u_char;

// Cabecalhos BMP do Windows: o loader de textura le esses campos do arquivo.
#pragma pack(push, 1)
typedef struct tagBITMAPFILEHEADER
{
    WORD  bfType;
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER;
typedef struct tagBITMAPINFOHEADER
{
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;
typedef struct tagRGBQUAD
{
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;
typedef struct tagBITMAPINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;
typedef struct tagPALETTEENTRY
{
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY;
#pragma pack(pop)

// --- Subsistema de texto -----------------------------------------------------
//
// Estas constantes e tipos alimentam o mini-GDI implementado em PlatformText.cpp
// sobre stb_truetype. Os nomes e valores sao os do SDK do Windows para que as
// chamadas existentes (CreateFont.cpp, UIControls.cpp) compilem sem alteracao.
typedef DWORD COLORREF;
typedef void* HGDIOBJ;

#ifndef RGB
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | ((WORD)((BYTE)(g)) << 8) | ((DWORD)((BYTE)(b)) << 16)))
#endif
#define GetRValue(c) ((BYTE)((c) & 0xFF))
#define GetGValue(c) ((BYTE)(((c) >> 8) & 0xFF))
#define GetBValue(c) ((BYTE)(((c) >> 16) & 0xFF))

#define BI_RGB           0
#define DIB_RGB_COLORS   0
#define DIB_PAL_COLORS   1

#define FW_DONTCARE      0
#define FW_NORMAL        400
#define FW_BOLD          700

#define ANSI_CHARSET        0
#define DEFAULT_CHARSET     1
#define SYMBOL_CHARSET      2
#define HANGUL_CHARSET      129
#define SHIFTJIS_CHARSET    128
#define GB2312_CHARSET      134
#define CHINESEBIG5_CHARSET 136

#define OUT_DEFAULT_PRECIS    0
#define CLIP_DEFAULT_PRECIS   0
#define DEFAULT_QUALITY       0
#define DRAFT_QUALITY         1
#define PROOF_QUALITY         2
#define NONANTIALIASED_QUALITY 3
#define ANTIALIASED_QUALITY   4
#define DEFAULT_PITCH         0
#define FIXED_PITCH           1
#define VARIABLE_PITCH        2
#define FF_DONTCARE           0

#include <ctime>

// Relogio monotonico em milissegundos, equivalente ao GetTickCount do Win32.
inline DWORD GetTickCount()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (DWORD)(now.tv_sec * 1000ULL + now.tv_nsec / 1000000ULL);
}

// Funcoes "seguras" do CRT da Microsoft (Annex K).
//
// Sao FUNCOES, nao macros: cada uma tem duas formas — a explicita (dst, tamanho,
// src) e a template que deduz o tamanho de um array. Macro so atende uma aridade
// e quebra a outra. As versoes abaixo tambem garantem terminacao nula, que
// strncpy nao garante ao truncar.
inline int strcpy_s(char* dst, size_t size, const char* src)
{
    if (dst == NULL || size == 0) return 1;
    if (src == NULL) { dst[0] = '\0'; return 1; }
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
    return 0;
}
template <size_t N> inline int strcpy_s(char (&dst)[N], const char* src)
{
    return strcpy_s(dst, N, src);
}

inline int strncpy_s(char* dst, size_t size, const char* src, size_t count)
{
    if (dst == NULL || size == 0) return 1;
    size_t limit = (count < size - 1) ? count : size - 1;
    if (src == NULL) { dst[0] = '\0'; return 1; }
    strncpy(dst, src, limit);
    dst[limit] = '\0';
    return 0;
}
template <size_t N> inline int strncpy_s(char (&dst)[N], const char* src, size_t count)
{
    return strncpy_s(dst, N, src, count);
}

inline int strcat_s(char* dst, size_t size, const char* src)
{
    if (dst == NULL || size == 0 || src == NULL) return 1;
    size_t used = strlen(dst);
    if (used + 1 >= size) return 1;
    strncat(dst, src, size - used - 1);
    return 0;
}
template <size_t N> inline int strcat_s(char (&dst)[N], const char* src)
{
    return strcat_s(dst, N, src);
}

// -Wformat-security aponta o repasse de `fmt` para snprintf, porque num template
// variadico o compilador nao tem como verificar o formato. Aqui o alerta e falso:
// quem define o formato e o CHAMADOR de sprintf_s, e e la que a verificacao
// acontece. Silenciado apenas neste bloco para nao mascarar chamadas reais.
#if defined(__clang__) || defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wformat-security"
#  pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
template <typename... A> inline int sprintf_s(char* buf, size_t size, const char* fmt, A... args)
{
    return snprintf(buf, size, fmt, args...);
}
template <size_t N, typename... A> inline int sprintf_s(char (&buf)[N], const char* fmt, A... args)
{
    return snprintf(buf, N, fmt, args...);
}
#if defined(__clang__) || defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

inline int memcpy_s(void* dst, size_t size, const void* src, size_t count)
{
    if (dst == NULL || src == NULL || count > size) return 1;
    memcpy(dst, src, count);
    return 0;
}

inline int fopen_s(FILE** out, const char* path, const char* mode)
{
    if (out == NULL) return 1;
    *out = fopen(path, mode);
    return (*out != NULL) ? 0 : 1;
}

#define _snprintf_s(buf, size, cnt, ...) snprintf((buf), (size), __VA_ARGS__)
#define _snprintf                       snprintf
#define sscanf_s                        sscanf
#define _strdup                         strdup

// TODO(Platform): codigo de erro do Win32. Sem SetLastError equivalente, devolve
// o errno corrente, que cobre os usos de I/O e socket.
inline DWORD GetLastError() { return (DWORD)errno; }
inline void SetLastError(DWORD code) { errno = (int)code; }

// ---- strsafe.h --------------------------------------------------------------
//
// Familia StringCch* do Win32: como str*, mas recebendo o tamanho do DESTINO em
// caracteres e sempre terminando em nulo, mesmo quando trunca. Devolvem HRESULT;
// o cliente ignora o retorno em todos os usos, mas ele e reproduzido para o caso
// de alguem passar a testar.
//
// STRSAFE_E_INSUFFICIENT_BUFFER e o codigo de truncamento do Win32.
#ifndef STRSAFE_E_INSUFFICIENT_BUFFER
#define STRSAFE_E_INSUFFICIENT_BUFFER ((HRESULT)0x8007007AL)
#endif

inline HRESULT StringCchCopyA(char* destino, size_t capacidade, const char* origem)
{
    if (destino == NULL || capacidade == 0) return STRSAFE_E_INSUFFICIENT_BUFFER;
    if (origem == NULL) { destino[0] = '\0'; return 0; }

    size_t i = 0;
    while (i + 1 < capacidade && origem[i] != '\0') { destino[i] = origem[i]; ++i; }
    destino[i] = '\0';
    return (origem[i] == '\0') ? 0 : STRSAFE_E_INSUFFICIENT_BUFFER;
}

inline HRESULT StringCchLengthA(const char* texto, size_t capacidade, size_t* tamanho)
{
    if (texto == NULL) return STRSAFE_E_INSUFFICIENT_BUFFER;
    size_t i = 0;
    while (i < capacidade && texto[i] != '\0') ++i;
    if (tamanho != NULL) *tamanho = i;
    return (i < capacidade) ? 0 : STRSAFE_E_INSUFFICIENT_BUFFER;
}

// StringCchPrintf e snprintf com o tamanho do destino em caracteres. snprintf ja
// trunca e termina em nulo, que e exatamente o contrato.
#define StringCchPrintfA snprintf
#define StringCchPrintf  snprintf

// Variante com va_list. Nao pode ser macro para vsnprintf: a ordem dos
// argumentos e a mesma, mas o nome aparece em contextos onde a macro
// atrapalharia a leitura. Inline mantem a assinatura explicita.
inline HRESULT StringCchVPrintfA(char* destino, size_t capacidade,
                                 const char* formato, va_list argumentos)
{
    if (destino == NULL || capacidade == 0) return STRSAFE_E_INSUFFICIENT_BUFFER;
    const int escritos = vsnprintf(destino, capacidade, formato, argumentos);
    return (escritos >= 0 && (size_t)escritos < capacidade)
         ? 0 : STRSAFE_E_INSUFFICIENT_BUFFER;
}
#define StringCchVPrintf StringCchVPrintfA

// Variantes com o sufixo generico: o projeto compila em ANSI, entao apontam para
// a mesma implementacao.
#define StringCchCopy   StringCchCopyA
#define StringCchLength StringCchLengthA

// ---- Diretorio e remocao de arquivo -----------------------------------------
inline BOOL CreateDirectoryA(const char* caminho, void* /*seguranca*/)
{
    if (caminho == NULL) return FALSE;
    // Sucesso se criou OU se ja existia, como o Win32 (que sinaliza a diferenca
    // por GetLastError, e nenhum chamador do cliente consulta).
    if (mkdir(caminho, 0777) == 0) return TRUE;
    return (errno == EEXIST) ? TRUE : FALSE;
}
#define CreateDirectory CreateDirectoryA

inline BOOL DeleteFileA(const char* caminho)
{
    if (caminho == NULL) return FALSE;
    return (remove(caminho) == 0) ? TRUE : FALSE;
}
#define DeleteFile DeleteFileA

// ---- Threads do downloader da loja ------------------------------------------
//
// O cliente usa _beginthreadex + WaitForSingleObject com timeout em UM lugar:
// baixar o script da loja por FTP, em segundo plano, com prazo maximo.
//
// Aqui a rotina roda de forma SINCRONA e WaitForSingleObject devolve
// WAIT_OBJECT_0 na hora. Isso e deliberado, e seguro NESTE caso porque o
// downloader de FTP (FTPFileDownLoader.cpp, sobre WinINet) nao esta portado:
// a rotina falha de imediato em vez de baixar, entao nao ha o que bloquear. O
// caminho de timeout simplesmente nunca e exercitado.
//
// ATENCAO: se algum dia outro subsistema passar a usar estas funcoes para
// trabalho de verdade, esta equivalencia deixa de valer -- rodar sincrono
// travaria o quadro. O registro abaixo torna esse uso visivel em vez de
// silencioso.
#ifndef INFINITE
#define INFINITE      0xFFFFFFFF
#define WAIT_OBJECT_0 0x00000000
#define WAIT_TIMEOUT  0x00000102
#endif

typedef unsigned int (*PlatformThreadRoutine)(void*);

inline uintptr_t _beginthreadex(void* /*seguranca*/, unsigned /*pilha*/,
                                unsigned int (*rotina)(void*), void* argumento,
                                unsigned /*criacao*/, unsigned* idThread)
{
    PLATFORM_STUB_ONCE("_beginthreadex (executando de forma sincrona)");
    if (idThread != NULL) *idThread = 0;
    if (rotina == NULL) return 0;
    rotina(argumento);
    // Handle nao-nulo: o chamador compara com INVALID_HANDLE_VALUE.
    return (uintptr_t)1;
}

inline DWORD WaitForSingleObject(HANDLE /*objeto*/, DWORD /*milissegundos*/)
{
    // A rotina ja terminou em _beginthreadex.
    return WAIT_OBJECT_0;
}

// Modo de wrap de textura removido do GLES3 (use GL_CLAMP_TO_EDGE).
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif

// Traduz o wrap legado para o que o GLES3 aceita.
//
// GL_CLAMP (0x2900) nao existe no GLES3, e dezenas de LoadBitmap do cliente o
// passam. Cada um virava INVALID_ENUM, e a textura ficava com o wrap PADRAO
// (GL_REPEAT) em vez de grampeada -- borda espelhando para o outro lado, sem
// nada no console alem de um aviso de WebGL que passa batido.
//
// A traducao e feita AQUI, em funcao, e nao redefinindo a macro: os cabecalhos
// de `Dependencies/include` (gl/GL.h, glew.h, glad.h) tambem definem GL_CLAMP
// como 0x2900, entao quem vence o `#ifndef` depende da ordem de inclusao de
// cada arquivo -- frágil e silencioso quando erra. O literal aqui nao depende
// de macro nenhuma.
//
// A diferenca real entre os dois modos e a cor de borda, que o cliente nunca
// define; com a borda no padrao o resultado e o mesmo que o driver de desktop
// produzia.
inline unsigned int TraduzirWrapLegado(unsigned int wrap)
{
    // Os dois lados sao literais pelo mesmo motivo: este cabecalho e incluido
    // antes do cabecalho de GL em varios modulos, e GL_CLAMP_TO_EDGE nem sempre
    // esta definido aqui. 0x2900 = GL_CLAMP (legado), 0x812F = GL_CLAMP_TO_EDGE.
    return (wrap == 0x2900u) ? 0x812Fu : wrap;
}

inline void glAlphaFunc(unsigned int, float) {}
#ifndef GL_GREATER
#define GL_GREATER 0x0204
#endif

#define lstrlen  strlen
#define lstrcpy  strcpy
#define lstrcat  strcat
#define lstrcmp  strcmp

typedef struct _SYSTEMTIME
{
    WORD wYear, wMonth, wDayOfWeek, wDay;
    WORD wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

inline void GetLocalTime(LPSYSTEMTIME out)
{
    if (out == NULL) return;
    time_t raw = time(NULL);
    struct tm parts;
    localtime_r(&raw, &parts);
    out->wYear = (WORD)(parts.tm_year + 1900);
    out->wMonth = (WORD)(parts.tm_mon + 1);
    out->wDayOfWeek = (WORD)parts.tm_wday;
    out->wDay = (WORD)parts.tm_mday;
    out->wHour = (WORD)parts.tm_hour;
    out->wMinute = (WORD)parts.tm_min;
    out->wSecond = (WORD)parts.tm_sec;
    out->wMilliseconds = 0;
}

// Codigos de tecla virtual do Win32 usados pelo gameplay. A camada de input ja
// traduz eventos por plataforma; estes valores mantem os mapeamentos existentes.
#ifndef VK_ESCAPE
#define VK_LBUTTON   0x01
#define VK_RBUTTON   0x02
#define VK_BACK      0x08
#define VK_TAB       0x09
#define VK_RETURN    0x0D
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_MENU      0x12
#define VK_ESCAPE    0x1B
#define VK_SPACE     0x20
#define VK_PRIOR     0x21
#define VK_NEXT      0x22
#define VK_END       0x23
#define VK_HOME      0x24
#define VK_LEFT      0x25
#define VK_UP        0x26
#define VK_RIGHT     0x27
#define VK_DOWN      0x28
#define VK_SNAPSHOT  0x2C
#define VK_INSERT    0x2D
#define VK_DELETE    0x2E
#define VK_F1        0x70
#define VK_F2        0x71
#define VK_F3        0x72
#define VK_F4        0x73
#define VK_F5        0x74
#define VK_F6        0x75
#define VK_F7        0x76
#define VK_F8        0x77
#define VK_F9        0x78
#define VK_F10       0x79
#define VK_F11       0x7A
#define VK_F12       0x7B
#define VK_MBUTTON   0x04
#define VK_LSHIFT    0xA0
#define VK_RSHIFT    0xA1
#define VK_LCONTROL  0xA2
#define VK_RCONTROL  0xA3
#define VK_LMENU     0xA4
#define VK_RMENU     0xA5
#endif

// Ponteiros basicos declarados cedo: os blocos de registro e WinInet abaixo
// dependem deles.
typedef BYTE* LPBYTE;
typedef WORD* LPWORD;
typedef void* HINTERNET;

#ifndef WM_USER
#define WM_USER 0x0400
#endif

// Macro de importacao do CRT da Microsoft; sem significado fora do Windows.
#ifndef _CRTIMP
#define _CRTIMP
#endif

typedef uintptr_t ULONG_PTR;
typedef intptr_t  LONG_PTR;
typedef ULONG_PTR DWORD_PTR;
typedef unsigned long ULONG;

// TODO(Platform): handles de arquivo Win32. Os modulos de asset usam stdio; estas
// definicoes existem para os subsistemas Windows-only compilarem, e as operacoes
// devolvem falha em vez de simular sucesso.
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif
inline BOOL CloseHandle(HANDLE)
{ PLATFORM_STUB_ONCE("CloseHandle (I/O Win32 sem implementacao)"); return FALSE; }
inline HANDLE CreateFileA(LPCSTR, DWORD, DWORD, LPVOID, DWORD, DWORD, HANDLE)
{ return INVALID_HANDLE_VALUE; }
#define CreateFile CreateFileA
inline BOOL ReadFile(HANDLE, LPVOID, DWORD, LPDWORD read, LPVOID)
{ if (read != NULL) *read = 0; return FALSE; }
inline BOOL WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD written, LPVOID)
{ if (written != NULL) *written = 0; return FALSE; }
inline DWORD GetFileSize(HANDLE, LPDWORD) { return 0; }
inline DWORD GetFileAttributesA(LPCSTR) { return (DWORD)-1; }
#define GetFileAttributes GetFileAttributesA
#ifndef GENERIC_READ
#define GENERIC_READ          0x80000000
#define GENERIC_WRITE         0x40000000
#define FILE_SHARE_READ       0x00000001
#define FILE_SHARE_WRITE      0x00000002
#define OPEN_EXISTING         3
#define OPEN_ALWAYS           4
#define CREATE_ALWAYS         2
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#endif

// Verificacao de ponteiro do Win32; sem equivalente portatil, assume valido.
inline BOOL IsBadReadPtr(const void* p, UINT_PTR) { return (p == NULL) ? TRUE : FALSE; }
inline BOOL IsBadWritePtr(void* p, UINT_PTR) { return (p == NULL) ? TRUE : FALSE; }

// TODO(Platform): registro do Windows. Guarda de configuracao precisa de
// equivalente (localStorage no Web, SharedPreferences no Android).
typedef void* HKEY;
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0L
#define HKEY_CURRENT_USER  ((HKEY)(intptr_t)0x80000001)
#define HKEY_LOCAL_MACHINE ((HKEY)(intptr_t)0x80000002)
#define KEY_READ  0x20019
#define KEY_WRITE 0x20006
#define REG_DWORD 4
#define REG_SZ    1
#endif
inline LONG RegCloseKey(HKEY) { return ERROR_SUCCESS; }
inline LONG RegOpenKeyExA(HKEY, LPCSTR, DWORD, DWORD, HKEY*) { return 1; }
inline LONG RegCreateKeyExA(HKEY, LPCSTR, DWORD, LPSTR, DWORD, DWORD, LPVOID, HKEY*, LPDWORD) { return 1; }
inline LONG RegQueryValueExA(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD) { return 1; }
inline LONG RegSetValueExA(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD) { return 1; }
#define RegOpenKeyEx   RegOpenKeyExA
#define RegCreateKeyEx RegCreateKeyExA
#define RegQueryValueEx RegQueryValueExA
#define RegSetValueEx  RegSetValueExA

// TODO(Platform): WinInet. Sem cliente HTTP proprio, as chamadas falham.
inline BOOL InternetCloseHandle(HINTERNET) { return FALSE; }


// Nome CRT da Microsoft para tzset.
#define _tzset tzset

// Janela ativa: SEMPRE a superficie de desenho, neste alvo.
//
// Devolvia NULL, e isso nao era neutro -- era destrutivo. O fim de CInput::Update
// (Input.cpp:253-265, dentro de `#if defined WINDOWMODE`, que ESTA definido) faz:
//
//   if (GetActiveWindow() == NULL) {
//       ... = m_bLBtnDn = m_bRBtnDn = ... = false;
//       m_ptCursor.x = m_ptCursor.y = 0;
//       return;
//   }
//
// ou seja, com NULL o cliente zerava TODOS os flags de botao e o cursor no fim de
// cada quadro. O clique chegava do navegador (verificado: evento DOM com buttons=1),
// CInput o registrava, e a ultima linha da funcao o apagava antes de qualquer botao
// olhar. Nada era clicavel, e o motivo estava a tres camadas de distancia do
// sintoma.
//
// Aqui nao existe "outra janela com foco": no navegador e no Android o alvo de
// desenho e o unico. Devolver um handle nao-nulo e o equivalente honesto.
inline HWND GetActiveWindow() { return (HWND)(intptr_t)1; }

// TODO(Platform): GetPrivateProfileString le config.ini e precisa de um parser
// proprio (o valor default e devolvido para nao inventar configuracao).
inline DWORD GetPrivateProfileStringA(LPCSTR, LPCSTR, LPCSTR defaultValue,
                                      LPSTR returned, DWORD size, LPCSTR)
{
    PLATFORM_STUB_ONCE("GetPrivateProfileString (config.ini ignorado, usando defaults)");
    if (returned == NULL || size == 0) return 0;
    const char* src = (defaultValue != NULL) ? defaultValue : "";
    strncpy(returned, src, size - 1);
    returned[size - 1] = '\0';
    return (DWORD)strlen(returned);
}
#define GetPrivateProfileString GetPrivateProfileStringA

inline void Sleep(DWORD milliseconds) { usleep(milliseconds * 1000); }

inline BOOL IntersectRect(LPRECT out, const RECT* a, const RECT* b)
{
    if (out == NULL || a == NULL || b == NULL) return FALSE;
    out->left   = (a->left   > b->left)   ? a->left   : b->left;
    out->top    = (a->top    > b->top)    ? a->top    : b->top;
    out->right  = (a->right  < b->right)  ? a->right  : b->right;
    out->bottom = (a->bottom < b->bottom) ? a->bottom : b->bottom;
    if (out->left >= out->right || out->top >= out->bottom)
    {
        out->left = out->top = out->right = out->bottom = 0;
        return FALSE;
    }
    return TRUE;
}

// Estado de cor corrente do pipeline fixo; o adapter e a fonte da verdade.
#ifndef GL_CURRENT_COLOR
#define GL_CURRENT_COLOR 0x0B00
#endif

typedef void* HICON;
typedef void* HIMC;
typedef void* HMENU;
typedef void* HBRUSH;

// TODO(Platform): IME. A composicao de texto asiatico depende do IME do Windows;
// Web e Android tem seus proprios mecanismos (composition events / InputMethod).
inline HIMC ImmGetContext(HWND)
{ PLATFORM_STUB_ONCE("ImmGetContext (IME indisponivel)"); return NULL; }
inline BOOL ImmReleaseContext(HWND, HIMC) { return FALSE; }
inline BOOL ImmSetCompositionWindow(HIMC, void*) { return FALSE; }
inline LONG ImmGetCompositionStringA(HIMC, DWORD, LPVOID, DWORD) { return 0; }
inline LONG ImmGetCompositionStringW(HIMC, DWORD, LPVOID, DWORD) { return 0; }
// No SDK, ImmGetCompositionString e macro para a variante A ou W conforme
// UNICODE. O cliente compila sem UNICODE, entao aponta para a variante A.
#define ImmGetCompositionString ImmGetCompositionStringA
inline BOOL ImmSetConversionStatus(HIMC, DWORD, DWORD) { return FALSE; }
inline BOOL ImmGetConversionStatus(HIMC, LPDWORD conversion, LPDWORD sentence)
{
    if (conversion != NULL) *conversion = 0;
    if (sentence != NULL) *sentence = 0;
    return FALSE;
}
#ifndef IME_CMODE_NATIVE
#define IME_CMODE_NATIVE  0x0001
#define IME_CMODE_ALPHANUMERIC 0x0000
#define IME_SMODE_NONE    0x0000
#endif

// timeGetTime vem de winmm; mesma base monotonica do GetTickCount.
inline DWORD timeGetTime() { return GetTickCount(); }

#define wsprintf  sprintf
#define wsprintfA sprintf

// TODO(Platform): linha de comando. No Web/Android nao ha equivalente direto.
inline LPSTR GetCommandLineA() { return (LPSTR)""; }
#define GetCommandLine GetCommandLineA

// Teclado assincrono. Implementado em Platform/PlatformKeyboard.cpp sobre uma tabela
// de 256 teclas alimentada pelos backends de plataforma. E por aqui que TODO o teclado
// do cliente passa: SEASON3B::CNewKeyInput::ScanAsyncKeyState varre estas duas funcoes.
SHORT GetAsyncKeyState(int codigoVirtual);
SHORT GetKeyState(int codigoVirtual);

// Comprimento em bytes do caractere multibyte corrente. Sem suporte a DBCS fora
// do Windows, trata tudo como 1 byte.
inline size_t _mbclen(const unsigned char*) { return 1; }

// Foco de teclado. Implementado em Platform/PlatformEditControl.cpp: aponta para o
// campo de texto sintetico que recebe a digitacao (NULL = teclado do jogo).
HWND SetFocus(HWND janela);
inline BOOL SwapBuffers(HDC)
{ PLATFORM_STUB_ONCE("SwapBuffers (use Platform::IRenderContext)"); return TRUE; }

// Fog do pipeline fixo: nao existe em GLES3, o valor serve so para compilar os
// pontos que ainda referenciam a constante.
#ifndef GL_FOG
#define GL_FOG 0x0B60
#define GL_FOG_MODE 0x0B65
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_COLOR 0x0B66
#endif

// Mensagens de janela. Sem fila Win32, so ha um destino possivel: o controle EDIT
// sintetico de Platform/PlatformEditControl.cpp. Para qualquer outro handle (a
// janela principal) continuam sendo no-op -- o encerramento e o redimensionamento
// passam por Platform::IWindow.
LRESULT SendMessage(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam);
BOOL PostMessage(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam);
inline void PostQuitMessage(int) {}

// Empacotamento de versao usado pelo WinSock (WSAStartup).
#ifndef MAKEWORD
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | (((WORD)((BYTE)(b))) << 8)))
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | (((DWORD)((WORD)(b))) << 16)))
#endif

// Projeto sempre compila em char; TEXT() e identidade.
#ifndef TEXT
#define TEXT(quote) quote
#define _T(quote)   quote
#endif

inline BOOL SetRect(LPRECT rect, int left, int top, int right, int bottom)
{
    if (rect == NULL) return FALSE;
    rect->left = left; rect->top = top;
    rect->right = right; rect->bottom = bottom;
    return TRUE;
}
inline BOOL SetRectEmpty(LPRECT rect) { return SetRect(rect, 0, 0, 0, 0); }

// TODO(Platform): hook de mouse do Win32. A UI le esta struct nos handlers; o
// estado real deve vir do IInputBackend.
typedef struct tagMOUSEHOOKSTRUCT
{
    POINT pt;
    HWND  hwnd;
    UINT  wHitTestCode;
    UINT_PTR dwExtraInfo;
} MOUSEHOOKSTRUCT, *LPMOUSEHOOKSTRUCT;

typedef struct tagMOUSEHOOKSTRUCTEX
{
    MOUSEHOOKSTRUCT mouseHookStruct;
    DWORD mouseData;
} MOUSEHOOKSTRUCTEX, *LPMOUSEHOOKSTRUCTEX;

// Ambiente de textura do pipeline fixo (GL_MODULATE): no shader a multiplicacao
// entre textura e cor de vertice ja e explicita, entao aqui e no-op.
inline void glTexEnvf(unsigned int, unsigned int, float) {}
inline void glTexEnvi(unsigned int, unsigned int, int) {}

#include <cassert>
#ifndef _ASSERT
#define _ASSERT(expression) assert(expression)
#endif
#ifndef _ASSERTE
#define _ASSERTE(expression) assert(expression)
#endif

// TODO(Platform): encerramento de processo e mensagens de janela Win32.
inline void ExitProcess(unsigned int code) { std::exit((int)code); }
#ifndef WM_DESTROY
#define WM_DESTROY 0x0002
#define WM_CLOSE   0x0010
#define WM_QUIT    0x0012
#endif

// Codigos de mensagem de janela usados pelo despacho de teclado/IME de
// CUITextInputBox. Sem fila Win32 nenhuma delas chega, mas os valores precisam
// existir para o switch compilar — e sao os valores reais do SDK, para o caso de
// a camada Platform passar a sintetizar essas mensagens.
#ifndef WM_KEYDOWN
#define WM_KEYDOWN               0x0100
#define WM_KEYUP                 0x0101
#define WM_CHAR                  0x0102
#define WM_SYSKEYDOWN            0x0104
#define WM_SYSKEYUP              0x0105
#define WM_SYSCHAR               0x0106
#define WM_IME_STARTCOMPOSITION  0x010D
#define WM_IME_ENDCOMPOSITION    0x010E
#define WM_IME_COMPOSITION       0x010F
#define WM_IME_SETCONTEXT        0x0281
#define WM_IME_NOTIFY            0x0282
#define WM_IME_CHAR              0x0286
#endif

// Indices de SetWindowLong/GetWindowLong.
#ifndef GWL_WNDPROC
#define GWL_WNDPROC   (-4)
#define GWL_HINSTANCE (-6)
#define GWL_HWNDPARENT (-8)
#define GWL_STYLE     (-16)
#define GWL_EXSTYLE   (-20)
#define GWL_USERDATA  (-21)
#define GWL_ID        (-12)
#endif

// TODO(Platform): area de transferencia. O Web tem navigator.clipboard, que e
// assincrono e exige permissao, e o Android tem ClipboardManager; nenhum dos
// dois encaixa nesta API sincrona. Ate existir um equivalente na camada
// Platform, colar nao traz texto — OpenClipboard falhando e o caminho que o
// proprio codigo ja trata.
typedef void* HGLOBAL;
#ifndef CF_TEXT
#define CF_TEXT        1
#define CF_UNICODETEXT 13
#endif
inline BOOL OpenClipboard(HWND)
{ PLATFORM_STUB_ONCE("OpenClipboard (colar do sistema indisponivel)"); return FALSE; }
inline BOOL CloseClipboard() { return FALSE; }
inline BOOL EmptyClipboard() { return FALSE; }
inline HGLOBAL GetClipboardData(UINT) { return NULL; }
inline HANDLE SetClipboardData(UINT, HANDLE) { return NULL; }
inline LPVOID GlobalLock(HGLOBAL memory) { return memory; }
inline BOOL GlobalUnlock(HGLOBAL) { return FALSE; }
inline HGLOBAL GlobalAlloc(UINT, size_t size) { return (HGLOBAL)::malloc(size); }
inline HGLOBAL GlobalFree(HGLOBAL memory) { ::free(memory); return NULL; }

// Janelas filhas do Win32. CUITextInputBox cria um EDIT nativo para guardar,
// desenhar e receber o texto digitado. Fora do Windows nao existe janela filha, mas
// existe um EDIT SINTETICO com esse mesmo contrato, em
// Platform/PlatformEditControl.cpp -- inclusive o subclass por GWL_WNDPROC, de que o
// cliente depende para filtrar teclas.
BOOL DestroyWindow(HWND janela);
LONG SetWindowLongW(HWND janela, int indice, LONG valor);
LONG SetWindowLongA(HWND janela, int indice, LONG valor);
LONG GetWindowLongW(HWND janela, int indice);
LONG GetWindowLongA(HWND janela, int indice);
LRESULT CallWindowProcW(WNDPROC proc, HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam);
LRESULT CallWindowProcA(WNDPROC proc, HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam);

// Estilos e mensagens do controle EDIT nativo, usados por CUITextInputBox::Init.
// Os valores sao os do SDK; sem CreateWindowW real nenhum deles tem efeito, mas
// mante-los corretos evita que a migracao do controle precise reescreve-los.
typedef void* HMENU;
#ifndef WS_CHILD
#define WS_CHILD        0x40000000L
#define WS_VISIBLE      0x10000000L
#define WS_VSCROLL      0x00200000L
#define WS_HSCROLL      0x00100000L
#endif
#ifndef ES_PASSWORD
#define ES_LEFT         0x0000L
#define ES_MULTILINE    0x0004L
#define ES_AUTOVSCROLL  0x0040L
#define ES_AUTOHSCROLL  0x0080L
#define ES_PASSWORD     0x0020L
#define ES_NUMBER       0x2000L
#endif
#ifndef EM_SETSEL
#define EM_GETSEL       0x00B0
#define EM_SETSEL       0x00B1
#define EM_SETLIMITTEXT 0x00C5
#define EM_REPLACESEL   0x00C2
#endif
#ifndef SWP_NOMOVE
#define SWP_NOSIZE      0x0001
#define SWP_NOMOVE      0x0002
#define SWP_NOZORDER    0x0004
#define SWP_NOACTIVATE  0x0010
#define SWP_SHOWWINDOW  0x0040
#endif
#ifndef SW_HIDE
#define SW_HIDE         0
#define SW_SHOWNORMAL   1
#define SW_SHOW         5
#define SW_NORMAL       1
#endif
#ifndef IMN_SETOPENSTATUS
#define IMN_CLOSESTATUSWINDOW 0x0001
#define IMN_OPENSTATUSWINDOW  0x0002
#define IMN_SETCONVERSIONMODE 0x0006
#define IMN_SETSENTENCEMODE   0x0007
#define IMN_SETOPENSTATUS     0x0008
#endif
#ifndef CFS_FORCE_POSITION
#define CFS_DEFAULT         0x0000
#define CFS_POINT           0x0002
#define CFS_FORCE_POSITION  0x0020
#endif

// Janela de composicao do IME. A struct precisa existir com os mesmos campos
// porque o codigo le e escreve ptCurrentPos diretamente.
typedef struct tagCOMPOSITIONFORM
{
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
} COMPOSITIONFORM;

// Todas implementadas em Platform/PlatformEditControl.cpp. CreateWindowW so conhece
// a classe "edit"; qualquer outra continua devolvendo NULL.
HWND CreateWindowW(LPCWSTR classe, LPCWSTR texto, DWORD estilo, int x, int y,
                   int largura, int altura, HWND pai, HMENU menu, HINSTANCE instancia,
                   LPVOID parametro);
BOOL ShowWindow(HWND janela, int comando);
BOOL SetWindowPos(HWND janela, HWND depoisDe, int x, int y, int largura, int altura, UINT sinalizadores);
LRESULT SendMessageW(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam);
int  GetWindowText(HWND janela, LPSTR destino, int tamanho);
int  GetWindowTextA(HWND janela, LPSTR destino, int tamanho);
int  GetWindowTextW(HWND janela, LPWSTR destino, int tamanho);
BOOL SetWindowTextW(HWND janela, LPCWSTR texto);
BOOL SetWindowTextA(HWND janela, LPCSTR texto);
BOOL GetCaretPos(LPPOINT ponto);
inline HDC GetDC(HWND) { return NULL; }
inline int ReleaseDC(HWND, HDC) { return 0; }
BOOL IsWindowVisible(HWND janela);
BOOL PostMessageW(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam);
BOOL PostMessageA(HWND janela, UINT mensagem, WPARAM wParam, LPARAM lParam);
int  GetScrollPos(HWND janela, int barra);
int  SetScrollPos(HWND janela, int barra, int posicao, BOOL redesenhar);
// TODO(Platform): temporizador do Win32. O loop de quadro ja tem relogio proprio
// (GetTickCount); quem depender de WM_TIMER precisa migrar para ele.
inline UINT_PTR SetTimer(HWND, UINT_PTR id, UINT, void*)
{ PLATFORM_STUB_ONCE("SetTimer"); return id; }
inline BOOL KillTimer(HWND, UINT_PTR) { return FALSE; }
#ifndef GCS_COMPSTR
#define GCS_COMPREADSTR    0x0001
#define GCS_COMPSTR        0x0008
#define GCS_COMPATTR       0x0010
#define GCS_CURSORPOS      0x0080
#define GCS_RESULTSTR      0x0800
#endif
inline BOOL ImmGetCompositionWindow(HIMC, COMPOSITIONFORM* form)
{
    if (form != NULL)
    {
        form->dwStyle = 0;
        form->ptCurrentPos.x = 0; form->ptCurrentPos.y = 0;
        form->rcArea.left = 0; form->rcArea.top = 0;
        form->rcArea.right = 0; form->rcArea.bottom = 0;
    }
    return FALSE;
}

#ifndef WM_PAINT
#define WM_PAINT        0x000F
#define WM_ERASEBKGND   0x0014
#define WM_SETFONT      0x0030
#define WM_GETFONT      0x0031
#endif
#ifndef SB_VERT
#define SB_HORZ         0
#define SB_VERT         1
#define SB_CTL          2
#define SB_LINEUP       0
#define SB_LINEDOWN     1
#define SB_PAGEUP       2
#define SB_PAGEDOWN     3
#define SB_THUMBPOSITION 4
#define SB_THUMBTRACK   5
#define SB_TOP          6
#define SB_BOTTOM       7
#endif
#ifndef EM_GETLINECOUNT
#define EM_GETLINECOUNT  0x00BA
#define EM_LINESCROLL    0x00B6
#define EM_SCROLL        0x00B5
#define EM_LINEFROMCHAR  0x00C9
#define EM_LINEINDEX     0x00BB
#define EM_GETFIRSTVISIBLELINE 0x00CE
#endif

inline BOOL PtInRect(const RECT* rect, POINT point)
{
    if (rect == NULL) return FALSE;
    return (point.x >= rect->left && point.x < rect->right &&
            point.y >= rect->top  && point.y < rect->bottom) ? TRUE : FALSE;
}

// Combinador de textura do pipeline fixo; no shader a multiplicacao e explicita.
#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#endif

// glPolygonMode nao existe em GLES3: o modo de debug wireframe precisa de shader.
#ifndef GL_FILL
#define GL_FILL 0x1B02
#define GL_LINE 0x1B01
#define GL_FRONT_AND_BACK 0x0408
#endif
inline void glPolygonMode(unsigned int, unsigned int) {}

// Alpha test e fog do pipeline fixo. Ambos viraram codigo de shader no
// GlslLegacyRenderAdapter (discard por uAlphaTest, mistura linear por uFog); as
// constantes ficam para os modulos que ainda as mencionam compilarem.
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_FOG_START
#define GL_FOG_START 0x0B63
#define GL_FOG_END   0x0B64
#define GL_FOG_MODE  0x0B65
#define GL_FOG_COLOR 0x0B66
#define GL_FOG_DENSITY 0x0B62
#endif
#ifndef GL_ADD
#define GL_ADD 0x0104
#endif
// Roteados por SetLegacyFog em ZzzOpenglUtil.cpp; aqui sao no-op para nao
// duplicar o estado que ja vive no adapter.
inline void glFogf(unsigned int, float) {}
inline void glFogfv(unsigned int, const float*) {}
inline void glFogi(unsigned int, int) {}

// TODO(Platform): WinInet. Apenas os tipos e limites usados nas assinaturas do
// downloader da loja, para que os cabecalhos compilem. Nao ha implementacao:
// Web/Android precisam de um cliente HTTP proprio na camada Platform.
typedef unsigned short INTERNET_PORT;
#ifndef INTERNET_MAX_URL_LENGTH
#define INTERNET_MAX_URL_LENGTH 2084
#endif
#ifndef INTERNET_MAX_USER_NAME_LENGTH
#define INTERNET_MAX_USER_NAME_LENGTH 128
#endif
#ifndef INTERNET_MAX_PASSWORD_LENGTH
#define INTERNET_MAX_PASSWORD_LENGTH 128
#endif

// Foco de teclado: o campo de texto sintetico que esta recebendo digitacao, ou NULL.
// CUITextInputBox::WriteText usa isto para piscar o caret so no campo ativo.
// O caminho do teclado para o campo em foco esta declarado em Platform/PlatformInput.h
// (Platform::EnviarCaractereDeTexto / EnviarTeclaDeTexto), onde nao ha tipo Win32.
HWND GetFocus();

// Texto GDI, implementado de verdade em Platform/PlatformText.cpp sobre
// stb_truetype. Antes estas funcoes eram stubs que devolviam metricas ZERADAS,
// o que quebrava todo o layout de texto da UI. O subconjunto abaixo e exatamente
// o que CUIRenderTextOriginal e CMultiLanguage usam:
//
//   Create()     -> CreateDIBSection + CreateCompatibleDC + SelectObject
//   medicao      -> GetTextExtentPoint32W/A
//   desenho      -> SetBkColor + SetTextColor + TextOutW/A
//
// O DC e a DIB sao objetos proprios da camada Platform, nao handles do Windows;
// os tipos HDC/HBITMAP/HFONT sao void* opacos e so trafegam por aqui.
HFONT   CreateFontA(int height, int width, int escapement, int orientation,
                    int weight, DWORD italic, DWORD underline, DWORD strikeOut,
                    DWORD charSet, DWORD outputPrecision, DWORD clipPrecision,
                    DWORD quality, DWORD pitchAndFamily, LPCSTR faceName);
#define CreateFont CreateFontA
HDC     CreateCompatibleDC(HDC reference);
HBITMAP CreateDIBSection(HDC reference, const BITMAPINFO* info, UINT usage,
                         void** bits, HANDLE section, DWORD offset);
HGDIOBJ SelectObject(HDC dc, HGDIOBJ object);
BOOL    DeleteObject(HGDIOBJ object);
BOOL    DeleteDC(HDC dc);
COLORREF SetTextColor(HDC dc, COLORREF color);
COLORREF SetBkColor(HDC dc, COLORREF color);
BOOL    TextOutA(HDC dc, int x, int y, LPCSTR text, int length);
BOOL    TextOutW(HDC dc, int x, int y, LPCWSTR text, int length);
BOOL    GetTextExtentPoint32A(HDC dc, LPCSTR text, int length, LPSIZE size);
BOOL    GetTextExtentPoint32W(HDC dc, LPCWSTR text, int length, LPSIZE size);
BOOL    GetTextExtentPointA(HDC dc, LPCSTR text, int length, LPSIZE size);

// Conversao de code page, usada por CMultiLanguage para chegar em UTF-16/32.
// Suporta CP_UTF8 e trata qualquer outra code page como Latin-1, que e o
// comportamento seguro para os textos do cliente que nao sao UTF-8.
int MultiByteToWideChar(UINT codePage, DWORD flags, LPCSTR source, int sourceLength,
                        LPWSTR destination, int destinationLength);
int WideCharToMultiByte(UINT codePage, DWORD flags, LPCWSTR source, int sourceLength,
                        LPSTR destination, int destinationLength,
                        LPCSTR defaultChar, BOOL* usedDefaultChar);

namespace Platform
{
    // Aponta o rasterizador para um arquivo TTF. Precisa ser chamado antes do
    // primeiro CreateFontA; sem isso nao ha glifos e as metricas voltam a ser
    // zero. Se `path` for NULL, tenta a lista de candidatos padrao
    // (Data/Fonts/GameFont.ttf e as fontes de sistema do Android).
    bool LoadTextFont(const char* path);
    // true quando ha uma fonte carregada e o texto pode ser medido/desenhado.
    bool IsTextFontLoaded();
}

// Secao critica real, recursiva como a do Win32. Antes era no-op, o que so era
// seguro enquanto todo o caminho de render fosse de thread unica.
//
// O mutex vive DENTRO da struct, nao no heap: o codigo legado copia objetos que
// contem CRITICAL_SECTION (CCriticalSection nao tem construtor de copia), e com
// um ponteiro compartilhado a copia destruia o mutex do original.
struct CRITICAL_SECTION
{
    pthread_mutex_t mutex;
    int             initialized;
};
typedef CRITICAL_SECTION* LPCRITICAL_SECTION;
void InitializeCriticalSection(LPCRITICAL_SECTION section);
void DeleteCriticalSection(LPCRITICAL_SECTION section);
void EnterCriticalSection(LPCRITICAL_SECTION section);
void LeaveCriticalSection(LPCRITICAL_SECTION section);

// --- CRT da Microsoft e limites de caminho -----------------------------------
#ifndef _MAX_PATH
#define _MAX_PATH   260
#endif
#ifndef _MAX_DRIVE
#define _MAX_DRIVE  3
#define _MAX_DIR    256
#define _MAX_FNAME  256
#define _MAX_EXT    256
#endif

#define _strcmpi  strcasecmp
#define _strnicmp strncasecmp

// Maiusculiza no lugar. O CRT da Microsoft devolve o proprio ponteiro.
inline char* _strupr(char* text)
{
    if (text == NULL) return text;
    for (char* p = text; *p != '\0'; ++p)
        *p = (char)toupper((unsigned char)*p);
    return text;
}
inline int _ultoa_s(unsigned long value, char* buffer, size_t size, int radix)
{
    if (buffer == NULL || size == 0) return 1;
    // Somente as bases que o cliente usa; outras nao aparecem no codigo.
    if (radix == 10) return snprintf(buffer, size, "%lu", value) < 0 ? 1 : 0;
    if (radix == 16) return snprintf(buffer, size, "%lx", value) < 0 ? 1 : 0;
    buffer[0] = '\0';
    return 1;
}
// Forma que deduz o tamanho do array, como as demais funcoes "_s" do CRT.
template <size_t N>
inline int _ultoa_s(unsigned long value, char (&buffer)[N], int radix)
{ return _ultoa_s(value, buffer, N, radix); }

// `CONST` e `_asm` sao extensoes da MSVC. O bloco de assembly inline que o
// cliente usa e x86 de 32 bits e nao tem equivalente aqui; onde aparece, o
// codigo ja esta guardado por _WIN32.
#ifndef CONST
#define CONST const
#endif

// Decompoe um caminho no estilo do CRT da Microsoft. Qualquer ponteiro de saida
// pode ser NULL, e o chamador reserva _MAX_DRIVE/_MAX_DIR/_MAX_FNAME/_MAX_EXT.
inline void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext)
{
    if (drive != NULL) drive[0] = '\0';
    if (dir   != NULL) dir[0]   = '\0';
    if (fname != NULL) fname[0] = '\0';
    if (ext   != NULL) ext[0]   = '\0';
    if (path == NULL) return;

    // Aceita os dois separadores: os caminhos do cliente sao escritos com '\'.
    const char* lastSlash = NULL;
    for (const char* p = path; *p != '\0'; ++p)
        if (*p == '/' || *p == '\\') lastSlash = p;

    const char* nameStart = (lastSlash != NULL) ? lastSlash + 1 : path;
    if (dir != NULL && lastSlash != NULL)
    {
        const size_t length = (size_t)(lastSlash + 1 - path);
        memcpy(dir, path, length);
        dir[length] = '\0';
    }

    const char* dot = strrchr(nameStart, '.');
    const size_t nameLength = (dot != NULL) ? (size_t)(dot - nameStart) : strlen(nameStart);
    if (fname != NULL)
    {
        memcpy(fname, nameStart, nameLength);
        fname[nameLength] = '\0';
    }
    if (ext != NULL && dot != NULL)
        strcpy(ext, dot);
}
inline void _splitpath_s(const char* path, char* drive, size_t, char* dir, size_t,
                         char* fname, size_t, char* ext, size_t)
{ _splitpath(path, drive, dir, fname, ext); }

inline DWORD GetCurrentDirectoryA(DWORD size, LPSTR buffer)
{
    if (buffer == NULL || size == 0) return 0;
    if (getcwd(buffer, size) == NULL) { buffer[0] = '\0'; return 0; }
    return (DWORD)strlen(buffer);
}
#define GetCurrentDirectory GetCurrentDirectoryA

inline BOOL OffsetRect(LPRECT rect, int dx, int dy)
{
    if (rect == NULL) return FALSE;
    rect->left += dx; rect->right  += dx;
    rect->top  += dy; rect->bottom += dy;
    return TRUE;
}

inline char* _itoa(int value, char* buffer, int radix)
{
    if (buffer == NULL) return buffer;
    if (radix == 16) sprintf(buffer, "%x", (unsigned int)value);
    else             sprintf(buffer, "%d", value);
    return buffer;
}
inline BOOL SetCursorPos(int, int)
{ PLATFORM_STUB_ONCE("SetCursorPos (cursor do sistema indisponivel)"); return FALSE; }

// TODO(Platform): informacoes de sistema e volume. Sem equivalente direto; o
// cliente as usa para identificacao de maquina, que e caminho Windows-only.
typedef struct
{
    WORD  wProcessorArchitecture;
    WORD  wReserved;
    DWORD dwPageSize;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    WORD  wProcessorLevel;
    WORD  wProcessorRevision;
} SYSTEM_INFO;
inline void GetSystemInfo(SYSTEM_INFO* info)
{
    PLATFORM_STUB_ONCE("GetSystemInfo");
    if (info != NULL) memset(info, 0, sizeof(*info));
}
typedef struct { DWORD Data1; WORD Data2; WORD Data3; BYTE Data4[8]; } UUID;
inline long UuidCreateSequential(UUID* id)
{
    PLATFORM_STUB_ONCE("UuidCreateSequential (identificacao de maquina indisponivel)");
    if (id != NULL) memset(id, 0, sizeof(*id));
    return 1;
}
inline BOOL GetVolumeInformation(LPCSTR, LPSTR, DWORD, LPDWORD serial, LPDWORD, LPDWORD, LPSTR, DWORD)
{
    PLATFORM_STUB_ONCE("GetVolumeInformation");
    if (serial != NULL) *serial = 0;
    return FALSE;
}
typedef struct
{
    DWORD dwFileAttributes;
    char  cFileName[MAX_PATH];
} WIN32_FIND_DATA, *LPWIN32_FIND_DATA;

// Varredura de diretorio do Win32, implementada sobre opendir/readdir em
// Platform/LegacyDirectoryScan.cpp. A assinatura do Win32 e mantida porque o unico
// chamador que importa (CLuaOpenFolder::LoadFolder) e codigo compartilhado com o PC.
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif
HANDLE FindFirstFile(LPCSTR padrao, LPWIN32_FIND_DATA dados);
BOOL   FindNextFile(HANDLE handle, LPWIN32_FIND_DATA dados);
BOOL   FindClose(HANDLE handle);
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE   0x00000020
#define FILE_ATTRIBUTE_NORMAL    0x00000080
#endif
#ifndef KEY_ALL_ACCESS
#define KEY_ALL_ACCESS 0xF003F
#define KEY_READ       0x20019
#define KEY_WRITE      0x20006
#endif

// Atributos de cor do console do Windows, usados so por logs de depuracao.
#ifndef FOREGROUND_BLUE
#define FOREGROUND_BLUE      0x0001
#define FOREGROUND_GREEN     0x0002
#define FOREGROUND_RED       0x0004
#define FOREGROUND_INTENSITY 0x0008
#endif

// Notificacoes de socket assincrono do WinSock. A rede usa ASIO; estes valores
// existem so para as tabelas de despacho compilarem.
#ifndef FD_READ
#define FD_READ    0x01
#define FD_WRITE   0x02
#define FD_CONNECT 0x10
#define FD_CLOSE   0x20
#endif

// MCI (reproducao de midia) e IME adicionais.
#ifndef MCI_SEQ_MAPPER
#define MCI_SEQ_MAPPER 0xFFFFFFFF
#endif
#ifndef WM_TIMER
#define WM_TIMER 0x0113
#endif
#ifndef WM_IME_CONTROL
#define WM_IME_CONTROL 0x0283
#endif
#ifndef IMC_SETCOMPOSITIONWINDOW
#define IMC_SETCOMPOSITIONWINDOW 0x000C
#endif
#ifndef IME_SMODE_AUTOMATIC
#define IME_SMODE_AUTOMATIC 0x0004
#endif
inline HWND ImmGetDefaultIMEWnd(HWND) { return NULL; }

// TODO(Platform): janela em primeiro plano. Pertence a Platform::IWindow.
inline HWND GetForegroundWindow()
{ PLATFORM_STUB_ONCE("GetForegroundWindow"); return NULL; }

// Convencoes de chamada Win32 nao existem fora do Windows.
// FAR e PASCAL sao herancas do modelo de memoria de 16 bits.
#define FAR
#define NEAR
#define PASCAL
#define WINAPI
#define APIENTRY
#define CALLBACK
#define __stdcall
#define __cdecl

// Comparacao de string case-insensitive: nomes CRT da Microsoft.
#define _stricmp   strcasecmp
#define _strnicmp  strncasecmp
#define stricmp    strcasecmp
#define strnicmp   strncasecmp

#endif // _WIN32
