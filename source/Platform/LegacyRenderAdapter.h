#pragma once

namespace Platform
{
    // Contadores do caminho legado moderno desde o ultimo Reset. O ciclo de frame
    // (e a apresentacao do overlay/CSV) pode ser conectado sem expor detalhes GL
    // aos chamadores do renderer.
    struct LegacyRenderFrameStats
    {
        LegacyRenderFrameStats()
            : drawCalls(0), vertices(0), vertexUploadBytes(0), bufferDataCalls(0), bufferSubDataCalls(0), batchFlushes(0),
              textureUploads(0), textureUploadBytes(0), textureChanges(0), matrixFlushes(0), textureFlushes(0), blendFlushes(0),
              depthFlushes(0), alphaFlushes(0), fogFlushes(0), programChanges(0), depthStateChanges(0), alphaTestChanges(0),
              fogChanges(0), blendStateChanges(0) {}

        unsigned long long drawCalls;
        unsigned long long vertices;
        unsigned long long vertexUploadBytes;
        unsigned long long bufferDataCalls;
        unsigned long long bufferSubDataCalls;
        unsigned long long batchFlushes;
        unsigned long long textureUploads;
        unsigned long long textureUploadBytes;
        unsigned long long textureChanges;
        unsigned long long matrixFlushes;
        unsigned long long textureFlushes;
        unsigned long long blendFlushes;
        unsigned long long depthFlushes;
        unsigned long long alphaFlushes;
        unsigned long long fogFlushes;
        unsigned long long programChanges;
        unsigned long long depthStateChanges;
        unsigned long long alphaTestChanges;
        unsigned long long fogChanges;
        unsigned long long blendStateChanges;
    };

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
        virtual void SetBlendMode(int mode) { (void)mode; }
        // Lote explicito para UI: preserva a ordem e so agrega quads consecutivos
        // com o mesmo estado. O caminho legado padrao continua imediato.
        virtual void BeginBatch() {}
        virtual void EndBatch() {}
        // Barreira para leituras de framebuffer e codigo GL externo.
        virtual void FlushBatch() {}
        virtual void ResetFrameStats() {}
        virtual LegacyRenderFrameStats GetFrameStats() const { return LegacyRenderFrameStats(); }
        virtual void RecordTextureUpload(unsigned long long bytes) { (void)bytes; }
        // Deve ser chamado por codigo que altera estado GL diretamente, antes
        // de devolver o controle ao adaptador.
        virtual void InvalidateStateCache() {}
        // Descarta shader/VAO/VBO apos a destruicao do contexto grafico. Sem isso
        // o backend GLSL segue usando nomes de objeto de um contexto morto e para
        // de desenhar silenciosamente.
        virtual void InvalidateGraphicsResources() {}
    };

    ILegacyRenderAdapter& GetLegacyRenderAdapter();
    void SetLegacyRenderAdapter(ILegacyRenderAdapter* adapter);
    void EnableGlslLegacyBackend(bool enabled);
    bool IsGlslLegacyBackendEnabled();

    // Seguro de chamar antes de qualquer adapter ter sido instalado.
    void InvalidateLegacyRenderResources();
    // Materializa os sprites pendentes antes de observar ou alterar o framebuffer.
    void FlushLegacyRenderBatch();
    void ResetLegacyRenderFrameStats();
    LegacyRenderFrameStats GetLegacyRenderFrameStats();
    void RecordLegacyTextureUpload(unsigned long long bytes);
    void InvalidateLegacyRenderStateCache();

    // Diagnostico do backend GLSL. Sem um logger registrado, falhas de
    // carregamento de funcoes, compilacao e link de shader ficam silenciosas e o
    // adapter apenas para de desenhar.
    typedef void (*LegacyRenderLogFn)(const char* message);
    void SetLegacyRenderLogger(LegacyRenderLogFn logger);
    void LegacyRenderLog(const char* message);
}
