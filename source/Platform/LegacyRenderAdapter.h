#pragma once

#include <cstddef>

namespace Platform
{
    enum GpuSkinningFallbackReason
    {
        GpuSkinningFallbackMaterial,
        GpuSkinningFallbackGeometry,
        GpuSkinningFallbackResource
    };
    // Formato residente para malhas imutaveis. Mantem o layout de atributos do
    // shader de compatibilidade, mas permite VBO/IBO persistentes em vez de
    // reenviar os triangulos expandidos a cada frame.
    struct StaticMeshVertex
    {
        float position[3];
        float color[4];
        float texCoord[2];
        float normal[3];
        float positionBone;
        float normalBone;
        float waveSeed;
    };
    // Contadores do caminho legado moderno desde o ultimo Reset. O ciclo de frame
    // (e a apresentacao do overlay/CSV) pode ser conectado sem expor detalhes GL
    // aos chamadores do renderer.
    struct LegacyRenderFrameStats
    {
        LegacyRenderFrameStats()
            : drawCalls(0), vertices(0), vertexUploadBytes(0), bufferDataCalls(0), bufferSubDataCalls(0), batchFlushes(0),
              textureUploads(0), textureUploadBytes(0), textureChanges(0), matrixFlushes(0), textureFlushes(0), blendFlushes(0),
              depthFlushes(0), alphaFlushes(0), fogFlushes(0), programChanges(0), depthStateChanges(0), alphaTestChanges(0),
              fogChanges(0), blendStateChanges(0), staticMeshDrawCalls(0), staticMeshIndices(0), staticMeshUploadBytes(0), bonePaletteUploadBytes(0), cpuSkinningVertices(0), cpuSkinningNormals(0), gpuSkinningFallbacks(0), gpuSkinningMaterialFallbacks(0), gpuSkinningGeometryFallbacks(0), gpuSkinningResourceFallbacks(0) {}

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
        unsigned long long staticMeshDrawCalls;
        unsigned long long staticMeshIndices;
        unsigned long long staticMeshUploadBytes;
        unsigned long long bonePaletteUploadBytes;
        unsigned long long cpuSkinningVertices;
        unsigned long long cpuSkinningNormals;
        unsigned long long gpuSkinningFallbacks;
        unsigned long long gpuSkinningMaterialFallbacks;
        unsigned long long gpuSkinningGeometryFallbacks;
        unsigned long long gpuSkinningResourceFallbacks;
    };

    enum LegacyPrimitive
    {
        LegacyPrimitiveQuads,
        LegacyPrimitiveLineStrip,
        LegacyPrimitiveTriangles,
        LegacyPrimitiveTriangleFan,
        LegacyPrimitiveLines
    };

    enum GpuSkinningMode
    {
        GpuSkinningOff,
        GpuSkinningOn,
        // Alterna por frame entre CPU e GPU para comparacao visual segura.
        GpuSkinningCompare
    };

    // Politica de rollout independente do modo de comparacao. Em producao a
    // GPU e liberada apenas para os IDs explicitamente autorizados.
    enum GpuSkinningDeployment
    {
        GpuSkinningDevelopment,
        GpuSkinningQA,
        GpuSkinningProductionWhitelist,
        GpuSkinningProductionDefault
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
        virtual bool UploadStaticMesh(const void* key, const StaticMeshVertex* vertices, size_t vertexCount,
            const unsigned int* indices, size_t indexCount)
        { (void)key; (void)vertices; (void)vertexCount; (void)indices; (void)indexCount; return false; }
        // modelMatrix e uma matriz afim 3x4 em ordem de linhas, igual ao BMD.
        virtual bool DrawStaticMesh(const void* key, const float* color, const float* modelMatrix,
            const float* boneMatrices = NULL, size_t boneCount = 0, float bodyScale = 1.f,
            bool lighting = false, const float* lightPosition = NULL, const float* postTranslation = NULL,
            bool wave = false, float worldTime = 0.f, int materialEffect = 0,
            bool shadowMap = false, const float* bodyOrigin = NULL, float boneScale = 1.f)
        { (void)key; (void)color; (void)modelMatrix; (void)boneMatrices; (void)boneCount; (void)bodyScale; (void)lighting; (void)lightPosition; (void)postTranslation; (void)wave; (void)worldTime; (void)materialEffect; (void)shadowMap; (void)bodyOrigin; (void)boneScale; return false; }
        virtual void ReleaseStaticMesh(const void* key) { (void)key; }
        virtual void RecordCpuSkinningWork(unsigned long long vertices, unsigned long long normals)
        { (void)vertices; (void)normals; }
        virtual void RecordGpuSkinningFallback(GpuSkinningFallbackReason reason) { (void)reason; }
    };

    ILegacyRenderAdapter& GetLegacyRenderAdapter();
    void SetLegacyRenderAdapter(ILegacyRenderAdapter* adapter);
    void EnableGlslLegacyBackend(bool enabled);
    bool IsGlslLegacyBackendEnabled();
    void SetGpuSkinningMode(GpuSkinningMode mode);
    GpuSkinningMode GetGpuSkinningMode();
    void SetGpuSkinningDeployment(GpuSkinningDeployment deployment);
    GpuSkinningDeployment GetGpuSkinningDeployment();
    // Lista separada por virgulas de IDs de Models[]; NULL/vazia nao libera
    // modelos quando o deployment e ProductionWhitelist.
    void SetGpuSkinningModelWhitelist(const char* modelIds);
    void BeginGpuSkinningFrame();
    bool ShouldUseGpuSkinning();
    bool ShouldUseGpuSkinningForModel(int modelId);

    // Seguro de chamar antes de qualquer adapter ter sido instalado.
    void InvalidateLegacyRenderResources();
    // Materializa os sprites pendentes antes de observar ou alterar o framebuffer.
    void FlushLegacyRenderBatch();
    void ResetLegacyRenderFrameStats();
    LegacyRenderFrameStats GetLegacyRenderFrameStats();
    void RecordLegacyTextureUpload(unsigned long long bytes);
    void RecordCpuSkinningWork(unsigned long long vertices, unsigned long long normals);
    void RecordGpuSkinningFallback(GpuSkinningFallbackReason reason);
    void InvalidateLegacyRenderStateCache();

    // Diagnostico do backend GLSL. Sem um logger registrado, falhas de
    // carregamento de funcoes, compilacao e link de shader ficam silenciosas e o
    // adapter apenas para de desenhar.
    typedef void (*LegacyRenderLogFn)(const char* message);
    void SetLegacyRenderLogger(LegacyRenderLogFn logger);
    void LegacyRenderLog(const char* message);
}
