#include "stdafx.h"
#include "LegacyRenderAdapter.h"

#ifdef glAttachShader
#undef glAttachShader
#undef glActiveTexture
#undef glBindBuffer
#undef glBindVertexArray
#undef glBufferData
#undef glBufferSubData
#undef glCompileShader
#undef glCreateProgram
#undef glCreateShader
#undef glDeleteBuffers
#undef glDeleteProgram
#undef glDeleteShader
#undef glDeleteVertexArrays
#undef glDrawArrays
#undef glDrawElements
#undef glEnableVertexAttribArray
#undef glGenBuffers
#undef glGenVertexArrays
#undef glGetProgramiv
#undef glGetShaderiv
#undef glGetUniformLocation
#undef glLinkProgram
#undef glShaderSource
#undef glUniformMatrix4fv
#undef glUniform1i
#undef glUniform1f
#undef glUniform3f
#undef glUseProgram
#undef glVertexAttribPointer
#endif

namespace
{
#ifdef _WIN32
    typedef void (APIENTRY *PFNLEGACYDRAWARRAYSPROC)(GLenum mode, GLint first, GLsizei count);
    typedef void (APIENTRY *PFNLEGACYDRAWELEMENTSPROC)(GLenum mode, GLsizei count, GLenum type, const void* indices);
#else
    typedef void (*PFNLEGACYDRAWARRAYSPROC)(GLenum mode, GLint first, GLsizei count);
    typedef void (*PFNLEGACYDRAWELEMENTSPROC)(GLenum mode, GLsizei count, GLenum type, const void* indices);
#endif

    struct ModernGlApi
    {
        PFNGLATTACHSHADERPROC AttachShader;
        PFNGLACTIVETEXTUREPROC ActiveTexture;
        PFNGLBINDBUFFERPROC BindBuffer;
        PFNGLBINDVERTEXARRAYPROC BindVertexArray;
        PFNGLBUFFERDATAPROC BufferData;
        PFNGLBUFFERSUBDATAPROC BufferSubData;
        PFNGLCOMPILESHADERPROC CompileShader;
        PFNGLCREATEPROGRAMPROC CreateProgram;
        PFNGLCREATESHADERPROC CreateShader;
        PFNGLDELETEBUFFERSPROC DeleteBuffers;
        PFNGLDELETEPROGRAMPROC DeleteProgram;
        PFNGLDELETESHADERPROC DeleteShader;
        PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays;
        PFNLEGACYDRAWARRAYSPROC DrawArrays;
        PFNLEGACYDRAWELEMENTSPROC DrawElements;
        PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
        PFNGLGENBUFFERSPROC GenBuffers;
        PFNGLGENVERTEXARRAYSPROC GenVertexArrays;
        PFNGLGETPROGRAMIVPROC GetProgramiv;
        PFNGLGETSHADERIVPROC GetShaderiv;
        PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
        PFNGLLINKPROGRAMPROC LinkProgram;
        PFNGLSHADERSOURCEPROC ShaderSource;
        PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv;
        PFNGLUNIFORM1IPROC Uniform1i;
        PFNGLUNIFORM1FPROC Uniform1f;
        PFNGLUNIFORM3FPROC Uniform3f;
        PFNGLUSEPROGRAMPROC UseProgram;
        PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;
        PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;
        PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog;

        // Nome da primeira funcao que nao pode ser resolvida, para diagnostico.
        const char* MissingName() const
        {
#define CHECK_GL(name) if (name == NULL) return "gl" #name " nao resolvida";
            CHECK_GL(AttachShader); CHECK_GL(ActiveTexture); CHECK_GL(BindBuffer); CHECK_GL(BindVertexArray); CHECK_GL(BufferData); CHECK_GL(BufferSubData);
            CHECK_GL(CompileShader); CHECK_GL(CreateProgram); CHECK_GL(CreateShader); CHECK_GL(DeleteBuffers);
            CHECK_GL(DeleteProgram); CHECK_GL(DeleteShader); CHECK_GL(DeleteVertexArrays); CHECK_GL(DrawArrays); CHECK_GL(DrawElements);
            CHECK_GL(EnableVertexAttribArray); CHECK_GL(GenBuffers); CHECK_GL(GenVertexArrays); CHECK_GL(GetProgramiv);
            CHECK_GL(GetShaderiv); CHECK_GL(GetUniformLocation); CHECK_GL(LinkProgram); CHECK_GL(ShaderSource);
            CHECK_GL(UniformMatrix4fv); CHECK_GL(Uniform1i); CHECK_GL(UseProgram); CHECK_GL(VertexAttribPointer);
#undef CHECK_GL
            return "todas as funcoes resolvidas";
        }

