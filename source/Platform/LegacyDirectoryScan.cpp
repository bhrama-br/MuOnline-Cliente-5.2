// Enumeracao de diretorio: FindFirstFile/FindNextFile/FindClose sobre opendir/readdir.
//
// POR QUE EXISTE: `CLuaOpenFolder::LoadFolder` (LuaOpenFolder.cpp:18) varre uma pasta
// com `FindFirstFile("<pasta>*.lua")` e carrega cada script achado. Fora do Windows a
// funcao era um stub que devolvia INVALID_HANDLE_VALUE, e `LoadFolder` trata isso
// como "pasta vazia" -- em SILENCIO, sem erro nenhum.
//
// A consequencia era grande e distante da causa: a PRIMEIRA linha de quase todo script
// do cliente e `OpenFolder("Definitions")`, que e o que carrega
// Data/Configs/Lua/Manager/Definitions/Defines.lua -- onde vivem `GET_ITEM` e
// `GET_ITEM_MODEL`. Sem a varredura, `GET_ITEM` ficava nil e CADA script abortava na
// sua primeira chamada, deixando todas as suas funcoes indefinidas. O sintoma final
// aparecia a tres camadas de distancia:
//
//   luacall_Generic_Call error running function 'RenderProc': 'attempt to call a nil value'
//
// (a lista de personagens da Season 13 nao desenhava nada), alem de 'RenderModelBody',
// 'CreateEffectSetPlayer', 'LoadImageCape' e outras -- ~20 mil linhas de erro por
// sessao, todas com a mesma raiz.
//
// O padrao aceito e o do Win32 (`*` e `?`), e a comparacao ignora a caixa, como no
// Windows. Diretorios recebem FILE_ATTRIBUTE_DIRECTORY porque `LoadFolder` os pula por
// esse bit.

#if !defined(_WIN32)

#include "WindowsCompat.h"
#include "LegacyFileAccess.h"

#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace
{
    struct VarreduraDeDiretorio
    {
        DIR* diretorio;
        char caminho[1024];   // diretorio ja resolvido, com barra no fim
        char padrao[256];     // so a parte depois da ultima barra
    };

    // Casa `nome` com um padrao de curinga do Win32, ignorando a caixa.
    //
    // Recursivo no `*` de proposito: os padroes aqui tem no maximo um curinga
    // ("*.lua"), entao a simplicidade vale mais que uma versao iterativa.
    bool Casa(const char* padrao, const char* nome)
    {
        if (*padrao == '\0') return *nome == '\0';
        if (*padrao == '*')
        {
            // Tenta consumir zero ou mais caracteres do nome.
            for (const char* corte = nome; ; ++corte)
            {
                if (Casa(padrao + 1, corte)) return true;
                if (*corte == '\0') return false;
            }
        }
        if (*nome == '\0') return false;
        if (*padrao != '?')
        {
            const char a = (char)tolower((unsigned char)*padrao);
            const char b = (char)tolower((unsigned char)*nome);
            if (a != b) return false;
        }
        return Casa(padrao + 1, nome + 1);
    }

    bool ENomeDeDiretorio(const VarreduraDeDiretorio& varredura, const struct dirent* entrada)
    {
#ifdef DT_DIR
        if (entrada->d_type == DT_DIR) return true;
        if (entrada->d_type != DT_UNKNOWN) return false;
#endif
        // d_type nao e obrigatorio no POSIX, e o MEMFS do Emscripten nem sempre o
        // preenche: cair para stat mantem o bit correto em qualquer caso.
        char completo[1024];
        if (snprintf(completo, sizeof(completo), "%s%s",
                     varredura.caminho, entrada->d_name) >= (int)sizeof(completo))
            return false;
        struct stat info;
        if (stat(completo, &info) != 0) return false;
        return S_ISDIR(info.st_mode);
    }

    // Avanca ate a proxima entrada que case com o padrao. Devolve false no fim.
    bool ProximaEntrada(VarreduraDeDiretorio& varredura, LPWIN32_FIND_DATA dados)
    {
        for (struct dirent* entrada = readdir(varredura.diretorio);
             entrada != NULL;
             entrada = readdir(varredura.diretorio))
        {
            if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0)
                continue;
            if (!Casa(varredura.padrao, entrada->d_name)) continue;

            const size_t tamanho = strlen(entrada->d_name);
            if (tamanho + 1 > sizeof(dados->cFileName)) continue;
            memcpy(dados->cFileName, entrada->d_name, tamanho + 1);
            dados->dwFileAttributes = ENomeDeDiretorio(varredura, entrada)
                                    ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE;
            return true;
        }
        return false;
    }
}

