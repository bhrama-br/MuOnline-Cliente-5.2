#include "stdafx.h"
#include "RenderPipeline.h"
#include "LegacyRenderAdapter.h"

namespace
{
    static Platform::RenderQueue g_opaqueWorldQueue;
    static Platform::OpenGL33RenderBackend g_opaqueWorldBackend;
    static bool g_opaqueWorldQueueActive = false;
    static Platform::InstanceBatchFlushFn g_instanceBatchFlush = NULL;

    static bool IsOpaqueBefore(const Platform::RenderCommand& left, const Platform::RenderCommand& right)
    {
        if (left.pass != right.pass) return left.pass < right.pass;
        if (left.material.transparent != right.material.transparent)
            return !left.material.transparent;

        // Transparencias nao podem ser reordenadas: o cliente legado depende
        // da ordem de emissao para compor blend, mesmo quando a profundidade
        // calculada sugere outra ordem.
        if (left.material.transparent)
            return left.sequence < right.sequence;

        if (left.material.shader != right.material.shader) return left.material.shader < right.material.shader;
        if (left.material.blendMode != right.material.blendMode) return left.material.blendMode < right.material.blendMode;
        if (left.material.depthTest != right.material.depthTest) return left.material.depthTest < right.material.depthTest;
        if (left.material.alphaTest != right.material.alphaTest) return left.material.alphaTest < right.material.alphaTest;
        if (left.material.alphaReference != right.material.alphaReference)
            return left.material.alphaReference < right.material.alphaReference;
        if (left.material.texture.id != right.material.texture.id) return left.material.texture.id < right.material.texture.id;
        return left.sequence < right.sequence;
    }

    // O reinterpret_cast em Draw depende disto. Se algum dia os dois layouts
    // divergirem, o build quebra aqui em vez de renderizar lixo.
    static_assert(sizeof(Platform::RenderVertex) == sizeof(Platform::LegacyBulkVertex),
        "RenderVertex e LegacyBulkVertex precisam ter o mesmo layout");
    static_assert(offsetof(Platform::RenderVertex, color) == offsetof(Platform::LegacyBulkVertex, color),
        "campo color desalinhado entre RenderVertex e LegacyBulkVertex");
    static_assert(offsetof(Platform::RenderVertex, texCoord) == offsetof(Platform::LegacyBulkVertex, texCoord),
        "campo texCoord desalinhado entre RenderVertex e LegacyBulkVertex");
    static_assert(offsetof(Platform::RenderVertex, normal) == offsetof(Platform::LegacyBulkVertex, normal),
        "campo normal desalinhado entre RenderVertex e LegacyBulkVertex");

    static Platform::LegacyPrimitive ToLegacyPrimitive(Platform::RenderTopology topology)
    {
        switch (topology)
        {
        case Platform::RenderTopologyQuads: return Platform::LegacyPrimitiveQuads;
        case Platform::RenderTopologyLines: return Platform::LegacyPrimitiveLines;
        case Platform::RenderTopologyTriangles: default: return Platform::LegacyPrimitiveTriangles;
        }
    }
}

void Platform::RenderQueue::BeginFrame()
{
    Clear();
}

void Platform::RenderQueue::Clear()
{
    m_commands.clear();
    m_vertices.clear();
    m_nextSequence = 0;
}

void Platform::RenderQueue::Submit(const RenderCommand& source, const RenderVertex* vertices, size_t vertexCount)
{
    if (vertices == NULL || vertexCount == 0)
        return;

    RenderCommand command = source;
    // Um material que usa blend nao e seguro para a ordenacao de opacos. Isto
    // protege migracoes graduais que ainda nao tenham marcado transparent.
    if (command.material.blendMode != 0)
        command.material.transparent = true;
    command.firstVertex = m_vertices.size();
    command.vertexCount = vertexCount;
    command.sequence = m_nextSequence++;
    m_vertices.insert(m_vertices.end(), vertices, vertices + vertexCount);
    m_commands.push_back(command);
}

namespace
{
    // Mesmo passe, mesmo material, mesma topologia: os vertices podem ir num
    // draw so. Como a fusao e sempre entre comandos ADJACENTES na ordem final,
    // ela preserva a ordem de composicao — inclusive para transparentes.
    bool PodeFundir(const Platform::RenderCommand& a, const Platform::RenderCommand& b)
    {
        return a.pass == b.pass && a.topology == b.topology &&
            a.material.shader == b.material.shader &&
            a.material.blendMode == b.material.blendMode &&
            a.material.depthTest == b.material.depthTest &&
            a.material.alphaTest == b.material.alphaTest &&
            a.material.alphaReference == b.material.alphaReference &&
            a.material.transparent == b.material.transparent &&
            a.material.texture.id == b.material.texture.id;
    }
}

