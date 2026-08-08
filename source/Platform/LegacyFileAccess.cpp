// Este arquivo nao usa o cabecalho pre-compilado: o vcxproj marca
// PrecompiledHeader=NotUsing na propria entrada dele.
#include "LegacyFileAccess.h"

#include <cstring>

#ifndef _WIN32
#  include <dirent.h>
#  include <strings.h>
#  include <sys/stat.h>
#endif

namespace Platform
{
    const char* NormalizeLegacyPath(const char* path, char* destination, size_t capacity)
    {
        if (path == NULL || destination == NULL || capacity == 0) return path;

#ifdef _WIN32
        // No Windows a barra invertida e o separador nativo: nada a fazer.
        (void)capacity;
        return path;
#else
        size_t i = 0;
        for (; path[i] != '\0' && i + 1 < capacity; ++i)
            destination[i] = (path[i] == '\\') ? '/' : path[i];
        destination[i] = '\0';
        return destination;
#endif
    }

#ifndef _WIN32
    namespace
    {
        // Procura, num diretorio, uma entrada que case com `name` ignorando
        // maiusculas/minusculas. Devolve false se nao houver nenhuma.
        bool FindCaseInsensitiveEntry(const char* directory, const char* name,
                                      char* result, size_t capacity)
        {
            DIR* dir = opendir(directory[0] != '\0' ? directory : ".");
            if (dir == NULL) return false;

            bool found = false;
            for (struct dirent* entry = readdir(dir); entry != NULL; entry = readdir(dir))
            {
                if (strcasecmp(entry->d_name, name) != 0) continue;
                const size_t length = strlen(entry->d_name);
                if (length + 1 > capacity) break;
                memcpy(result, entry->d_name, length + 1);
                found = true;
                break;
            }
            closedir(dir);
            return found;
        }

        // Reconstroi o caminho componente a componente, trocando cada um pela
        // entrada real do disco quando so a capitalizacao diverge.
        //
        // Os assets do MU tem capitalizacao inconsistente ("Data\Local\Mix.bmd"
        // no codigo, "mix.bmd" no disco). No Windows isso e indiferente; em
        // Android/Web o arquivo simplesmente nao abre — e varios carregadores
        // legados nao tratam o NULL e quebram.
        //
        // So e chamada quando o fopen direto ja falhou, entao nao pesa no caminho
        // normal.
        bool ResolveCasePath(const char* path, char* resolved, size_t capacity)
        {
            if (path == NULL || resolved == NULL || capacity == 0) return false;

            size_t resolvedLength = 0;
            resolved[0] = '\0';
            if (path[0] == '/')
            {
                if (capacity < 2) return false;
                resolved[0] = '/';
                resolved[1] = '\0';
                resolvedLength = 1;
            }

            const char* cursor = path + resolvedLength;
            while (*cursor != '\0')
            {
                const char* slash = strchr(cursor, '/');
                const size_t length = (slash != NULL) ? (size_t)(slash - cursor) : strlen(cursor);
                if (length == 0) { cursor += 1; continue; }

                char component[256];
                if (length + 1 > sizeof(component)) return false;
                memcpy(component, cursor, length);
                component[length] = '\0';

                // Monta o candidato com a capitalizacao original e testa primeiro:
                // se existir, nao ha por que varrer o diretorio.
                char candidate[1024];
                const int written = snprintf(candidate, sizeof(candidate), "%s%s%s",
                                             resolved,
                                             (resolvedLength > 0 && resolved[resolvedLength - 1] != '/') ? "/" : "",
                                             component);
                if (written < 0 || (size_t)written >= sizeof(candidate)) return false;

                struct stat info;
                if (stat(candidate, &info) != 0)
                {
                    // Varre o diretorio pai atras de uma diferenca so de caixa.
                    char parent[1024];
                    if (resolvedLength == 0) parent[0] = '\0';
                    else
                    {
                        if (resolvedLength + 1 > sizeof(parent)) return false;
                        memcpy(parent, resolved, resolvedLength + 1);
                    }
                    char actual[256];
                    if (!FindCaseInsensitiveEntry(parent, component, actual, sizeof(actual)))
                        return false;
                    const int rewritten = snprintf(candidate, sizeof(candidate), "%s%s%s",
                                                   resolved,
                                                   (resolvedLength > 0 && resolved[resolvedLength - 1] != '/') ? "/" : "",
                                                   actual);
                    if (rewritten < 0 || (size_t)rewritten >= sizeof(candidate)) return false;
                }

                const size_t candidateLength = strlen(candidate);
                if (candidateLength + 1 > capacity) return false;
                memcpy(resolved, candidate, candidateLength + 1);
                resolvedLength = candidateLength;

                cursor += length;
                if (*cursor == '/') ++cursor;
            }
            return resolvedLength > 0;
        }
    }
#endif

    bool ResolveLegacyCasePath(const char* path, char* resolved, size_t capacity)
    {
#ifdef _WIN32
        (void)path; (void)resolved; (void)capacity;
        return false;
#else
        return ResolveCasePath(path, resolved, capacity);
#endif
    }

    static char g_lastPath[1024] = {0};

    const char* LastOpenedPath()
    {
        return g_lastPath;
    }

    static LegacyAssetFetchHook g_assetFetchHook = NULL;
    static LegacyFileMissHook g_fileMissHook = NULL;
    static LegacyFileOpenHook g_fileOpenHook = NULL;

    void SetLegacyFileOpenHook(LegacyFileOpenHook hook)
    {
        g_fileOpenHook = hook;
    }

    void SetLegacyFileMissHook(LegacyFileMissHook hook)
    {
        g_fileMissHook = hook;
    }

    void SetLegacyAssetFetchHook(LegacyAssetFetchHook hook)
    {
        g_assetFetchHook = hook;
    }

    FILE* LegacyFileOpen(const char* path, const char* mode)
    {
        if (path == NULL || mode == NULL) return NULL;

#ifdef _WIN32
        return fopen(path, mode);
#else
        char normalized[1024];
        const char* target = NormalizeLegacyPath(path, normalized, sizeof(normalized));
        strncpy(g_lastPath, target, sizeof(g_lastPath) - 1);
        g_lastPath[sizeof(g_lastPath) - 1] = '\0';
        // Uma saida so, para o gancho de abertura nao deixar caminho de fora.
        FILE* file = fopen(target, mode);

        // Escrita nao se beneficia da busca: o arquivo pode legitimamente nao
        // existir ainda, e criar com a caixa pedida e o comportamento correto.
        const bool escrita = strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL;

        if (file == NULL && !escrita)
        {
            char resolved[1024];
            if (ResolveCasePath(target, resolved, sizeof(resolved)))
                file = fopen(resolved, mode);

            // Ultimo recurso: pedir ao alvo que traga o arquivo. So depois da
            // busca por caixa, para nao pagar uma requisicao por um arquivo que
            // ja existe localmente com outra capitalizacao.
            if (file == NULL && g_assetFetchHook != NULL && g_assetFetchHook(target))
                file = fopen(target, mode);
        }

        if (file != NULL)
        {
            if (g_fileOpenHook != NULL) g_fileOpenHook(target);
            return file;
        }

        if (!escrita && g_fileMissHook != NULL) g_fileMissHook(target);
        return NULL;
#endif
    }
}
