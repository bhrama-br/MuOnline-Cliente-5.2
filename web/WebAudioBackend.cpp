// Backend de audio do navegador, sobre a Web Audio API.
//
// O PCM ja vem decodificado por Platform::LoadWavClip, entao nao se usa
// decodeAudioData: o buffer e montado direto com createBuffer + getChannelData.
// Isso evita depender de o navegador saber ler .wav do MU (varios sao PCM de 8
// bits, que alguns decodificadores tratam de forma inconsistente) e usa o MESMO
// caminho de leitura das outras plataformas.
//
// Cada slot tem um GainNode proprio; tocar cria um AudioBufferSourceNode, que a
// especificacao trata como descartavel (um uso cada). Os nos ativos ficam
// registrados para que Stop consiga interrompe-los.

#include "PlatformAudio.h"

#include <emscripten.h>

#include <math.h>

#include <map>
#include <stdio.h>

namespace
{
    // Converte o volume do DirectSound (centesimos de decibel) em ganho linear,
    // que e o que o GainNode usa. -10000 (silencio) vira 0.
    double ToLinearGain(long directSoundVolume)
    {
        if (directSoundVolume <= -10000) return 0.0;
        // 10^(dB/20), com dB = centesimos/100.
        return pow(10.0, (double)directSoundVolume / 2000.0);
    }

    class WebAudioBackend : public Platform::IAudioBackend
    {
    public:
        WebAudioBackend() : m_masterVolume(0), m_ready(false) {}

        bool Initialize() override
        {
            if (m_ready) return true;
            // Sem virgulas no nivel superior do bloco: o pre-processador as trata
            // como separador de argumento da macro e o EM_ASM deixa de compilar.
            // Por isso os campos sao atribuidos um a um em vez de um literal.
            const int ok = MAIN_THREAD_EM_ASM_INT({
                try {
                    if (!Module.muAudio) {
                        var Context = window.AudioContext || window.webkitAudioContext;
                        if (!Context) return 0;
                        var audio = {};
                        audio.context = new Context();
                        audio.buffers = {};   /* slot -> AudioBuffer */
                        audio.gains = {};     /* slot -> GainNode */
                        audio.active = {};    /* slot -> lista de AudioBufferSourceNode */
                        Module.muAudio = audio;
                    }
                    return 1;
                } catch (e) { return 0; }
            });
            m_ready = ok != 0;
            if (!m_ready) fprintf(stderr, "[Platform] audio: Web Audio indisponivel\n");
            return m_ready;
        }

        void Shutdown() override
        {
            MAIN_THREAD_EM_ASM({
                if (Module.muAudio && Module.muAudio.context) {
                    try { Module.muAudio.context.close(); } catch (e) {}
                    Module.muAudio = null;
                }
            });
            m_ready = false;
        }

        bool LoadSound(int slot, const Platform::AudioClip& clip, int, bool) override
        {
            if (!m_ready && !Initialize()) return false;
            if (clip.samples == 0 || clip.frameCount == 0) return false;

            // O PCM e copiado para um AudioBuffer; o wasm pode realocar a memoria
            // depois, entao nada de guardar o ponteiro do lado JS.
            const int ok = MAIN_THREAD_EM_ASM_INT({
                var audio = Module.muAudio;
                if (!audio) return 0;
                var slot = $0;
                var ptr = $1;
                var frames = $2;
                var channels = $3;
                var rate = $4;
                var buffer = audio.context.createBuffer(channels, frames, rate);
                for (var c = 0; c < channels; ++c) {
                    var out = buffer.getChannelData(c);
                    for (var i = 0; i < frames; ++i) {
                        // HEAP16 e indexado por elemento de 16 bits.
                        var sample = HEAP16[(ptr >> 1) + i * channels + c];
                        out[i] = sample / 32768.0;
                    }
                }
                audio.buffers[slot] = buffer;
                if (!audio.gains[slot]) {
                    var gain = audio.context.createGain();
                    gain.connect(audio.context.destination);
                    audio.gains[slot] = gain;
                }
                audio.active[slot] = [];
                return 1;
            }, slot, (int)(size_t)clip.samples, (int)clip.frameCount,
               clip.channelCount, clip.sampleRate);

            if (ok) m_slotVolume[slot] = 0;
            return ok != 0;
        }

