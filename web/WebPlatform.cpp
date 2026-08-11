#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <cstdio>
#include <cstring>
#include "LegacyRenderAdapter.h"
#include "LegacySceneMatrices.h"
#include "PlatformInput.h"
#include "LegacyFileAccess.h"
#include "PlatformShell.h"
#include "LegacySocketPump.h"
#include "LegacySceneBringup.h"

namespace Platform { void InitializeWebAudio(); }

// Tela de titulo montada com o codigo do jogo; implementada em WebTitleScene.cpp,
// que inclui os cabecalhos do cliente.
namespace Platform
{
    bool CreateLegacyTitleScene(int screenWidth, int screenHeight);
    bool IsLegacyTitleSceneReady();
    void RenderLegacyTitleScene();

    // WebLazyAssets.cpp: busca de assets sob demanda no lugar de --preload-file.
    void InstalarBuscaSobDemanda();
    void RelatarBuscaSobDemanda(void (*log)(const char*));

    // Declaradas em Platform/LegacySceneBringup.h
    bool EntrarNaCenaDeLogin();

    // Platform/LegacyGlobalAllocations.cpp
    void AlocarGlobaisDoCliente();
}

// Audio do cliente, implementado por Platform/LegacyAudioBridge.cpp.
class OBJECT;
void LoadWaveFile(int Buffer, char* strFileName, int BufferChannel, bool Enable3DSound);
long PlayBuffer(int Buffer, OBJECT* Object, int bLooped);

namespace Platform { void InitializeWebInput(); void GetWebPointerState(long& x, long& y); }

// Codigo de jogo real, vindo de source/Math/ZzzMathLib.cpp. Serve para provar que
// os modulos migrados nao apenas compilam, mas linkam e EXECUTAM no navegador —
// sem isso o linker descarta a biblioteca inteira por falta de referencia.
typedef float vec3_t[3];
extern "C" {
    void AngleMatrix(const vec3_t angles, float matrix[3][4]);
    void VectorRotate(const vec3_t in1, const float in2[3][4], vec3_t out);
}

// Terreno real: carregado por ZzzLodTerrain.cpp a partir do World10 empacotado.
extern bool OpenTerrainHeight(char* filename);
extern float BackTerrainHeight[256 * 256];

extern "C" {
#include "jpeglib.h"
}

namespace
{
    const int kTerrainSize = 256;
    bool g_terrainLoaded = false;
    float g_terrainMin = 0.f;
    float g_terrainMax = 1.f;
    GLuint g_terrainTexture = 0;

    // Carrega uma textura .OZJ: JPEG precedido de 24 bytes de cabecalho do MU.
    // Reproduz o que ZzzTexture::OpenJpeg faz na leitura, e sobe para o GL.
    GLuint LoadOzjTexture(const char* legacyPath)
    {
        FILE* file = Platform::LegacyFileOpen(legacyPath, "rb");
        if (file == NULL) return 0;
        fseek(file, 24, SEEK_SET);

        jpeg_decompress_struct cinfo;
        jpeg_error_mgr jerr;
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, file);
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);

        const int width = cinfo.output_width;
        const int height = cinfo.output_height;
        const int comps = cinfo.output_components;
        const int stride = width * comps;

        unsigned char* pixels = new unsigned char[stride * height];
        while (cinfo.output_scanline < cinfo.output_height)
        {
            // O legado inverte a ordem das linhas ao montar o buffer; aqui o
            // mesmo efeito e obtido escrevendo de baixo para cima.
            unsigned char* dst = pixels + (height - 1 - cinfo.output_scanline) * stride;
            unsigned char* rows[1] = { dst };
            jpeg_read_scanlines(&cinfo, rows, 1);
        }
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(file);

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, (comps == 3) ? GL_RGB : GL_LUMINANCE,
                     width, height, 0, (comps == 3) ? GL_RGB : GL_LUMINANCE,
                     GL_UNSIGNED_BYTE, pixels);
        delete[] pixels;
        return texture;
    }
}

