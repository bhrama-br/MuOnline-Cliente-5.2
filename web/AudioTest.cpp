// Valida o leitor de WAV da camada Platform contra arquivos REAIS do cliente.
//
// O backend de audio nao pode ser testado sem placa de som, mas o passo anterior
// pode: se o PCM sair errado, nenhum backend salva. As checagens abaixo olham
// para propriedades que precisam valer, nao para bytes especificos:
//
//   1. o arquivo abre e devolve quadros;
//   2. taxa e canais estao em faixas plausiveis;
//   3. as amostras nao sao todas zero (silencio indicaria parse errado);
//   4. o pico cabe em 16 bits com sinal;
//   5. um arquivo inexistente falha em vez de devolver lixo;
//   6. um arquivo que NAO e WAV e recusado.

#include "PlatformAudio.h"

#include <cstdio>
#include <cstdlib>

namespace
{
    int g_failures = 0;

    void Check(bool condition, const char* description)
    {
        printf("%s %s\n", condition ? "[ok]  " : "[FALHA]", description);
        if (!condition) ++g_failures;
    }

    bool TestClip(const char* path)
    {
        Platform::AudioClip clip;
        if (!Platform::LoadWavClip(path, clip))
        {
            printf("[FALHA] nao carregou %s\n", path);
            ++g_failures;
            return false;
        }

        printf("       %s: %zu quadros, %d canais, %d Hz\n",
               path, clip.frameCount, clip.channelCount, clip.sampleRate);

        Check(clip.frameCount > 0, "o clipe tem quadros");
        Check(clip.channelCount == 1 || clip.channelCount == 2,
              "canais e mono ou estereo");
        Check(clip.sampleRate >= 8000 && clip.sampleRate <= 48000,
              "taxa de amostragem em faixa plausivel");

        long long sum = 0;
        int peak = 0;
        const size_t total = clip.frameCount * (size_t)clip.channelCount;
        for (size_t i = 0; i < total; ++i)
        {
            const int value = clip.samples[i];
            const int magnitude = value < 0 ? -value : value;
            if (magnitude > peak) peak = magnitude;
            sum += magnitude;
        }
        printf("       pico=%d  media=%lld\n", peak, (long long)(sum / (long long)total));
        Check(peak > 0, "as amostras nao sao todas zero");
        Check(peak <= 32768, "o pico cabe em 16 bits com sinal");

        Platform::FreeAudioClip(clip);
        Check(clip.samples == 0, "FreeAudioClip zera o ponteiro");
        return true;
    }
}

int main()
{
    printf("== teste do leitor de WAV ==\n");

    // Os dois caminhos do leitor: 8 bits (sem sinal, centrado em 128) e 16 bits
    // (com sinal). Juntos cobrem 99% dos .wav do cliente — a varredura dos 464
    // arquivos da raiz de Data\Sound deu 78% PCM 16 bits e 21% PCM 8 bits.
    TestClip("Data\\Sound\\mSpider1.wav");   // PCM 8 bits
    TestClip("Data\\Sound\\iRepair.wav");    // PCM 16 bits

    Platform::AudioClip missing;
    Check(!Platform::LoadWavClip("Data\\Sound\\nao_existe.wav", missing),
          "arquivo inexistente e recusado");

    // beep.wav e o UNICO MS ADPCM do cliente (formatTag 2). Recusar e o
    // comportamento certo: tratar ADPCM como PCM sairia como ruido alto.
    Platform::AudioClip adpcm;
    Check(!Platform::LoadWavClip("Data\\Sound\\beep.wav", adpcm),
          "WAV comprimido (MS ADPCM) e recusado em vez de virar ruido");

    // Um .OZB nao e RIFF: o leitor precisa recusar em vez de tratar o cabecalho
    // como formato e devolver ruido.
    Platform::AudioClip notWav;
    Check(!Platform::LoadWavClip("Data\\World10\\TerrainHeight.OZB", notWav),
          "arquivo que nao e WAV e recusado");

    printf("== %s ==\n", g_failures == 0 ? "todos passaram" : "HOUVE FALHAS");
    return g_failures == 0 ? 0 : 1;
}
