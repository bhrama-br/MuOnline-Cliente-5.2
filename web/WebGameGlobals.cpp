// Globais que os modulos de jogo esperam mas cujos donos ainda nao compilam para
// Web (ErrorReport.cpp usa API Win32 de arquivo; Winmain.cpp e o ponto de entrada
// Win32).
//
// Simbolos de DADOS nao podem ser resolvidos por -sERROR_ON_UNDEFINED_SYMBOLS=0,
// que so cria stub para funcoes — precisam existir de verdade no link.
//
// TODO(Platform): substituir por implementacoes reais quando o log e a janela
// tiverem contrapartida na camada Platform.

#include "stdafx.h"
#include "Utilities/Log/ErrorReport.h"
#include "Protect.h"
#include "WSctlc.h"
#include "SimpleModulus.h"
#include "Console.h"
#include "KeyGenerater.h"
#include "UIWindows.h"
#include "UIGuildInfo.h"
#include "_define.h"
#include "Time/Timer.h"
#include "WindowTray.h"
#include "UIMapName.h"

#include <time.h>

// Os globais do Winmain foram para Platform/LegacyClientGlobals.cpp, para o
// Android usar as MESMAS definicoes (ver o comentario de la). Aqui fica so o log,
// que difere por plataforma: no Web vai para stderr (console do navegador), no
// Android para o logcat.

CErrorReport g_ErrorReport;

CErrorReport::CErrorReport()  { m_hFile = NULL; m_lpszFileName[0] = '\0'; m_iKey = 0; }
CErrorReport::~CErrorReport() {}

void CErrorReport::Write(const char* lpszFormat, ...)
{
    if (lpszFormat == NULL) return;
    char buffer[1024];
    va_list args;
    va_start(args, lpszFormat);
    vsnprintf(buffer, sizeof(buffer), lpszFormat, args);
    va_end(args);
    fputs(buffer, stderr);
}

// Volta de bytes crus no log. WSclient.cpp (11890-11946) a usa para despejar o
// pacote quando a decifragem falha -- justamente o caminho do login. Sem esta
// definicao o link so avisava, e o cliente abortava com
// "missing function: HexWrite" no primeiro pacote suspeito.
void CErrorReport::HexWrite(void* pBuffer, int iSize)
{
    if (pBuffer == NULL || iSize <= 0) return;
    const unsigned char* bytes = (const unsigned char*)pBuffer;
    // Mesmo formato do original: 16 bytes por linha, grupos de 4.
    for (int i = 0; i < iSize; i += 16)
    {
        char linha[128];
        int posicao = 0;
        for (int j = i; j < i + 16 && j < iSize; ++j)
        {
            posicao += snprintf(linha + posicao, sizeof(linha) - posicao, "%02X", bytes[j]);
            if ((j % 4) == 3 && j < iSize - 1)
                posicao += snprintf(linha + posicao, sizeof(linha) - posicao, " ");
        }
        fprintf(stderr, "%s\n", linha);
    }
}

void CErrorReport::Create(char*)      {}
void CErrorReport::Destroy(void)      {}
void CErrorReport::Clear(void)        {}
void CErrorReport::AddSeparator(void) { fputs("\n", stderr); }

// Carimbo de horario no log. O original usa GetLocalTime; time/localtime_r sao
// padrao e dao o mesmo resultado nas tres plataformas.
void CErrorReport::WriteCurrentTime(BOOL bLineShift)
{
    time_t agora = time(NULL);
    struct tm local;
    localtime_r(&agora, &local);
    fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d%s",
            local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
            local.tm_hour, local.tm_min, local.tm_sec,
            bLineShift ? "\n" : " ");
}
