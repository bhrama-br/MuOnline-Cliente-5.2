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
        Platform::RenderFeatureDisabled,  // RenderFeatureInstancing
        Platform::RenderFeatureDisabled,  // RenderFeatureStaticTransformCache
        Platform::RenderFeatureDisabled   // RenderFeatureBatching
    };

    bool IsWhitelistedGpuSkinningModel(int modelId)
    {
        if (modelId < 0 || g_GpuSkinningModelWhitelist[0] == 0)
            return false;
        const char* entry = g_GpuSkinningModelWhitelist;
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

const char* Platform::GetRenderFeatureModeName(RenderFeature feature)
{
    switch (GetRenderFeatureMode(feature))
    {
    case RenderFeatureEnabled: return "on";
    case RenderFeatureCompare: return "compare";
    default: return "off";
    }
}
