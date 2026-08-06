#pragma once

#include <vector>

namespace Platform
{
    // API de alto nivel do renderer. Os identificadores sao neutros: nenhum
    // chamador precisa conhecer GLuint, WebGLTexture ou um backend futuro.
    typedef unsigned int RenderResourceId;

    // IDs estaveis de shader. Acrescentar uma versao, em vez de trocar a
    // semantica de um ID existente, permite comparar capturas entre backends.
    enum RenderShader
    {
        RenderShaderLegacyCompatV1 = 1,
        RenderShaderUiV1 = 2,
        RenderShaderTextV1 = 3,
        RenderShaderParticlesV1 = 4
    };

    enum RenderPass
    {
        RenderPassUi,
        RenderPassText,
        RenderPassParticles,
        RenderPassTerrain,
        RenderPassStaticObjects,
        RenderPassCharacters,
        RenderPassEffects
    };

    enum RenderTopology
    {
        RenderTopologyTriangles,
        RenderTopologyQuads,
        RenderTopologyLines
    };

    struct Texture
    {
        Texture(RenderResourceId value = 0) : id(value) {}
        RenderResourceId id;
        bool IsValid() const { return id != 0; }
    };

    struct Mesh
    {
        Mesh(RenderResourceId value = 0) : id(value) {}
        RenderResourceId id;
        bool IsValid() const { return id != 0; }
    };

    struct Material
    {
        Material()
            : texture(), shader(0), blendMode(0), depthTest(true), alphaTest(false), alphaReference(0.25f), transparent(false) {}

        Texture texture;
        RenderResourceId shader;
        int blendMode;
        bool depthTest;
        bool alphaTest;
        float alphaReference;
        bool transparent;
    };

    struct RenderVertex
    {
        float position[3];
        float color[4];
        float texCoord[2];
        float normal[3];
    };

    struct RenderCommand
    {
        RenderCommand()
            : pass(RenderPassUi), mesh(), material(), topology(RenderTopologyTriangles), firstVertex(0), vertexCount(0), depth(0.f), sequence(0) {}

        RenderPass pass;
        Mesh mesh;
        Material material;
        RenderTopology topology;
        size_t firstVertex;
        size_t vertexCount;
        float depth;
        unsigned long sequence;
    };

    class IRenderBackend
    {
    public:
        virtual ~IRenderBackend() {}
        virtual void BeginPass(RenderPass pass) = 0;
        virtual void Draw(const RenderCommand& command, const RenderVertex* vertices) = 0;
        virtual void EndPass(RenderPass pass) = 0;
    };

    // Fila por frame. Comandos transparentes mantem ordem de emissao; opacos
    // sao agrupados por shader/material/textura somente dentro do mesmo passe.
    class RenderQueue
    {
    public:
        void BeginFrame();
        void Clear();
        void Submit(const RenderCommand& command, const RenderVertex* vertices, size_t vertexCount);
        void Execute(IRenderBackend& backend);
        size_t GetCommandCount() const { return m_commands.size(); }

    private:
        std::vector<RenderCommand> m_commands;
        std::vector<RenderVertex> m_vertices;
        unsigned long m_nextSequence = 0;
    };

    // Primeiro backend da nova API. Materializa comandos no adaptador GLSL,
    // que usa VAO/VBO e shader em OpenGL 3.3/WebGL 2. O adaptador de funcao
    // fixa permanece selecionavel para comparar o resultado durante a
    // migracao de cada passe.
    class OpenGL33RenderBackend : public IRenderBackend
    {
    public:
        virtual void BeginPass(RenderPass pass);
        virtual void Draw(const RenderCommand& command, const RenderVertex* vertices);
        virtual void EndPass(RenderPass pass);
    };

    // Nome de compatibilidade para pontos de migracao que ainda precisam
    // deixar explicito que usam o backend de comparacao atual.
    typedef OpenGL33RenderBackend LegacyCompatibilityRenderBackend;

    // Ponte para migracao gradual: um emissor pode usar material e comando
    // explicitos sem precisar esperar que todo o passe seja convertido.
    void ExecuteRenderCommand(const RenderCommand& command, const RenderVertex* vertices);

    // Fila de opacos do mundo. Ela e ativada somente no backend GLSL e deve
    // ser descarregada antes de qualquer emissao transparente/legada.
    bool BeginOpaqueWorldRenderQueue();
    bool IsOpaqueWorldRenderQueueActive();
    void SubmitOpaqueWorldRenderCommand(const RenderCommand& command, const RenderVertex* vertices, size_t vertexCount);
    // Barreira entre blocos: desenha o que foi acumulado, mas mantem a fila
    // pronta para continuar recebendo opacos no mesmo frame.
    void FlushOpaqueWorldRenderQueue();
    void ExecuteOpaqueWorldRenderQueue();
}
