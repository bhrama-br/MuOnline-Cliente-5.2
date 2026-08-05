#pragma once

namespace Platform
{
    enum LegacyPrimitive
    {
        LegacyPrimitiveQuads,
        LegacyPrimitiveLineStrip,
        LegacyPrimitiveTriangles,
        LegacyPrimitiveTriangleFan,
        LegacyPrimitiveLines
    };

    class ILegacyRenderAdapter
    {
    public:
        virtual ~ILegacyRenderAdapter() {}
        virtual void Begin(LegacyPrimitive primitive) = 0;
        virtual void End() = 0;
        virtual void Color4f(float red, float green, float blue, float alpha) = 0;
        virtual void TexCoord2f(float u, float v) = 0;
        virtual void Normal3f(float x, float y, float z) = 0;
        virtual void Vertex3f(float x, float y, float z) = 0;
        virtual void Vertex3fv(const float* vertex) = 0;
        virtual void SetMatrices(const float* projection, const float* modelView) { (void)projection; (void)modelView; }
        virtual void SetDepthTest(bool enabled) { (void)enabled; }
        virtual void SetAlphaTest(bool enabled) { (void)enabled; }
        // Referencia do glAlphaFunc(GL_GREATER, ref); em GLES3 vira discard no shader.
        virtual void SetAlphaTestRef(float reference) { (void)reference; }
        // Fog linear do pipeline fixo. start/end correspondem a GL_FOG_START e
        // GL_FOG_END; o legado nunca os define, entao ficam nos defaults do GL
        // (0 e 1) ate serem calibrados contra o build PC.
        virtual void SetFog(bool enabled, const float* color, float start, float end)
        { (void)enabled; (void)color; (void)start; (void)end; }
        virtual void SetTexture2D(bool enabled) { (void)enabled; }
        virtual void BindTexture(unsigned int texture) { (void)texture; }
        // Descarta shader/VAO/VBO apos a destruicao do contexto grafico. Sem isso
        // o backend GLSL segue usando nomes de objeto de um contexto morto e para
        // de desenhar silenciosamente.
        virtual void InvalidateGraphicsResources() {}
    };

    ILegacyRenderAdapter& GetLegacyRenderAdapter();
    void SetLegacyRenderAdapter(ILegacyRenderAdapter* adapter);
    void EnableGlslLegacyBackend(bool enabled);

    // Seguro de chamar antes de qualquer adapter ter sido instalado.
    void InvalidateLegacyRenderResources();

    // Diagnostico do backend GLSL. Sem um logger registrado, falhas de
    // carregamento de funcoes, compilacao e link de shader ficam silenciosas e o
    // adapter apenas para de desenhar.
    typedef void (*LegacyRenderLogFn)(const char* message);
    void SetLegacyRenderLogger(LegacyRenderLogFn logger);
    void LegacyRenderLog(const char* message);
}
