#include <android/log.h>
#include <jni.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <unistd.h>
#include "LegacyRenderAdapter.h"
#include "LegacySceneMatrices.h"
#include "PlatformInput.h"
#include "PlatformShell.h"
#include "WindowsCompat.h"
#include "LegacySocketPump.h"
#include "LegacySceneBringup.h"
#include "LegacyFileAccess.h"

#include <stdlib.h>
#include <unistd.h>
#include <exception>
#include <pthread.h>
#include <string.h>
#include <malloc.h>   // mallinfo2, para atribuir memoria a arquivo
#include <errno.h>
#include <sys/stat.h>   // chmod, para o shell poder ler o rastro

namespace Platform { void InitializeAndroidAudio(); }

// Funcoes de audio do cliente, implementadas por Platform/LegacyAudioBridge.cpp.
// Sao declaradas aqui em vez de incluir DSPlaySound.h para nao arrastar a cadeia
// de cabecalhos do jogo para dentro da camada Android.
class OBJECT;
void    LoadWaveFile(int Buffer, TCHAR* strFileName, int BufferChannel, bool Enable3DSound);
HRESULT PlayBuffer(int Buffer, OBJECT* Object, BOOL bLooped);

// Codigo do proprio jogo. Referenciar estes simbolos e o que faz o linker puxar
// os modulos de ZzzLodTerrain.cpp para dentro da .so: sem uma chamada real, o
// arquivo estatico de 163 MB e descartado inteiro.
extern bool OpenTerrainHeight(char* filename);
extern float BackTerrainHeight[256 * 256];

// Roda ANTES de qualquer construtor global de C++ (prioridade 101; os construtores
// comuns usam a prioridade padrao). E o unico ponto em que da para posicionar o
// diretorio de trabalho a tempo: varios modulos do jogo abrem arquivos ja no
// construtor global, e sem isso o carregador segfaultava no arquivo ausente.
//
// A raiz vem de MU_DATA_ROOT, publicada pela Activity com Os.setenv antes de
// System.loadLibrary — nao da para chamar um metodo nativo desta mesma .so antes
// dela carregar.
// Manda stdout/stderr para o logcat.
//
// No Android o stderr vai para /dev/null: TODO diagnostico da camada Platform que
// usa `fprintf(stderr, ...)` -- e sao muitos, incluindo os avisos de asset e de
// audio -- era invisivel aqui, enquanto no navegador aparece no console. Isso me
// custou uma rodada: instrumentei uma medicao com fprintf e conclui que o codigo
// nem tinha sido alcancado.
//
// A tecnica e a usual: um pipe no lugar dos descritores 1 e 2, e uma thread lendo
// e reemitindo linha a linha.
static void* MuLegacyLogPump(void*)
{
    char line[512];
    ssize_t readCount;
    size_t used = 0;
    while ((readCount = read(STDIN_FILENO, line + used, sizeof(line) - used - 1)) > 0)
    {
        used += (size_t)readCount;
        line[used] = '\0';

        char* start = line;
        char* fim;
        while ((fim = strchr(start, '\n')) != 0)
        {
            *fim = '\0';
            if (*start != '\0')
                __android_log_print(ANDROID_LOG_INFO, "MuLegacyStdio", "%s", start);
            start = fim + 1;
        }
        used = strlen(start);
        memmove(line, start, used + 1);
    }
    return 0;
}

__attribute__((constructor(100)))
static void MuLegacyRedirectStdio()
{
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    int canal[2];
    if (pipe(canal) != 0) return;
    dup2(canal[1], STDOUT_FILENO);
    dup2(canal[1], STDERR_FILENO);
    dup2(canal[0], STDIN_FILENO);   // a thread le daqui

    pthread_t thread;
    if (pthread_create(&thread, 0, &MuLegacyLogPump, 0) == 0)
        pthread_detach(thread);
}

__attribute__((constructor(101)))
static void MuLegacySetWorkingDirectory()
{
    const char* root = getenv("MU_DATA_ROOT");
    if (root == 0 || root[0] == '\0')
    {
        __android_log_print(ANDROID_LOG_ERROR, "MuLegacy",
                            "MU_DATA_ROOT ausente; os assets nao serao encontrados");
        return;
    }
    if (chdir(root) != 0)
        __android_log_print(ANDROID_LOG_ERROR, "MuLegacy", "chdir falhou para %s", root);
    else
        __android_log_print(ANDROID_LOG_INFO, "MuLegacy", "raiz de dados: %s", root);
}