        bool Load()
        {
#ifdef _WIN32
#define LOAD_GL(name) name = reinterpret_cast<decltype(name)>(wglGetProcAddress("gl" #name));
#elif defined(__EMSCRIPTEN__)
#define LOAD_GL(name) name = &gl##name;
#else
#define LOAD_GL(name) name = reinterpret_cast<decltype(name)>(eglGetProcAddress("gl" #name));
#endif
            LOAD_GL(AttachShader); LOAD_GL(ActiveTexture); LOAD_GL(BindBuffer); LOAD_GL(BindVertexArray); LOAD_GL(BufferData); LOAD_GL(BufferSubData);
            LOAD_GL(CompileShader); LOAD_GL(CreateProgram); LOAD_GL(CreateShader); LOAD_GL(DeleteBuffers);
            LOAD_GL(DeleteProgram); LOAD_GL(DeleteShader); LOAD_GL(DeleteVertexArrays); LOAD_GL(DrawArrays); LOAD_GL(DrawElements);
            LOAD_GL(EnableVertexAttribArray); LOAD_GL(GenBuffers); LOAD_GL(GenVertexArrays); LOAD_GL(GetProgramiv);
            LOAD_GL(GetShaderiv); LOAD_GL(GetUniformLocation); LOAD_GL(LinkProgram); LOAD_GL(ShaderSource);
            LOAD_GL(UniformMatrix4fv); LOAD_GL(UseProgram); LOAD_GL(VertexAttribPointer);
            LOAD_GL(Uniform1i); LOAD_GL(Uniform1f); LOAD_GL(Uniform3f);
            LOAD_GL(GetShaderInfoLog); LOAD_GL(GetProgramInfoLog);
#undef LOAD_GL
            return AttachShader != NULL && ActiveTexture != NULL && BindBuffer != NULL && BindVertexArray != NULL && BufferData != NULL && BufferSubData != NULL &&
                CompileShader != NULL && CreateProgram != NULL && CreateShader != NULL && DeleteBuffers != NULL &&
                DeleteProgram != NULL && DeleteShader != NULL && DeleteVertexArrays != NULL && DrawArrays != NULL && DrawElements != NULL &&
                EnableVertexAttribArray != NULL && GenBuffers != NULL && GenVertexArrays != NULL && GetProgramiv != NULL &&
                GetShaderiv != NULL && GetUniformLocation != NULL && LinkProgram != NULL && ShaderSource != NULL &&
                UniformMatrix4fv != NULL && Uniform1i != NULL && Uniform1f != NULL && Uniform3f != NULL && UseProgram != NULL && VertexAttribPointer != NULL;
        }
    };

    ModernGlApi g_ModernGl;
#define glAttachShader g_ModernGl.AttachShader
#define glActiveTexture g_ModernGl.ActiveTexture
#define glBindBuffer g_ModernGl.BindBuffer
#define glBindVertexArray g_ModernGl.BindVertexArray
#define glBufferData g_ModernGl.BufferData
#define glBufferSubData g_ModernGl.BufferSubData
#define glCompileShader g_ModernGl.CompileShader
#define glCreateProgram g_ModernGl.CreateProgram
#define glCreateShader g_ModernGl.CreateShader
#define glDeleteBuffers g_ModernGl.DeleteBuffers
#define glDeleteProgram g_ModernGl.DeleteProgram
#define glDeleteShader g_ModernGl.DeleteShader
#define glDeleteVertexArrays g_ModernGl.DeleteVertexArrays
#define glDrawArrays g_ModernGl.DrawArrays
#define glDrawElements g_ModernGl.DrawElements
#define glEnableVertexAttribArray g_ModernGl.EnableVertexAttribArray
#define glGenBuffers g_ModernGl.GenBuffers
#define glGenVertexArrays g_ModernGl.GenVertexArrays
#define glGetProgramiv g_ModernGl.GetProgramiv
#define glGetShaderiv g_ModernGl.GetShaderiv
#define glGetUniformLocation g_ModernGl.GetUniformLocation
#define glLinkProgram g_ModernGl.LinkProgram
#define glShaderSource g_ModernGl.ShaderSource
#define glUniformMatrix4fv g_ModernGl.UniformMatrix4fv
#define glUniform1i g_ModernGl.Uniform1i
#define glUniform1f g_ModernGl.Uniform1f
#define glUniform3f g_ModernGl.Uniform3f
#define glUseProgram g_ModernGl.UseProgram
#define glVertexAttribPointer g_ModernGl.VertexAttribPointer

    struct LegacyVertex
    {
        float position[3];
        float color[4];
        float texCoord[2];
        float normal[3];
    };

    // UiDrawList e a fila de sprites 2D. Ela mantem os vertices na ordem de
    // emissao; uma troca de textura, blend, alpha/depth/fog ou matriz descarrega
    // a fila antes de o novo estado ser aceito. Isto preserva transparencias.
    struct UiDrawList
    {
        std::vector<LegacyVertex> vertices;
        bool Empty() const { return vertices.empty(); }
        void Clear() { vertices.clear(); }
    };

