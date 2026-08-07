#include "stdafx.h"
#include "LegacyRenderAdapter.h"

#include <map>

#ifdef glAttachShader
#undef glAttachShader
#undef glActiveTexture
#undef glBindBuffer
#undef glBindBufferBase
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
#undef glGetUniformBlockIndex
#undef glLinkProgram
#undef glShaderSource
#undef glUniformMatrix4fv
#undef glUniform1i
#undef glUniform1f
#undef glUniform3f
#undef glUniformBlockBinding
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
        PFNGLBINDBUFFERBASEPROC BindBufferBase;
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
        PFNGLGETUNIFORMBLOCKINDEXPROC GetUniformBlockIndex;
        PFNGLLINKPROGRAMPROC LinkProgram;
        PFNGLSHADERSOURCEPROC ShaderSource;
        PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv;
        PFNGLUNIFORM1IPROC Uniform1i;
        PFNGLUNIFORM1FPROC Uniform1f;
        PFNGLUNIFORM3FPROC Uniform3f;
        PFNGLUNIFORMBLOCKBINDINGPROC UniformBlockBinding;
        PFNGLUSEPROGRAMPROC UseProgram;
        PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;
        PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;
        PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog;
        // Instancing. Ambas sao core em GL 3.3 e em GLES 3.0/WebGL 2, mas ficam
        // fora da checagem obrigatoria: se faltarem, o adapter apenas nao oferece
        // o caminho instanciado, em vez de desistir de desenhar.
        PFNGLVERTEXATTRIBDIVISORPROC VertexAttribDivisor;
        PFNGLDRAWELEMENTSINSTANCEDPROC DrawElementsInstanced;
        PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray;

        bool HasInstancing() const
        {
            return VertexAttribDivisor != NULL && DrawElementsInstanced != NULL &&
                DisableVertexAttribArray != NULL;
        }

        // Nome da primeira funcao que nao pode ser resolvida, para diagnostico.
        const char* MissingName() const
        {
#define CHECK_GL(name) if (name == NULL) return "gl" #name " nao resolvida";
            CHECK_GL(AttachShader); CHECK_GL(ActiveTexture); CHECK_GL(BindBuffer); CHECK_GL(BindBufferBase); CHECK_GL(BindVertexArray); CHECK_GL(BufferData); CHECK_GL(BufferSubData);
            CHECK_GL(CompileShader); CHECK_GL(CreateProgram); CHECK_GL(CreateShader); CHECK_GL(DeleteBuffers);
            CHECK_GL(DeleteProgram); CHECK_GL(DeleteShader); CHECK_GL(DeleteVertexArrays); CHECK_GL(DrawArrays); CHECK_GL(DrawElements);
            CHECK_GL(EnableVertexAttribArray); CHECK_GL(GenBuffers); CHECK_GL(GenVertexArrays); CHECK_GL(GetProgramiv);
            CHECK_GL(GetShaderiv); CHECK_GL(GetUniformLocation); CHECK_GL(GetUniformBlockIndex); CHECK_GL(LinkProgram); CHECK_GL(ShaderSource);
            CHECK_GL(UniformMatrix4fv); CHECK_GL(Uniform1i); CHECK_GL(UniformBlockBinding); CHECK_GL(UseProgram); CHECK_GL(VertexAttribPointer);
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
            LOAD_GL(AttachShader); LOAD_GL(ActiveTexture); LOAD_GL(BindBuffer); LOAD_GL(BindBufferBase); LOAD_GL(BindVertexArray); LOAD_GL(BufferData); LOAD_GL(BufferSubData);
            LOAD_GL(CompileShader); LOAD_GL(CreateProgram); LOAD_GL(CreateShader); LOAD_GL(DeleteBuffers);
            LOAD_GL(DeleteProgram); LOAD_GL(DeleteShader); LOAD_GL(DeleteVertexArrays); LOAD_GL(DrawArrays); LOAD_GL(DrawElements);
            LOAD_GL(EnableVertexAttribArray); LOAD_GL(GenBuffers); LOAD_GL(GenVertexArrays); LOAD_GL(GetProgramiv);
            LOAD_GL(GetShaderiv); LOAD_GL(GetUniformLocation); LOAD_GL(GetUniformBlockIndex); LOAD_GL(LinkProgram); LOAD_GL(ShaderSource);
            LOAD_GL(UniformMatrix4fv); LOAD_GL(UseProgram); LOAD_GL(VertexAttribPointer);
            LOAD_GL(Uniform1i); LOAD_GL(Uniform1f); LOAD_GL(Uniform3f); LOAD_GL(UniformBlockBinding);
            LOAD_GL(GetShaderInfoLog); LOAD_GL(GetProgramInfoLog);
            LOAD_GL(VertexAttribDivisor); LOAD_GL(DrawElementsInstanced); LOAD_GL(DisableVertexAttribArray);
#undef LOAD_GL
            return AttachShader != NULL && ActiveTexture != NULL && BindBuffer != NULL && BindBufferBase != NULL && BindVertexArray != NULL && BufferData != NULL && BufferSubData != NULL &&
                CompileShader != NULL && CreateProgram != NULL && CreateShader != NULL && DeleteBuffers != NULL &&
                DeleteProgram != NULL && DeleteShader != NULL && DeleteVertexArrays != NULL && DrawArrays != NULL && DrawElements != NULL &&
                EnableVertexAttribArray != NULL && GenBuffers != NULL && GenVertexArrays != NULL && GetProgramiv != NULL &&
                GetShaderiv != NULL && GetUniformLocation != NULL && GetUniformBlockIndex != NULL && LinkProgram != NULL && ShaderSource != NULL &&
                UniformMatrix4fv != NULL && Uniform1i != NULL && Uniform1f != NULL && Uniform3f != NULL && UniformBlockBinding != NULL && UseProgram != NULL && VertexAttribPointer != NULL;
        }
    };

    ModernGlApi g_ModernGl;
