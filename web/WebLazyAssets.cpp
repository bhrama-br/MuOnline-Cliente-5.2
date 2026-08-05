// Busca de assets sob demanda, substituindo --preload-file para Data/Interface.
//
// O pacote inicial tinha 52 MB porque Data/Interface (35 MB, ~400 arquivos) era
// empacotada inteira, e a pagina so comecava a desenhar depois de baixar tudo.
// Na pratica a tela de titulo usa 14 dessas texturas.
//
// A dificuldade nao e baixar, e QUANDO baixar: o codigo legado le assets de
// forma sincrona em centenas de pontos (LoadBitmap, OpenBmd, OpenTerrain...),
// dentro de funcoes que nao podem devolver o controle ao navegador no meio.
// Uma busca assincrona exigiria reescrever todos esses pontos.
//
// A saida e um XMLHttpRequest SINCRONO disparado no gancho de LegacyFileOpen: o
// arquivo e gravado no MEMFS e a abertura e repetida, entao o chamador legado
// nao percebe diferenca. Custo aceito: a thread principal bloqueia durante a
// requisicao. E o mesmo custo que FS.createLazyFile do Emscripten paga, sem a
// maquinaria de HEAD e Range que ele usa para ler em pedacos.
//
// TODO(Platform): pre-buscar em paralelo nos limites de cena (antes de
// OpenBasicData, por exemplo), para que o caminho sincrono quase nunca seja
// exercitado. Isso vale mais quando o jogo carregar modelos, nao so a UI.

#include "../source/Platform/LegacyFileAccess.h"

#include <emscripten.h>
#include <stdio.h>
#include <string.h>

namespace
{
    // Evita repetir a requisicao de um arquivo que ja deu 404. O legado tenta
    // varias extensoes para o mesmo asset (.jpg, .OZJ, .tga, .OZT), entao a
    // falha e um caminho NORMAL, nao excepcional: sem cache negativo, cada
    // tentativa custaria uma viagem ate o servidor.
    const int kMaxAusentes = 512;
    char g_ausentes[kMaxAusentes][256];
    int  g_totalAusentes = 0;

    // Contadores para o relatorio: quanto o carregamento sob demanda de fato
    // buscou, contra os 35 MB que o pacote trazia de antemao.
    int  g_buscados = 0;
    long g_bytesBuscados = 0;

    bool JaFalhou(const char* caminho)
    {
        for (int i = 0; i < g_totalAusentes; ++i)
            if (strcmp(g_ausentes[i], caminho) == 0) return true;
        return false;
    }

    void MarcarAusente(const char* caminho)
    {
        if (g_totalAusentes >= kMaxAusentes) return;
        if (strlen(caminho) >= sizeof(g_ausentes[0])) return;
        strcpy(g_ausentes[g_totalAusentes++], caminho);
    }

    // Rastro de cada tentativa. Deixar LIGADO custa caro -- sao milhares de
    // mensagens, e so o custo do log dominou a medicao quando foi usado. Mas e
    // a unica forma de saber em QUE arquivo uma carga longa morre: o contador de
    // progresso so registra sucessos, entao o culpado nunca aparece nele. Foi
    // assim que `Data/Gate.bmd` apareceu, depois de tres hipoteses erradas.
    const bool kRastrearTentativas = false;

    // Rastro TARDIO: liga o log so depois de N buscas. Rastrear tudo custa
    // milhares de mensagens pelo CDP e o proprio log passa a dominar o tempo --
    // com isso a carga nem chegava ao ponto de falha dentro da janela de teste.
    // Zero desliga.
    const int kRastrearAPartirDe = 0;

    // Arquivos buscados que ainda estao no MEMFS, em ordem de chegada.
    //
    // POR QUE LIBERAR: cada asset baixado era gravado no MEMFS e ficava lá para
    // sempre. Na carga completa isso acumula ~105 MB, e por volta desse ponto o
    // FS.writeFile passa a falhar -- o gancho devolve -1 e o cliente conclui "arquivo
    // ausente" para um arquivo que o servidor entrega normalmente (confirmado com
    // curl: HTTP 200). O sintoma seguinte e LoadBitmap Failed e saida do cliente,
    // longe da causa.
    //
    // Cada asset e lido UMA vez durante a carga, entao guardar todos e desperdicio.
    // Um anel de 8 da folga para carregadores que abrem arquivos de forma aninhada
    // (um .bmd que abre suas texturas antes de fechar): so o mais antigo sai, e
    // apagar um arquivo que ainda estivesse aberto exigiria 8 aberturas simultaneas.
    //
    // Se algo for pedido de novo depois de sair do anel, a proxima abertura falha e a
    // busca acontece outra vez -- mais lento, e correto.
    const int kAnelMemfs = 8;
    char g_anel[kAnelMemfs][256];
    int  g_anelProximo = 0;
    bool g_anelCheio = false;
    long g_bytesLiberados = 0;
    int  g_arquivosLiberados = 0;

    void LembrarNoMemfs(const char* caminho)
    {
        if (strlen(caminho) >= sizeof(g_anel[0])) return;

        // A posicao que vamos ocupar guarda o mais antigo: apaga antes de sobrescrever.
        if (g_anelCheio && g_anel[g_anelProximo][0] != '\0')
        {
            const int liberados = EM_ASM_INT({
                var caminho = UTF8ToString($0);
                try {
                    var tamanho = FS.stat(caminho).size;
                    FS.unlink(caminho);
                    return tamanho;
                } catch (e) { return 0; }
            }, g_anel[g_anelProximo]);
            if (liberados > 0)
            {
                g_bytesLiberados += liberados;
                ++g_arquivosLiberados;
            }
        }

        strcpy(g_anel[g_anelProximo], caminho);
        g_anelProximo = (g_anelProximo + 1) % kAnelMemfs;
        if (g_anelProximo == 0) g_anelCheio = true;
    }

