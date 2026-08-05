#ifndef PLATFORM_AUDIO_H
#define PLATFORM_AUDIO_H

#include <stddef.h>

// Camada de audio multiplataforma.
//
// O cliente fala com o audio por funcoes livres declaradas em DSPlaySound.h
// (LoadWaveFile, PlayBuffer, StopBuffer, ...) implementadas sobre DirectSound em
// DSplaysound.cpp. Esse arquivo depende de <dsound.h> e nao existe fora do
// Windows, entao as chamadas ficavam como simbolos indefinidos.
//
// Aqui a interface e a mesma, so que atras de um backend. No Windows nada muda:
// DSplaysound.cpp continua sendo a implementacao. Nas demais plataformas as
// funcoes livres viram encaminhamento para IAudioBackend.
namespace Platform
{
    // PCM decodificado de um .wav, no formato que os backends consomem.
    struct AudioClip
    {
        short*       samples;      // intercalado, 16 bits com sinal
        size_t       frameCount;   // quadros (nao amostras)
        int          channelCount;
        int          sampleRate;
    };

    // Le um RIFF/WAVE PCM e converte para 16 bits com sinal.
    //
    // O cliente so distribui .wav PCM de 8 ou 16 bits, entao o parser cobre
    // exatamente isso e recusa o resto em vez de tentar adivinhar. O chamador
    // fica dono de `clip.samples` e libera com FreeAudioClip.
    bool LoadWavClip(const char* path, AudioClip& clip);
    void FreeAudioClip(AudioClip& clip);

    class IAudioBackend
    {
    public:
        virtual ~IAudioBackend() {}

        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        // `slot` e o indice SOUND_* do legado; o backend guarda o clipe por slot.
        virtual bool LoadSound(int slot, const AudioClip& clip, int voiceCount, bool enable3d) = 0;
        virtual void ReleaseSound(int slot) = 0;

        // `position` e NULL quando o som nao e posicional.
        virtual bool Play(int slot, bool looped, const float* position) = 0;
        virtual void Stop(int slot, bool resetPosition) = 0;
        virtual void StopAll() = 0;

        // Volume na escala do DirectSound: centesimos de decibel, 0 = original,
        // -10000 = silencio. Mantida para nao reescrever os chamadores.
        virtual void SetVolume(int slot, long volume) = 0;
        virtual void SetMasterVolume(long volume) = 0;

        // Posicao e orientacao do ouvinte, em unidades de mundo do jogo.
        virtual void SetListener(const float* position, const float* forward) = 0;

        // Musica de fundo. O legado usa MP3 via MCI; o backend decide o formato
        // que consegue tocar.
        virtual bool PlayMusic(const char* path, bool restartIfSame) = 0;
        virtual void StopMusic(const char* path, bool force) = 0;
        virtual bool IsMusicFinished() = 0;
        virtual int  GetMusicPosition() = 0;
    };

    // Backend ativo. Sem nenhum registrado, devolve um que nao toca nada e
    // reporta uma vez — melhor do que silencio inexplicado.
    IAudioBackend& GetAudioBackend();
    void SetAudioBackend(IAudioBackend* backend);
}

#endif // PLATFORM_AUDIO_H