HANDLE FindFirstFile(LPCSTR padraoCompleto, LPWIN32_FIND_DATA dados)
{
    if (padraoCompleto == NULL || dados == NULL) return INVALID_HANDLE_VALUE;

    char normalizado[1024];
    Platform::NormalizeLegacyPath(padraoCompleto, normalizado, sizeof(normalizado));

    // Separa diretorio e padrao na ULTIMA barra. Sem barra, o diretorio e o corrente.
    char diretorio[1024];
    const char* padrao = normalizado;
    const char* ultimaBarra = strrchr(normalizado, '/');
    if (ultimaBarra != NULL)
    {
        const size_t tamanho = (size_t)(ultimaBarra - normalizado) + 1;   // inclui a barra
        if (tamanho + 1 > sizeof(diretorio)) return INVALID_HANDLE_VALUE;
        memcpy(diretorio, normalizado, tamanho);
        diretorio[tamanho] = '\0';
        padrao = ultimaBarra + 1;
    }
    else
    {
        diretorio[0] = '.';
        diretorio[1] = '/';
        diretorio[2] = '\0';
    }

    // `FindFirstFile("pasta\\")` sem padrao nenhum lista tudo, como no Win32.
    if (*padrao == '\0') padrao = "*";

    DIR* aberto = opendir(diretorio);
    if (aberto == NULL)
    {
        // Mesma tolerancia de caixa da abertura de arquivo: o codigo legado escreve
        // "Data\\Configs\\..." e o disco pode ter outra capitalizacao.
        char resolvido[1024];
        if (!Platform::ResolveLegacyCasePath(diretorio, resolvido, sizeof(resolvido)))
            return INVALID_HANDLE_VALUE;
        aberto = opendir(resolvido);
        if (aberto == NULL) return INVALID_HANDLE_VALUE;
        const size_t tamanho = strlen(resolvido);
        if (tamanho + 2 > sizeof(diretorio)) { closedir(aberto); return INVALID_HANDLE_VALUE; }
        memcpy(diretorio, resolvido, tamanho + 1);
        if (tamanho > 0 && diretorio[tamanho - 1] != '/')
        {
            diretorio[tamanho] = '/';
            diretorio[tamanho + 1] = '\0';
        }
    }

    VarreduraDeDiretorio* varredura = new VarreduraDeDiretorio();
    varredura->diretorio = aberto;
    snprintf(varredura->caminho, sizeof(varredura->caminho), "%s", diretorio);
    snprintf(varredura->padrao, sizeof(varredura->padrao), "%s", padrao);

    if (!ProximaEntrada(*varredura, dados))
    {
        // Nenhuma entrada casa: o Win32 devolve INVALID_HANDLE_VALUE, e nao um handle
        // que falha no primeiro FindNextFile.
        closedir(aberto);
        delete varredura;
        return INVALID_HANDLE_VALUE;
    }
    return (HANDLE)varredura;
}

BOOL FindNextFile(HANDLE handle, LPWIN32_FIND_DATA dados)
{
    if (handle == INVALID_HANDLE_VALUE || handle == NULL || dados == NULL) return FALSE;
    VarreduraDeDiretorio* varredura = (VarreduraDeDiretorio*)handle;
    return ProximaEntrada(*varredura, dados) ? TRUE : FALSE;
}

BOOL FindClose(HANDLE handle)
{
    if (handle == INVALID_HANDLE_VALUE || handle == NULL) return FALSE;
    VarreduraDeDiretorio* varredura = (VarreduraDeDiretorio*)handle;
    if (varredura->diretorio != NULL) closedir(varredura->diretorio);
    delete varredura;
    return TRUE;
}

#endif  // !_WIN32
