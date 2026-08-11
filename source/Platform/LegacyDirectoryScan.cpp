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
    struct DirectoryScan
    {
        DIR* directory;
        char path[1024];   // diretorio ja resolvido, com barra no fim
        char pattern[256];     // so a parte depois da ultima barra
    };

    // Casa `nome` com um padrao de curinga do Win32, ignorando a caixa.
    //
    // Recursivo no `*` de proposito: os padroes aqui tem no maximo um curinga
    // ("*.lua"), entao a simplicidade vale mais que uma versao iterativa.
    bool Matches(const char* pattern, const char* name)
    {
        if (*pattern == '\0') return *name == '\0';
        if (*pattern == '*')
        {
            // Tenta consumir zero ou mais caracteres do nome.
            for (const char* cut = name; ; ++cut)
            {
                if (Matches(pattern + 1, cut)) return true;
                if (*cut == '\0') return false;
            }
        }
        if (*name == '\0') return false;
        if (*pattern != '?')
        {
            const char a = (char)tolower((unsigned char)*pattern);
            const char b = (char)tolower((unsigned char)*name);
            if (a != b) return false;
        }
        return Matches(pattern + 1, name + 1);
    }

    bool EDirectoryName(const DirectoryScan& varredura, const struct dirent* entry)
    {
#ifdef DT_DIR
        if (entry->d_type == DT_DIR) return true;
        if (entry->d_type != DT_UNKNOWN) return false;
#endif
        // d_type nao e obrigatorio no POSIX, e o MEMFS do Emscripten nem sempre o
        // preenche: cair para stat mantem o bit correto em qualquer caso.
        char completo[1024];
        if (snprintf(completo, sizeof(completo), "%s%s",
                     varredura.path, entry->d_name) >= (int)sizeof(completo))
            return false;
        struct stat info;
        if (stat(completo, &info) != 0) return false;
        return S_ISDIR(info.st_mode);
    }

    // Avanca ate a proxima entrada que case com o padrao. Devolve false no fim.
    bool NextEntry(DirectoryScan& varredura, LPWIN32_FIND_DATA data)
    {
        for (struct dirent* entry = readdir(varredura.directory);
             entry != NULL;
             entry = readdir(varredura.directory))
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            if (!Matches(varredura.pattern, entry->d_name)) continue;

            const size_t size = strlen(entry->d_name);
            if (size + 1 > sizeof(data->cFileName)) continue;
            memcpy(data->cFileName, entry->d_name, size + 1);
            data->dwFileAttributes = EDirectoryName(varredura, entry)
                                    ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE;
            return true;
        }
        return false;
    }
}

HANDLE FindFirstFile(LPCSTR fullPattern, LPWIN32_FIND_DATA data)
{
    if (fullPattern == NULL || data == NULL) return INVALID_HANDLE_VALUE;

    char normalizado[1024];
    Platform::NormalizeLegacyPath(fullPattern, normalizado, sizeof(normalizado));

    // Separa diretorio e padrao na ULTIMA barra. Sem barra, o diretorio e o corrente.
    char directory[1024];
    const char* pattern = normalizado;
    const char* lastSlash = strrchr(normalizado, '/');
    if (lastSlash != NULL)
    {
        const size_t size = (size_t)(lastSlash - normalizado) + 1;   // inclui a barra
        if (size + 1 > sizeof(directory)) return INVALID_HANDLE_VALUE;
        memcpy(directory, normalizado, size);
        directory[size] = '\0';
        pattern = lastSlash + 1;
    }
    else
    {
        directory[0] = '.';
        directory[1] = '/';
        directory[2] = '\0';
    }

    // `FindFirstFile("pasta\\")` sem padrao nenhum lista tudo, como no Win32.
    if (*pattern == '\0') pattern = "*";

    DIR* aberto = opendir(directory);
    if (aberto == NULL)
    {
        // Mesma tolerancia de caixa da abertura de arquivo: o codigo legado escreve
        // "Data\\Configs\\..." e o disco pode ter outra capitalizacao.
        char resolvido[1024];
        if (!Platform::ResolveLegacyCasePath(directory, resolvido, sizeof(resolvido)))
            return INVALID_HANDLE_VALUE;
        aberto = opendir(resolvido);
        if (aberto == NULL) return INVALID_HANDLE_VALUE;
        const size_t size = strlen(resolvido);
        if (size + 2 > sizeof(directory)) { closedir(aberto); return INVALID_HANDLE_VALUE; }
        memcpy(directory, resolvido, size + 1);
        if (size > 0 && directory[size - 1] != '/')
        {
            directory[size] = '/';
            directory[size + 1] = '\0';
        }
    }

    DirectoryScan* varredura = new DirectoryScan();
    varredura->directory = aberto;
    snprintf(varredura->path, sizeof(varredura->path), "%s", directory);
    snprintf(varredura->pattern, sizeof(varredura->pattern), "%s", pattern);

    if (!NextEntry(*varredura, data))
    {
        // Nenhuma entrada casa: o Win32 devolve INVALID_HANDLE_VALUE, e nao um handle
        // que falha no primeiro FindNextFile.
        closedir(aberto);
        delete varredura;
        return INVALID_HANDLE_VALUE;
    }
    return (HANDLE)varredura;
}

BOOL FindNextFile(HANDLE handle, LPWIN32_FIND_DATA data)
{
    if (handle == INVALID_HANDLE_VALUE || handle == NULL || data == NULL) return FALSE;
    DirectoryScan* varredura = (DirectoryScan*)handle;
    return NextEntry(*varredura, data) ? TRUE : FALSE;
}

BOOL FindClose(HANDLE handle)
{
    if (handle == INVALID_HANDLE_VALUE || handle == NULL) return FALSE;
    DirectoryScan* varredura = (DirectoryScan*)handle;
    if (varredura->directory != NULL) closedir(varredura->directory);
    delete varredura;
    return TRUE;
}

#endif  // !_WIN32
