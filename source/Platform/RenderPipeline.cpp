#include "stdafx.h"
#include "RenderPipeline.h"
#include "LegacyRenderAdapter.h"

namespace
{
    static Platform::RenderQueue g_opaqueWorldQueue;
    static Platform::OpenGL33RenderBackend g_opaqueWorldBackend;
    static bool g_opaqueWorldQueueActive = false;

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

void Platform::RenderQueue::Execute(IRenderBackend& backend)
{
    if (m_commands.empty())
        return;

    std::stable_sort(m_commands.begin(), m_commands.end(), IsOpaqueBefore);
    RenderPass activePass = m_commands[0].pass;
    backend.BeginPass(activePass);
    for (size_t index = 0; index < m_commands.size(); ++index)
    {
        const RenderCommand& command = m_commands[index];
        if (command.pass != activePass)
        {
            backend.EndPass(activePass);
            activePass = command.pass;
            backend.BeginPass(activePass);
        }
        backend.Draw(command, &m_vertices[command.firstVertex]);
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

    renderer.Begin(ToLegacyPrimitive(command.topology));
    for (size_t index = 0; index < command.vertexCount; ++index)
    {
        const RenderVertex& vertex = vertices[index];
        renderer.Color4f(vertex.color[0], vertex.color[1], vertex.color[2], vertex.color[3]);
        renderer.TexCoord2f(vertex.texCoord[0], vertex.texCoord[1]);
        renderer.Normal3f(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
        renderer.Vertex3fv(vertex.position);
    }
    renderer.End();
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

void Platform::FlushOpaqueWorldRenderQueue()
{
    if (!g_opaqueWorldQueueActive)
        return;
    g_opaqueWorldQueue.Execute(g_opaqueWorldBackend);
    g_opaqueWorldQueue.BeginFrame();
}

void Platform::ExecuteOpaqueWorldRenderQueue()
{
    if (!g_opaqueWorldQueueActive)
        return;
    FlushOpaqueWorldRenderQueue();
    g_opaqueWorldQueueActive = false;
}