    static const char* VertexShaderSource =
#ifdef LEGACY_GLES_RENDERER
        "#version 300 es\nprecision highp float;\n"
#else
        "#version 330 core\n"
#endif
        "layout(location=0) in vec3 aPosition;\n"
        "layout(location=1) in vec4 aColor;\n"
        "layout(location=2) in vec2 aTexCoord;\n"
        "layout(location=3) in vec3 aNormal;\n"
        "uniform mat4 uProjection;\n"
        "uniform mat4 uModelView;\n"
        "out vec4 vColor;\n"
        "out vec2 vTexCoord;\n"
        "out float vEyeDistance;\n"
        "void main() {\n"
        "  vec4 eye = uModelView * vec4(aPosition, 1.0);\n"
        "  gl_Position = uProjection * eye;\n"
        "  vColor = aColor; vTexCoord = aTexCoord;\n"
        // Distancia no espaco de visao, base do fog linear.
        "  vEyeDistance = length(eye.xyz);\n"
        "}\n";

    static const char* FragmentShaderSource =
#ifdef LEGACY_GLES_RENDERER
        "#version 300 es\nprecision mediump float;\n"
#else
        "#version 330 core\n"
#endif
        "in vec4 vColor;\n"
        "in vec2 vTexCoord;\n"
        "in float vEyeDistance;\n"
        "uniform sampler2D uTexture; uniform int uUseTexture;\n"
        // Alpha test do pipeline fixo: glAlphaFunc(GL_GREATER, ref) vira discard.
        "uniform int uAlphaTest; uniform float uAlphaRef;\n"
        // Fog linear equivalente a GL_LINEAR: fator = (end - dist) / (end - start).
        "uniform int uFog; uniform vec3 uFogColor;\n"
        "uniform float uFogStart; uniform float uFogEnd;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "  vec4 color = (uUseTexture != 0) ? texture(uTexture, vTexCoord) * vColor : vColor;\n"
        "  if (uAlphaTest != 0 && color.a <= uAlphaRef) discard;\n"
        "  if (uFog != 0) {\n"
        "    float span = uFogEnd - uFogStart;\n"
        "    float factor = (span > 0.0) ? clamp((uFogEnd - vEyeDistance) / span, 0.0, 1.0) : 1.0;\n"
        "    color.rgb = mix(uFogColor, color.rgb, factor);\n"
        "  }\n"
        "  fragColor = color;\n"
        "}\n";

    class GlslLegacyRenderAdapter : public Platform::ILegacyRenderAdapter
    {
    public:
        GlslLegacyRenderAdapter()
            : m_primitive(Platform::LegacyPrimitiveQuads), m_uiDrawList(), m_vertices(m_uiDrawList.vertices), m_program(0), m_vertexBuffer(0), m_vertexBufferCapacity(0), m_quadIndexBuffer(0), m_quadIndexCapacity(0), m_vertexArray(0),
              m_projectionLocation(-1), m_modelViewLocation(-1), m_matricesSet(false),
              m_alphaTestLocation(-1), m_alphaRefLocation(-1), m_alphaTestReference(0.25f),
              m_fogLocation(-1), m_fogColorLocation(-1), m_fogStartLocation(-1), m_fogEndLocation(-1),
              m_fogEnabled(false), m_fogStart(0.f), m_fogEnd(1.f),
              m_alphaTestEnabled(false), m_texture2DEnabled(false), m_depthTestEnabled(false), m_texture(0), m_batching(false), m_failed(false), m_logged(false)
        {
            SetIdentity(m_projection);
            SetIdentity(m_modelView);
            SetColor(1.f, 1.f, 1.f, 1.f);
            m_fogColor[0] = m_fogColor[1] = m_fogColor[2] = 0.f;
            m_current.texCoord[0] = 0.f;
            m_current.texCoord[1] = 0.f;
            m_current.normal[0] = 0.f;
            m_current.normal[1] = 0.f;
            m_current.normal[2] = 1.f;
            InvalidarCacheDeEstado();
        }

        // Esquece o que foi enviado ao GL.
        //
        // Obrigatorio depois de perder o contexto (todo estado volta ao padrao) e no
        // construtor. Sem isto, o cache diria "ja enviei" para um contexto novo, e a
        // cena sairia com matriz e uniformes de lixo.
        void InvalidarCacheDeEstado()
        {
            m_programaAtivo = false;
            m_vaoAtivo = false;
            m_matrizProjEnviada = false;
            m_matrizMvEnviada = false;
            // Valores impossiveis, para o primeiro envio nunca ser considerado igual.
            m_usarTexturaEnviado = -1;
            m_alphaTestEnviado = -1;
            m_fogEnviado = -1;
            m_alphaRefEnviado = -1.f;
            m_fogStartEnviado = -1e30f;
            m_fogEndEnviado = -1e30f;
            m_fogColorEnviada[0] = m_fogColorEnviada[1] = m_fogColorEnviada[2] = -1.f;
            m_texturaEnviadaConhecida = false;
            m_depthTestConhecido = false;
            m_blendModeEnviado = -1;
        }