namespace
{
    JavaVM* g_javaVm = 0;
    jobject g_activityRef = 0;

    // Platform::OpenExternalUrl precisa da Activity para chamar startActivity, e a
    // chamada acontece muito depois deste JNI — dai a referencia global.
    void RememberActivity(JNIEnv* environment, jobject activity)
    {
        if (g_activityRef != 0 || activity == 0 || environment == 0) return;
        g_activityRef = environment->NewGlobalRef(activity);
        Platform::SetAndroidShellContext(g_javaVm, g_activityRef);
    }

    ANativeWindow* g_surface = 0;
    EGLDisplay g_display = EGL_NO_DISPLAY;
    EGLSurface g_eglSurface = EGL_NO_SURFACE;
    EGLContext g_context = EGL_NO_CONTEXT;
    int g_width = 0;
    int g_height = 0;
    float g_cameraX = 0.0f;
    float g_cameraY = 0.0f;

    // Distancia de orbita da camera, na escala de CameraDistance do gameplay.
    // Mesmos valores do build Web, que enquadram o pedaco de terreno carregado.
    const float kSceneCameraDistance = 9000.0f;
    const float kSceneViewFar = 30000.0f;

    const int kTerrainSize = 256;
    bool  g_terrainLoaded = false;
    float g_terrainMin = 0.0f;
    float g_terrainMax = 0.0f;

    void ForwardRenderLog(const char* message)
    {
        __android_log_print(ANDROID_LOG_ERROR, "MuLegacy", "%s", message);
    }

    void DestroyContext()
    {
        // O adapter GLSL precisa esquecer shader/VAO/VBO: os nomes pertencem ao
        // contexto que esta sendo destruido e nao valem no proximo.
        Platform::InvalidateLegacyRenderResources();
        if (g_display != EGL_NO_DISPLAY)
        {
            eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (g_eglSurface != EGL_NO_SURFACE) eglDestroySurface(g_display, g_eglSurface);
            if (g_context != EGL_NO_CONTEXT) eglDestroyContext(g_display, g_context);
            eglTerminate(g_display);
        }
        g_display = EGL_NO_DISPLAY;
        g_eglSurface = EGL_NO_SURFACE;
        g_context = EGL_NO_CONTEXT;
    }

    bool CreateContext()
    {
        const EGLint configAttributes[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24, EGL_NONE
        };
        const EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        EGLConfig config = 0;
        EGLint count = 0;
        g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (g_display == EGL_NO_DISPLAY || !eglInitialize(g_display, 0, 0) ||
            !eglChooseConfig(g_display, configAttributes, &config, 1, &count) || count == 0)
            return false;
        g_context = eglCreateContext(g_display, config, EGL_NO_CONTEXT, contextAttributes);
        g_eglSurface = eglCreateWindowSurface(g_display, config, g_surface, 0);
        if (g_context == EGL_NO_CONTEXT || g_eglSurface == EGL_NO_SURFACE ||
            !eglMakeCurrent(g_display, g_eglSurface, g_eglSurface, g_context))
        {
            DestroyContext();
            return false;
        }
        return true;
    }

