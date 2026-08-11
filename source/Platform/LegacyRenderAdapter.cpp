#include "stdafx.h"
#include "LegacyRenderAdapter.h"

namespace Platform
{
    ILegacyRenderAdapter* CreateGlslLegacyRenderAdapter();
}

namespace
{
    #if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    class FixedFunctionLegacyAdapter : public Platform::ILegacyRenderAdapter
    {
    public:
        virtual void Begin(Platform::LegacyPrimitive primitive)
        {
            GLenum glPrimitive = GL_QUADS;
            switch (primitive)
            {
            case Platform::LegacyPrimitiveLineStrip: glPrimitive = GL_LINE_STRIP; break;
            case Platform::LegacyPrimitiveTriangles: glPrimitive = GL_TRIANGLES; break;
            case Platform::LegacyPrimitiveTriangleFan: glPrimitive = GL_TRIANGLE_FAN; break;
            case Platform::LegacyPrimitiveLines: glPrimitive = GL_LINES; break;
            case Platform::LegacyPrimitiveQuads: default: glPrimitive = GL_QUADS; break;
            }
            ::glBegin(glPrimitive);
        }

        virtual void End() { ::glEnd(); }
        virtual void Color4f(float red, float green, float blue, float alpha) { ::glColor4f(red, green, blue, alpha); }
        virtual void TexCoord2f(float u, float v) { ::glTexCoord2f(u, v); }
        virtual void Normal3f(float x, float y, float z) { ::glNormal3f(x, y, z); }
        virtual void Vertex3f(float x, float y, float z) { ::glVertex3f(x, y, z); }
        virtual void Vertex3fv(const float* vertex) { ::glVertex3fv(vertex); }
        virtual void SetDepthTest(bool enabled) { enabled ? ::glEnable(GL_DEPTH_TEST) : ::glDisable(GL_DEPTH_TEST); }
        virtual void SetAlphaTest(bool enabled) { enabled ? ::glEnable(GL_ALPHA_TEST) : ::glDisable(GL_ALPHA_TEST); }
        virtual void SetTexture2D(bool enabled) { enabled ? ::glEnable(GL_TEXTURE_2D) : ::glDisable(GL_TEXTURE_2D); }
        virtual void BindTexture(unsigned int texture) { ::glBindTexture(GL_TEXTURE_2D, texture); }
    };
    #endif

    #if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    FixedFunctionLegacyAdapter g_FixedFunctionAdapter;
    Platform::ILegacyRenderAdapter* g_LegacyRenderAdapter = &g_FixedFunctionAdapter;
    #else
    Platform::ILegacyRenderAdapter* g_LegacyRenderAdapter = NULL;
    #endif
    bool g_GlslLegacyBackendEnabled = false;
    Platform::GpuSkinningMode g_GpuSkinningMode = Platform::GpuSkinningOn;
    Platform::GpuSkinningDeployment g_GpuSkinningDeployment = Platform::GpuSkinningQA;
    unsigned long g_GpuSkinningFrame = 0;
    char g_GpuSkinningModelWhitelist[512] = { 0 };