#define glAttachShader g_ModernGl.AttachShader
#define glActiveTexture g_ModernGl.ActiveTexture
#define glBindBuffer g_ModernGl.BindBuffer
#define glBindBufferBase g_ModernGl.BindBufferBase
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
#define glGetUniformBlockIndex g_ModernGl.GetUniformBlockIndex
#define glLinkProgram g_ModernGl.LinkProgram
#define glShaderSource g_ModernGl.ShaderSource
#define glUniformMatrix4fv g_ModernGl.UniformMatrix4fv
#define glUniform1i g_ModernGl.Uniform1i
#define glUniform1f g_ModernGl.Uniform1f
#define glUniform3f g_ModernGl.Uniform3f
#define glUniformBlockBinding g_ModernGl.UniformBlockBinding
#define glUseProgram g_ModernGl.UseProgram
#define glVertexAttribPointer g_ModernGl.VertexAttribPointer
#define glVertexAttribDivisor g_ModernGl.VertexAttribDivisor
#define glDrawElementsInstanced g_ModernGl.DrawElementsInstanced
#define glDisableVertexAttribArray g_ModernGl.DisableVertexAttribArray

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
        "layout(location=4) in float aPositionBone;\n"
        "layout(location=5) in float aNormalBone;\n"
        "layout(location=6) in float aWaveSeed;\n"
        // Atributos por instancia (divisor 1). Sao lidos apenas quando
        // uInstanced != 0; no caminho de instancia unica os uniformes valem.
        "layout(location=7) in vec4 iColor;\n"
        "layout(location=8) in vec4 iPostTransScale;\n"
        "layout(location=9) in vec4 iLightPosLighting;\n"
        "layout(location=10) in vec4 iBodyOriginBoneScale;\n"
        // x = linha da paleta, y = materialEffect, z = bits (1 wave, 2 shadowMap,
        // 4 skinning), w = reservado.
        "layout(location=11) in vec4 iParams;\n"
        "uniform int uInstanced;\n"
        // Paleta de ossos das instancias: RGBA32F, 3 texels por osso, uma linha
        // por instancia. Textura em vez de UBO porque o limite de 64 KB da UBO
        // daria ~6 instancias, e nem SSBO nem samplerBuffer existem no WebGL 2.
        "uniform sampler2D uBonePalette;\n"
        "uniform mat4 uProjection; uniform mat4 uModel;\n"
        "uniform vec3 uDrawColor;\n"
        "uniform int uSkinning;\n"
        "uniform int uLighting; uniform vec3 uLightPosition;\n"
        "uniform float uBodyScale;\n"
        "uniform vec3 uPostTranslation;\n"
        "uniform int uWave; uniform float uWorldTime;\n"
        "uniform int uMaterialEffect;\n"
        "uniform int uShadowMap; uniform vec3 uBodyOrigin;\n"
        "uniform float uBoneScale;\n"
        "layout(std140) uniform BmdBones { vec4 uBoneRows[600]; };\n"
        "uniform mat4 uModelView;\n"
        "out vec4 vColor;\n"
        "out vec2 vTexCoord;\n"
        "out float vEyeDistance;\n"
        // Uma linha da paleta da instancia corrente. texelFetch nao exige
        // filtragem, entao RGBA32F amostravel basta (core em GLES 3.0).
        "vec4 InstanceBoneRow(int row) {\n"
        "  return texelFetch(uBonePalette, ivec2(row, int(iParams.x + 0.5)), 0);\n"
        "}\n"
        "void main() {\n"
        "  bool instanced = uInstanced != 0;\n"
        "  int instanceBits = instanced ? int(iParams.z + 0.5) : 0;\n"
        "  bool skinning = instanced ? ((instanceBits & 4) != 0) : (uSkinning != 0);\n"
        "  float boneScale = instanced ? iBodyOriginBoneScale.w : uBoneScale;\n"
        "  float bodyScale = instanced ? iPostTransScale.w : uBodyScale;\n"
        "  vec3 postTranslation = instanced ? iPostTransScale.xyz : uPostTranslation;\n"
        "  vec3 bodyOrigin = instanced ? iBodyOriginBoneScale.xyz : uBodyOrigin;\n"
        "  bool wave = instanced ? ((instanceBits & 1) != 0) : (uWave != 0);\n"
        "  bool shadowMap = instanced ? ((instanceBits & 2) != 0) : (uShadowMap != 0);\n"
        "  int materialEffect = instanced ? int(iParams.y + 0.5) : uMaterialEffect;\n"
        "  bool lighting = instanced ? (iLightPosLighting.w != 0.0) : (uLighting != 0);\n"
        "  vec3 lightPosition = instanced ? iLightPosLighting.xyz : uLightPosition;\n"
        "  vec3 drawColor = instanced ? iColor.rgb : uDrawColor;\n"
        "  vec4 localPosition; vec3 localNormal;\n"
        "  if (skinning) {\n"
        "    int row = int(aPositionBone + 0.5) * 3;\n"
        "    vec4 p = vec4(aPosition, 1.0);\n"
        "    vec4 r0 = instanced ? InstanceBoneRow(row) : uBoneRows[row];\n"
        "    vec4 r1 = instanced ? InstanceBoneRow(row + 1) : uBoneRows[row + 1];\n"
        "    vec4 r2 = instanced ? InstanceBoneRow(row + 2) : uBoneRows[row + 2];\n"
        "    localPosition = vec4(dot(r0, p), dot(r1, p), dot(r2, p), 1.0);\n"
        "    if (boneScale != 1.0) localPosition.xyz = vec3(dot(r0.xyz, aPosition) * boneScale + r0.w, dot(r1.xyz, aPosition) * boneScale + r1.w, dot(r2.xyz, aPosition) * boneScale + r2.w);\n"
        "    int normalRow = int(aNormalBone + 0.5) * 3; vec4 n = vec4(aNormal, 0.0);\n"
        "    vec4 n0 = instanced ? InstanceBoneRow(normalRow) : uBoneRows[normalRow];\n"
        "    vec4 n1 = instanced ? InstanceBoneRow(normalRow + 1) : uBoneRows[normalRow + 1];\n"
        "    vec4 n2 = instanced ? InstanceBoneRow(normalRow + 2) : uBoneRows[normalRow + 2];\n"
        "    localNormal = vec3(dot(n0, n), dot(n1, n), dot(n2, n));\n"
        "  } else { localPosition = uModel * vec4(aPosition, 1.0); localNormal = aNormal; }\n"
        "  localPosition.xyz *= bodyScale;\n"
        "  localPosition.xyz += postTranslation;\n"
        "  if (wave) localPosition.xyz += localNormal * (sin((floor(uWorldTime) + aWaveSeed * 931.0) * 0.007) * 28.0);\n"
        "  if (shadowMap) { vec3 p = localPosition.xyz - bodyOrigin; p.x += p.z * (p.x + 2000.0) / (p.z - 4000.0); p.z = 5.0; localPosition.xyz = p + bodyOrigin; }\n"
        "  vec4 eye = uModelView * localPosition;\n"
        "  gl_Position = uProjection * eye;\n"
        "  float luminosity = lighting ? max(dot(localNormal, lightPosition) * 0.8 + 0.4, 0.2) : 1.0;\n"
        // glColor do pipeline fixo satura *depois* de combinar a cor do corpo
        // com a iluminacao por normal. Fazer isso no shader preserva o resultado
        // do cliente para BodyLight e luminosity acima de 1.0.
        "  float alpha = instanced ? iColor.a : 1.0;\n"
        "  vColor = clamp(aColor * vec4(drawColor * luminosity, alpha), 0.0, 1.0); vTexCoord = aTexCoord;\n"
        "  float waveTime = floor(uWorldTime) * 0.0001;\n"
        "  if (materialEffect == 1) vTexCoord = vec2(localNormal.z * 0.5 + waveTime, localNormal.y * 0.5 + waveTime * 2.0);\n"
        "  else if (materialEffect == 2) { float w2 = mod(floor(uWorldTime), 5000.0) * 0.00024 - 0.4; vTexCoord = vec2((localNormal.z + localNormal.x) * 0.8 + w2 * 2.0, (localNormal.y + localNormal.x) + w2 * 3.0); }\n"
        "  else if (materialEffect == 3) vTexCoord = vec2(localNormal.z * 0.5 + 0.2, localNormal.y * 0.5 + 0.5);\n"
        "  else if (materialEffect == 4) vTexCoord = vec2(localNormal.x * aTexCoord.x, localNormal.y * aTexCoord.y);\n"
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
              m_projectionLocation(-1), m_modelViewLocation(-1), m_modelLocation(-1), m_drawColorLocation(-1), m_skinningLocation(-1), m_lightingLocation(-1), m_lightPositionLocation(-1), m_bodyScaleLocation(-1), m_postTranslationLocation(-1), m_waveLocation(-1), m_worldTimeLocation(-1), m_materialEffectLocation(-1), m_shadowMapLocation(-1), m_bodyOriginLocation(-1), m_boneScaleLocation(-1), m_boneBlockIndex(GL_INVALID_INDEX), m_boneBuffer(0), m_boneBufferIndex(0), m_matricesSet(false),
              m_alphaTestLocation(-1), m_alphaRefLocation(-1), m_alphaTestReference(0.25f),
              m_fogLocation(-1), m_fogColorLocation(-1), m_fogStartLocation(-1), m_fogEndLocation(-1),
              m_fogEnabled(false), m_fogStart(0.f), m_fogEnd(1.f),
              m_alphaTestEnabled(false), m_texture2DEnabled(false), m_depthTestEnabled(false), m_texture(0), m_batching(false), m_failed(false), m_logged(false),
              m_resourceGeneration(1), m_bonePaletteTexture(0), m_bonePaletteWidth(0), m_bonePaletteHeight(0), m_instanceBuffer(0),
              m_instancedLocation(-1), m_bonePaletteLocation(-1), m_instancedEnviado(-1),
              m_instancingUnavailable(false)
        {
			// Capacidade inicial para os lotes comuns de UI/mundo. clear() preserva
			// essa memoria entre frames, evitando realocacoes no aquecimento.
            m_uiDrawList.vertices.reserve(8192);
            m_drawVertices.reserve(8192);
            m_boneRows.reserve(200 * 12);
            m_boneBuffers[0] = m_boneBuffers[1] = m_boneBuffers[2] = 0;
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
            m_modeloEnviadoConhecido = false;
            m_drawColorEnviada[0] = m_drawColorEnviada[1] = m_drawColorEnviada[2] = -1e30f;
            m_skinningEnviado = -1;
            m_lightingEnviado = -1;
            m_lightPositionEnviada[0] = m_lightPositionEnviada[1] = m_lightPositionEnviada[2] = -1e30f;
            m_bodyScaleEnviado = -1e30f;
            m_postTranslationEnviada[0] = m_postTranslationEnviada[1] = m_postTranslationEnviada[2] = -1e30f;
            m_waveEnviado = -1;
            m_worldTimeEnviado = -1e30f;
            m_materialEffectEnviado = -1;
            m_shadowMapEnviado = -1;
            m_bodyOriginEnviada[0] = m_bodyOriginEnviada[1] = m_bodyOriginEnviada[2] = -1e30f;
            m_boneScaleEnviado = -1e30f;
            m_texturaUnidadeEnviada = false;
            m_instancedEnviado = -1;
        }

        // Setters com espelho. Devolvem sem tocar no GL quando o valor ja esta la;
        // com o batching desligado, sempre enviam, para reproduzir exatamente o
        // trafego do caminho anterior.
        bool CacheDeUniformesAtivo() const
        {
            return Platform::IsRenderFeatureActive(Platform::RenderFeatureBatching);
        }
        void EnviarUniform1i(GLint location, int value, int& espelho)
        {
            if (location < 0) return;
            if (CacheDeUniformesAtivo() && espelho == value) { ++m_frameStats.uniformCallsSaved; return; }
            glUniform1i(location, value);
            espelho = value;
        }
        void EnviarUniform1f(GLint location, float value, float& espelho)
        {
            if (location < 0) return;
            if (CacheDeUniformesAtivo() && espelho == value) { ++m_frameStats.uniformCallsSaved; return; }
            glUniform1f(location, value);
            espelho = value;
        }
        void EnviarUniform3f(GLint location, float x, float y, float z, float* espelho)
        {
            if (location < 0) return;
            if (CacheDeUniformesAtivo() && espelho[0] == x && espelho[1] == y && espelho[2] == z)
            { ++m_frameStats.uniformCallsSaved; return; }
            glUniform3f(location, x, y, z);
            espelho[0] = x; espelho[1] = y; espelho[2] = z;
        }
        void EnviarUniformMatriz4(GLint location, const float* value, float* espelho, bool& conhecido)
        {
            if (location < 0) return;
            if (CacheDeUniformesAtivo() && conhecido && memcmp(espelho, value, sizeof(float) * 16) == 0)
            { ++m_frameStats.uniformCallsSaved; return; }
            glUniformMatrix4fv(location, 1, GL_FALSE, value);
            memcpy(espelho, value, sizeof(float) * 16);
            conhecido = true;
        }

        virtual ~GlslLegacyRenderAdapter()
        {
            for (size_t i = 0; i < m_staticMeshes.size(); ++i)
            {
                StaticMesh& mesh = m_staticMeshes[i];
                if (!mesh.alive) continue;
                if (mesh.vertexArray != 0) glDeleteVertexArrays(1, &mesh.vertexArray);
                if (mesh.vertexBuffer != 0) glDeleteBuffers(1, &mesh.vertexBuffer);
                if (mesh.indexBuffer != 0) glDeleteBuffers(1, &mesh.indexBuffer);
            }
            if (m_vertexArray != 0) glDeleteVertexArrays(1, &m_vertexArray);
            if (m_vertexBuffer != 0) glDeleteBuffers(1, &m_vertexBuffer);
            if (m_quadIndexBuffer != 0) glDeleteBuffers(1, &m_quadIndexBuffer);
            glDeleteBuffers(3, m_boneBuffers);
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
            // Linhas, fans e triangulos precisam de uma conversao antes do
            // desenho. Reutilizar este armazenamento evita alocar/liberar um
            // vetor temporario a cada flush desses primitivos.
            std::vector<LegacyVertex>& drawVertices = m_drawVertices;
            drawVertices.clear();
            const LegacyVertex* drawVertexData = NULL;
            size_t drawVertexCount = 0;
            const bool indexedQuads = (m_primitive == Platform::LegacyPrimitiveQuads);
            if (indexedQuads)
            {
                // Quads compartilham os quatro vertices entre os dois triangulos.
                // Ignora a cauda incompleta, igual ao GL_QUADS legado.
                const size_t quadCount = m_vertices.size() / 4;
                drawVertexCount = quadCount * 4;
                EnsureQuadIndexCapacity(quadCount);
                if (drawVertexCount > 0)
                    drawVertexData = &m_vertices[0];
            }
            else
            {
                BuildDrawVertices(drawVertices);
                drawVertexCount = drawVertices.size();
                if (drawVertexCount > 0)
                    drawVertexData = &drawVertices[0];
            }

            // Um Begin(GL_QUADS) incompleto nao gera primitiva. Depois da
            // conversao para triangulos ele tambem pode produzir vetor vazio.
            if (drawVertexCount == 0)
            {
                m_vertices.clear();
                return;
            }

            ++m_frameStats.batchFlushes;

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
            // Volta ao neutro para o caminho imediato. Sem espelho isto custava 8
            // chamadas por lote de UI, mesmo quando nenhum DrawStaticMesh havia
            // mexido em nada desde o lote anterior.
            static const float identity[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
            EnviarUniformMatriz4(m_modelLocation, identity, m_modeloEnviado, m_modeloEnviadoConhecido);
            EnviarUniform3f(m_drawColorLocation, 1.f, 1.f, 1.f, m_drawColorEnviada);
            EnviarUniform1i(m_skinningLocation, 0, m_skinningEnviado);
            EnviarUniform1i(m_lightingLocation, 0, m_lightingEnviado);
            EnviarUniform1f(m_bodyScaleLocation, 1.f, m_bodyScaleEnviado);
            EnviarUniform3f(m_postTranslationLocation, 0.f, 0.f, 0.f, m_postTranslationEnviada);
            EnviarUniform1i(m_waveLocation, 0, m_waveEnviado);
            EnviarUniform1i(m_materialEffectLocation, 0, m_materialEffectEnviado);
            // uShadowMap e uBoneScale nao eram zerados aqui. Com skinning=0 o
            // shader ignora boneScale, mas shadowMap deforma a posicao mesmo sem
            // skinning: deixa-lo ligado de um DrawStaticMesh anterior achataria a
            // geometria imediata seguinte.
            EnviarUniform1i(m_shadowMapLocation, 0, m_shadowMapEnviado);
            // OBRIGATORIO. Um atributo de vertice DESABILITADO le (0,0,0,1), nao
            // lixo: com uInstanced preso em 1 apos um draw instanciado, o caminho
            // imediato passaria a tirar a cor de iColor.rgb, que vale (0,0,0), e
            // desenharia UI, sprites e terreno pretos.
            EnviarUniform1i(m_instancedLocation, 0, m_instancedEnviado);

            if (!m_vaoAtivo)
            {
                glBindVertexArray(m_vertexArray);
                glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
                m_vaoAtivo = true;
            }
            const GLsizeiptr uploadBytes = static_cast<GLsizeiptr>(drawVertexCount * sizeof(LegacyVertex));
            EnsureVertexBufferCapacity(uploadBytes);
            glBufferSubData(GL_ARRAY_BUFFER, 0, uploadBytes, drawVertexData);
            ++m_frameStats.bufferSubDataCalls;
            if (indexedQuads)
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>((drawVertexCount / 4) * 6), GL_UNSIGNED_INT, NULL);
            else
                glDrawArrays(ToGlPrimitive(), 0, static_cast<GLsizei>(drawVertexCount));
            ++m_frameStats.drawCalls;
            m_frameStats.vertices += static_cast<unsigned long long>(drawVertexCount);
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
                    (int)m_primitive, (int)drawVertexCount,
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

        virtual void DrawVertices(Platform::LegacyPrimitive primitive, const Platform::LegacyBulkVertex* vertices, size_t count)
        {
            if (vertices == NULL || count == 0) return;
            if (!Platform::IsRenderFeatureActive(Platform::RenderFeatureBatching))
            {
                Platform::ILegacyRenderAdapter::DrawVertices(primitive, vertices, count);
                return;
            }
            Begin(primitive);
            const size_t base = m_vertices.size();
            m_vertices.resize(base + count);
            // LegacyBulkVertex e LegacyVertex tem o mesmo layout; a copia e uma
            // so, em vez de 4 chamadas virtuais por vertice.
            memcpy(&m_vertices[base], vertices, count * sizeof(LegacyVertex));
            // Color4f/TexCoord2f/Normal3f deixavam o ultimo valor como corrente,
            // e ha emissores que contam com essa heranca no draw seguinte.
            HerdarUltimoVertice(m_vertices[m_vertices.size() - 1]);
            End();
        }

        virtual void DrawVertexArrays(Platform::LegacyPrimitive primitive, const float (*positions)[3],
            const float (*colors)[4], const float (*texCoords)[2], size_t count)
        {
            if (positions == NULL || count == 0) return;
            if (!Platform::IsRenderFeatureActive(Platform::RenderFeatureBatching))
            {
                Platform::ILegacyRenderAdapter::DrawVertexArrays(primitive, positions, colors, texCoords, count);
                return;
            }
            Begin(primitive);
            const size_t base = m_vertices.size();
            m_vertices.resize(base + count);
            LegacyVertex* out = &m_vertices[base];
            for (size_t i = 0; i < count; ++i)
            {
                LegacyVertex& v = out[i];
                v.position[0] = positions[i][0]; v.position[1] = positions[i][1]; v.position[2] = positions[i][2];
                if (colors != NULL)
                {
                    // A saturacao e obrigatoria: glColor*f guardava a cor em ponto
                    // fixo, e a iluminacao de modelo passa de 1.0 com frequencia.
                    v.color[0] = Saturar(colors[i][0]); v.color[1] = Saturar(colors[i][1]);
                    v.color[2] = Saturar(colors[i][2]); v.color[3] = Saturar(colors[i][3]);
                }
                else
                    memcpy(v.color, m_current.color, sizeof(v.color));
                if (texCoords != NULL)
                { v.texCoord[0] = texCoords[i][0]; v.texCoord[1] = texCoords[i][1]; }
                else
                    memcpy(v.texCoord, m_current.texCoord, sizeof(v.texCoord));
                // O caminho por vertice nunca chamava Normal3f aqui: herdava a
                // normal corrente. Reproduzir isso mantem o resultado igual.
                memcpy(v.normal, m_current.normal, sizeof(v.normal));
            }
            HerdarUltimoVertice(out[count - 1]);
            End();
        }
        virtual void SetMatrices(const float* projection, const float* modelView)
        {
            const bool projectionChanged = projection != NULL &&
                memcmp(m_projection, projection, sizeof(m_projection)) != 0;
            const bool modelViewChanged = modelView != NULL &&
                memcmp(m_modelView, modelView, sizeof(m_modelView)) != 0;

            // A matriz faz parte do estado do lote; so e uma barreira quando o
            // proximo desenho realmente usara outra transformacao. Antes disso,
            // SyncLegacyRenderMatrices dividia a UI mesmo ao reenviar os mesmos
            // valores de projecao/modelview.
            if (projectionChanged || modelViewChanged)
                FlushPendingBatch(FlushMatrix);
            if (projectionChanged) memcpy(m_projection, projection, sizeof(m_projection));
            if (modelViewChanged) memcpy(m_modelView, modelView, sizeof(m_modelView));
            m_matricesSet = true;
        }
        virtual void SetDepthTest(bool enabled)
        {
            if (m_depthTestConhecido && m_depthTestEnabled == enabled)
                return;
            FlushPendingBatch(FlushDepth);
            enabled ? ::glEnable(GL_DEPTH_TEST) : ::glDisable(GL_DEPTH_TEST);
            m_depthTestEnabled = enabled;
            m_depthTestConhecido = true;
            ++m_frameStats.depthStateChanges;
        }
        virtual void SetAlphaTest(bool enabled)
        {
            if (m_alphaTestEnabled != enabled) FlushPendingBatch(FlushAlpha);
            m_alphaTestEnabled = enabled;
        }
        virtual void SetAlphaTestRef(float reference)
        {
            if (m_alphaTestReference != reference) FlushPendingBatch(FlushAlpha);
            m_alphaTestReference = reference;
        }
        virtual void SetFog(bool enabled, const float* color, float start, float end)
        {
            if (m_fogEnabled != enabled || m_fogStart != start || m_fogEnd != end ||
                (color != NULL && memcmp(m_fogColor, color, sizeof(m_fogColor)) != 0))
                FlushPendingBatch(FlushFog);
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
            if (m_texture2DEnabled != enabled) FlushPendingBatch(FlushTexture);
            m_texture2DEnabled = enabled;
        }
        virtual void BindTexture(unsigned int texture)
        {
            const GLuint glTexture = static_cast<GLuint>(texture);
            if (m_texture != glTexture) FlushPendingBatch(FlushTexture);
            if (m_texture != glTexture)
                ++m_frameStats.textureChanges;
            m_texture = glTexture;
        }
        virtual void SetBlendMode(int mode)
        {
            if (m_blendModeEnviado == mode)
                return;
            FlushPendingBatch(FlushBlend);
            m_blendModeEnviado = mode;
            ++m_frameStats.blendStateChanges;
        }
        virtual void ResetFrameStats() { m_frameStats = Platform::LegacyRenderFrameStats(); }
        virtual Platform::LegacyRenderFrameStats GetFrameStats() const { return m_frameStats; }
        virtual void RecordTextureUpload(unsigned long long bytes)
        {
            ++m_frameStats.textureUploads;
            m_frameStats.textureUploadBytes += bytes;
        }
        virtual void RecordCpuSkinningWork(unsigned long long vertices, unsigned long long normals)
        {
            m_frameStats.cpuSkinningVertices += vertices;
            m_frameStats.cpuSkinningNormals += normals;
        }
        virtual void RecordGpuSkinningFallback(Platform::GpuSkinningFallbackReason reason)
        {
            ++m_frameStats.gpuSkinningFallbacks;
            if (reason == Platform::GpuSkinningFallbackMaterial) ++m_frameStats.gpuSkinningMaterialFallbacks;
            else if (reason == Platform::GpuSkinningFallbackGeometry) ++m_frameStats.gpuSkinningGeometryFallbacks;
            else ++m_frameStats.gpuSkinningResourceFallbacks;
        }
        virtual void RecordCpuTransformWork(unsigned long long transformsExecuted, unsigned long long transformsSkipped,
            unsigned long long animationsExecuted, unsigned long long animationsSkipped)
        {
            m_frameStats.transformsExecuted += transformsExecuted;
            m_frameStats.transformsSkipped += transformsSkipped;
            m_frameStats.animationsExecuted += animationsExecuted;
            m_frameStats.animationsSkipped += animationsSkipped;
        }
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
            // Avancar a geracao aposenta todos os handles ja distribuidos sem
            // precisar avisar cada BMD: o proximo IsStaticMeshResident responde
            // false e o cliente reconstroi a geometria sob demanda.
            ++m_resourceGeneration;
            m_staticMeshes.clear();
            m_staticMeshFreeList.clear();
            m_program = 0;
            m_vertexBuffer = 0;
            m_vertexBufferCapacity = 0;
            m_quadIndexBuffer = 0;
            m_quadIndexCapacity = 0;
            m_vertexArray = 0;
            m_boneBuffer = 0;
            m_boneBuffers[0] = m_boneBuffers[1] = m_boneBuffers[2] = 0;
            m_boneBufferIndex = 0;
            m_projectionLocation = -1;
            m_modelViewLocation = -1;
            m_modelLocation = -1;
            m_drawColorLocation = -1;
            m_skinningLocation = -1;
            m_lightingLocation = -1;
            m_lightPositionLocation = -1;
            m_bodyScaleLocation = -1;
            m_postTranslationLocation = -1;
            m_waveLocation = -1;
            m_worldTimeLocation = -1;
            m_materialEffectLocation = -1;
            m_shadowMapLocation = -1;
            m_bodyOriginLocation = -1;
            m_boneScaleLocation = -1;
            m_boneBlockIndex = GL_INVALID_INDEX;
            m_textureLocation = -1;
            m_useTextureLocation = -1;
            m_instancedLocation = -1;
            m_bonePaletteLocation = -1;
            m_bonePaletteTexture = 0;
            m_bonePaletteWidth = 0;
            m_bonePaletteHeight = 0;
            m_instanceBuffer = 0;
            m_instancingUnavailable = false;
            m_depthTestEnabled = false;
            m_texture = 0;
            m_failed = false;
            m_logged = false;
            m_batching = false;
            m_vertices.clear();
            m_boneRows.clear();
            // Sem isto o cache afirmaria que uniformes e matrizes ja estao no GL, e o
            // contexto novo comecaria com estado indefinido.
            InvalidarCacheDeEstado();
            Platform::LegacyRenderLog("GlslLegacyRenderAdapter: recursos invalidados apos recriacao de contexto");
        }

        virtual bool IsStaticMeshResident(unsigned int handle) const
        {
            return ResolveStaticMesh(handle) != NULL;
        }

        // Estado de passe, comum ao desenho de uma malha e ao instanciado: nao
        // varia por instancia e por isso permanece uniforme.
        void SetupSharedStaticMeshUniforms()
        {
            if (!m_programaAtivo)
            {
                glUseProgram(m_program);
                m_programaAtivo = true;
                ++m_frameStats.programChanges;
            }
            // A unidade de textura e constante durante toda a vida do programa.
            if (!m_texturaUnidadeEnviada || !CacheDeUniformesAtivo())
            {
                glUniform1i(m_textureLocation, 0);
                m_texturaUnidadeEnviada = true;
            }
            EnviarUniform1i(m_useTextureLocation, (m_texture2DEnabled && m_texture != 0) ? 1 : 0, m_usarTexturaEnviado);
            EnviarUniform1i(m_alphaTestLocation, m_alphaTestEnabled ? 1 : 0, m_alphaTestEnviado);
            EnviarUniform1f(m_alphaRefLocation, m_alphaTestReference, m_alphaRefEnviado);
            EnviarUniform1i(m_fogLocation, m_fogEnabled ? 1 : 0, m_fogEnviado);
            EnviarUniform3f(m_fogColorLocation, m_fogColor[0], m_fogColor[1], m_fogColor[2], m_fogColorEnviada);
            EnviarUniform1f(m_fogStartLocation, m_fogStart, m_fogStartEnviado);
            EnviarUniform1f(m_fogEndLocation, m_fogEnd, m_fogEndEnviado);
            EnviarUniformMatriz4(m_projectionLocation, m_projection, m_projEnviada, m_matrizProjEnviada);
            EnviarUniformMatriz4(m_modelViewLocation, m_modelView, m_mvEnviada, m_matrizMvEnviada);
        }

        // Textura de paleta e VBO de instancias, criados sob demanda. Uma falha
        // aqui desliga o caminho instanciado de vez, sem derrubar o resto.
        bool EnsureInstancingResources()
        {
            if (m_instancingUnavailable) return false;
            if (m_bonePaletteTexture != 0 && m_instanceBuffer != 0) return true;
            if (m_bonePaletteTexture == 0)
            {
                glGenTextures(1, &m_bonePaletteTexture);
                if (m_bonePaletteTexture == 0) { m_instancingUnavailable = true; return false; }
                m_bonePaletteWidth = 0;
                m_bonePaletteHeight = 0;
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, m_bonePaletteTexture);
                // texelFetch nao filtra, mas GL exige filtro completo para a
                // textura ser considerada valida.
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glActiveTexture(GL_TEXTURE0);
                m_texturaEnviadaConhecida = false;
            }
            if (m_instanceBuffer == 0)
            {
                glGenBuffers(1, &m_instanceBuffer);
                if (m_instanceBuffer == 0) { m_instancingUnavailable = true; return false; }
            }
            return true;
        }

        virtual unsigned int UploadStaticMesh(const Platform::StaticMeshVertex* vertices, size_t vertexCount,
            const unsigned int* indices, size_t indexCount)
        {
            if (vertices == NULL || indices == NULL || vertexCount == 0 || indexCount == 0 || !EnsureResources())
                return 0;

            StaticMesh mesh;
            mesh.vertexCount = vertexCount;
            mesh.indexCount = indexCount;
            mesh.alive = true;
            mesh.generation = m_resourceGeneration;
            glGenVertexArrays(1, &mesh.vertexArray);
            glGenBuffers(1, &mesh.vertexBuffer);
            glGenBuffers(1, &mesh.indexBuffer);
            glBindVertexArray(mesh.vertexArray);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexCount * sizeof(Platform::StaticMeshVertex)), vertices, GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer);
            // A conversao para 16 bits acontece aqui, e nao no cliente: o formato
            // do IBO e uma decisao do backend, e o buffer temporario morre nesta
            // funcao em vez de virar mais um array vivo no Mesh_t.
            GLsizeiptr indexBytes;
            if (vertexCount <= 65536 && Platform::IsRenderFeatureActive(Platform::RenderFeatureMeshCache))
            {
                mesh.indexType = GL_UNSIGNED_SHORT;
                indexBytes = static_cast<GLsizeiptr>(indexCount * sizeof(unsigned short));
                std::vector<unsigned short> narrow(indexCount);
                for (size_t i = 0; i < indexCount; ++i)
                    narrow[i] = static_cast<unsigned short>(indices[i]);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBytes, &narrow[0], GL_STATIC_DRAW);
            }
            else
            {
                mesh.indexType = GL_UNSIGNED_INT;
                indexBytes = static_cast<GLsizeiptr>(indexCount * sizeof(unsigned int));
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBytes, indices, GL_STATIC_DRAW);
            }
            glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Platform::StaticMeshVertex), (void*)offsetof(Platform::StaticMeshVertex, position));
            glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Platform::StaticMeshVertex), (void*)offsetof(Platform::StaticMeshVertex, color));
            glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Platform::StaticMeshVertex), (void*)offsetof(Platform::StaticMeshVertex, texCoord));
            glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Platform::StaticMeshVertex), (void*)offsetof(Platform::StaticMeshVertex, normal));
            glEnableVertexAttribArray(4); glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Platform::StaticMeshVertex), (void*)offsetof(Platform::StaticMeshVertex, positionBone));
            glEnableVertexAttribArray(5); glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Platform::StaticMeshVertex), (void*)offsetof(Platform::StaticMeshVertex, normalBone));
            glEnableVertexAttribArray(6); glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Platform::StaticMeshVertex), (void*)offsetof(Platform::StaticMeshVertex, waveSeed));
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            size_t slot = m_staticMeshFreeList.empty() ? m_staticMeshes.size() : m_staticMeshFreeList.back();
            if (m_staticMeshFreeList.empty())
                m_staticMeshes.push_back(mesh);
            else { m_staticMeshFreeList.pop_back(); m_staticMeshes[slot] = mesh; }

            m_frameStats.bufferDataCalls += 2;
            ++m_frameStats.staticMeshCacheMisses;
            m_frameStats.staticMeshVerticesResident += static_cast<unsigned long long>(vertexCount);
            m_frameStats.staticMeshIndicesResident += static_cast<unsigned long long>(indexCount);
            m_frameStats.staticMeshUploadBytes += static_cast<unsigned long long>(vertexCount * sizeof(Platform::StaticMeshVertex)) +
                static_cast<unsigned long long>(indexBytes);
            return MakeStaticMeshHandle(slot);
        }

        virtual bool DrawStaticMesh(unsigned int handle, const float* color, const float* modelMatrix,
            const float* boneMatrices = NULL, size_t boneCount = 0, float bodyScale = 1.f,
            bool lighting = false, const float* lightPosition = NULL, const float* postTranslation = NULL,
            bool wave = false, float worldTime = 0.f, int materialEffect = 0, bool shadowMap = false, const float* bodyOrigin = NULL, float boneScale = 1.f)
        {
            const StaticMesh* resolved = ResolveStaticMesh(handle);
            if (resolved == NULL || color == NULL || modelMatrix == NULL || !EnsureResources())
                return false;
            FlushPendingBatch();
            ++m_frameStats.staticMeshCacheHits;
            const StaticMesh& mesh = *resolved;
            SetupSharedStaticMeshUniforms();
            EnviarUniform1i(m_instancedLocation, 0, m_instancedEnviado);
            const float model[16] = {
                modelMatrix[0], modelMatrix[4], modelMatrix[8], 0.f,
                modelMatrix[1], modelMatrix[5], modelMatrix[9], 0.f,
                modelMatrix[2], modelMatrix[6], modelMatrix[10], 0.f,
                modelMatrix[3], modelMatrix[7], modelMatrix[11], 1.f };
            EnviarUniformMatriz4(m_modelLocation, model, m_modeloEnviado, m_modeloEnviadoConhecido);
            // Nao limite a cor antes da iluminacao no vertex shader. No caminho
            // legado, a saturacao acontece apos BodyLight * IntensityTransform.
            EnviarUniform3f(m_drawColorLocation, color[0], color[1], color[2], m_drawColorEnviada);
            const bool skinning = boneMatrices != NULL && boneCount > 0 && boneCount <= 200 && m_boneBlockIndex != GL_INVALID_INDEX;
            EnviarUniform1i(m_skinningLocation, skinning ? 1 : 0, m_skinningEnviado);
            EnviarUniform1i(m_lightingLocation, lighting ? 1 : 0, m_lightingEnviado);
            EnviarUniform1f(m_bodyScaleLocation, bodyScale, m_bodyScaleEnviado);
            if (postTranslation != NULL)
                EnviarUniform3f(m_postTranslationLocation, postTranslation[0], postTranslation[1], postTranslation[2], m_postTranslationEnviada);
            EnviarUniform1i(m_waveLocation, wave ? 1 : 0, m_waveEnviado);
            // uWorldTime so entra na conta quando wave ou um efeito de material o
            // usa; enviar por malha em cena parada era puro trafego.
            if (wave || materialEffect != 0)
                EnviarUniform1f(m_worldTimeLocation, worldTime, m_worldTimeEnviado);
            EnviarUniform1i(m_materialEffectLocation, materialEffect, m_materialEffectEnviado);
            EnviarUniform1i(m_shadowMapLocation, shadowMap ? 1 : 0, m_shadowMapEnviado);
            if (shadowMap && bodyOrigin != NULL)
                EnviarUniform3f(m_bodyOriginLocation, bodyOrigin[0], bodyOrigin[1], bodyOrigin[2], m_bodyOriginEnviada);
            EnviarUniform1f(m_boneScaleLocation, boneScale, m_boneScaleEnviado);
            if (lighting && lightPosition != NULL)
                EnviarUniform3f(m_lightPositionLocation, lightPosition[0], lightPosition[1], lightPosition[2], m_lightPositionEnviada);
            if (skinning)
            {
                std::vector<float>& rows = m_boneRows;
                const size_t rowCount = boneCount * 12;
                const size_t byteCount = rowCount * sizeof(float);
                // Diversas meshes do mesmo BMD compartilham a mesma pose. So
                // reenviamos a UBO se as matrizes realmente mudaram; comparar
                // o conteudo, e nao o ponteiro, tambem cobre BoneTransform
                // global reutilizado por instancias consecutivas.
                const bool paletteChanged = rows.size() != rowCount || rows.empty() ||
                    memcmp(&rows[0], boneMatrices, byteCount) != 0;
                if (paletteChanged)
                {
                    rows.resize(rowCount);
                    memcpy(&rows[0], boneMatrices, byteCount);
                    m_boneBuffer = m_boneBuffers[m_boneBufferIndex];
                    m_boneBufferIndex = (m_boneBufferIndex + 1) % 3;
                    glBindBuffer(GL_UNIFORM_BUFFER, m_boneBuffer);
                    // Orphaning explicito evita esperar uma leitura da paleta
                    // pelo GPU antes de gravar a pose seguinte.
                    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(byteCount), NULL, GL_STREAM_DRAW);
                    glBufferSubData(GL_UNIFORM_BUFFER, 0, static_cast<GLsizeiptr>(byteCount), &rows[0]);
                    ++m_frameStats.bufferDataCalls;
                    ++m_frameStats.bufferSubDataCalls;
                    m_frameStats.bonePaletteUploadBytes += static_cast<unsigned long long>(byteCount);
                }
                glBindBuffer(GL_UNIFORM_BUFFER, m_boneBuffer);
                glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_boneBuffer);
            }
            if (!CacheDeUniformesAtivo() || !m_texturaEnviadaConhecida || m_texturaEnviada != m_texture)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_texture);
                m_texturaEnviada = m_texture;
                m_texturaEnviadaConhecida = true;
                ++m_frameStats.textureChanges;
            }
            glBindVertexArray(mesh.vertexArray);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), mesh.indexType, NULL);
            ++m_frameStats.drawCalls;
            ++m_frameStats.staticMeshDrawCalls;
            m_frameStats.vertices += mesh.indexCount;
            m_frameStats.staticMeshIndices += mesh.indexCount;
            if (CacheDeUniformesAtivo())
            {
                // O unico estado que esta funcao realmente estraga para o caminho
                // imediato e o VAO: o programa, os uniformes e a textura ficam
                // espelhados corretamente. Invalidar tudo, como antes, obrigava a
                // proxima malha do MESMO modelo a reenviar as ~20 chamadas.
                m_vaoAtivo = false;
            }
            else
                InvalidarCacheDeEstado();
            return true;
        }

        virtual size_t GetMaxInstanceBoneCount() const
        {
            return (g_ModernGl.HasInstancing() && !m_instancingUnavailable) ? kMaxInstanceBones : 0;
        }

        virtual bool DrawStaticMeshInstanced(unsigned int handle, const Platform::StaticMeshInstance* instances, size_t count,
            float worldTime = 0.f)
        {
            if (instances == NULL || count == 0) return false;
            if (!g_ModernGl.HasInstancing() || m_instancingUnavailable) return false;
            const StaticMesh* resolved = ResolveStaticMesh(handle);
            if (resolved == NULL || !EnsureResources() || !EnsureInstancingResources()) return false;

            // Uma unica instancia nao paga o upload da paleta em textura nem o do
            // buffer de instancias: o caminho normal sai na frente.
            if (count == 1) return false;

            size_t boneCount = instances[0].boneCount;
            for (size_t i = 0; i < count; ++i)
            {
                if (instances[i].boneCount != boneCount || instances[i].boneMatrices == NULL)
                    return false;
            }
            if (boneCount == 0 || boneCount > kMaxInstanceBones) return false;

            FlushPendingBatch();
            const StaticMesh& mesh = *resolved;

            // Paleta: 3 texels por osso na horizontal, uma instancia por linha.
            const size_t rowTexels = boneCount * 3;
            m_instancePalette.assign(rowTexels * count * 4, 0.f);
            for (size_t i = 0; i < count; ++i)
                memcpy(&m_instancePalette[i * rowTexels * 4], instances[i].boneMatrices, boneCount * 12 * sizeof(float));

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_bonePaletteTexture);
            // glTexImage2D REALOCA o armazenamento. Chamado por lote, ele criava
            // uma bolha de pipeline por draw e foi metade do custo que fez o
            // instancing piorar o frame em 42% na primeira medicao. A textura
            // agora cresce por potencia de dois e o caminho normal e TexSubImage.
            if (static_cast<GLsizei>(rowTexels) > m_bonePaletteWidth ||
                static_cast<GLsizei>(count) > m_bonePaletteHeight)
            {
                GLsizei width = m_bonePaletteWidth > 0 ? m_bonePaletteWidth : 64;
                GLsizei height = m_bonePaletteHeight > 0 ? m_bonePaletteHeight : 16;
                while (width < static_cast<GLsizei>(rowTexels)) width *= 2;
                while (height < static_cast<GLsizei>(count)) height *= 2;
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
                m_bonePaletteWidth = width;
                m_bonePaletteHeight = height;
            }
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(rowTexels),
                static_cast<GLsizei>(count), GL_RGBA, GL_FLOAT, &m_instancePalette[0]);
            m_frameStats.bonePaletteUploadBytes += static_cast<unsigned long long>(m_instancePalette.size() * sizeof(float));

            // Atributos por instancia, no mesmo layout declarado no shader.
            m_instanceAttributes.resize(count * kInstanceFloats);
            for (size_t i = 0; i < count; ++i)
            {
                const Platform::StaticMeshInstance& source = instances[i];
                float* out = &m_instanceAttributes[i * kInstanceFloats];
                out[0] = source.color[0]; out[1] = source.color[1]; out[2] = source.color[2]; out[3] = source.color[3];
                out[4] = source.postTranslation[0]; out[5] = source.postTranslation[1]; out[6] = source.postTranslation[2];
                out[7] = source.bodyScale;
                out[8] = source.lightPosition[0]; out[9] = source.lightPosition[1]; out[10] = source.lightPosition[2];
                out[11] = source.lighting ? 1.f : 0.f;
                out[12] = source.bodyOrigin[0]; out[13] = source.bodyOrigin[1]; out[14] = source.bodyOrigin[2];
                out[15] = source.boneScale;
                out[16] = static_cast<float>(i);
                out[17] = static_cast<float>(source.materialEffect);
                out[18] = static_cast<float>((source.wave ? 1 : 0) | (source.shadowMap ? 2 : 0) | 4);
                out[19] = 0.f;
            }

            glBindVertexArray(mesh.vertexArray);
            glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
            const GLsizeiptr instanceBytes = static_cast<GLsizeiptr>(m_instanceAttributes.size() * sizeof(float));
            // Orphaning: nao esperar a GPU terminar de ler o lote anterior.
            glBufferData(GL_ARRAY_BUFFER, instanceBytes, NULL, GL_STREAM_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, instanceBytes, &m_instanceAttributes[0]);
            ++m_frameStats.bufferDataCalls;
            ++m_frameStats.bufferSubDataCalls;
            const GLsizei stride = static_cast<GLsizei>(kInstanceFloats * sizeof(float));
            for (int slot = 0; slot < 5; ++slot)
            {
                const GLuint location = static_cast<GLuint>(7 + slot);
                glEnableVertexAttribArray(location);
                glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, stride,
                    reinterpret_cast<const void*>(static_cast<uintptr_t>(slot * 4 * sizeof(float))));
                glVertexAttribDivisor(location, 1);
            }

            SetupSharedStaticMeshUniforms();
            EnviarUniform1i(m_instancedLocation, 1, m_instancedEnviado);
            // Incondicional aqui: o lote pode misturar instancias com e sem wave
            // ou efeito de material, e nao ha como saber pela chave — justamente
            // porque esses flags foram tirados dela de proposito.
            EnviarUniform1f(m_worldTimeLocation, worldTime, m_worldTimeEnviado);
            if (m_bonePaletteLocation >= 0)
                glUniform1i(m_bonePaletteLocation, 1);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_texture);
            m_texturaEnviada = m_texture;
            m_texturaEnviadaConhecida = true;

            glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), mesh.indexType,
                NULL, static_cast<GLsizei>(count));

            // Os atributos de instancia vivem no VAO da malha. Deixa-los ligados
            // faria o proximo DrawStaticMesh nao instanciado ler lixo do buffer
            // de instancias, entao sao desligados aqui.
            for (int slot = 0; slot < 5; ++slot)
            {
                const GLuint location = static_cast<GLuint>(7 + slot);
                glVertexAttribDivisor(location, 0);
                glDisableVertexAttribArray(location);
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            ++m_frameStats.drawCalls;
            ++m_frameStats.instancedDrawCalls;
            ++m_frameStats.instanceBatchesFlushed;
            m_frameStats.instancesSubmitted += static_cast<unsigned long long>(count);
            if (static_cast<unsigned long long>(count) > m_frameStats.largestInstanceBatch)
                m_frameStats.largestInstanceBatch = static_cast<unsigned long long>(count);
            m_frameStats.staticMeshIndices += static_cast<unsigned long long>(mesh.indexCount) * count;
            m_frameStats.vertices += static_cast<unsigned long long>(mesh.indexCount) * count;
            m_vaoAtivo = false;
            return true;
        }

        virtual void ReleaseStaticMesh(unsigned int handle)
        {
            if (ResolveStaticMesh(handle) == NULL) return;
            const size_t slot = static_cast<size_t>(handle & StaticMeshSlotMask) - 1;
            StaticMesh& mesh = m_staticMeshes[slot];
            if (mesh.vertexArray != 0) glDeleteVertexArrays(1, &mesh.vertexArray);
            if (mesh.vertexBuffer != 0) glDeleteBuffers(1, &mesh.vertexBuffer);
            if (mesh.indexBuffer != 0) glDeleteBuffers(1, &mesh.indexBuffer);
            mesh = StaticMesh();
            m_staticMeshFreeList.push_back(slot);
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
        enum FlushReason
        {
            FlushUnknown,
            FlushMatrix,
            FlushTexture,
            FlushBlend,
            FlushDepth,
            FlushAlpha,
            FlushFog
        };
        void FlushPendingBatch(FlushReason reason = FlushUnknown)
        {
            if (m_batching && !m_vertices.empty())
            {
                switch (reason)
                {
                case FlushMatrix: ++m_frameStats.matrixFlushes; break;
                case FlushTexture: ++m_frameStats.textureFlushes; break;
                case FlushBlend: ++m_frameStats.blendFlushes; break;
                case FlushDepth: ++m_frameStats.depthFlushes; break;
                case FlushAlpha: ++m_frameStats.alphaFlushes; break;
                case FlushFog: ++m_frameStats.fogFlushes; break;
                default: break;
                }
                Flush();
            }
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
        // O caminho por vertice deixava cor/UV/normal do ultimo vertice como
        // estado corrente. A submissao em bloco tem que deixar o mesmo.
        void HerdarUltimoVertice(const LegacyVertex& ultimo)
        {
            memcpy(m_current.color, ultimo.color, sizeof(m_current.color));
            memcpy(m_current.texCoord, ultimo.texCoord, sizeof(m_current.texCoord));
            memcpy(m_current.normal, ultimo.normal, sizeof(m_current.normal));
        }

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
        struct StaticMesh
        {
            StaticMesh() : vertexArray(0), vertexBuffer(0), indexBuffer(0), vertexCount(0), indexCount(0),
                indexType(GL_UNSIGNED_INT), generation(0), alive(false) {}
            GLuint vertexArray;
            GLuint vertexBuffer;
            GLuint indexBuffer;
            size_t vertexCount;
            size_t indexCount;
            // 16 bits sempre que os vertices unicos couberem: metade da banda de
            // indice e do espaco em VRAM, sem custo nenhum do lado do desenho.
            GLenum indexType;
            unsigned int generation;
            bool alive;
        };

        // Handle = geracao (12 bits altos) + slot + 1 (20 bits baixos). O "+1"
        // reserva o zero para "nao residente", e a geracao faz um handle sobreviver
        // como valor mas se identificar como morto apos a recriacao do contexto.
        enum { StaticMeshSlotBits = 20, StaticMeshSlotMask = (1 << StaticMeshSlotBits) - 1 };

        unsigned int MakeStaticMeshHandle(size_t slot) const
        {
            return ((m_resourceGeneration & 0xFFFu) << StaticMeshSlotBits) |
                (static_cast<unsigned int>(slot + 1) & StaticMeshSlotMask);
        }

        const StaticMesh* ResolveStaticMesh(unsigned int handle) const
        {
            if (handle == 0) return NULL;
            if (((handle >> StaticMeshSlotBits) & 0xFFFu) != (m_resourceGeneration & 0xFFFu)) return NULL;
            const size_t slot = static_cast<size_t>(handle & StaticMeshSlotMask) - 1;
            if (slot >= m_staticMeshes.size()) return NULL;
            const StaticMesh& mesh = m_staticMeshes[slot];
            return mesh.alive ? &mesh : NULL;
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
            ++m_frameStats.bufferDataCalls;
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
            ++m_frameStats.bufferDataCalls;
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
            m_modelLocation = glGetUniformLocation(m_program, "uModel");
            m_drawColorLocation = glGetUniformLocation(m_program, "uDrawColor");
            m_skinningLocation = glGetUniformLocation(m_program, "uSkinning");
            m_lightingLocation = glGetUniformLocation(m_program, "uLighting");
            m_lightPositionLocation = glGetUniformLocation(m_program, "uLightPosition");
            m_bodyScaleLocation = glGetUniformLocation(m_program, "uBodyScale");
            m_postTranslationLocation = glGetUniformLocation(m_program, "uPostTranslation");
            m_waveLocation = glGetUniformLocation(m_program, "uWave");
            m_worldTimeLocation = glGetUniformLocation(m_program, "uWorldTime");
            m_materialEffectLocation = glGetUniformLocation(m_program, "uMaterialEffect");
            m_shadowMapLocation = glGetUniformLocation(m_program, "uShadowMap");
            m_bodyOriginLocation = glGetUniformLocation(m_program, "uBodyOrigin");
            m_boneScaleLocation = glGetUniformLocation(m_program, "uBoneScale");
            m_instancedLocation = glGetUniformLocation(m_program, "uInstanced");
            m_bonePaletteLocation = glGetUniformLocation(m_program, "uBonePalette");
            m_boneBlockIndex = glGetUniformBlockIndex(m_program, "BmdBones");
            if (m_boneBlockIndex != GL_INVALID_INDEX)
                glUniformBlockBinding(m_program, m_boneBlockIndex, 0);
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
            glGenBuffers(3, m_boneBuffers);
            m_boneBuffer = m_boneBuffers[0];
            m_boneBufferIndex = 1;
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
        std::vector<LegacyVertex> m_drawVertices;
        std::vector<float> m_boneRows;
        LegacyVertex m_current;
        GLuint m_program;
        GLuint m_vertexBuffer;
        GLsizeiptr m_vertexBufferCapacity;
        GLuint m_quadIndexBuffer;
        size_t m_quadIndexCapacity;
        GLuint m_vertexArray;
        GLint m_projectionLocation;
        GLint m_modelViewLocation;
        GLint m_modelLocation;
        GLint m_drawColorLocation;
        GLint m_skinningLocation;
        GLint m_lightingLocation;
        GLint m_lightPositionLocation;
        GLint m_bodyScaleLocation;
        GLint m_postTranslationLocation;
        GLint m_waveLocation;
        GLint m_worldTimeLocation;
        GLint m_materialEffectLocation;
        GLint m_shadowMapLocation;
        GLint m_bodyOriginLocation;
        GLint m_boneScaleLocation;
        GLuint m_boneBlockIndex;
        GLuint m_boneBuffer;
        GLuint m_boneBuffers[3];
        unsigned int m_boneBufferIndex;
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
        // Uniformes do bloco de modelo/skinning. Antes eram reenviados
        // incondicionalmente: 8 por Flush do caminho imediato (so para voltar ao
        // neutro) e ~20 por malha em DrawStaticMesh. Malhas consecutivas do mesmo
        // BMD compartilham quase todos.
        bool  m_modeloEnviadoConhecido;
        float m_modeloEnviado[16];
        float m_drawColorEnviada[3];
        int   m_skinningEnviado;
        int   m_lightingEnviado;
        float m_lightPositionEnviada[3];
        float m_bodyScaleEnviado;
        float m_postTranslationEnviada[3];
        int   m_waveEnviado;
        float m_worldTimeEnviado;
        int   m_materialEffectEnviado;
        int   m_shadowMapEnviado;
        float m_bodyOriginEnviada[3];
        float m_boneScaleEnviado;
        bool  m_texturaUnidadeEnviada;
        Platform::LegacyRenderFrameStats m_frameStats;
        // Tabela de slots com free list: lookup O(1) por handle, sem comparacao
        // de ponteiros nem alocacao por malha no caminho de desenho.
        std::vector<StaticMesh> m_staticMeshes;
        std::vector<size_t> m_staticMeshFreeList;
        unsigned int m_resourceGeneration;

        // Instancing. kMaxInstanceBones espelha o limite do bloco de ossos do
        // caminho nao instanciado, para que a decisao de elegibilidade do cliente
        // sirva aos dois. kInstanceFloats sao 5 vec4 por instancia.
        enum { kMaxInstanceBones = 200, kInstanceFloats = 20 };
        GLuint m_bonePaletteTexture;
        GLsizei m_bonePaletteWidth;
        GLsizei m_bonePaletteHeight;
        GLuint m_instanceBuffer;
        std::vector<float> m_instancePalette;
        std::vector<float> m_instanceAttributes;
        GLint m_instancedLocation;
        GLint m_bonePaletteLocation;
        int m_instancedEnviado;
        bool m_instancingUnavailable;
    };
}

namespace Platform
{
    ILegacyRenderAdapter* CreateGlslLegacyRenderAdapter() { return new GlslLegacyRenderAdapter(); }
}
