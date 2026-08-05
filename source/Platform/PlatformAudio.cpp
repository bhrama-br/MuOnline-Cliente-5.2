#include "PlatformAudio.h"
#include "LegacyFileAccess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Platform
{
    // ---- Leitor de WAV --------------------------------------------------------
    //
    // Formato RIFF: "RIFF" <tamanho> "WAVE" e depois blocos <id><tamanho><dados>.
    // Interessam "fmt " (formato) e "data" (amostras). Os demais sao pulados —
    // varios .wav do cliente trazem "LIST"/"fact" antes do "data".
    namespace
    {
        struct RiffChunk { char id[4]; unsigned int size; };

        bool ReadExact(FILE* file, void* destination, size_t size)
        {
            return fread(destination, 1, size, file) == size;
        }
    }

    bool LoadWavClip(const char* path, AudioClip& clip)
    {
        clip.samples = 0;
        clip.frameCount = 0;
        clip.channelCount = 0;
        clip.sampleRate = 0;
        if (path == 0) return false;

        FILE* file = LegacyFileOpen(path, "rb");
        if (file == 0) return false;

        char riff[12];
        if (!ReadExact(file, riff, sizeof(riff)) ||
            memcmp(riff, "RIFF", 4) != 0 || memcmp(riff + 8, "WAVE", 4) != 0)
        {
            fclose(file);
            return false;
        }

        int   formatTag = 0;
        int   channels = 0;
        int   sampleRate = 0;
        int   bitsPerSample = 0;
        bool  haveFormat = false;

        for (;;)
        {
            RiffChunk chunk;
            if (!ReadExact(file, &chunk, sizeof(chunk))) break;

            if (memcmp(chunk.id, "fmt ", 4) == 0)
            {
                unsigned char header[16];
                if (chunk.size < sizeof(header) || !ReadExact(file, header, sizeof(header))) break;
                formatTag     = header[0] | (header[1] << 8);
                channels      = header[2] | (header[3] << 8);
                sampleRate    = header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24);
                bitsPerSample = header[14] | (header[15] << 8);
                haveFormat = true;
                // Pula o resto do bloco (cbSize e extensoes).
                if (chunk.size > sizeof(header))
                    fseek(file, (long)(chunk.size - sizeof(header)), SEEK_CUR);
            }
            else if (memcmp(chunk.id, "data", 4) == 0)
            {
                // 1 = PCM sem compressao. Qualquer outro (ADPCM, mu-law) sairia
                // como ruido se fosse tratado como PCM, entao e recusado.
                if (!haveFormat || formatTag != 1 || channels <= 0 ||
                    (bitsPerSample != 8 && bitsPerSample != 16))
                    break;

                const size_t bytes = chunk.size;
                const size_t bytesPerSample = (size_t)(bitsPerSample / 8);
                const size_t frames = bytes / (bytesPerSample * (size_t)channels);
                if (frames == 0) break;

                short* samples = (short*)malloc(frames * (size_t)channels * sizeof(short));
                if (samples == 0) break;

                bool ok = true;
                if (bitsPerSample == 16)
                {
                    ok = ReadExact(file, samples, frames * (size_t)channels * sizeof(short));
                }
                else
                {
                    // 8 bits em WAV e SEM sinal, centrado em 128.
                    unsigned char* raw = (unsigned char*)malloc(frames * (size_t)channels);
                    ok = raw != 0 && ReadExact(file, raw, frames * (size_t)channels);
                    if (ok)
                    {
                        for (size_t i = 0; i < frames * (size_t)channels; ++i)
                            samples[i] = (short)((int)raw[i] - 128) << 8;
                    }
                    free(raw);
                }

                if (!ok) { free(samples); break; }

                clip.samples      = samples;
                clip.frameCount   = frames;
                clip.channelCount = channels;
                clip.sampleRate   = sampleRate;
                fclose(file);
                return true;
            }
            else
            {
                // Blocos de tamanho impar sao seguidos de um byte de preenchimento.
                fseek(file, (long)(chunk.size + (chunk.size & 1)), SEEK_CUR);
            }
        }

        fclose(file);
        return false;
    }

    void FreeAudioClip(AudioClip& clip)
    {
        free(clip.samples);
        clip.samples = 0;
        clip.frameCount = 0;
    }

    // ---- Backend padrao -------------------------------------------------------

    namespace
    {
        // Nao toca nada, mas avisa uma vez. Silencio sem explicacao seria pior:
        // daria para confundir "backend ausente" com "arquivo nao carregou".
        class NullAudioBackend : public IAudioBackend
        {
        public:
            bool Initialize() override
            {
                fprintf(stderr, "[Platform] audio sem backend registrado; nada sera tocado\n");
                return false;
            }
            void Shutdown() override {}
            bool LoadSound(int, const AudioClip&, int, bool) override { return false; }
            void ReleaseSound(int) override {}
            bool Play(int, bool, const float*) override { return false; }
            void Stop(int, bool) override {}
            void StopAll() override {}
            void SetVolume(int, long) override {}
            void SetMasterVolume(long) override {}
            void SetListener(const float*, const float*) override {}
            bool PlayMusic(const char*, bool) override { return false; }
            void StopMusic(const char*, bool) override {}
            bool IsMusicFinished() override { return true; }
            int  GetMusicPosition() override { return 0; }
        };

        NullAudioBackend g_nullBackend;
        IAudioBackend*   g_backend = 0;
    }

    IAudioBackend& GetAudioBackend()
    {
        return (g_backend != 0) ? *g_backend : (IAudioBackend&)g_nullBackend;
    }

    void SetAudioBackend(IAudioBackend* backend)
    {
        g_backend = backend;
    }
}