    // Default por fase. Uma otimizacao entra aqui como Disabled e so vira
    // Enabled depois de passar pelo modo Compare nas cenas de referencia.
    Platform::RenderFeatureMode g_RenderFeatureModes[Platform::RenderFeatureCount] =
    {
        // Ligada por decisao de projeto, nao por ganho medido. Em
        // RenderPerformance_v16 o coletor rendeu 2 a 3 draws instanciados por
        // frame cobrindo 4 a 6 objetos, com instance_batch_max = 2: inerte numa
        // cena de ~1800 draws. Em mundo 94 (sem personagens, ~55 draws) a
        // captura com a flag ligada ficou 10% mais lenta que a desligada.
        // Quem precisar do caminho antigo usa -instancing=off.
        //
        // FOI A CAUSA do item invisivel no inventario (2026-08-11), e o defeito nao
        // estava no agrupamento: estava na barreira que faltava. O coletor reaplica
        // textura/blend/profundidade no flush, mas nao a matriz, e o passe de UI
        // terminava sem descarga -- item de inventario adiado para o passe de mundo
        // do frame seguinte. Consertado em GlslLegacyRenderAdapter.cpp, em
        // SetMatrices: trocar de matriz agora descarrega o coletor, como ja
        // descarregava o lote de vertices. A flag segue ligada.
        Platform::RenderFeatureEnabled,   // RenderFeatureInstancing
        // Ligada por padrao em 2026-08-10, com a evidencia e a lacuna registradas:
        //
        // GANHO MEDIDO sob multidao, que e o regime em que ela importa. Lorencia,
        // -crowd=50 -wheeltrail=0, chars_visible casado em 135/136 e 59/59:
        //   100 monstros: frame 29.065 -> 24.585 us, fps_period 34,4 -> 40,7  (-15%)
        //   50 players:   frame 28.508 -> 26.965 us, fps_period 35,1 -> 37,1  ( -5%)
        // `transforms_skipped` saiu de 0 para 394, e cpu_skinning_vertices caiu 74%.
        // O ganho e menor no player porque o custo dele nunca foi o laco por vertice.
        //
        // CORRETUDE AUDITADA. Ela depende de uma propriedade: ninguem le
        // VertexTransform/NormalTransform/IntensityTransform -- arrays GLOBAIS,
        // ZzzBMD.h:424 -- sem EnsureVerticesTransformed() ter rodado. Foram auditadas 57
        // leituras em 4 arquivos (PhysicsManager x3, GMNewTown x2, GM_Raklion,
        // ZzzBMD x8) e todas materializam antes de ler. Virou teste com sensibilidade
        // comprovada por mutacao: diagnostic/check_transform_cache_readers.py.
        //
        // O QUE FALTA, e nao e pouco: a etapa `compare` da escada do projeto NAO foi
        // rodada. Diferente de -cpumatrices, que subiu com 50 amostras de divergencia
        // medida, esta sobe com ganho medido e corretude argumentada. O risco residual e
        // ordem entre modelos no mesmo frame quando o snapshot e de frame anterior -- e
        // esse risco EXISTE tambem no caminho ansioso, onde os globais guardam o
        // resultado do ultimo Transform, entao o cache nao o introduz.
        //
        // -statictransformcache=off reverte; =compare alterna por frame e divergencia
        // aparece como cintilacao.
        // TESTADO E DESCARTADO como causa (2026-08-11): com ela DESLIGADA, arma e set
        // pegos do chao continuaram invisiveis no inventario ate outro item ser pego.
        // O sintoma tambem e retroativo e permanente, o que nao combina com um cache
        // de pose por quadro. Segue ligada pelo ganho ja medido.
        Platform::RenderFeatureEnabled,   // RenderFeatureStaticTransformCache
        // Ligada por padrao: em mundo 2 cena 5 baixou os draws de 4.366 para
        // ~1.835 por frame, zerou flush_matrix (era 449) e evitou ~20.000
        // chamadas de uniforme por frame. -batching=off reverte.
        Platform::RenderFeatureEnabled,   // RenderFeatureBatching
        Platform::RenderFeatureEnabled,   // RenderFeatureMeshCache
        // Ligada por padrao apos a etapa compare: 50 amostras em 5 mundos e 2
        // cenas, todas com divergencia de 1,0e-6 contra o driver, e queda de
        // 41% no tempo de CPU medido. -cpumatrices=off reverte.
        Platform::RenderFeatureEnabled    // RenderFeatureCpuMatrices
    };

    char g_InstancingModelWhitelist[512] = { 0 };

    bool IsModelInList(const char* list, int modelId)
    {
        if (modelId < 0 || list == NULL || list[0] == 0)
            return false;
        const char* entry = list;
        while (*entry != 0)
        {
            while (*entry == ',' || *entry == ' ' || *entry == '\t') ++entry;
            char* end = NULL;
            const long candidate = strtol(entry, &end, 10);
            if (end == entry)
            {
                while (*entry != 0 && *entry != ',') ++entry;
                continue;
            }
            if (candidate == modelId) return true;
            entry = end;
            while (*entry != 0 && *entry != ',') ++entry;
        }
        return false;
    }