    // Desenha o terreno REAL do World10, com as alturas que OpenTerrainHeight
    // (ZzzLodTerrain.cpp, codigo do jogo) deixou em BackTerrainHeight. Mesma
    // malha do build Web, para poder comparar as duas plataformas lado a lado.
    //
    void RenderRealTerrain(Platform::ILegacyRenderAdapter& renderer)
    {
        if (!g_terrainLoaded) return;

        const float scale = 100.0f;   // TERRAIN_SCALE do legado
        const int step = 2;
        const float span = (g_terrainMax > g_terrainMin) ? (g_terrainMax - g_terrainMin) : 1.0f;
        const int half = 48;
        const int cx = kTerrainSize / 2;
        const int cy = kTerrainSize / 2;

        renderer.Begin(Platform::LegacyPrimitiveTriangles);
        for (int y = cy - half; y < cy + half; y += step)
        {
            for (int x = cx - half; x < cx + half; x += step)
            {
                const int x2 = x + step;
                const int y2 = y + step;
                const float h00 = BackTerrainHeight[y * kTerrainSize + x];
                const float h10 = BackTerrainHeight[y * kTerrainSize + x2];
                const float h11 = BackTerrainHeight[y2 * kTerrainSize + x2];
                const float h01 = BackTerrainHeight[y2 * kTerrainSize + x];

                const float px = (x - cx) * scale;
                const float py = (y - cy) * scale;
                const float px2 = (x2 - cx) * scale;
                const float py2 = (y2 - cy) * scale;

                struct Corner { float x, y, h; };
                const Corner quad[4] = {
                    { px,  py,  h00 }, { px2, py,  h10 },
                    { px2, py2, h11 }, { px,  py2, h01 },
                };
                const int order[6] = { 0, 1, 2, 0, 2, 3 };
                for (int i = 0; i < 6; ++i)
                {
                    const Corner& c = quad[order[i]];
                    const float t = (c.h - g_terrainMin) / span;
                    const float shade = 0.35f + 0.65f * t;
                    renderer.Color4f(shade * 0.55f, shade * 0.75f, shade * 0.45f, 1.0f);
                    renderer.Vertex3f(c.x, c.y, c.h);
                }
            }
        }
        renderer.End();
    }

    // Estado da subida de cena. A tentativa acontece no PRIMEIRO quadro, e nao
    // na inicializacao, porque so ali existem contexto GL e tamanho de tela.
    //
    // Se falhar -- tipicamente por os assets do cliente nao estarem no
    // dispositivo -- cai de volta no terreno de teste em vez de mostrar tela
    // preta. A degradacao e explicita e registrada.
    bool g_sceneRequested = false;
    bool g_sceneActive = false;

    void LogScene(const char* message)
    {
        __android_log_print(ANDROID_LOG_ERROR, "MuLegacy", "%s", message);
    }

    // Varios pontos do cliente chamam exit(0) quando um arquivo falta. No Web o
    // build linka -lnoexit e a chamada e inofensiva, entao o jogo continua; no
    // Android ela encerra o processo de verdade -- e a saida acontece longe da
    // causa, porque os destrutores estaticos ainda tocam o GL e o que aparece no
    // log e um SIGSEGV em glDeleteTextures.
    //
    // Este gancho diz QUAL arquivo o cliente estava abrindo quando desistiu.
    // Nomeia cada arquivo que o cliente pediu e nao existe. Volume baixo: sao
    // dezenas, nao milhares, porque o cache negativo do proprio LegacyFileOpen
    // nao repete a tentativa.
    //
    // Substitui uma tentativa que NAO funcionou: um gancho `atexit`. Ele nunca
    // rodava, porque `atexit` executa em ordem inversa de registro e os objetos
    // `static` locais criados DURANTE a carga registram seus destrutores depois
    // do meu gancho -- entao eles rodam primeiro, e um deles derruba o processo
    // tocando o GL apos o contexto morrer.
    void ReportMissingFile(const char* path)
    {
        __android_log_print(ANDROID_LOG_WARN, "MuLegacy", "ausente: %s", path);
    }

    // Atribui consumo de memoria a arquivo.
    //
    // Diagnostico de uma morte que nao deixa rastro: o heap nativo vai a 2,6 GB em
    // ~4 s durante a carga e o LMK mata o processo com SIGKILL -- sem tombstone,
    // sem mensagem, e o ultimo log do app fica a minutos da causa. Amostrar por
    // fora (`dumpsys meminfo`) mostrou a curva mas nao O QUE alocava.
    //
    // Como todo asset passa por LegacyFileOpen, medir aqui cobre todos os
    // carregadores sem tocar em codigo que o PC compila. O custo de um arquivo
    // aparece na abertura do SEGUINTE, entao o que se reporta e sempre o anterior.
    //
    // So imprime acima de um limiar: a carga abre milhares de arquivos, e registrar
    // todos afogaria o logcat (que descarta linhas em excesso, justamente as que
    // interessam).
    const size_t kLimiarRelato = 8u * 1024u * 1024u;   // 8 MB