namespace
{
    // O pedaco de terreno desenhado cobre ~9600 unidades de mundo (96 celulas de
    // TERRAIN_SCALE=100). A camera de gameplay fica a 700 com far plane em 2800,
    // o que deixaria o observador dentro da malha — aqui ela e afastada o
    // suficiente para enquadrar o relevo inteiro.
    const float kSceneCameraDistance = 9000.0f;
    const float kSceneViewFar = 30000.0f;

    // Diagnostico do backend de render.
    //
    // Tudo vai para o console. Mensagens de FALHA aparecem tambem numa faixa
    // sobre a pagina, porque erro de shader no navegador some no console se
    // ninguem abrir o DevTools — e o sintoma vira "tela azul e nada mais".
    // Mensagens informativas nao cobrem a cena.
    void ForwardRenderLog(const char* message)
    {
        if (message == NULL) return;
        emscripten_log(EM_LOG_ERROR, "%s", message);

        const bool isFailure =
            strstr(message, "falha") != NULL ||
            strstr(message, "nao resolvida") != NULL ||
            strstr(message, "NAO IMPLEMENTADO") != NULL;
        if (!isFailure) return;

        EM_ASM({
            var box = document.getElementById('mu_diag');
            if (!box) {
                box = document.createElement('pre');
                box.id = 'mu_diag';
                box.style.cssText = 'position:fixed;left:0;top:0;right:0;max-height:50%;' +
                    'overflow:auto;margin:0;padding:8px;z-index:9999;' +
                    'background:rgba(120,0,0,.9);color:#fff;font:12px monospace;' +
                    'white-space:pre-wrap';
                document.body.appendChild(box);
            }
            box.textContent += UTF8ToString($0) + "\n";
        }, message);
    }

    void ReportGlState(const char* label)
    {
        char buffer[256];
        const char* vendor = (const char*)glGetString(GL_VENDOR);
        const char* version = (const char*)glGetString(GL_VERSION);
        snprintf(buffer, sizeof(buffer), "%s | vendor=%s | version=%s | erro=0x%04X",
                 label, vendor ? vendor : "?", version ? version : "?", glGetError());
        ForwardRenderLog(buffer);
    }

    // O contexto WebGL pode ser perdido a qualquer momento; o adapter precisa
    // descartar shader/VAO/VBO para reconstrui-los na restauracao.
    EM_BOOL OnContextLost(int, const void*, void*)
    {
        Platform::InvalidateLegacyRenderResources();
        return EM_TRUE;
    }