    bool IsWhitelistedGpuSkinningModel(int modelId)
    {
        return IsModelInList(g_GpuSkinningModelWhitelist, modelId);
    }
}

namespace
{
    Platform::LegacyRenderLogFn g_LegacyRenderLogger = NULL;
}

void Platform::SetLegacyRenderLogger(LegacyRenderLogFn logger) { g_LegacyRenderLogger = logger; }
void Platform::LegacyRenderLog(const char* message)
{
    if (g_LegacyRenderLogger != NULL && message != NULL)
        g_LegacyRenderLogger(message);
}

void Platform::InvalidateLegacyRenderResources()
{
    if (g_LegacyRenderAdapter != NULL)
        g_LegacyRenderAdapter->InvalidateGraphicsResources();
}

void Platform::FlushLegacyRenderBatch()
{
    if (g_LegacyRenderAdapter != NULL)
        g_LegacyRenderAdapter->FlushBatch();
}

void Platform::ResetLegacyRenderFrameStats()
{
    if (g_LegacyRenderAdapter != NULL)
        g_LegacyRenderAdapter->ResetFrameStats();
}

Platform::LegacyRenderFrameStats Platform::GetLegacyRenderFrameStats()
{
    return (g_LegacyRenderAdapter != NULL)
        ? g_LegacyRenderAdapter->GetFrameStats()
        : LegacyRenderFrameStats();
}

void Platform::RecordLegacyTextureUpload(unsigned long long bytes)
{
    if (g_LegacyRenderAdapter != NULL)
        g_LegacyRenderAdapter->RecordTextureUpload(bytes);
}

void Platform::RecordCpuSkinningWork(unsigned long long vertices, unsigned long long normals)
{
    if (g_LegacyRenderAdapter != NULL)
        g_LegacyRenderAdapter->RecordCpuSkinningWork(vertices, normals);
}

void Platform::RecordGpuSkinningFallback(Platform::GpuSkinningFallbackReason reason)
{
    if (g_LegacyRenderAdapter != NULL)
        g_LegacyRenderAdapter->RecordGpuSkinningFallback(reason);
}

void Platform::RecordCpuTransformWork(unsigned long long transformsExecuted, unsigned long long transformsSkipped,
    unsigned long long animationsExecuted, unsigned long long animationsSkipped)
{
    if (g_LegacyRenderAdapter != NULL)
        g_LegacyRenderAdapter->RecordCpuTransformWork(transformsExecuted, transformsSkipped, animationsExecuted, animationsSkipped);
}

void Platform::InvalidateLegacyRenderStateCache()
{
    if (g_LegacyRenderAdapter != NULL)
        g_LegacyRenderAdapter->InvalidateStateCache();
}

Platform::ILegacyRenderAdapter& Platform::GetLegacyRenderAdapter() { return *g_LegacyRenderAdapter; }
void Platform::SetLegacyRenderAdapter(ILegacyRenderAdapter* adapter)
{
    g_LegacyRenderAdapter = adapter;
    g_GlslLegacyBackendEnabled = false;
}

void Platform::EnableGlslLegacyBackend(bool enabled)
{
    static ILegacyRenderAdapter* glslAdapter = NULL;
    if (enabled)
    {
        if (glslAdapter == NULL)
            glslAdapter = Platform::CreateGlslLegacyRenderAdapter();
        if (glslAdapter != NULL)
        {
            g_LegacyRenderAdapter = glslAdapter;
            g_GlslLegacyBackendEnabled = true;
        }
    }
    else
    {
        g_GlslLegacyBackendEnabled = false;
        #if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
        g_LegacyRenderAdapter = &g_FixedFunctionAdapter;
        #else
        g_LegacyRenderAdapter = NULL;
        #endif
    }
}

bool Platform::IsGlslLegacyBackendEnabled()
{
    return g_GlslLegacyBackendEnabled;
}