    // Acima deste total, TODA abertura e registrada.
    //
    // Relatar o custo de um arquivo na abertura do seguinte tem um ponto cego
    // fatal justamente aqui: se o processo morre carregando o arquivo, nunca ha um
    // "seguinte", e o culpado nao aparece no log -- foi o que aconteceu na primeira
    // tentativa, que terminou sem uma unica linha. Depois de o heap entrar na faixa
    // suspeita o volume deixa de importar, e o nome ser impresso na ENTRADA passa a
    // valer mais do que o custo exato.
    //
    // Ficou em ZERO durante a investigacao: com 300 MB o rastro saia vazio, porque a
    // morte chegava antes de qualquer abertura acontecer nessa faixa -- a explosao
    // estava dentro da carga de UM arquivo (pet.bmd), e nomear todas foi o que
    // apontou o culpado. De volta a 300 MB agora que o defeito esta fechado: com
    // zero sao ~2900 linhas por execucao. Baixar de novo se outra alocacao suspeita
    // aparecer.
    const size_t kLimiarNomear = 300u * 1024u * 1024u;

    size_t BytesEmUso()
    {
        // mallinfo2 tem campos de 64 bits; o mallinfo antigo usa `int` e estoura
        // silenciosamente em 2 GB -- exatamente a faixa que se quer medir aqui.
        const struct mallinfo2 info = mallinfo2();
        return (size_t)info.uordblks;
    }

    char g_previousFile[1024] = {0};
    size_t g_bytesNaAbertura = 0;
    size_t g_picoBytes = 0;
    FILE* g_rastro = NULL;

    // Abre o rastro numa via independente do logcat E do diretorio corrente.
    //
    // Caminho absoluto vindo de MU_DATA_ROOT em vez de relativo: uma tentativa com
    // "mem-trace.log" relativo nao criou arquivo nenhum, e sem separar "gancho nao
    // dispara" de "escrita nao funciona" nao havia como saber qual das duas era.
    // Escrever aqui, no momento da instalacao, responde isso: se o cabecalho
    // aparecer e nao houver mais nada, o gancho e que nao dispara.
    //
    // O chmod existe porque arquivo criado pelo app nao e legivel pelo shell, e
    // `adb shell tail` e justamente como ele vai ser lido.
    void OpenMemoryTrace()
    {
        const char* raiz = getenv("MU_DATA_ROOT");
        if (raiz == NULL || raiz[0] == '\0') return;

        char path[1024];
        snprintf(path, sizeof(path), "%s/mem-trace.log", raiz);
        g_rastro = fopen(path, "w");
        if (g_rastro == NULL)
        {
            __android_log_print(ANDROID_LOG_ERROR, "MuLegacy",
                "rastro: nao consegui abrir %s (errno %d)", path, errno);
            return;
        }
        chmod(path, 0666);
        fprintf(g_rastro, "rastro aberto; heap em %.1f MB\n",
                BytesEmUso() / (1024.0 * 1024.0));
        fflush(g_rastro);
    }

    void MeasureOpenedFile(const char* path)
    {
        const size_t now = BytesEmUso();

        if (g_previousFile[0] != '\0' && now > g_bytesNaAbertura)
        {
            const size_t custo = now - g_bytesNaAbertura;
            if (custo >= kLimiarRelato)
            {
                __android_log_print(ANDROID_LOG_WARN, "MuLegacy",
                    "memoria: %s custou %.1f MB (total %.1f MB)",
                    g_previousFile, custo / (1024.0 * 1024.0),
                    now / (1024.0 * 1024.0));
            }
        }

        // Marca de agua a cada 256 MB, para localizar o momento da explosao mesmo
        // que ela venha diluida em muitos arquivos pequenos.
        if (now > g_picoBytes + 256u * 1024u * 1024u)
        {
            g_picoBytes = now;
            __android_log_print(ANDROID_LOG_WARN, "MuLegacy",
                "memoria: passou de %.0f MB abrindo %s",
                now / (1024.0 * 1024.0), path);
        }

        // Na faixa suspeita, nomear na entrada: se este for o arquivo que mata o
        // processo, esta linha e a ultima do rastro e a resposta.
        //
        // Vai para ARQUIVO, nao para o logcat. Nomear toda abertura sao milhares de
        // linhas em poucos segundos, e nessa vazao o logd descarta -- a tentativa
        // por logcat voltou com ZERO linhas do app, inclusive as iniciais, e o
        // processo passou a morrer em 6 s em vez de 40 s. O arquivo, com flush por
        // linha, sobrevive ao SIGKILL, que e a unica forma de morte aqui.
        if (now >= kLimiarNomear && g_rastro != NULL)
        {
            fprintf(g_rastro, "%8.1f MB  %s\n", now / (1024.0 * 1024.0), path);
            fflush(g_rastro);   // sem isto o buffer morre com o processo
        }

        strncpy(g_previousFile, path, sizeof(g_previousFile) - 1);
        g_previousFile[sizeof(g_previousFile) - 1] = '\0';
        g_bytesNaAbertura = now;
    }

