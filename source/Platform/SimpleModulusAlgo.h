#pragma once

// Algoritmo do SimpleModulus, reimplementado de forma portavel.
//
// A SimpleModulus.lib do repositorio nao tem codigo-fonte e e COFF x86, entao
// nao pode ser linkada em wasm nem no Android. Este cabecalho contem so o
// algoritmo, sobre arrays de chave explicitos, sem depender da classe: assim a
// MESMA implementacao pode ser exercitada pelo verificador no PC (que compara
// com a lib original byte a byte) e usada pela classe fora do Windows.
//
// Derivado da desmontagem da lib, nao de suposicao. Formato do bloco: 8 bytes de
// entrada viram 11 de saida (88 bits = 4 valores de 18 bits + 16 bits de
// verificacao).

#include <string.h>

namespace Platform
{
namespace SimpleModulusAlgo
{
    typedef unsigned int  u32;
    typedef unsigned short u16;
    typedef unsigned char  u8;

    enum { kBlocoEntrada = 8, kBlocoSaida = 11, kNumChaves = 4 };

    // Mascara aplicada aos valores gravados no arquivo de chave.
    //
    // Nao veio da desmontagem: foi DEDUZIDA comparando os bytes crus de
    // Enc1.dat com as chaves que a lib original produz ao carrega-lo, e depois
    // confirmada em Dec2.dat, que tem chaves completamente diferentes. As 12
    // palavras do arquivo usam a mascara de indice `posicao % 4`.
    const u32 kMascaraArquivo[kNumChaves] = {
        0x3F08A79Bu, 0xE25CC287u, 0x93D27AB9u, 0x20DEA7BFu
    };

    // Cabecalho do arquivo: id (2 bytes) + tamanho (4 bytes), sem alinhamento.
    enum { kIdUmaChave = 0x1112, kCabecalhoChave = 6, kBytesChave = 54 };

    // Le um arquivo de chave ja em memoria. `chave` recebe o bloco do meio, que
    // e a chave de cifragem ou de decifragem conforme o arquivo.
    inline bool LerChaves(const u8* conteudo, int tamanho,
                          u32* modulus, u32* chave, u32* chaveXor)
    {
        if (conteudo == 0 || tamanho < kBytesChave) return false;

        u16 id;
        memcpy(&id, conteudo, sizeof(id));
        if (id != kIdUmaChave) return false;

        const u8* corpo = conteudo + kCabecalhoChave;
        for (int i = 0; i < kNumChaves * 3; ++i)
        {
            u32 bruto;
            memcpy(&bruto, corpo + i * 4, sizeof(bruto));
            const u32 valor = bruto ^ kMascaraArquivo[i % kNumChaves];

            if (i < kNumChaves)                 modulus[i] = valor;
            else if (i < kNumChaves * 2)        chave[i - kNumChaves] = valor;
            else                                chaveXor[i - kNumChaves * 2] = valor;
        }
        return true;
    }

    // Deslocamento de bits sobre um buffer de nBytes.
    //
    // nShift > 0 desloca para a direita, < 0 para a esquerda. Reproduz o
    // percurso da original: da direita para a esquerda no caso positivo, da
    // esquerda para a direita no negativo, para nao sobrescrever o byte que
    // ainda sera lido.
    inline void Deslocar(u8* buffer, int nBytes, int nShift)
    {
        if (nShift == 0 || nBytes <= 0) return;

        if (nShift > 0)
        {
            for (int i = nBytes - 1; i > 0; --i)
                buffer[i] = (u8)((buffer[i - 1] << (8 - nShift)) | (buffer[i] >> nShift));
            buffer[0] = (u8)(buffer[0] >> nShift);
        }
        else
        {
            const int n = -nShift;
            for (int i = 0; i < nBytes - 1; ++i)
                buffer[i] = (u8)((buffer[i + 1] >> (8 - n)) | (buffer[i] << n));
            buffer[nBytes - 1] = (u8)(buffer[nBytes - 1] << n);
        }
    }

    inline int ByteDoBit(int nBit)
    {
        return nBit >> 3;
    }

    // Resto com sinal, como o `and 80000007h` + ajuste da original faz.
    inline int RestoOito(int valor)
    {
        return valor % 8;
    }

