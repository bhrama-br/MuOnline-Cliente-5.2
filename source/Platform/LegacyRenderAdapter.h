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
    // Estado por instancia de uma malha residente. Tudo que variava por objeto e
    // era uniforme vira atributo, para que diferencas de wave, efeito de material
    // ou shadow map nao quebrem o lote.
    struct StaticMeshInstance
    {
        StaticMeshInstance()
            : bodyScale(1.f), lighting(false), boneScale(1.f), materialEffect(0),
              wave(false), shadowMap(false), boneMatrices(NULL), boneCount(0)
        {
            color[0] = color[1] = color[2] = color[3] = 1.f;
            postTranslation[0] = postTranslation[1] = postTranslation[2] = 0.f;
            lightPosition[0] = lightPosition[1] = lightPosition[2] = 0.f;
            bodyOrigin[0] = bodyOrigin[1] = bodyOrigin[2] = 0.f;
        }

        float color[4];
        float postTranslation[3];
        float bodyScale;
        float lightPosition[3];
        bool lighting;
        float bodyOrigin[3];
        float boneScale;
        int materialEffect;
        bool wave;
        bool shadowMap;
        // boneCount * 12 floats em ordem de linha, igual ao BMD. NULL desliga o
        // skinning para esta instancia.
        const float* boneMatrices;
        size_t boneCount;
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
              fogChanges(0), blendStateChanges(0), staticMeshDrawCalls(0), staticMeshIndices(0), staticMeshUploadBytes(0), bonePaletteUploadBytes(0), cpuSkinningVertices(0), cpuSkinningNormals(0), gpuSkinningFallbacks(0), gpuSkinningMaterialFallbacks(0), gpuSkinningGeometryFallbacks(0), gpuSkinningResourceFallbacks(0),
              instancedDrawCalls(0), instancesSubmitted(0), instanceBatchesFlushed(0), largestInstanceBatch(0), instancePaletteDedupHits(0),
              staticMeshCacheHits(0), staticMeshCacheMisses(0), staticMeshVerticesResident(0), staticMeshIndicesResident(0),
              transformsExecuted(0), transformsSkipped(0), animationsExecuted(0), animationsSkipped(0), uniformCallsSaved(0) {}

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

        // Fase 4: um draw instanciado cobre `instancesSubmitted / instancedDrawCalls`
        // instancias em media. largestInstanceBatch mostra o teto alcancado no frame,
        // que e o indicador de que a chave de lote nao esta fragmentando demais.
        unsigned long long instancedDrawCalls;
        unsigned long long instancesSubmitted;
        unsigned long long instanceBatchesFlushed;
        unsigned long long largestInstanceBatch;
        unsigned long long instancePaletteDedupHits;

        // Fase 1: residencia de geometria. Hits/misses medem se a chave do cache
        // esta estavel entre frames; os "resident" mostram o efeito da indexacao.
        unsigned long long staticMeshCacheHits;
        unsigned long long staticMeshCacheMisses;
        unsigned long long staticMeshVerticesResident;
        unsigned long long staticMeshIndicesResident;

        // Fase 2: quanto do skinning/animacao de CPU foi realmente evitado.
        unsigned long long transformsExecuted;
        unsigned long long transformsSkipped;
        unsigned long long animationsExecuted;
        unsigned long long animationsSkipped;

        // Fase 3: chamadas glUniform* suprimidas pelo shadow state do programa.
        unsigned long long uniformCallsSaved;
    };

    // Layout identico ao vertice interno do backend GLSL e ao RenderVertex da
    // fila. Existe para que um emissor entregue um bloco pronto em vez de pagar
    // quatro chamadas virtuais por vertice.
    struct LegacyBulkVertex
    {
        float position[3];
        float color[4];
        float texCoord[2];
        float normal[3];
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
        // Submissao em bloco. A implementacao padrao reproduz exatamente o
        // caminho por vertice, entao um backend que nao a especialize continua
        // correto; o backend GLSL a especializa com um append em memoria.
        //
        // Forma AoS: o chamador ja tem os vertices no layout final.
        virtual void DrawVertices(LegacyPrimitive primitive, const LegacyBulkVertex* vertices, size_t count)
        {
            if (vertices == NULL || count == 0) return;
            Begin(primitive);
            for (size_t i = 0; i < count; ++i)
            {
                const LegacyBulkVertex& v = vertices[i];
                Color4f(v.color[0], v.color[1], v.color[2], v.color[3]);
                TexCoord2f(v.texCoord[0], v.texCoord[1]);
                Normal3f(v.normal[0], v.normal[1], v.normal[2]);
                Vertex3fv(v.position);
            }
            End();
        }
        // Forma SoA: o formato em que o caminho legado do BMD e o terreno ja
        // mantem os dados. colors/texCoords podem ser NULL, e nesse caso vale o
        // valor corrente — igual ao GL_COLOR_ARRAY desabilitado do original.
        virtual void DrawVertexArrays(LegacyPrimitive primitive, const float (*positions)[3],
            const float (*colors)[4], const float (*texCoords)[2], size_t count)
        {
            if (positions == NULL || count == 0) return;
            Begin(primitive);
            for (size_t i = 0; i < count; ++i)
            {
                if (colors != NULL) Color4f(colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
                if (texCoords != NULL) TexCoord2f(texCoords[i][0], texCoords[i][1]);
                Vertex3fv(positions[i]);
            }
            End();
        }
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
        // Devolve um handle opaco (0 = falha). O handle carrega a geracao do
        // contexto grafico: apos InvalidateGraphicsResources todos os handles
        // antigos passam a responder false em IsStaticMeshResident, e o chamador
        // reconstroi a geometria sem precisar guardar uma copia viva na RAM.
        virtual unsigned int UploadStaticMesh(const StaticMeshVertex* vertices, size_t vertexCount,
            const unsigned int* indices, size_t indexCount)
        { (void)vertices; (void)vertexCount; (void)indices; (void)indexCount; return 0; }
        virtual bool IsStaticMeshResident(unsigned int handle) const { (void)handle; return false; }
        // modelMatrix e uma matriz afim 3x4 em ordem de linhas, igual ao BMD.
        virtual bool DrawStaticMesh(unsigned int handle, const float* color, const float* modelMatrix,
            const float* boneMatrices = NULL, size_t boneCount = 0, float bodyScale = 1.f,
            bool lighting = false, const float* lightPosition = NULL, const float* postTranslation = NULL,
            bool wave = false, float worldTime = 0.f, int materialEffect = 0,
            bool shadowMap = false, const float* bodyOrigin = NULL, float boneScale = 1.f)
        { (void)handle; (void)color; (void)modelMatrix; (void)boneMatrices; (void)boneCount; (void)bodyScale; (void)lighting; (void)lightPosition; (void)postTranslation; (void)wave; (void)worldTime; (void)materialEffect; (void)shadowMap; (void)bodyOrigin; (void)boneScale; return false; }
        // Desenha N instancias da mesma malha residente numa chamada. Devolve
        // false quando o backend nao suporta instancing ou a paleta nao cabe; o
        // chamador entao emite instancia por instancia por DrawStaticMesh.
        virtual bool DrawStaticMeshInstanced(unsigned int handle, const StaticMeshInstance* instances, size_t count)
        { (void)handle; (void)instances; (void)count; return false; }
        // 0 quando nao ha caminho instanciado disponivel.
        virtual size_t GetMaxInstanceBoneCount() const { return 0; }
        virtual void ReleaseStaticMesh(unsigned int handle) { (void)handle; }
        virtual void RecordCpuSkinningWork(unsigned long long vertices, unsigned long long normals)
        { (void)vertices; (void)normals; }
        virtual void RecordGpuSkinningFallback(GpuSkinningFallbackReason reason) { (void)reason; }
        // Trabalho de CPU por frame que o cache de pose evitou (ou nao). Fica no
        // adapter, e nao numa global do cliente, para zerar junto com o resto das
        // estatisticas em ResetFrameStats.
        virtual void RecordCpuTransformWork(unsigned long long transformsExecuted, unsigned long long transformsSkipped,
            unsigned long long animationsExecuted, unsigned long long animationsSkipped)
        { (void)transformsExecuted; (void)transformsSkipped; (void)animationsExecuted; (void)animationsSkipped; }
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
    // Indice do frame corrente. Serve para invalidar caches por frame sem que
    // cada um tenha que manter o proprio contador.
    unsigned long GetRenderFrameIndex();
    bool ShouldUseGpuSkinning();
    bool ShouldUseGpuSkinningForModel(int modelId);

    // Otimizacoes que entram por fase. Cada uma tem um interruptor proprio para
    // que uma regressao possa ser isolada em campo sem recompilar, e um modo
    // Compare que alterna por frame contra o caminho antigo.
    enum RenderFeature
    {
        RenderFeatureInstancing,
        RenderFeatureStaticTransformCache,
        RenderFeatureBatching,
        RenderFeatureCount
    };

    enum RenderFeatureMode
    {
        RenderFeatureDisabled,
        RenderFeatureEnabled,
        RenderFeatureCompare
    };

    void SetRenderFeatureMode(RenderFeature feature, RenderFeatureMode mode);
    RenderFeatureMode GetRenderFeatureMode(RenderFeature feature);
    // Em Compare, alterna com a mesma paridade de frame usada pelo GPU skinning,
    // entao uma captura lado a lado compara frames adjacentes.
    bool IsRenderFeatureActive(RenderFeature feature);
    const char* GetRenderFeatureModeName(RenderFeature feature);

    // Seguro de chamar antes de qualquer adapter ter sido instalado.
    void InvalidateLegacyRenderResources();
    // Materializa os sprites pendentes antes de observar ou alterar o framebuffer.
    void FlushLegacyRenderBatch();
    void ResetLegacyRenderFrameStats();
    LegacyRenderFrameStats GetLegacyRenderFrameStats();
    void RecordLegacyTextureUpload(unsigned long long bytes);
    void RecordCpuSkinningWork(unsigned long long vertices, unsigned long long normals);
    void RecordGpuSkinningFallback(GpuSkinningFallbackReason reason);
    void RecordCpuTransformWork(unsigned long long transformsExecuted, unsigned long long transformsSkipped,
        unsigned long long animationsExecuted, unsigned long long animationsSkipped);
    void InvalidateLegacyRenderStateCache();

    // Diagnostico do backend GLSL. Sem um logger registrado, falhas de
    // carregamento de funcoes, compilacao e link de shader ficam silenciosas e o
    // adapter apenas para de desenhar.
    typedef void (*LegacyRenderLogFn)(const char* message);
    void SetLegacyRenderLogger(LegacyRenderLogFn logger);
    void LegacyRenderLog(const char* message);
}
