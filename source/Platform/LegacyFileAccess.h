#pragma once

#include <cstdio>

// Acesso a arquivo compativel com os caminhos do legado.
//
// O carregamento de assets (textura, BMD, terreno, dados) ja usa stdio, que e
// portavel — o que nao e portavel sao os separadores: o codigo monta caminhos
// como "Data\\Player\\Player.bmd". Fora do Windows a barra invertida nao separa
// diretorio, entao o caminho e normalizado aqui, num unico ponto, em vez de
// reescrever milhares de literais.
//
// No Windows a funcao e repasse direto: o comportamento do build PC nao muda.

namespace Platform
{
    // Copia o caminho normalizando '\\' para '/' fora do Windows.
    // Devolve destination.
    const char* NormalizeLegacyPath(const char* path, char* destination, size_t capacity);

    // Equivalente a fopen aplicando NormalizeLegacyPath.
    FILE* LegacyFileOpen(const char* path, const char* mode);

    // Reescreve `path` (ja normalizado) com a capitalizacao REAL de cada componente,
    // quando a unica diferenca for a caixa. Devolve false se algum componente nao
    // existir. No Windows devolve false sempre: la a caixa e indiferente.
    //
    // Exposta porque a enumeracao de diretorio (FindFirstFile em
    // Platform/LegacyDirectoryScan.cpp) precisa do mesmo tratamento que a abertura de
    // arquivo -- o codigo legado escreve "Data\\Configs\\Lua\\Manager\\Definitions\\"
    // e no disco pode estar com outra caixa.
    bool ResolveLegacyCasePath(const char* path, char* resolved, size_t capacity);

    // Ultimo caminho passado a LegacyFileOpen, normalizado.
    //
    // Existe para diagnostico: varios pontos do cliente chamam `exit(0)` quando um
    // arquivo falta, e no Android isso encerra o processo de verdade (no Web o
    // build linka -lnoexit e a chamada e inofensiva). Sem registrar o caminho, a
    // saida nao diz QUAL arquivo a causou.
    const char* LastOpenedPath();

    // Observador de FALHA de abertura.
    //
    // Existe porque varios pontos do cliente reagem a arquivo ausente chamando
    // `exit(0)` -- e no Android isso encerra o processo longe da causa (os
    // destrutores estaticos ainda tocam o GL, e o log mostra um SIGSEGV em
    // glDeleteTextures). Sem um observador aqui, descobrir QUAL arquivo faltou
    // exige adivinhar.
    //
    // Chamado depois de todas as tentativas (caixa e gancho de busca) falharem.
    typedef void (*LegacyFileMissHook)(const char* normalizedPath);
    void SetLegacyFileMissHook(LegacyFileMissHook hook);

    // Ponto de extensao para alvos que nao empacotam todos os assets de antemao.
    //
    // Chamado quando a abertura em modo leitura falha, com o caminho ja
    // normalizado. Se devolver true, LegacyFileOpen tenta abrir de novo: o
    // gancho e responsavel por deixar o arquivo disponivel. Existe porque o
    // codigo legado le assets de forma SINCRONA em centenas de pontos, e tornar
    // a busca assincrona exigiria reescrever todos eles.
    typedef bool (*LegacyAssetFetchHook)(const char* normalizedPath);
    void SetLegacyAssetFetchHook(LegacyAssetFetchHook hook);

    // Observador de abertura BEM-SUCEDIDA.
    //
    // Existe para atribuir consumo de memoria a arquivo. O Android aloca ~2,6 GB
    // em 4 segundos durante a carga e o LMK mata o processo, sem tombstone e sem
    // dizer onde. Instrumentar os carregadores um por um significaria mexer em
    // codigo que o PC tambem compila; um gancho aqui cobre TODOS eles, porque
    // todo asset passa por esta funcao.
    //
    // Chamado depois de o arquivo abrir, antes de o chamador ler qualquer coisa —
    // ou seja, a memoria que aparecer depois desta chamada e ate a proxima e o
    // custo de carregar este arquivo.
    typedef void (*LegacyFileOpenHook)(const char* normalizedPath);
    void SetLegacyFileOpenHook(LegacyFileOpenHook hook);

    // TODO(Platform): sistemas de arquivo case-sensitive. Os assets do MU usam
    // capitalizacao inconsistente ("Data\\Player" vs "data\\player") e no Windows
    // isso e indiferente. Em Android/Web sera necessario um indice de nomes reais
    // ou normalizar a capitalizacao dos arquivos empacotados.
}
