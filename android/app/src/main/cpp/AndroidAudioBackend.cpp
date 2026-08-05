// Backend de audio do Android, sobre OpenSL ES.
//
// OpenSL ES foi escolhido em vez de AAudio porque funciona desde a API 21 (o
// projeto tem minSdk 24) e porque a fila de buffers PCM encaixa direto no
// modelo do cliente: som ja decodificado, tocado inteiro, sem streaming.
//
// Cada slot SOUND_* guarda o PCM e um conjunto de "vozes". Uma voz e um player
// independente; ter varias permite o mesmo som soar sobreposto, que e o que o
// parametro BufferChannel do LoadWaveFile pedia no DirectSound.

#include "PlatformAudio.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>

#include <map>
#include <math.h>
#include <string.h>
#include <vector>

namespace
{
    void LogAudio(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        __android_log_vprint(ANDROID_LOG_ERROR, "MuLegacy", format, args);
        va_end(args);
    }

    // O legado usa a escala do DirectSound: centesimos de decibel, 0 = original,
    // -10000 = silencio. OpenSL ES usa milibeis, que e a MESMA unidade — mas com
    // limites proprios, entao o valor so precisa ser recortado.
    SLmillibel ToMillibel(long directSoundVolume, SLmillibel maximum)
    {
        if (directSoundVolume <= -10000) return SL_MILLIBEL_MIN;
        long value = directSoundVolume;
        if (value > maximum) value = maximum;
        return (SLmillibel)value;
    }

    struct Voice
    {
        SLObjectItf                    player;
        SLPlayItf                      play;
        SLAndroidSimpleBufferQueueItf  queue;
        SLVolumeItf                    volume;
    };

    struct Slot
    {
        std::vector<short> samples;
        int                channelCount;
        int                sampleRate;
        std::vector<Voice> voices;
        size_t             nextVoice;
        long               volume;
    };

    class AndroidAudioBackend : public Platform::IAudioBackend
    {
    public:
        AndroidAudioBackend()
            : m_engineObject(0), m_engine(0), m_outputMix(0), m_masterVolume(0), m_ready(false) {}