    // Nomeia o arquivo em uso quando uma excecao nao tratada derruba o processo.
    //
    // `std::set_terminate` e o gancho certo aqui: roda ANTES do abort e antes dos
    // destrutores estaticos. Foi o que resolveu depois de duas tentativas que nao
    // servem -- `atexit` (roda em ordem inversa, os destrutores vem primeiro) e ler
    // o logcat (o SIGSEGV/abort aparece longe da causa).
    void ReportTermination()
    {
        __android_log_print(ANDROID_LOG_ERROR, "MuLegacy",
            "TERMINATE: excecao nao tratada. Ultimo arquivo aberto: %s",
            Platform::LastOpenedPath());
        _exit(1);   // evita os destrutores estaticos, que tocam um GL ja morto
    }

    // Etapa da subida a executar no proximo quadro.
    //
    // A subida era UMA chamada dentro de um quadro so. Como ela faz minutos de I/O
    // sincrono, a thread de render ficava esse tempo todo sem voltar ao laco -- e
    // enquanto nao volta, ela nao ve o pedido de liberacao da superficie. Se a
    // Activity fosse recriada nesse meio (o que acontece: o log mostrou
    // "finishDrawing of relaunch: 89415ms"), surfaceDestroyed esperava na thread de
    // UI e o Android acusava ANR no app.
    //
    // Uma etapa por quadro nao elimina o problema -- CarregarDadosBasicos sozinha
    // leva a maior parte do tempo --, mas reduz a janela em que a thread esta
    // inalcancavel, e as etapas ja vinham separadas em LegacySceneBringup. O que
    // fecha o buraco de verdade e a espera limitada em superficiePerdida, do lado
    // Java: sem ela, qualquer etapa longa volta a travar a UI.
    enum BringupStage
    {
        STAGE_TITLE = 0,
        STAGE_BASIC_DATA,
        STAGE_INTERFACE,
        STAGE_LOGIN,
        STAGE_DONE
    };
    BringupStage g_bringupStage = STAGE_TITLE;

    void AdvanceSceneBringup(int width, int height)
    {
        switch (g_bringupStage)
        {
        case STAGE_TITLE:
            Platform::SetLegacySceneLogger(&LogScene);
            Platform::SetLegacyFileMissHook(&ReportMissingFile);
            OpenMemoryTrace();
            Platform::SetLegacyFileOpenHook(&MeasureOpenedFile);
            std::set_terminate(&ReportTermination);

            if (!Platform::CreateTitleScene(width, height))
            {
                __android_log_print(ANDROID_LOG_WARN, "MuLegacy",
                    "cena do cliente indisponivel (assets?); seguindo com o terreno de teste");
                g_sceneRequested = true;   // nao insiste a cada quadro
                g_bringupStage = STAGE_DONE;
                return;
            }
            g_bringupStage = STAGE_BASIC_DATA;
            return;

        case STAGE_BASIC_DATA:
            Platform::LoadBasicData();
            g_bringupStage = STAGE_INTERFACE;
            return;

        case STAGE_INTERFACE:
            Platform::LoadMainInterface();
            g_bringupStage = STAGE_LOGIN;
            return;

        case STAGE_LOGIN:
            Platform::EnterLoginScene();
            g_sceneRequested = true;
            g_sceneActive = true;
            g_bringupStage = STAGE_DONE;
            return;

        case STAGE_DONE:
            return;
        }
    }