        void ReleaseSound(int slot) override
        {
            Stop(slot, true);
            MAIN_THREAD_EM_ASM({
                var audio = Module.muAudio;
                if (!audio) return;
                delete audio.buffers[$0];
                if (audio.gains[$0]) { audio.gains[$0].disconnect(); delete audio.gains[$0]; }
                delete audio.active[$0];
            }, slot);
            m_slotVolume.erase(slot);
        }

        bool Play(int slot, bool looped, const float*) override
        {
            if (!m_ready) return false;
            const int ok = MAIN_THREAD_EM_ASM_INT({
                var audio = Module.muAudio;
                if (!audio || !audio.buffers[$0]) return 0;
                // Navegadores suspendem o contexto ate um gesto do usuario; sem
                // isto o primeiro som seria descartado em silencio.
                if (audio.context.state === 'suspended') audio.context.resume();
                var source = audio.context.createBufferSource();
                source.buffer = audio.buffers[$0];
                source.loop = ($1 != 0);
                source.connect(audio.gains[$0]);
                var list = audio.active[$0] || (audio.active[$0] = []);
                source.onended = function() {
                    var index = list.indexOf(source);
                    if (index >= 0) list.splice(index, 1);
                };
                list.push(source);
                source.start(0);
                return 1;
            }, slot, looped ? 1 : 0);
            return ok != 0;
        }

        void Stop(int slot, bool) override
        {
            MAIN_THREAD_EM_ASM({
                var audio = Module.muAudio;
                if (!audio || !audio.active[$0]) return;
                var list = audio.active[$0];
                for (var i = 0; i < list.length; ++i) {
                    try { list[i].stop(0); } catch (e) {}
                }
                audio.active[$0] = [];
            }, slot);
        }

        void StopAll() override
        {
            MAIN_THREAD_EM_ASM({
                var audio = Module.muAudio;
                if (!audio) return;
                for (var slot in audio.active) {
                    var list = audio.active[slot];
                    for (var i = 0; i < list.length; ++i) {
                        try { list[i].stop(0); } catch (e) {}
                    }
                    audio.active[slot] = [];
                }
            });
        }

        void SetVolume(int slot, long volume) override
        {
            m_slotVolume[slot] = volume;
            ApplyGain(slot);
        }

        void SetMasterVolume(long volume) override
        {
            m_masterVolume = volume;
            for (std::map<int, long>::const_iterator it = m_slotVolume.begin();
                 it != m_slotVolume.end(); ++it)
                ApplyGain(it->first);
        }

        void SetListener(const float*, const float*) override
        {
            // TODO(Platform): audio posicional. A Web Audio tem PannerNode; falta
            // definir a escala entre unidades de mundo do MU e metros.
        }

        bool PlayMusic(const char*, bool) override
        {
            // TODO(Platform): musica de fundo. Os arquivos sao .mp3 no sistema de
            // arquivos virtual; daria para le-los e usar decodeAudioData.
            return false;
        }
        void StopMusic(const char*, bool) override {}
        bool IsMusicFinished() override { return true; }
        int  GetMusicPosition() override { return 0; }

    private:
        void ApplyGain(int slot)
        {
            std::map<int, long>::const_iterator it = m_slotVolume.find(slot);
            if (it == m_slotVolume.end()) return;
            // Os dois volumes somam em decibel, que e multiplicar os ganhos.
            const double gain = ToLinearGain(it->second) * ToLinearGain(m_masterVolume);
            MAIN_THREAD_EM_ASM({
                var audio = Module.muAudio;
                if (audio && audio.gains[$0]) audio.gains[$0].gain.value = $1;
            }, slot, gain);
        }

        std::map<int, long> m_slotVolume;
        long m_masterVolume;
        bool m_ready;
    };

    WebAudioBackend g_backend;
}

namespace Platform
{
    void InitializeWebAudio()
    {
        SetAudioBackend(&g_backend);
        g_backend.Initialize();
    }
}
