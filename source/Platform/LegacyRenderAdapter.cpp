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

void Platform::InvalidateLegacyRenderStateCache()
{
    if (g_LegacyRenderAdapter != NULL)
        g_LegacyRenderAdapter->InvalidateStateCache();
}

Platform::ILegacyRenderAdapter& Platform::GetLegacyRenderAdapter() { return *g_LegacyRenderAdapter; }
void Platform::SetLegacyRenderAdapter(ILegacyRenderAdapter* adapter)
{
    g_LegacyRenderAdapter = adapter;
}

void Platform::EnableGlslLegacyBackend(bool enabled)
{
    static ILegacyRenderAdapter* glslAdapter = NULL;
    if (enabled)
    {
        if (glslAdapter == NULL)
            glslAdapter = Platform::CreateGlslLegacyRenderAdapter();
        if (glslAdapter != NULL)
            g_LegacyRenderAdapter = glslAdapter;
    }
    else
    {
        #if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
        g_LegacyRenderAdapter = &g_FixedFunctionAdapter;
        #else
        g_LegacyRenderAdapter = NULL;
        #endif
    }
}