    bool BuscarSincrono(const char* caminho)
    {
        if (kRastrearTentativas ||
            (kRastrearAPartirDe > 0 && g_buscados >= kRastrearAPartirDe))
            emscripten_log(EM_LOG_ERROR, "tentando: %s", caminho);

        // Rastro FILTRADO: so os caminhos que contem esta substring. Rastrear
        // tudo afoga o log e o proprio custo do log domina a medicao; filtrar por
        // subsistema mantem o volume baixo e a informacao util. Nulo desliga.
        const char* kFiltroRastro = 0;
        if (kFiltroRastro != 0 && strstr(caminho, kFiltroRastro) != 0)
            emscripten_log(EM_LOG_ERROR, "mundo95: %s", caminho);
        if (JaFalhou(caminho)) return false;

        // O caminho virtual do MEMFS e o mesmo da URL: os assets sao servidos a
        // partir da raiz da pagina, na mesma arvore Data/... que o jogo monta.
        const int bytes = EM_ASM_INT({
            var caminho = UTF8ToString($0);
            var url = caminho.charAt(0) === '/' ? caminho.substring(1) : caminho;
            var xhr = new XMLHttpRequest();
            xhr.open('GET', url, false);
            // responseType nao pode ser definido em requisicao sincrona na thread
            // principal. Este mimetype faz o navegador entregar os bytes crus em
            // responseText, um char por byte, sem tentar decodificar UTF-8.
            xhr.overrideMimeType('text/plain; charset=x-user-defined');
            try { xhr.send(null); } catch (e) { return -1; }
            if (xhr.status !== 200 && xhr.status !== 0) return -1;

            var texto = xhr.responseText;
            var dados = new Uint8Array(texto.length);
            for (var i = 0; i < texto.length; ++i)
                dados[i] = texto.charCodeAt(i) & 0xFF;

            var barra = caminho.lastIndexOf('/');
            if (barra > 0) {
                try { FS.mkdirTree(caminho.substring(0, barra)); } catch (e) {}
            }
            // -2 distingue falha de GRAVACAO de falha de rede (-1), e a mensagem do
            // erro e impressa: engolir a excecao fazia toda falha parecer "arquivo
            // ausente", e foi assim que um esgotamento de memoria passou por asset
            // faltando.
            try {
                FS.writeFile(caminho, dados);
            } catch (e) {
                err('[assets] FS.writeFile falhou em ' + caminho + ': ' + e);
                return -2;
            }
            return dados.length;
        }, caminho);

        if (bytes == -2)
        {
            // Nao entra no cache negativo: a rede trouxe o arquivo, o que faltou foi
            // espaco. Marcar como ausente esconderia o problema e impediria uma nova
            // tentativa depois de o anel liberar memoria.
            emscripten_log(EM_LOG_ERROR, "  gravacao falhou: %s", caminho);
            return false;
        }

        if (bytes < 0)
        {
            // Todos os ausentes, nao os primeiros 120.
            //
            // O corte em 120 escondia exatamente o que se procurava: a lista chegava
            // ao limite dentro de Data/Object95 e as texturas do ceu (que vem depois)
            // nunca apareciam. Um corte silencioso num log de diagnostico faz a
            // lista parecer completa quando nao e.
            if (g_totalAusentes < kMaxAusentes)
                emscripten_log(EM_LOG_ERROR, "  ausente: %s", caminho);
            else if (g_totalAusentes == kMaxAusentes)
                emscripten_log(EM_LOG_ERROR, "  ausente: (limite de %d atingido; "
                               "os proximos nao serao listados)", kMaxAusentes);
            MarcarAusente(caminho);
            return false;
        }
        ++g_buscados;
        g_bytesBuscados += bytes;
        LembrarNoMemfs(caminho);

        // Sinal de progresso durante cargas longas. Sem isto, uma carga que
        // trava e indistinguivel de uma que so esta demorando: as duas mostram
        // uma aba parada.
        if ((g_buscados % 100) == 0)
        {
            char linha[300];
            snprintf(linha, sizeof(linha), "  ... %d arquivos (%.1f MB), ultimo: %s",
                     g_buscados, g_bytesBuscados / (1024.0 * 1024.0), caminho);
            emscripten_log(EM_LOG_ERROR, "%s", linha);
        }
        return true;
    }
}

namespace Platform
{
    void InstalarBuscaSobDemanda()
    {
        SetLegacyAssetFetchHook(&BuscarSincrono);
    }

    void RelatarBuscaSobDemanda(void (*log)(const char*))
    {
        char linha[200];
        snprintf(linha, sizeof(linha),
                 "assets sob demanda: %d arquivos, %.1f MB (%d ausentes em cache; "
                 "%d liberados do MEMFS, %.1f MB)",
                 g_buscados, g_bytesBuscados / (1024.0 * 1024.0), g_totalAusentes,
                 g_arquivosLiberados, g_bytesLiberados / (1024.0 * 1024.0));
        log(linha);
    }
}