    // Desenha o terreno REAL do World10, com as alturas carregadas por
    // ZzzLodTerrain::OpenTerrainHeight. Sem textura ainda: a cor vem da altura,
    // para tornar o relevo visivel e provar que o dado do asset chegou ate o
    // WebGL2 passando pelo ILegacyRenderAdapter.
    void RenderRealTerrain(Platform::ILegacyRenderAdapter& renderer)
    {
        if (!g_terrainLoaded) return;

        // TERRAIN_SCALE do legado; um passo maior mantem a malha leve no browser.
        const float scale = 100.f;
        const int step = 2;
        const float span = (g_terrainMax > g_terrainMin) ? (g_terrainMax - g_terrainMin) : 1.f;

        // Centraliza o pedaco visivel na origem, onde a camera legada aponta.
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

                struct Corner { float x, y, h, u, v; };
                const Corner quad[4] = {
                    { px,  py,  h00, 0.f, 0.f }, { px2, py,  h10, 1.f, 0.f },
                    { px2, py2, h11, 1.f, 1.f }, { px,  py2, h01, 0.f, 1.f },
                };
                const int order[6] = { 0, 1, 2, 0, 2, 3 };

                for (int i = 0; i < 6; ++i)
                {
                    const Corner& c = quad[order[i]];
                    // Com textura, a cor vira iluminacao por altura em vez de
                    // substituir o material — o shader multiplica as duas.
                    const float t = (c.h - g_terrainMin) / span;
                    const float shade = 0.65f + 0.35f * t;
                    renderer.Color4f(shade, shade, shade, 1.f);
                    renderer.TexCoord2f(c.u, c.v);
                    renderer.Vertex3f(c.x, c.y, c.h);
                }
            }
        }
        renderer.End();
    }

    void RenderLegacyFrame()
    {
        // Mantem a mesma semantica da versao PC: uma amostra corresponde a
        // uma execucao completa do callback de animation frame.
        Platform::ResetLegacyRenderFrameStats();
        // Eventos de socket. No Windows o Winsock os entrega ao window
        // proc; aqui um select() por quadro faz o mesmo trabalho.
        Platform::PumpLegacySocket();

        double width = 0.0;
        double height = 0.0;
        emscripten_get_element_css_size("#canvas", &width, &height);
        const int pixelWidth = static_cast<int>(width > 1.0 ? width : 1.0);
        const int pixelHeight = static_cast<int>(height > 1.0 ? height : 1.0);
        int canvasWidth = 0;
        int canvasHeight = 0;
        emscripten_get_canvas_element_size("#canvas", &canvasWidth, &canvasHeight);
        if (canvasWidth != pixelWidth || canvasHeight != pixelHeight)
            emscripten_set_canvas_element_size("#canvas", pixelWidth, pixelHeight);
        glViewport(0, 0, pixelWidth, pixelHeight);
        // Preserve the legacy scene's dark-blue clear used during scene initialization.
        glClearColor(3.0f / 256.0f, 25.0f / 256.0f, 44.0f / 256.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Com a tela de titulo montada, quem desenha e o codigo do jogo
        // (CUIMng::RenderTitleSceneUI). O terreno abaixo continua servindo de
        // caminho alternativo enquanto a cena real nao cobre tudo.
        if (Platform::IsLegacyTitleSceneReady())
        {
            Platform::RenderLegacyTitleScene();
            return;
        }

        long pointerX = 0;
        long pointerY = 0;
        Platform::GetWebPointerState(pointerX, pointerY);

        // Camera do fluxo legado (BeginOpengl): FOV/near/far e angulos de ZzzScene,
        // com o ponteiro girando a cena como a rotacao de camera do gameplay.
        Platform::LegacySceneCamera camera = Platform::GetLegacySceneCameraDefaults();
        camera.viewFar = kSceneViewFar;
        camera.angles[2] += (static_cast<float>(pointerX) / static_cast<float>(pixelWidth) - 0.5f) * 180.0f;
        camera.angles[0] += (static_cast<float>(pointerY) / static_cast<float>(pixelHeight) - 0.5f) * 60.0f;
        const float target[3] = { 0.0f, 0.0f, 0.0f };
        Platform::BuildLegacyCameraPosition(camera.angles, camera.topViewEnable, target, kSceneCameraDistance, camera.position);

        Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
        float projection[16];
        float modelView[16];
        Platform::BuildLegacySceneProjection(camera, static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight), projection);
        Platform::BuildLegacyCameraView(camera, modelView);
        renderer.SetMatrices(projection, modelView);
        renderer.SetDepthTest(true);
        renderer.SetTexture2D(g_terrainTexture != 0);
        renderer.BindTexture(g_terrainTexture);
        RenderRealTerrain(renderer);
    }
}