    // Copia nNumBits bits de lpBits (a partir de nInitialBit) para o final de
    // lpBuffer, que ja contem nNumBufferBits bits. Devolve a nova contagem.
    inline int AdicionarBits(u8* lpBuffer, int nNumBufferBits,
                             const u8* lpBits, int nInitialBit, int nNumBits)
    {
        const int byteFinal   = ByteDoBit(nInitialBit + nNumBits - 1);
        const int byteInicial = ByteDoBit(nInitialBit);
        const int nBytes      = byteFinal - byteInicial + 1;

        // A original aloca nBytes+1: o deslocamento para a posicao de destino
        // pode empurrar bits para um byte a mais.
        u8 temp[16];
        memset(temp, 0, sizeof(temp));
        memcpy(temp, lpBits + byteInicial, (size_t)nBytes);

        // Zera o que passa de nNumBits no ultimo byte.
        const int sobra = RestoOito(nInitialBit + nNumBits);
        if (sobra != 0)
            temp[nBytes - 1] &= (u8)(0xFF << (8 - sobra));

        const int deslocOrigem  = RestoOito(nInitialBit);
        const int deslocDestino = RestoOito(nNumBufferBits);

        Deslocar(temp, nBytes, -deslocOrigem);
        Deslocar(temp, nBytes + 1, deslocDestino);

        // Um byte a mais so quando o deslocamento de destino empurrou bits
        // alem do ultimo byte copiado.
        const int nBytesDestino = nBytes + ((deslocDestino > deslocOrigem) ? 1 : 0);

        u8* destino = lpBuffer + ByteDoBit(nNumBufferBits);
        for (int i = 0; i < nBytesDestino; ++i)
            destino[i] |= temp[i];

        return nNumBufferBits + nNumBits;
    }

    // Cifra um bloco de ate 8 bytes em exatamente 11.
    inline void CifrarBloco(u8* destino, const u8* origem, int nBytesOrigem,
                            const u32* modulus, const u32* chaveEnc, const u32* chaveXor)
    {
        memset(destino, 0, kBlocoSaida);

        // 1) Passo modular, encadeado: cada valor leva os 16 bits baixos do
        //    resto anterior.
        u32 valores[kNumChaves];
        u32 encadeado = 0;
        for (int i = 0; i < kNumChaves; ++i)
        {
            u16 palavra;
            memcpy(&palavra, origem + i * 2, sizeof(palavra));

            u32 v = ((u32)palavra ^ chaveXor[i]) ^ encadeado;
            v = v * chaveEnc[i];
            valores[i] = v % modulus[i];
            encadeado = valores[i] & 0xFFFF;
        }

        // 2) Passo reverso: de tras para frente, cada valor recebe a chave XOR e
        //    os 16 bits baixos do valor ORIGINAL do vizinho de cima.
        u32 anterior = valores[kNumChaves - 1] & 0xFFFF;
        for (int i = kNumChaves - 2; i >= 0; --i)
        {
            const u32 original = valores[i];
            valores[i] = original ^ chaveXor[i] ^ anterior;
            anterior = original & 0xFFFF;
        }

        // 3) Empacota 18 bits por valor: os 16 baixos, mais 2 bits vindos do bit
        //    22 (onde o resto da divisao pode alcancar).
        int bits = 0;
        for (int i = 0; i < kNumChaves; ++i)
        {
            const u8* pv = (const u8*)&valores[i];
            bits = AdicionarBits(destino, bits, pv, 0, 16);
            bits = AdicionarBits(destino, bits, pv, 22, 2);
        }

        // 4) Dois bytes de verificacao: soma XOR dos 8 bytes de ENTRADA a partir
        //    de 0xF8, e o tamanho misturado com ela.
        u8 verificacao = 0xF8;
        for (int i = 0; i < kBlocoEntrada; ++i)
            verificacao ^= origem[i];

        u8 par[2];
        par[0] = (u8)(((u8)nBytesOrigem ^ 0x3D) ^ verificacao);
        par[1] = verificacao;
        AdicionarBits(destino, bits, par, 0, 16);
    }

