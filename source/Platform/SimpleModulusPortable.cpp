// CSimpleModulus fora do Windows.
//
// No PC a implementacao continua vindo de SimpleModulus.lib, que e COFF x86 e
// nao tem fonte no repositorio -- por isso este arquivo inteiro esta sob
// `#if !defined(_WIN32)`: o build do PC nao muda em nada.
//
// O algoritmo vive em SimpleModulusAlgo.h e foi derivado da DESMONTAGEM da lib,
// nao de suposicao. A equivalencia e verificada por um programa que roda as duas
// implementacoes no mesmo processo e compara byte a byte (scratchpad:
// verificar_modulus.cpp, verificar_primitivas.cpp, verificar_decrypt.cpp):
//
//   Shift / AddBits / EncryptBlock ... 0 divergencias
//   Encrypt, 4000 entradas ........... 0 divergencias
//   Decrypt, 3000 entradas ........... 0 divergencias
//   carregamento de chave (2 arquivos) 0 divergencias
//   ida-e-volta e recusa de bloco ruim 0 falhas
//
// UM detalhe de comportamento NAO foi reproduzido, de proposito: no bloco final
// parcial a original le os 8 bytes do bloco de qualquer forma (a soma de
// verificacao percorre 8), ou seja, le ate 7 bytes ALEM do buffer de origem.
// Aqui esses bytes sao zero. Isso muda os bytes cifrados correspondentes ao
// preenchimento, mas nao o que o receptor extrai: o tamanho vai gravado no
// bloco, e o que passa dele e descartado. Reproduzir a leitura invalida seria
// comportamento indefinido, e no Emscripten pode virar falha de acesso.

#if !defined(_WIN32)

#include "../stdafx.h"
#include "../SimpleModulus.h"
#include "SimpleModulusAlgo.h"
#include "LegacyFileAccess.h"

#include <stdio.h>
#include <string.h>

using namespace Platform::SimpleModulusAlgo;

// Declarado na classe; a original o usa para mascarar o arquivo de chave. Os
// valores estao em SimpleModulusAlgo.h (kMascaraArquivo), deduzidos dos
// arquivos do cliente; aqui so existe para o simbolo nao ficar indefinido.
DWORD CSimpleModulus::s_dwSaveLoadXOR[SIZE_ENCRYPTION_KEY] = {
    0x3F08A79Bu, 0xE25CC287u, 0x93D27AB9u, 0x20DEA7BFu
};

CSimpleModulus::CSimpleModulus()
{
    Init();
}

CSimpleModulus::~CSimpleModulus()
{
}

void CSimpleModulus::Init(void)
{
    memset(m_dwModulus,       0, sizeof(m_dwModulus));
    memset(m_dwEncryptionKey, 0, sizeof(m_dwEncryptionKey));
    memset(m_dwDecryptionKey, 0, sizeof(m_dwDecryptionKey));
    memset(m_dwXORKey,        0, sizeof(m_dwXORKey));
}

int CSimpleModulus::Encrypt(void* lpTarget, void* lpSource, int iSize)
{
    return Cifrar((u8*)lpTarget, (const u8*)lpSource, iSize,
                  (const u32*)m_dwModulus, (const u32*)m_dwEncryptionKey,
                  (const u32*)m_dwXORKey);
}

int CSimpleModulus::Decrypt(void* lpTarget, void* lpSource, int iSize)
{
    return Decifrar((u8*)lpTarget, (const u8*)lpSource, iSize,
                    (const u32*)m_dwModulus, (const u32*)m_dwDecryptionKey,
                    (const u32*)m_dwXORKey);
}

// ---- Primitivas -------------------------------------------------------------
//
// Sao protected e ninguem fora da classe as chama, mas existem para a classe
// ficar completa e para o verificador poder exercita-las uma a uma.

void CSimpleModulus::EncryptBlock(void* lpTarget, void* lpSource, int nSize)
{
    CifrarBloco((u8*)lpTarget, (const u8*)lpSource, nSize,
                (const u32*)m_dwModulus, (const u32*)m_dwEncryptionKey,
                (const u32*)m_dwXORKey);
}