void Platform::SetGpuSkinningMode(GpuSkinningMode mode) { g_GpuSkinningMode = mode; }
Platform::GpuSkinningMode Platform::GetGpuSkinningMode() { return g_GpuSkinningMode; }
void Platform::SetGpuSkinningDeployment(GpuSkinningDeployment deployment) { g_GpuSkinningDeployment = deployment; }
Platform::GpuSkinningDeployment Platform::GetGpuSkinningDeployment() { return g_GpuSkinningDeployment; }
void Platform::SetGpuSkinningModelWhitelist(const char* modelIds)
{
    if (modelIds == NULL) { g_GpuSkinningModelWhitelist[0] = 0; return; }
    strncpy(g_GpuSkinningModelWhitelist, modelIds, sizeof(g_GpuSkinningModelWhitelist) - 1);
    g_GpuSkinningModelWhitelist[sizeof(g_GpuSkinningModelWhitelist) - 1] = 0;
}
void Platform::BeginGpuSkinningFrame() { ++g_GpuSkinningFrame; }
unsigned long Platform::GetRenderFrameIndex() { return g_GpuSkinningFrame; }
bool Platform::ShouldUseGpuSkinning()
{
    if (g_GpuSkinningMode == GpuSkinningOff) return false;
    return g_GpuSkinningMode == GpuSkinningOn || (g_GpuSkinningFrame & 1UL) == 0;
}
bool Platform::ShouldUseGpuSkinningForModel(int modelId)
{
    if (!ShouldUseGpuSkinning()) return false;
    return g_GpuSkinningDeployment != GpuSkinningProductionWhitelist ||
        IsWhitelistedGpuSkinningModel(modelId);
}

void Platform::SetRenderFeatureMode(RenderFeature feature, RenderFeatureMode mode)
{
    if (feature < 0 || feature >= RenderFeatureCount) return;
    g_RenderFeatureModes[feature] = mode;
}

Platform::RenderFeatureMode Platform::GetRenderFeatureMode(RenderFeature feature)
{
    if (feature < 0 || feature >= RenderFeatureCount) return RenderFeatureDisabled;
    return g_RenderFeatureModes[feature];
}

bool Platform::IsRenderFeatureActive(RenderFeature feature)
{
    const RenderFeatureMode mode = GetRenderFeatureMode(feature);
    if (mode == RenderFeatureDisabled) return false;
    if (mode == RenderFeatureEnabled) return true;
    return (g_GpuSkinningFrame & 1UL) == 0;
}

namespace Platform
{
    // Implementadas no adapter GLSL, que e quem tem o carregador de funcoes GL.
    void GlslBeginGpuFrameTimer();
    void GlslEndGpuFrameTimer();
    unsigned long long GlslGetLastGpuFrameTimeUs();
    int GlslGetGpuFrameTimerState();
}

void Platform::BeginGpuFrameTimer()
{
    if (IsGlslLegacyBackendEnabled()) GlslBeginGpuFrameTimer();
}

void Platform::EndGpuFrameTimer()
{
    if (IsGlslLegacyBackendEnabled()) GlslEndGpuFrameTimer();
}

unsigned long long Platform::GetLastGpuFrameTimeUs()
{
    return IsGlslLegacyBackendEnabled() ? GlslGetLastGpuFrameTimeUs() : 0;
}

int Platform::GetGpuFrameTimerState()
{
    return IsGlslLegacyBackendEnabled() ? GlslGetGpuFrameTimerState() : 0;
}

void Platform::SetInstancingModelWhitelist(const char* modelIds)
{
    if (modelIds == NULL) { g_InstancingModelWhitelist[0] = 0; return; }
    strncpy(g_InstancingModelWhitelist, modelIds, sizeof(g_InstancingModelWhitelist) - 1);
    g_InstancingModelWhitelist[sizeof(g_InstancingModelWhitelist) - 1] = 0;
}

bool Platform::ShouldInstanceModel(int modelId)
{
    if (!IsRenderFeatureActive(RenderFeatureInstancing)) return false;
    // Lista vazia libera todos: o interruptor de liberacao e a propria flag.
    return g_InstancingModelWhitelist[0] == 0 || IsModelInList(g_InstancingModelWhitelist, modelId);
}

const char* Platform::GetRenderFeatureModeName(RenderFeature feature)
{
    switch (GetRenderFeatureMode(feature))
    {
    case RenderFeatureEnabled: return "on";
    case RenderFeatureCompare: return "compare";
    default: return "off";
    }
}