    // Decifra um bloco de 11 bytes. Devolve quantos bytes uteis ele carregava
    // (1 a 8), ou -1 se a verificacao falhar.
    inline int DecifrarBloco(u8* destino, const u8* origem,
                             const u32* modulus, const u32* chaveDec, const u32* chaveXor)
    {
        memset(destino, 0, kBlocoEntrada);

        // 1) Desempacota os 4 valores de 18 bits.
        u32 valores[kNumChaves];
        memset(valores, 0, sizeof(valores));
        int bits = 0;
        for (int i = 0; i < kNumChaves; ++i)
        {
            u8* pv = (u8*)&valores[i];
            AdicionarBits(pv, 0, origem, bits, 16);
            bits += 16;
            AdicionarBits(pv, 22, origem, bits, 2);
            bits += 2;
        }

        // 2) Inverso do passo reverso da cifragem. Aqui o encadeamento leva o
        //    valor JA decifrado, nao o original: e o que desfaz a ida.
        u32 anterior = valores[kNumChaves - 1] & 0xFFFF;
        for (int i = kNumChaves - 2; i >= 0; --i)
        {
            valores[i] = valores[i] ^ chaveXor[i] ^ anterior;
            anterior = valores[i] & 0xFFFF;
        }

        // 3) Inverso do passo modular, com a chave de decifragem.
        u32 encadeado = 0;
        for (int i = 0; i < kNumChaves; ++i)
        {
            const u32 valor = valores[i];
            u32 v = (chaveDec[i] * valor) % modulus[i];
            v = v ^ chaveXor[i] ^ encadeado;
            encadeado = valor & 0xFFFF;

            const u16 palavra = (u16)v;
            memcpy(destino + i * 2, &palavra, sizeof(palavra));
        }

        // 4) Confere: os dois bytes finais guardam a soma de verificacao e o
        //    tamanho. Sem isso um bloco corrompido passaria como dado valido.
        u8 par[2] = { 0, 0 };
        AdicionarBits(par, 0, origem, bits, 16);
        const u8 verificacaoGravada = par[1];
        const u8 tamanho = (u8)((par[1] ^ par[0]) ^ 0x3D);

        u8 verificacao = 0xF8;
        for (int i = 0; i < kBlocoEntrada; ++i)
            verificacao ^= destino[i];

        if (verificacaoGravada != verificacao) return -1;
        return (int)tamanho;
    }

    // Devolve o total decifrado, ou -1 se algum bloco falhar na verificacao.
    // Com destino nulo, so calcula o tamanho maximo.
    inline int Decifrar(u8* destino, const u8* origem, int iSize,
                        const u32* modulus, const u32* chaveDec, const u32* chaveXor)
    {
        if (destino == 0) return ((iSize + 10) / kBlocoSaida) * kBlocoEntrada;

        int total = 0;
        for (int i = 0; i < iSize; i += kBlocoSaida)
        {
            const int neste = DecifrarBloco(destino, origem, modulus, chaveDec, chaveXor);
            if (neste < 0) return -1;
            total   += neste;
            destino += kBlocoEntrada;
            origem  += kBlocoSaida;
        }
        return total;
    }

    // Devolve o tamanho cifrado. Com destino nulo, so calcula (a original faz o
    // mesmo, e ha chamador que depende disso para dimensionar buffer).
    inline int Cifrar(u8* destino, const u8* origem, int iSize,
                      const u32* modulus, const u32* chaveEnc, const u32* chaveXor)
    {
        const int total = ((iSize + 7) / 8) * kBlocoSaida;
        if (destino == 0) return total;

        int restante = iSize;
        for (int i = 0; i < iSize; i += kBlocoEntrada)
        {
            const int neste = (restante < kBlocoEntrada) ? restante : kBlocoEntrada;

            // A original le os 8 bytes do bloco mesmo quando sobram menos: o
            // checksum percorre os 8. Copiar para um bloco zerado evita ler
            // alem do buffer de origem, que seria leitura invalida.
            u8 bloco[kBlocoEntrada];
            memset(bloco, 0, sizeof(bloco));
            memcpy(bloco, origem + i, (size_t)neste);

            CifrarBloco(destino, bloco, neste, modulus, chaveEnc, chaveXor);
            destino  += kBlocoSaida;
            restante -= kBlocoEntrada;
        }
        return total;
    }
}
}
