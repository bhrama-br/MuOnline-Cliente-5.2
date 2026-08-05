// Implementa, fora do Windows, as funcoes livres de audio que o cliente chama.
//
// No Windows quem responde e DSplaysound.cpp, sobre DirectSound; este arquivo
// nem entra no build de la. Nas demais plataformas ele traduz a mesma API para
// Platform::IAudioBackend, sem que nenhum chamador precise mudar.
//
// A conversao importante e a de volume: o legado usa a escala do DirectSound
// (centesimos de decibel, 0 = original, -10000 = silencio) e os backends
// modernos usam ganho linear. A conversao vive no backend, nao aqui, porque cada
// API tem a sua (OpenSL ES tambem usa milibeis; Web Audio usa ganho linear).

#if !defined(_WIN32)

// "../stdafx.h", nao "stdafx.h": existe um Platform/stdafx.h que sombreia o do
// jogo para os arquivos desta pasta, e ele nao traz os tipos do cliente.
#include "../stdafx.h"
#include "PlatformAudio.h"
#include "../DSPlaySound.h"
#include "../ZzzOpenglUtil.h"
#include "../w_ObjectInfo.h"

#include <map>
#include <string.h>

namespace
{
    // Slots ja carregados, para nao reler o .wav a cada PlayBuffer e para saber
    // o que liberar em DestroySound.
    std::map<int, bool> g_loadedSlots;
    bool g_soundEnabled = true;
}

void SetEnableSound(bool enable)
{
    g_soundEnabled = enable;
    if (!enable) Platform::GetAudioBackend().StopAll();
}

void LoadWaveFile(int Buffer, TCHAR* strFileName, int BufferChannel, bool Enable3DSound)
{
    if (strFileName == NULL) return;

    Platform::AudioClip clip;
    if (!Platform::LoadWavClip(strFileName, clip))
    {
        // Sem PLATFORM_STUB_ONCE: aqui interessa saber QUAL arquivo faltou.
        fprintf(stderr, "[Platform] audio: nao foi possivel carregar %s\n", strFileName);
        return;
    }

    const bool ok = Platform::GetAudioBackend().LoadSound(Buffer, clip, BufferChannel, Enable3DSound);
    // O backend copia o que precisa; o clipe e do chamador.
    Platform::FreeAudioClip(clip);
    if (ok) g_loadedSlots[Buffer] = true;
}

HRESULT PlayBuffer(int Buffer, OBJECT* Object, BOOL bLooped)
{
    if (!g_soundEnabled) return S_OK;

    Platform::GetAudioBackend().Play(Buffer, bLooped != 0,
                                     (Object != NULL) ? Object->Position : NULL);
    return S_OK;
}

void StopBuffer(int Buffer, BOOL bResetPosition)
{
    Platform::GetAudioBackend().Stop(Buffer, bResetPosition != 0);
}

void AllStopSound(void)
{
    Platform::GetAudioBackend().StopAll();
}

HRESULT ReleaseBuffer(int Buffer)
{
    Platform::GetAudioBackend().ReleaseSound(Buffer);
    g_loadedSlots.erase(Buffer);
    return S_OK;
}

HRESULT RestoreBuffers(int, int)
{
    // Exclusivo do DirectSound: buffers eram perdidos ao trocar o foco. Nem
    // OpenSL ES nem Web Audio tem esse conceito.
    return S_OK;
}

void SetVolume(int Buffer, long vol)
{
    Platform::GetAudioBackend().SetVolume(Buffer, vol);
}

void SetMasterVolume(long vol)
{
    Platform::GetAudioBackend().SetMasterVolume(vol);
}

void Set3DSoundPosition()
{
    // Posicao e orientacao do ouvinte vem da camera da cena (ZzzOpenglUtil.h).
    Platform::GetAudioBackend().SetListener(CameraPosition, CameraAngle);
}

void DestroySound()
{
    Platform::GetAudioBackend().StopAll();
    for (std::map<int, bool>::const_iterator it = g_loadedSlots.begin();
         it != g_loadedSlots.end(); ++it)
        Platform::GetAudioBackend().ReleaseSound(it->first);
    g_loadedSlots.clear();
    Platform::GetAudioBackend().Shutdown();
}

void FreeDirectSound()
{
    DestroySound();
}

// ---- Musica de fundo --------------------------------------------------------
//
// O legado toca MP3 por MCI. Os backends decidem o formato: o navegador toca o
// arquivo direto, e no Android o MediaPlayer faria o mesmo. Enquanto nenhum dos
// dois implementa, as chamadas caem no backend nulo e sao registradas la.

void PlayMp3(char* Name, BOOL bEnforce)
{
    if (!g_soundEnabled || Name == NULL) return;
    Platform::GetAudioBackend().PlayMusic(Name, bEnforce != 0);
}

void StopMp3(char* Name, BOOL bEnforce)
{
    Platform::GetAudioBackend().StopMusic(Name, bEnforce != 0);
}

bool IsEndMp3()
{
    return Platform::GetAudioBackend().IsMusicFinished();
}

int GetMp3PlayPosition()
{
    return Platform::GetAudioBackend().GetMusicPosition();
}

#endif // !_WIN32