        virtual ~GlslLegacyRenderAdapter()
        {
            if (m_vertexArray != 0) glDeleteVertexArrays(1, &m_vertexArray);
            if (m_vertexBuffer != 0) glDeleteBuffers(1, &m_vertexBuffer);
            if (m_quadIndexBuffer != 0) glDeleteBuffers(1, &m_quadIndexBuffer);
            if (m_program != 0) glDeleteProgram(m_program);
        }

        virtual void Begin(Platform::LegacyPrimitive primitive)
        {
            if (!m_vertices.empty())
            {
                if (m_batching && IsBatchablePrimitive(primitive) && m_primitive == primitive)
                    return;
                Flush();
            }
            m_primitive = primitive;
            m_vertices.clear();
        }

        virtual void End()
        {
            if (m_batching && IsBatchablePrimitive(m_primitive))
                return;
            Flush();
        }

        virtual void BeginBatch() { m_batching = true; }
        virtual void EndBatch()
        {
            Flush();
            m_batching = false;
        }
        virtual void FlushBatch() { FlushPendingBatch(); }

        void Flush()
        {
            if (m_vertices.empty() || !EnsureResources()) return;
            std::vector<LegacyVertex> drawVertices;
            const bool indexedQuads = (m_primitive == Platform::LegacyPrimitiveQuads);
            if (indexedQuads)
            {
                // Quads compartilham os quatro vertices entre os dois triangulos.
                // Ignora a cauda incompleta, igual ao GL_QUADS legado.
                const size_t quadCount = m_vertices.size() / 4;
                drawVertices.assign(m_vertices.begin(), m_vertices.begin() + quadCount * 4);
                EnsureQuadIndexCapacity(quadCount);
            }
            else
            {
                BuildDrawVertices(drawVertices);
            }

            // Um Begin(GL_QUADS) incompleto nao gera primitiva. Depois da
            // conversao para triangulos ele tambem pode produzir vetor vazio;
            // nunca passe &drawVertices[0] nesse caso.
            if (drawVertices.empty())
            {
                m_vertices.clear();
                return;
            }

            // ESTADO REDUNDANTE NAO E REENVIADO.
            //
            // A versao anterior fazia 16 chamadas de estado por Begin/End e, no fim,
            // desligava programa, VAO e buffer -- obrigando a religar tudo no draw
            // seguinte. Como cada draw do legado e um grupo pequeno de primitivas, o
            // custo era quase todo em travessias JS<->wasm, nao em desenhar.
            //
            // Medido com o perfil de CPU do Chrome, na cena de login:
            //
            //   32,6% uniformMatrix4fv   15,4% uniform1i    12,8% uniform1f
            //    9,3% bufferData          6,0% useProgram    5,6% uniform3f
            //    4,2% bindVertexArray     2,3% bindBuffer    1,9% activeTexture
            //    1,2% drawArrays   <-- o desenho em si era irrelevante
            //
            // ou seja ~90% do tempo de CPU era ajuste de estado. Cada bloco abaixo so
            // chama o GL quando o valor muda de fato.
            //
            // A textura continua sendo religada sempre, de proposito: o codigo legado
            // tambem chama glBindTexture direto em alguns caminhos (ver o comentario em
            // ZzzOpenglUtil.cpp sobre a dessincronizacao), entao um cache aqui ficaria
            // velho sem aviso. Custa ~2%, e nao vale o risco.
            if (!m_programaAtivo)
            {
                glUseProgram(m_program);
                m_programaAtivo = true;
                ++m_frameStats.programChanges;
                // O sampler aponta para a unidade 0 e nunca muda.
                glUniform1i(m_textureLocation, 0);
            }

            if (!m_matrizProjEnviada || memcmp(m_projEnviada, m_projection, sizeof(m_projEnviada)) != 0)
            {
                glUniformMatrix4fv(m_projectionLocation, 1, GL_FALSE, m_projection);
                memcpy(m_projEnviada, m_projection, sizeof(m_projEnviada));
                m_matrizProjEnviada = true;
            }
            if (!m_matrizMvEnviada || memcmp(m_mvEnviada, m_modelView, sizeof(m_mvEnviada)) != 0)
            {
                glUniformMatrix4fv(m_modelViewLocation, 1, GL_FALSE, m_modelView);
                memcpy(m_mvEnviada, m_modelView, sizeof(m_mvEnviada));
                m_matrizMvEnviada = true;
            }

            const int usarTextura = (m_texture2DEnabled && m_texture != 0) ? 1 : 0;
            if (usarTextura != m_usarTexturaEnviado)
            {
                glUniform1i(m_useTextureLocation, usarTextura);
                m_usarTexturaEnviado = usarTextura;
            }

            const int alphaTest = m_alphaTestEnabled ? 1 : 0;
            if (alphaTest != m_alphaTestEnviado)
            {
                glUniform1i(m_alphaTestLocation, alphaTest);
                m_alphaTestEnviado = alphaTest;
                ++m_frameStats.alphaTestChanges;
            }
            if (m_alphaTestReference != m_alphaRefEnviado)
            {
                glUniform1f(m_alphaRefLocation, m_alphaTestReference);
                m_alphaRefEnviado = m_alphaTestReference;
            }

            const int fog = m_fogEnabled ? 1 : 0;
            if (fog != m_fogEnviado)
            {
                glUniform1i(m_fogLocation, fog);
                m_fogEnviado = fog;
                ++m_frameStats.fogChanges;
            }
            if (memcmp(m_fogColorEnviada, m_fogColor, sizeof(m_fogColorEnviada)) != 0)
            {
                glUniform3f(m_fogColorLocation, m_fogColor[0], m_fogColor[1], m_fogColor[2]);
                memcpy(m_fogColorEnviada, m_fogColor, sizeof(m_fogColorEnviada));
            }
            if (m_fogStart != m_fogStartEnviado)
            {
                glUniform1f(m_fogStartLocation, m_fogStart);
                m_fogStartEnviado = m_fogStart;
            }
            if (m_fogEnd != m_fogEndEnviado)
            {
                glUniform1f(m_fogEndLocation, m_fogEnd);
                m_fogEndEnviado = m_fogEnd;
            }

            if (!m_texturaEnviadaConhecida || m_texturaEnviada != m_texture)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_texture);
                m_texturaEnviada = m_texture;
                m_texturaEnviadaConhecida = true;
            }

