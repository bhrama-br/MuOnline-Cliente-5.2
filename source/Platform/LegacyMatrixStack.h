#pragma once

// Pilha de matrizes em CPU equivalente ao GL_MODELVIEW/GL_PROJECTION do pipeline
// fixo, que nao existe em GLES3/WebGL2.
//
// No Windows este cabecalho apenas declara a API: o caminho legado continua
// usando as funcoes reais do OpenGL, para nao alterar o comportamento do build PC.
// Fora do Windows sao definidas funcoes globais com os mesmos nomes do GL
// (glPushMatrix, glRotatef, ...) que encaminham para esta pilha. Sao funcoes
// reais, nao macros, para nao vazarem para a implementacao dos adapters.

namespace Platform
{
    enum LegacyMatrixMode
    {
        LegacyMatrixModelView = 0,
        LegacyMatrixProjection = 1
    };

    void LegacySetMatrixMode(LegacyMatrixMode mode);
    LegacyMatrixMode LegacyGetMatrixMode();

    void LegacyLoadIdentity();
    void LegacyLoadMatrix(const float* matrix);
    void LegacyMultMatrix(const float* matrix);
    void LegacyPushMatrix();
    void LegacyPopMatrix();

    void LegacyTranslate(float x, float y, float z);
    void LegacyRotate(float degrees, float x, float y, float z);
    void LegacyScale(float x, float y, float z);

    void LegacyPerspective(float fieldOfViewDegrees, float aspect, float nearPlane, float farPlane);
    void LegacyOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane);

    // Ponteiro para a matriz corrente do modo indicado (coluna-maior, 16 floats).
    const float* LegacyGetMatrix(LegacyMatrixMode mode);

    // Empurra projecao e modelview correntes para o ILegacyRenderAdapter.
    void LegacyApplyMatricesToAdapter();
}

#ifndef _WIN32

// Nomes do pipeline fixo esperados pelo codigo legado. Definidos apenas fora do
// Windows; no Windows quem responde e o proprio OpenGL.
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif

inline void glMatrixMode(unsigned int mode)
{
    Platform::LegacySetMatrixMode(mode == GL_PROJECTION
        ? Platform::LegacyMatrixProjection
        : Platform::LegacyMatrixModelView);
}

inline void glLoadIdentity()                              { Platform::LegacyLoadIdentity(); }
inline void glLoadMatrixf(const float* matrix)            { Platform::LegacyLoadMatrix(matrix); }
inline void glMultMatrixf(const float* matrix)            { Platform::LegacyMultMatrix(matrix); }
inline void glPushMatrix()                                { Platform::LegacyPushMatrix(); }
inline void glPopMatrix()                                 { Platform::LegacyPopMatrix(); }
inline void glTranslatef(float x, float y, float z)       { Platform::LegacyTranslate(x, y, z); }
inline void glRotatef(float a, float x, float y, float z) { Platform::LegacyRotate(a, x, y, z); }
inline void glScalef(float x, float y, float z)           { Platform::LegacyScale(x, y, z); }

inline void gluPerspective(float fov, float aspect, float nearPlane, float farPlane)
{
    Platform::LegacyPerspective(fov, aspect, nearPlane, farPlane);
}

inline void gluOrtho2D(float left, float right, float bottom, float top)
{
    Platform::LegacyOrtho(left, right, bottom, top, -1.f, 1.f);
}

#endif // _WIN32