    void RenderLegacyFrame(int width, int height)
    {
        // Eventos de socket. No Windows o Winsock os entrega ao window
        // proc; aqui um select() por quadro faz o mesmo trabalho.
        Platform::PumpLegacySocket();

        if (g_display == EGL_NO_DISPLAY || g_eglSurface == EGL_NO_SURFACE) return;

        if (!g_sceneRequested && width > 0 && height > 0)
        {
            // Uma etapa por quadro: ver o comentario em AvancarSubidaDeCena.
            AdvanceSceneBringup(width, height);
            if (!g_sceneActive) return;   // ainda subindo; desenha no proximo quadro
        }

        if (g_sceneActive)
        {
            // Mesma orquestracao do Web: Platform/LegacySceneBringup.cpp.
            Platform::DrawLegacyFrame(width, height);
            // O swap tem de acontecer AQUI tambem.
            //
            // Este ramo fazia `return` e caia fora da funcao antes do
            // eglSwapBuffers do final, que atendia so o terreno de teste. A cena era
            // desenhada corretamente e nunca apresentada: o log dizia
            // "Login Scene init success." e a tela ficava preta -- o que parecia
            // defeito de render, e era so o quadro nao sendo publicado.
            eglSwapBuffers(g_display, g_eglSurface);
            return;
        }

        glViewport(0, 0, width, height);
        // Preserve the legacy scene's dark-blue clear used during scene initialization.
        glClearColor(3.0f / 256.0f, 25.0f / 256.0f, 44.0f / 256.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Camera do fluxo legado (BeginOpengl): FOV/near/far e angulos de ZzzScene,
        // com o toque girando a cena como a rotacao de camera do gameplay.
        Platform::LegacySceneCamera camera = Platform::GetLegacySceneCameraDefaults();
        camera.viewFar = kSceneViewFar;
        camera.angles[2] += g_cameraX * 90.0f;
        camera.angles[0] += g_cameraY * 30.0f;
        const float target[3] = { 0.0f, 0.0f, 0.0f };
        Platform::BuildLegacyCameraPosition(camera.angles, camera.topViewEnable, target, kSceneCameraDistance, camera.position);

        float projection[16];
        float modelView[16];
        Platform::BuildLegacySceneProjection(camera, static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1), projection);
        Platform::BuildLegacyCameraView(camera, modelView);
        Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
        renderer.SetMatrices(projection, modelView);

        if (g_terrainLoaded)
        {
            renderer.SetDepthTest(true);
            renderer.SetTexture2D(false);
            RenderRealTerrain(renderer);
        }
        else
        {
            // Sem asset: triangulo de smoke test, para separar "o contexto GL nao
            // subiu" de "o terreno nao carregou".
            renderer.SetDepthTest(false);
            renderer.Begin(Platform::LegacyPrimitiveTriangles);
            renderer.Color4f(0.15f, 0.65f, 0.95f, 1.0f);
            renderer.Vertex3f(-150.0f, -130.0f, 0.0f);
            renderer.Color4f(0.95f, 0.75f, 0.15f, 1.0f);
            renderer.Vertex3f(150.0f, -130.0f, 0.0f);
            renderer.Color4f(0.35f, 0.95f, 0.35f, 1.0f);
            renderer.Vertex3f(0.0f, 150.0f, 0.0f);
            renderer.End();
        }
        eglSwapBuffers(g_display, g_eglSurface);
    }