            if (!m_vaoAtivo)
            {
                glBindVertexArray(m_vertexArray);
                glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
                m_vaoAtivo = true;
            }
            const GLsizeiptr uploadBytes = static_cast<GLsizeiptr>(drawVertices.size() * sizeof(LegacyVertex));
            EnsureVertexBufferCapacity(uploadBytes);
            glBufferSubData(GL_ARRAY_BUFFER, 0, uploadBytes, &drawVertices[0]);
            if (indexedQuads)
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>((drawVertices.size() / 4) * 6), GL_UNSIGNED_INT, NULL);
            else
                glDrawArrays(ToGlPrimitive(), 0, static_cast<GLsizei>(drawVertices.size()));
            ++m_frameStats.drawCalls;
            m_frameStats.vertices += static_cast<unsigned long long>(drawVertices.size());
            m_frameStats.vertexUploadBytes += static_cast<unsigned long long>(uploadBytes);
            // Nada e desligado aqui: o VAO e o buffer sao nossos e o programa e unico.
            // Desligar so criava trabalho para o draw seguinte.

            // Um relatorio na primeira emissao: se a tela fica so com a cor de
            // limpeza, isto diz se o draw sequer aconteceu e com que estado.
            if (!m_logged)
            {
                m_logged = true;
                char report[256];
                snprintf(report, sizeof(report),
                    "primeiro draw: prim=%d verts=%d locProj=%d locMV=%d erro=0x%04X",
                    (int)m_primitive, (int)drawVertices.size(),
                    m_projectionLocation, m_modelViewLocation, ::glGetError());
                Platform::LegacyRenderLog(report);
            }
            m_vertices.clear();
        }

        virtual void Color4f(float red, float green, float blue, float alpha) { SetColor(red, green, blue, alpha); }
        virtual void TexCoord2f(float u, float v) { m_current.texCoord[0] = u; m_current.texCoord[1] = v; }
        virtual void Normal3f(float x, float y, float z) { m_current.normal[0] = x; m_current.normal[1] = y; m_current.normal[2] = z; }
        virtual void Vertex3f(float x, float y, float z)
        {
            m_current.position[0] = x; m_current.position[1] = y; m_current.position[2] = z;
            m_vertices.push_back(m_current);
        }
        virtual void Vertex3fv(const float* vertex) { Vertex3f(vertex[0], vertex[1], vertex[2]); }
        virtual void SetMatrices(const float* projection, const float* modelView)
        {
            FlushPendingBatch();
            if (projection != NULL) memcpy(m_projection, projection, sizeof(m_projection));
            if (modelView != NULL) memcpy(m_modelView, modelView, sizeof(m_modelView));
            m_matricesSet = true;
        }
        virtual void SetDepthTest(bool enabled)
        {
            if (m_depthTestConhecido && m_depthTestEnabled == enabled)
                return;
            FlushPendingBatch();
            enabled ? ::glEnable(GL_DEPTH_TEST) : ::glDisable(GL_DEPTH_TEST);
            m_depthTestEnabled = enabled;
            m_depthTestConhecido = true;
            ++m_frameStats.depthStateChanges;
        }
        virtual void SetAlphaTest(bool enabled)
        {
            if (m_alphaTestEnabled != enabled) FlushPendingBatch();
            m_alphaTestEnabled = enabled;
        }
        virtual void SetAlphaTestRef(float reference)
        {
            if (m_alphaTestReference != reference) FlushPendingBatch();
            m_alphaTestReference = reference;
        }
        virtual void SetFog(bool enabled, const float* color, float start, float end)
        {
            if (m_fogEnabled != enabled || m_fogStart != start || m_fogEnd != end ||
                (color != NULL && memcmp(m_fogColor, color, sizeof(m_fogColor)) != 0))
                FlushPendingBatch();
            m_fogEnabled = enabled;
            if (color != NULL)
            {
                m_fogColor[0] = color[0];
                m_fogColor[1] = color[1];
                m_fogColor[2] = color[2];
            }
            m_fogStart = start;
            m_fogEnd = end;
        }
        virtual void SetTexture2D(bool enabled)
        {
            if (m_texture2DEnabled != enabled) FlushPendingBatch();
            m_texture2DEnabled = enabled;
        }
        virtual void BindTexture(unsigned int texture)
        {
            const GLuint glTexture = static_cast<GLuint>(texture);
            if (m_texture != glTexture) FlushPendingBatch();
            if (m_texture != glTexture)
                ++m_frameStats.textureChanges;
            m_texture = glTexture;
        }
        virtual void SetBlendMode(int mode)
        {
            if (m_blendModeEnviado == mode)
                return;
            FlushPendingBatch();
            m_blendModeEnviado = mode;
            ++m_frameStats.blendStateChanges;
        }
        virtual void ResetFrameStats() { m_frameStats = Platform::LegacyRenderFrameStats(); }
        virtual Platform::LegacyRenderFrameStats GetFrameStats() const { return m_frameStats; }
        virtual void InvalidateStateCache()
        {
            FlushPendingBatch();
            InvalidarCacheDeEstado();
        }

        // Chamado quando o contexto grafico foi destruido: os nomes de objeto ja
        // nao valem nada, entao sao esquecidos (nao deletados) para que
        // EnsureResources reconstrua tudo no contexto novo.
        virtual void InvalidateGraphicsResources()
        {
            m_program = 0;
            m_vertexBuffer = 0;
            m_vertexBufferCapacity = 0;
            m_quadIndexBuffer = 0;
            m_quadIndexCapacity = 0;
            m_vertexArray = 0;
            m_projectionLocation = -1;
            m_modelViewLocation = -1;
            m_textureLocation = -1;
            m_useTextureLocation = -1;
            m_depthTestEnabled = false;
            m_texture = 0;
            m_failed = false;
            m_logged = false;
            m_batching = false;
            m_vertices.clear();
            // Sem isto o cache afirmaria que uniformes e matrizes ja estao no GL, e o
            // contexto novo comecaria com estado indefinido.
            InvalidarCacheDeEstado();
            Platform::LegacyRenderLog("GlslLegacyRenderAdapter: recursos invalidados apos recriacao de contexto");
        }

    private:
        // Quads e triangulos sao independentes entre chamadas Begin/End. Outros
        // primitivos (fans, linhas e strips) dependem da topologia da chamada e
        // continuam imediatos para preservar exatamente o resultado legado.
        static bool IsBatchablePrimitive(Platform::LegacyPrimitive primitive)
        {
            return primitive == Platform::LegacyPrimitiveQuads ||
                primitive == Platform::LegacyPrimitiveTriangles;
        }
        void FlushPendingBatch()
        {
            if (m_batching && !m_vertices.empty())
                Flush();
        }
        static void SetIdentity(float* matrix)
        {
            memset(matrix, 0, sizeof(float) * 16);
            matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.f;
        }
        // A cor de vertice e SATURADA em [0,1], como no pipeline fixo.
        //
        // POR QUE: `glColor*f` guarda a cor em ponto fixo, e a especificacao manda
        // saturar o valor no caminho. O cliente conta com isso: a iluminacao de modelo
        // (BodyLight * IntensityTransform, em BMD::RenderMesh) produz valores MUITO
        // acima de 1 -- medido 11.12 no ceu da cena de login -- e no PC eles chegam ao
        // rasterizador como 1.0.
        //
        // Sem saturar, o shader multiplicava a textura por 11.12: o ceu (sky.jpg, media
        // 21,16,13) saturava para BRANCO no framebuffer, e a cena inteira ficava lavada.
        // Era um caso em que nada acusava erro -- decodificacao, upload, nome de textura
        // e uniformes todos corretos, e a conta do fragmento e que estava fora de faixa.
        static float Saturar(float valor)
        {
            if (valor < 0.f) return 0.f;
            if (valor > 1.f) return 1.f;
            return valor;
        }
        void SetColor(float red, float green, float blue, float alpha)
        {
            m_current.color[0] = Saturar(red);   m_current.color[1] = Saturar(green);
            m_current.color[2] = Saturar(blue);  m_current.color[3] = Saturar(alpha);
        }
        void EnsureVertexBufferCapacity(GLsizeiptr requiredBytes)
        {
            if (requiredBytes <= m_vertexBufferCapacity)
                return;

            // Crescimento geometrico evita realocar o VBO para cada draw. WebGL
            // reutiliza este armazenamento via BufferSubData; no desktop e o mesmo
            // caminho seguro antes de introduzir um ring buffer persistente.
            GLsizeiptr newCapacity = (m_vertexBufferCapacity > 0) ? m_vertexBufferCapacity : 4096;
            while (newCapacity < requiredBytes)
                newCapacity *= 2;
            glBufferData(GL_ARRAY_BUFFER, newCapacity, NULL, GL_STREAM_DRAW);
            m_vertexBufferCapacity = newCapacity;
        }
        void EnsureQuadIndexCapacity(size_t requiredQuads)
        {
            if (requiredQuads <= m_quadIndexCapacity)
                return;

            size_t newCapacity = (m_quadIndexCapacity > 0) ? m_quadIndexCapacity : 256;
            while (newCapacity < requiredQuads)
                newCapacity *= 2;

            std::vector<GLuint> indices(newCapacity * 6);
            for (size_t quad = 0; quad < newCapacity; ++quad)
            {
                const GLuint base = static_cast<GLuint>(quad * 4);
                const size_t index = quad * 6;
                indices[index] = base;
                indices[index + 1] = base + 1;
                indices[index + 2] = base + 2;
                indices[index + 3] = base;
                indices[index + 4] = base + 2;
                indices[index + 5] = base + 3;
            }
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadIndexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)), &indices[0], GL_STATIC_DRAW);
            m_quadIndexCapacity = newCapacity;
        }
        GLenum ToGlPrimitive() const
        {
            switch (m_primitive)
            {
            case Platform::LegacyPrimitiveLineStrip: return GL_LINE_STRIP;
            case Platform::LegacyPrimitiveTriangles: return GL_TRIANGLES;
            case Platform::LegacyPrimitiveTriangleFan: return GL_TRIANGLE_FAN;
            case Platform::LegacyPrimitiveLines: return GL_LINES;
            case Platform::LegacyPrimitiveQuads: default: return GL_TRIANGLES;
            }
        }
        void BuildDrawVertices(std::vector<LegacyVertex>& output) const
        {
            if (m_primitive != Platform::LegacyPrimitiveQuads)
            {
                output = m_vertices;
                return;
            }
            for (size_t i = 0; i + 3 < m_vertices.size(); i += 4)
            {
                output.push_back(m_vertices[i]);
                output.push_back(m_vertices[i + 1]);
                output.push_back(m_vertices[i + 2]);
                output.push_back(m_vertices[i]);
                output.push_back(m_vertices[i + 2]);
                output.push_back(m_vertices[i + 3]);
            }
        }
        GLuint Compile(GLenum type, const char* source)
        {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, NULL);
            glCompileShader(shader);
            GLint compiled = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled)
            {
                char info[1024];
                info[0] = '\0';
                if (g_ModernGl.GetShaderInfoLog != NULL)
                    g_ModernGl.GetShaderInfoLog(shader, sizeof(info), NULL, info);
                Platform::LegacyRenderLog(type == GL_VERTEX_SHADER
                    ? "GlslLegacyRenderAdapter: falha ao compilar o vertex shader"
                    : "GlslLegacyRenderAdapter: falha ao compilar o fragment shader");
                Platform::LegacyRenderLog(info);
                glDeleteShader(shader);
                return 0;
            }
            return shader;
        }
        bool EnsureResources()
        {
            if (m_failed) return false;
            if (m_program != 0) return true;
            // Daqui para baixo, programa/VAO/buffer serao criados de novo: o cache de
            // estado descreve objetos que nao existem mais.
            InvalidarCacheDeEstado();
            if (!g_ModernGl.Load())
            {
                Platform::LegacyRenderLog("GlslLegacyRenderAdapter: falha ao carregar as funcoes GL modernas");
                Platform::LegacyRenderLog(g_ModernGl.MissingName());
                m_failed = true;
                return false;
            }
            GLuint vertexShader = Compile(GL_VERTEX_SHADER, VertexShaderSource);
            GLuint fragmentShader = Compile(GL_FRAGMENT_SHADER, FragmentShaderSource);
            if (vertexShader == 0 || fragmentShader == 0) { m_failed = true; return false; }
            m_program = glCreateProgram();
            glAttachShader(m_program, vertexShader);
            glAttachShader(m_program, fragmentShader);
            glLinkProgram(m_program);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            GLint linked = GL_FALSE;
            glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
            if (!linked)
            {
                char info[1024];
                info[0] = '\0';
                if (g_ModernGl.GetProgramInfoLog != NULL)
                    g_ModernGl.GetProgramInfoLog(m_program, sizeof(info), NULL, info);
                Platform::LegacyRenderLog("GlslLegacyRenderAdapter: falha ao linkar o programa");
                Platform::LegacyRenderLog(info);
                glDeleteProgram(m_program); m_program = 0; m_failed = true; return false;
            }
            Platform::LegacyRenderLog("GlslLegacyRenderAdapter: programa GLSL pronto");
            m_projectionLocation = glGetUniformLocation(m_program, "uProjection");
            m_modelViewLocation = glGetUniformLocation(m_program, "uModelView");
            m_textureLocation = glGetUniformLocation(m_program, "uTexture");
            m_useTextureLocation = glGetUniformLocation(m_program, "uUseTexture");
            m_alphaTestLocation = glGetUniformLocation(m_program, "uAlphaTest");
            m_alphaRefLocation = glGetUniformLocation(m_program, "uAlphaRef");
            m_fogLocation = glGetUniformLocation(m_program, "uFog");
            m_fogColorLocation = glGetUniformLocation(m_program, "uFogColor");
            m_fogStartLocation = glGetUniformLocation(m_program, "uFogStart");
            m_fogEndLocation = glGetUniformLocation(m_program, "uFogEnd");
            glGenVertexArrays(1, &m_vertexArray);
            glGenBuffers(1, &m_vertexBuffer);
            glGenBuffers(1, &m_quadIndexBuffer);
            glBindVertexArray(m_vertexArray);
            glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadIndexBuffer);
            glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LegacyVertex), (void*)offsetof(LegacyVertex, position));
            glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LegacyVertex), (void*)offsetof(LegacyVertex, color));
            glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(LegacyVertex), (void*)offsetof(LegacyVertex, texCoord));
            glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(LegacyVertex), (void*)offsetof(LegacyVertex, normal));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            return true;
        }

        Platform::LegacyPrimitive m_primitive;
        UiDrawList m_uiDrawList;
        // Alias para o buffer de vertices da fila; mantem o restante do
        // adaptador agnostico da representacao da UiDrawList.
        std::vector<LegacyVertex>& m_vertices;
        LegacyVertex m_current;
        GLuint m_program;
        GLuint m_vertexBuffer;
        GLsizeiptr m_vertexBufferCapacity;
        GLuint m_quadIndexBuffer;
        size_t m_quadIndexCapacity;
        GLuint m_vertexArray;
        GLint m_projectionLocation;
        GLint m_modelViewLocation;
        GLint m_textureLocation;
        GLint m_useTextureLocation;
        GLint m_alphaTestLocation;
        GLint m_alphaRefLocation;
        float m_alphaTestReference;
        GLint m_fogLocation;
        GLint m_fogColorLocation;
        GLint m_fogStartLocation;
        GLint m_fogEndLocation;
        bool  m_fogEnabled;
        float m_fogColor[3];
        float m_fogStart;
        float m_fogEnd;
        float m_projection[16];
        float m_modelView[16];
        bool m_matricesSet;
        bool m_alphaTestEnabled;
        bool m_texture2DEnabled;
        bool m_depthTestEnabled;
        bool m_depthTestConhecido;
        GLuint m_texture;
        bool m_batching;
        bool m_failed;
        bool m_logged;

        // Espelho do que ja foi enviado ao GL, para nao reenviar o que nao mudou.
        // Ver o comentario em End() com o perfil que motivou isto.
        bool  m_programaAtivo;
        bool  m_vaoAtivo;
        bool  m_matrizProjEnviada;
        bool  m_matrizMvEnviada;
        float m_projEnviada[16];
        float m_mvEnviada[16];
        int   m_usarTexturaEnviado;
        int   m_alphaTestEnviado;
        int   m_fogEnviado;
        float m_alphaRefEnviado;
        float m_fogColorEnviada[3];
        float m_fogStartEnviado;
        float m_fogEndEnviado;
        bool m_texturaEnviadaConhecida;
        GLuint m_texturaEnviada;
        int m_blendModeEnviado;
        Platform::LegacyRenderFrameStats m_frameStats;
    };
}

namespace Platform
{
    ILegacyRenderAdapter* CreateGlslLegacyRenderAdapter() { return new GlslLegacyRenderAdapter(); }
}