void Platform::RenderQueue::Execute(IRenderBackend& backend)
{
    if (m_commands.empty())
        return;

    std::stable_sort(m_commands.begin(), m_commands.end(), IsOpaqueBefore);

    // O terreno emite um comando por tile (4 vertices). Sem fundir, um mapa
    // inteiro vira milhares de draws de um quad cada.
    const bool fundir = IsRenderFeatureActive(RenderFeatureBatching);

    RenderPass activePass = m_commands[0].pass;
    backend.BeginPass(activePass);
    size_t index = 0;
    while (index < m_commands.size())
    {
        const RenderCommand& command = m_commands[index];
        if (command.pass != activePass)
        {
            backend.EndPass(activePass);
            activePass = command.pass;
            backend.BeginPass(activePass);
        }

        size_t run = 1;
        size_t totalVertices = command.vertexCount;
        if (fundir)
        {
            while (index + run < m_commands.size() && PodeFundir(command, m_commands[index + run]))
            {
                totalVertices += m_commands[index + run].vertexCount;
                ++run;
            }
        }

        if (run == 1)
        {
            backend.Draw(command, &m_vertices[command.firstVertex]);
        }
        else
        {
            // Os intervalos de vertice nao sao contiguos depois da ordenacao,
            // entao a corrida e reunida num buffer proprio. Ele e membro para
            // nao realocar a cada frame.
            m_mergedVertices.clear();
            m_mergedVertices.reserve(totalVertices);
            for (size_t step = 0; step < run; ++step)
            {
                const RenderCommand& part = m_commands[index + step];
                const RenderVertex* first = &m_vertices[part.firstVertex];
                m_mergedVertices.insert(m_mergedVertices.end(), first, first + part.vertexCount);
            }
            RenderCommand merged = command;
            merged.vertexCount = totalVertices;
            merged.firstVertex = 0;
            backend.Draw(merged, &m_mergedVertices[0]);
        }
        index += run;
    }
    backend.EndPass(activePass);
}

void Platform::OpenGL33RenderBackend::BeginPass(RenderPass pass)
{
    (void)pass;
    GetLegacyRenderAdapter().BeginBatch();
}

void Platform::OpenGL33RenderBackend::Draw(const RenderCommand& command, const RenderVertex* vertices)
{
    if (vertices == NULL || command.vertexCount == 0)
        return;

    ILegacyRenderAdapter& renderer = GetLegacyRenderAdapter();
    renderer.SetDepthTest(command.material.depthTest);
    renderer.SetAlphaTest(command.material.alphaTest);
    renderer.SetAlphaTestRef(command.material.alphaReference);
    renderer.SetTexture2D(command.material.texture.IsValid());
    if (command.material.texture.IsValid())
        renderer.BindTexture(command.material.texture.id);
    renderer.SetBlendMode(command.material.blendMode);

    // RenderVertex e LegacyBulkVertex sao o mesmo layout; a fila entrega o bloco
    // pronto em vez de reemitir vertice a vertice pelo adaptador.
    renderer.DrawVertices(ToLegacyPrimitive(command.topology),
        reinterpret_cast<const LegacyBulkVertex*>(vertices), command.vertexCount);
}

void Platform::OpenGL33RenderBackend::EndPass(RenderPass pass)
{
    (void)pass;
    GetLegacyRenderAdapter().EndBatch();
}

void Platform::ExecuteRenderCommand(const RenderCommand& command, const RenderVertex* vertices)
{
    static OpenGL33RenderBackend backend;
    backend.Draw(command, vertices);
}

bool Platform::BeginOpaqueWorldRenderQueue()
{
    if (!IsGlslLegacyBackendEnabled())
        return false;
    g_opaqueWorldQueue.BeginFrame();
    g_opaqueWorldQueueActive = true;
    return true;
}

bool Platform::IsOpaqueWorldRenderQueueActive()
{
    return g_opaqueWorldQueueActive;
}

void Platform::SubmitOpaqueWorldRenderCommand(const RenderCommand& command, const RenderVertex* vertices, size_t vertexCount)
{
    if (g_opaqueWorldQueueActive)
        g_opaqueWorldQueue.Submit(command, vertices, vertexCount);
}

void Platform::SetInstanceBatchFlushCallback(InstanceBatchFlushFn callback)
{
    g_instanceBatchFlush = callback;
}

void Platform::FlushInstanceBatches()
{
    if (g_instanceBatchFlush != NULL)
        g_instanceBatchFlush();
}

void Platform::FlushOpaqueWorldRenderQueue()
{
    // As instancias saem antes: elas sao opacas e desenham direto, enquanto a
    // fila reordena. Descarregar o coletor primeiro mantem a ordem de emissao
    // entre os dois caminhos igual a do codigo sem instancing.
    FlushInstanceBatches();
    if (!g_opaqueWorldQueueActive)
        return;
    g_opaqueWorldQueue.Execute(g_opaqueWorldBackend);
    g_opaqueWorldQueue.BeginFrame();
}

void Platform::ExecuteOpaqueWorldRenderQueue()
{
    FlushInstanceBatches();
    if (!g_opaqueWorldQueueActive)
        return;
    FlushOpaqueWorldRenderQueue();
    g_opaqueWorldQueueActive = false;
}