    void ReplaceSurface(JNIEnv* environment, jobject surface)
    {
        DestroyContext();
        if (g_surface != 0)
        {
            ANativeWindow_release(g_surface);
            g_surface = 0;
        }
        if (surface != 0)
            g_surface = ANativeWindow_fromSurface(environment, surface);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_roxgaming_mulegacy_MainActivity_nativeSetDataRoot(JNIEnv* environment, jobject, jstring path)
{
    if (path == 0) return;
    const char* root = environment->GetStringUTFChars(path, 0);
    // O chdir ja aconteceu em MuLegacySetWorkingDirectory, antes dos construtores
    // globais. Aqui so se confirma, para o caso de MU_DATA_ROOT nao ter chegado.
    if (root != 0 && chdir(root) != 0)
        __android_log_print(ANDROID_LOG_ERROR, "MuLegacy", "chdir falhou para %s", root);

    // O audio precisa da raiz de dados no lugar: LoadWaveFile abre .wav por
    // caminho relativo.
    Platform::InitializeAndroidAudio();

    // Exercita a cadeia completa do audio pelas MESMAS funcoes que o cliente
    // chama: LoadWaveFile -> Platform::LoadWavClip -> IAudioBackend -> OpenSL ES.
    // Sem isto o backend compilaria e linkaria sem nunca provar que toca.
    // mSpider1.wav e PCM 8 bits; beep.wav nao serve, e o unico MS ADPCM do
    // cliente e o leitor so trata PCM (99% dos 464 .wav sao PCM).
    LoadWaveFile(0, (TCHAR*)"Data\\Sound\\mSpider1.wav", 1, false);
    PlayBuffer(0, 0, 0);

    // Chamada real ao carregador do jogo (ZzzLodTerrain.cpp). O sufixo sem
    // extensao e a convencao do proprio OpenTerrainHeight, que anexa "OZB".
    if (OpenTerrainHeight((char*)"World10/TerrainHeight."))
    {
        g_terrainMin = BackTerrainHeight[0];
        g_terrainMax = BackTerrainHeight[0];
        for (int i = 1; i < kTerrainSize * kTerrainSize; ++i)
        {
            const float h = BackTerrainHeight[i];
            if (h < g_terrainMin) g_terrainMin = h;
            if (h > g_terrainMax) g_terrainMax = h;
        }
        g_terrainLoaded = true;
        __android_log_print(ANDROID_LOG_INFO, "MuLegacy",
                            "terreno World10 carregado: alturas de %.1f a %.1f",
                            g_terrainMin, g_terrainMax);
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR, "MuLegacy", "falha ao carregar o terreno do World10");
    }

    if (root != 0) environment->ReleaseStringUTFChars(path, root);
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*)
{
    g_javaVm = vm;
    return JNI_VERSION_1_6;
}

// ---- Ciclo de vida da superficie, na THREAD DE RENDER --------------------------
//
// Antes o contexto EGL era criado em `surfaceCreated` (thread de UI) e o desenho
// vinha do Choreographer, tambem na thread de UI. Com a subida de cena real isso
// deixou de funcionar: a carga leva minutos, a UI trava e o sistema mata o app
// (`SIGNALED status=9`, sem tombstone).
//
// Contexto GL tem AFINIDADE DE THREAD: `eglMakeCurrent` vale para a thread que o
// chamou. Entao criacao, carga e desenho tem de viver todos na mesma thread. Estas
// duas funcoes sao chamadas pela thread de render (ver MainActivity.RenderThread),
// nunca pela de UI.
extern "C" JNIEXPORT void JNICALL
Java_com_roxgaming_mulegacy_MainActivity_nativeSurfaceReady(JNIEnv* environment, jobject activity, jobject surface, jint width, jint height)
{
    RememberActivity(environment, activity);

    // A superficie pode ser trocada (rotacao, redimensionamento): descartar o
    // contexto antigo antes de criar o novo, senao o EGL fica com dois.
    if (g_display != EGL_NO_DISPLAY) DestroyContext();

    ReplaceSurface(environment, surface);
    g_width = width; g_height = height;

    if (g_surface == 0 || !CreateContext())
    {
        __android_log_print(ANDROID_LOG_ERROR, "MuLegacy",
                            "nao foi possivel criar o contexto GLES3/EGL");
        return;
    }
    Platform::SetLegacyRenderLogger(&ForwardRenderLog);
    Platform::EnableGlslLegacyBackend(true);
    __android_log_print(ANDROID_LOG_INFO, "MuLegacy",
                        "superficie pronta na thread de render: %dx%d", width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_roxgaming_mulegacy_MainActivity_nativeSurfaceGone(JNIEnv*, jobject)
{
    DestroyContext();
    if (g_surface != 0)
    {
        ANativeWindow_release(g_surface);
        g_surface = 0;
    }
    __android_log_print(ANDROID_LOG_INFO, "MuLegacy", "superficie liberada");
}

extern "C" JNIEXPORT void JNICALL
Java_com_roxgaming_mulegacy_MainActivity_nativeRenderFrame(JNIEnv*, jobject)
{
    RenderLegacyFrame(g_width, g_height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_roxgaming_mulegacy_MainActivity_nativeTouchEvent(JNIEnv*, jobject, jint action, jfloat x, jfloat y)
{
    Platform::SetAndroidPointerState(static_cast<long>(x), static_cast<long>(y), action != 1 && action != 3);
    if (g_width > 0 && g_height > 0)
    {
        g_cameraX = (x / static_cast<float>(g_width) - 0.5f) * 2.0f;
        g_cameraY = (0.5f - y / static_cast<float>(g_height)) * 2.0f;
    }
}