int main()
{
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes);
    attributes.majorVersion = 2;
    attributes.minorVersion = 0;
    // Os defaults do Emscripten nao servem para um cliente de jogo, e nenhum deles
    // havia sido escolhido: o contexto vinha com alpha, MSAA 4x e sem stencil.
    //
    // antialias: o cliente do PC nao tem MSAA, entao ligado aqui ele so paga o
    // resolve do buffer multiamostrado em cada apresentacao.
    attributes.antialias = EM_FALSE;
    // alpha: um canvas translucido obriga o compositor a mesclar a pagina inteira
    // atras dele a cada quadro. O jogo cobre 100% da area util.
    attributes.alpha = EM_FALSE;
    attributes.premultipliedAlpha = EM_FALSE;
    // stencil: o jogo CHAMA glEnable(GL_STENCIL_TEST) (~24 vezes por quadro, medido).
    // Sem plano de stencil no framebuffer padrao esses efeitos eram descartados em
    // silencio -- isto e correcao, nao desempenho.
    attributes.stencil = EM_TRUE;
    attributes.depth = EM_TRUE;
    attributes.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context("#canvas", &attributes);
    if (context <= 0 || emscripten_webgl_make_context_current(context) != EMSCRIPTEN_RESULT_SUCCESS)
        return 1;
    Platform::SetLegacyRenderLogger(&ForwardRenderLog);
    ReportGlState("contexto WebGL2 criado");
    Platform::EnableGlslLegacyBackend(true);

    // Antes de qualquer carregamento: a partir daqui todo fopen que falhar tenta
    // buscar o arquivo no servidor. Data/Interface deixou de ser empacotada.
    Platform::InstalarBuscaSobDemanda();

    // Vetores globais do cliente. Precisa vir antes de qualquer carregamento:
    // OpenGateScript e companhia escrevem direto nesses ponteiros.
    Platform::AlocarGlobaisDoCliente();

    // Carrega o terreno real pelo carregador do proprio jogo.
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
        // Caminho no formato legado; as barras precisam ser escapadas em C++.
        g_terrainTexture = LoadOzjTexture("Data\\World10\\TileGrass01.OZJ");
        if (g_terrainTexture == 0)
            ForwardRenderLog("falha ao carregar a textura TileGrass01.OZJ");
        char report[160];
        snprintf(report, sizeof(report),
                 "terreno World10 carregado: alturas de %.1f a %.1f",
                 g_terrainMin, g_terrainMax);
        ForwardRenderLog(report);
    }
    else
    {
        ForwardRenderLog("falha ao carregar o terreno do World10");
    }
    // Monta a tela de titulo do MU com o codigo do proprio jogo: as texturas de
    // BITMAP_TITLE pelo LoadBitmap do cliente e a UI por CUIMng::CreateTitleSceneUI.
    {
        int larguraCanvas = 0;
        int alturaCanvas = 0;
        emscripten_get_canvas_element_size("#canvas", &larguraCanvas, &alturaCanvas);
        if (larguraCanvas <= 0) larguraCanvas = 1280;
        if (alturaCanvas  <= 0) alturaCanvas  = 720;
        Platform::CreateLegacyTitleScene(larguraCanvas, alturaCanvas);
        Platform::RelatarBuscaSobDemanda(&ForwardRenderLog);

        // Segunda metade da WebzenScene. Bloqueia a aba enquanto carrega: sao
        // centenas de buscas sincronas. O relatorio depois mostra o custo real,
        // que e o dado que decide se vale pre-buscar em paralelo.
        const double start = emscripten_get_now();
        Platform::LoadBasicData();
        char tempo[120];
        snprintf(tempo, sizeof(tempo), "OpenBasicData levou %.1f s", (emscripten_get_now() - start) / 1000.0);
        ForwardRenderLog(tempo);
        Platform::RelatarBuscaSobDemanda(&ForwardRenderLog);

        // Ultima etapa da WebzenScene: as janelas do jogo.
        Platform::CarregarInterfacePrincipal();
        Platform::RelatarBuscaSobDemanda(&ForwardRenderLog);

        // Cauda da WebzenScene: passa o controle para a cena de login.
        Platform::EntrarNaCenaDeLogin();
    }

    emscripten_set_webglcontextlost_callback("#canvas", NULL, EM_FALSE, &OnContextLost);
    Platform::InitializeWebInput();
    Platform::InitializeWebAudio();
    // Exercita a cadeia de audio pelas MESMAS funcoes que o cliente chama.
    // O navegador mantem o AudioContext suspenso ate o primeiro gesto, entao o
    // som so sai depois de um clique — o carregamento, porem, ja acontece aqui.
    LoadWaveFile(0, (char*)"Data\\Sound\\mSpider1.wav", 1, false);
    PlayBuffer(0, 0, 0);
    emscripten_set_main_loop(RenderLegacyFrame, 0, 1);
    return 0;
}
