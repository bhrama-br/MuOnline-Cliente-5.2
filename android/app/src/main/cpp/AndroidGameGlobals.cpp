// Globais e implementacoes minimas que os modulos de jogo esperam mas cujos donos
// nao compilam para Android. Espelha web/WebGameGlobals.cpp.
//
// `Utilities/Log/ErrorReport.cpp` ficou de fora da lista de fontes de proposito:
// ele enumera dispositivos DirectSound, le o layout de teclado e a versao do SO
// pelo Win32. Nada disso tem contrapartida aqui, e o valor do modulo — registrar
// o que deu errado — cabe em poucas linhas escrevendo no logcat.
//
// `g_hWnd` e um simbolo de DADOS: o linker dinamico do Android o resolve no
// dlopen, entao ele precisa existir de verdade, nao basta um stub de funcao.

#include "stdafx.h"
#include "Utilities/Log/ErrorReport.h"

#include <android/log.h>
#include <stdarg.h>
#include <stdio.h>

CErrorReport g_ErrorReport;
// `g_hWnd` agora vem de Platform/LegacyClientGlobals.cpp, compartilhado com o
// Web -- ver la o motivo de os globais do Winmain terem deixado de ser
// duplicados por plataforma.

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
    __android_log_print(ANDROID_LOG_ERROR, "MuLegacy", "%s", buffer);
}

// Volta de bytes crus: mesmo papel da versao de web/WebGameGlobals.cpp -- ver o
// comentario de la sobre por que o cliente abortava sem esta definicao.
void CErrorReport::HexWrite(void* pBuffer, int iSize)
{
    if (pBuffer == NULL || iSize <= 0) return;
    const unsigned char* bytes = (const unsigned char*)pBuffer;
    for (int i = 0; i < iSize; i += 16)
    {
        char line[128];
        int position = 0;
        for (int j = i; j < i + 16 && j < iSize; ++j)
        {
            position += snprintf(line + position, sizeof(line) - position, "%02X", bytes[j]);
            if ((j % 4) == 3 && j < iSize - 1)
                position += snprintf(line + position, sizeof(line) - position, " ");
        }
        __android_log_print(ANDROID_LOG_ERROR, "MuLegacy", "%s", line);
    }
}

void CErrorReport::Create(char*)      {}
void CErrorReport::Destroy(void)      {}
void CErrorReport::Clear(void)        {}
void CErrorReport::AddSeparator(void) {}

void CErrorReport::WriteCurrentTime(BOOL)
{
    // O logcat ja carimba a hora de cada linha; repetir aqui so poluiria.
}