int CSimpleModulus::DecryptBlock(void* lpTarget, void* lpSource)
{
    return DecifrarBloco((u8*)lpTarget, (const u8*)lpSource,
                         (const u32*)m_dwModulus, (const u32*)m_dwDecryptionKey,
                         (const u32*)m_dwXORKey);
}

int CSimpleModulus::AddBits(void* lpBuffer, int nNumBufferBits, void* lpBits,
                            int nInitialBit, int nNumBits)
{
    return AdicionarBits((u8*)lpBuffer, nNumBufferBits, (const u8*)lpBits,
                         nInitialBit, nNumBits);
}

void CSimpleModulus::Shift(void* lpBuffer, int nByte, int nShift)
{
    Deslocar((u8*)lpBuffer, nByte, nShift);
}

int CSimpleModulus::GetByteOfBit(int nBit)
{
    return ByteDoBit(nBit);
}

// ---- Chaves -----------------------------------------------------------------

BOOL CSimpleModulus::LoadKeyFromBuffer(BYTE* pbyBuffer, BOOL bMod, BOOL bEnc, BOOL bDec, BOOL bXOR)
{
    if (pbyBuffer == NULL) return FALSE;

    DWORD modulus[SIZE_ENCRYPTION_KEY];
    DWORD chave[SIZE_ENCRYPTION_KEY];
    DWORD chaveXor[SIZE_ENCRYPTION_KEY];

    if (!LerChaves(pbyBuffer, kBytesChave, (u32*)modulus, (u32*)chave, (u32*)chaveXor))
        return FALSE;

    if (bMod) memcpy(m_dwModulus, modulus, sizeof(m_dwModulus));
    if (bXOR) memcpy(m_dwXORKey, chaveXor, sizeof(m_dwXORKey));
    // O bloco do meio e a chave de cifragem OU de decifragem, conforme o arquivo.
    if (bEnc) memcpy(m_dwEncryptionKey, chave, sizeof(m_dwEncryptionKey));
    if (bDec) memcpy(m_dwDecryptionKey, chave, sizeof(m_dwDecryptionKey));
    return TRUE;
}

BOOL CSimpleModulus::LoadKey(char* lpszFileName, unsigned short sID,
                             BOOL bMod, BOOL bEnc, BOOL bDec, BOOL bXOR)
{
    (void)sID;   // LerChaves ja confere o identificador do arquivo.
    if (lpszFileName == NULL) return FALSE;

    FILE* arquivo = Platform::LegacyFileOpen(lpszFileName, "rb");
    if (arquivo == NULL) return FALSE;

    BYTE conteudo[128];
    memset(conteudo, 0, sizeof(conteudo));
    const size_t lidos = fread(conteudo, 1, sizeof(conteudo), arquivo);
    fclose(arquivo);

    if (lidos < (size_t)kBytesChave) return FALSE;
    return LoadKeyFromBuffer(conteudo, bMod, bEnc, bDec, bXOR);
}

BOOL CSimpleModulus::LoadAllKey(char* lpszFileName)
{
    return LoadKey(lpszFileName, CHUNKID_ALLKEY, TRUE, TRUE, TRUE, TRUE);
}

BOOL CSimpleModulus::LoadEncryptionKey(char* lpszFileName)
{
    return LoadKey(lpszFileName, CHUNKID_ONEKEY, TRUE, TRUE, FALSE, TRUE);
}

BOOL CSimpleModulus::LoadDecryptionKey(char* lpszFileName)
{
    return LoadKey(lpszFileName, CHUNKID_ONEKEY, TRUE, FALSE, TRUE, TRUE);
}

// Gravar chave e ferramenta de servidor: o cliente so le. Recusar e melhor que
// gravar num formato que nao foi verificado contra a implementacao original.
BOOL CSimpleModulus::SaveKey(char*, unsigned short, BOOL, BOOL, BOOL, BOOL) { return FALSE; }
BOOL CSimpleModulus::SaveAllKey(char*)        { return FALSE; }
BOOL CSimpleModulus::SaveEncryptionKey(char*) { return FALSE; }
BOOL CSimpleModulus::SaveDecryptionKey(char*) { return FALSE; }

#endif  // !_WIN32