        bool Initialize() override
        {
            if (m_ready) return true;

            if (slCreateEngine(&m_engineObject, 0, 0, 0, 0, 0) != SL_RESULT_SUCCESS ||
                (*m_engineObject)->Realize(m_engineObject, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS ||
                (*m_engineObject)->GetInterface(m_engineObject, SL_IID_ENGINE, &m_engine) != SL_RESULT_SUCCESS)
            {
                LogAudio("audio: falha ao criar o motor OpenSL ES");
                Shutdown();
                return false;
            }

            if ((*m_engine)->CreateOutputMix(m_engine, &m_outputMix, 0, 0, 0) != SL_RESULT_SUCCESS ||
                (*m_outputMix)->Realize(m_outputMix, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS)
            {
                LogAudio("audio: falha ao criar a mixagem de saida");
                Shutdown();
                return false;
            }

            m_ready = true;
            __android_log_print(ANDROID_LOG_INFO, "MuLegacy", "audio: OpenSL ES pronto");
            return true;
        }

        void Shutdown() override
        {
            for (std::map<int, Slot>::iterator it = m_slots.begin(); it != m_slots.end(); ++it)
                DestroyVoices(it->second);
            m_slots.clear();

            if (m_outputMix != 0) { (*m_outputMix)->Destroy(m_outputMix); m_outputMix = 0; }
            if (m_engineObject != 0) { (*m_engineObject)->Destroy(m_engineObject); m_engineObject = 0; }
            m_engine = 0;
            m_ready = false;
        }

        bool LoadSound(int slot, const Platform::AudioClip& clip, int voiceCount, bool) override
        {
            if (!m_ready && !Initialize()) return false;
            if (clip.samples == 0 || clip.frameCount == 0) return false;

            Slot& target = m_slots[slot];
            DestroyVoices(target);

            const size_t total = clip.frameCount * (size_t)clip.channelCount;
            target.samples.assign(clip.samples, clip.samples + total);
            target.channelCount = clip.channelCount;
            target.sampleRate   = clip.sampleRate;
            target.nextVoice    = 0;
            target.volume       = 0;

            // Uma voz basta para a maioria; mais que quatro so desperdicaria
            // players, e quatro e o MAX_CHANNEL do legado.
            int voices = voiceCount;
            if (voices < 1) voices = 1;
            if (voices > 4) voices = 4;

            for (int i = 0; i < voices; ++i)
            {
                Voice voice;
                if (CreateVoice(target, voice)) target.voices.push_back(voice);
            }
            if (target.voices.empty())
            {
                LogAudio("audio: nenhum player criado para o slot %d", slot);
                m_slots.erase(slot);
                return false;
            }
            __android_log_print(ANDROID_LOG_INFO, "MuLegacy",
                                "audio: slot %d carregado (%zu quadros, %d canais, %d Hz, %zu vozes)",
                                slot, clip.frameCount, clip.channelCount, clip.sampleRate,
                                target.voices.size());
            return true;
        }

        void ReleaseSound(int slot) override
        {
            std::map<int, Slot>::iterator it = m_slots.find(slot);
            if (it == m_slots.end()) return;
            DestroyVoices(it->second);
            m_slots.erase(it);
        }

        bool Play(int slot, bool looped, const float*) override
        {
            std::map<int, Slot>::iterator it = m_slots.find(slot);
            if (it == m_slots.end() || it->second.voices.empty()) return false;

            Slot& target = it->second;
            // Rodizio entre as vozes: a mais antiga e reaproveitada, que e o que
            // o DirectSound fazia ao ficar sem canal livre.
            Voice& voice = target.voices[target.nextVoice];
            target.nextVoice = (target.nextVoice + 1) % target.voices.size();

            (*voice.play)->SetPlayState(voice.play, SL_PLAYSTATE_STOPPED);
            (*voice.queue)->Clear(voice.queue);

            const size_t bytes = target.samples.size() * sizeof(short);
            if ((*voice.queue)->Enqueue(voice.queue, &target.samples[0], (SLuint32)bytes) != SL_RESULT_SUCCESS)
                return false;

            // TODO(Platform): repeticao continua. A fila simples do OpenSL ES nao
            // tem loop nativo; seria preciso reenfileirar no callback. Os sons em
            // loop do cliente sao ambiente (vento, chuva), entao a primeira
            // passada ja da o efeito principal.
            (void)looped;
            (*voice.play)->SetPlayState(voice.play, SL_PLAYSTATE_PLAYING);
            return true;
        }

        void Stop(int slot, bool) override
        {
            std::map<int, Slot>::iterator it = m_slots.find(slot);
            if (it == m_slots.end()) return;
            for (size_t i = 0; i < it->second.voices.size(); ++i)
            {
                Voice& voice = it->second.voices[i];
                (*voice.play)->SetPlayState(voice.play, SL_PLAYSTATE_STOPPED);
                (*voice.queue)->Clear(voice.queue);
            }
        }

        void StopAll() override
        {
            for (std::map<int, Slot>::iterator it = m_slots.begin(); it != m_slots.end(); ++it)
                Stop(it->first, true);
        }

        void SetVolume(int slot, long volume) override
        {
            std::map<int, Slot>::iterator it = m_slots.find(slot);
            if (it == m_slots.end()) return;
            it->second.volume = volume;
            ApplyVolume(it->second);
        }

        void SetMasterVolume(long volume) override
        {
            m_masterVolume = volume;
            for (std::map<int, Slot>::iterator it = m_slots.begin(); it != m_slots.end(); ++it)
                ApplyVolume(it->second);
        }

        void SetListener(const float*, const float*) override
        {
            // TODO(Platform): audio 3D. OpenSL ES tem SL_IID_3DLOCATION, mas o
            // Android nao o implementa. Posicionar exigiria atenuar e panoramizar
            // por conta propria a partir da distancia ate a camera.
        }

        bool PlayMusic(const char*, bool) override
        {
            // TODO(Platform): musica de fundo. O caminho natural e MediaPlayer
            // pelo lado Java, nao OpenSL ES.
            return false;
        }
        void StopMusic(const char*, bool) override {}
        bool IsMusicFinished() override { return true; }
        int  GetMusicPosition() override { return 0; }

    private:
        bool CreateVoice(const Slot& slot, Voice& voice)
        {
            memset(&voice, 0, sizeof(voice));

            SLDataLocator_AndroidSimpleBufferQueue bufferQueue = {
                SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1
            };
            SLDataFormat_PCM format;
            format.formatType   = SL_DATAFORMAT_PCM;
            format.numChannels  = (SLuint32)slot.channelCount;
            // OpenSL ES mede em milihertz.
            format.samplesPerSec = (SLuint32)slot.sampleRate * 1000;
            format.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
            format.containerSize = 16;
            format.channelMask   = (slot.channelCount == 1)
                                 ? SL_SPEAKER_FRONT_CENTER
                                 : (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
            format.endianness    = SL_BYTEORDER_LITTLEENDIAN;

            SLDataSource source = { &bufferQueue, &format };
            SLDataLocator_OutputMix outputMix = { SL_DATALOCATOR_OUTPUTMIX, m_outputMix };
            SLDataSink sink = { &outputMix, 0 };

            const SLInterfaceID interfaces[] = { SL_IID_BUFFERQUEUE, SL_IID_VOLUME };
            const SLboolean required[]       = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

            if ((*m_engine)->CreateAudioPlayer(m_engine, &voice.player, &source, &sink,
                                               2, interfaces, required) != SL_RESULT_SUCCESS)
                return false;
            if ((*voice.player)->Realize(voice.player, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS ||
                (*voice.player)->GetInterface(voice.player, SL_IID_PLAY, &voice.play) != SL_RESULT_SUCCESS ||
                (*voice.player)->GetInterface(voice.player, SL_IID_BUFFERQUEUE, &voice.queue) != SL_RESULT_SUCCESS)
            {
                (*voice.player)->Destroy(voice.player);
                memset(&voice, 0, sizeof(voice));
                return false;
            }
            // Volume e opcional: sem ele o som ainda toca, so nao da para atenuar.
            (*voice.player)->GetInterface(voice.player, SL_IID_VOLUME, &voice.volume);
            return true;
        }

        void DestroyVoices(Slot& slot)
        {
            for (size_t i = 0; i < slot.voices.size(); ++i)
            {
                Voice& voice = slot.voices[i];
                if (voice.play != 0) (*voice.play)->SetPlayState(voice.play, SL_PLAYSTATE_STOPPED);
                if (voice.player != 0) (*voice.player)->Destroy(voice.player);
            }
            slot.voices.clear();
        }

        void ApplyVolume(Slot& slot)
        {
            // Os dois volumes se somam em decibel, que e multiplicacao em ganho —
            // a mesma composicao que o DirectSound fazia.
            const long combined = slot.volume + m_masterVolume;
            for (size_t i = 0; i < slot.voices.size(); ++i)
            {
                Voice& voice = slot.voices[i];
                if (voice.volume == 0) continue;
                SLmillibel maximum = 0;
                (*voice.volume)->GetMaxVolumeLevel(voice.volume, &maximum);
                (*voice.volume)->SetVolumeLevel(voice.volume, ToMillibel(combined, maximum));
            }
        }

        SLObjectItf m_engineObject;
        SLEngineItf m_engine;
        SLObjectItf m_outputMix;
        long        m_masterVolume;
        bool        m_ready;
        std::map<int, Slot> m_slots;
    };

    AndroidAudioBackend g_backend;
}

namespace Platform
{
    void InitializeAndroidAudio()
    {
        SetAudioBackend(&g_backend);
        g_backend.Initialize();
    }
}
