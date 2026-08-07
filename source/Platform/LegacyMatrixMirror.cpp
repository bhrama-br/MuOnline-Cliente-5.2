#include "stdafx.h"

// NAO inclui LegacyMatrixMirror.h: as macros de la reescreveriam justamente as
// chamadas reais do GL que este arquivo precisa fazer.
#include "LegacyMatrixStack.h"

#ifdef _WIN32

#include <GL/gl.h>
#include <GL/glu.h>

namespace
{
    bool g_primed = false;

    Platform::LegacyMatrixMode ToLegacyMode(unsigned int glMode)
    {
        return (glMode == GL_PROJECTION) ? Platform::LegacyMatrixProjection
                                         : Platform::LegacyMatrixModelView;
    }
}

namespace Platform
{
    // Cada operacao vai para os dois lados. O GL continua recebendo tudo porque
    // o adapter de funcao fixa ainda usa a pilha dele quando o backend GLSL esta
    // desligado; o espelho existe para que a LEITURA nao precise do driver.
    void MirrorMatrixMode(unsigned int mode)
    {
        ::glMatrixMode(mode);
        LegacySetMatrixMode(ToLegacyMode(mode));
        g_primed = true;
    }

    void MirrorPushMatrix()
    {
        ::glPushMatrix();
        LegacyPushMatrix();
    }

    void MirrorPopMatrix()
    {
        ::glPopMatrix();
        LegacyPopMatrix();
    }

    void MirrorLoadIdentity()
    {
        ::glLoadIdentity();
        LegacyLoadIdentity();
        g_primed = true;
    }

    void MirrorLoadMatrixf(const float* matrix)
    {
        ::glLoadMatrixf(matrix);
        LegacyLoadMatrix(matrix);
        g_primed = true;
    }

    void MirrorMultMatrixf(const float* matrix)
    {
        ::glMultMatrixf(matrix);
        LegacyMultMatrix(matrix);
    }

    void MirrorTranslatef(float x, float y, float z)
    {
        ::glTranslatef(x, y, z);
        LegacyTranslate(x, y, z);
    }

    void MirrorRotatef(float degrees, float x, float y, float z)
    {
        ::glRotatef(degrees, x, y, z);
        LegacyRotate(degrees, x, y, z);
    }

    void MirrorScalef(float x, float y, float z)
    {
        ::glScalef(x, y, z);
        LegacyScale(x, y, z);
    }

    void MirrorPerspective(float fieldOfViewDegrees, float aspect, float nearPlane, float farPlane)
    {
        ::gluPerspective(fieldOfViewDegrees, aspect, nearPlane, farPlane);
        LegacyPerspective(fieldOfViewDegrees, aspect, nearPlane, farPlane);
    }

    const float* MirrorGetProjection() { return LegacyGetMatrix(LegacyMatrixProjection); }
    const float* MirrorGetModelView()  { return LegacyGetMatrix(LegacyMatrixModelView); }
    bool MirrorIsPrimed() { return g_primed; }
}

#endif // _WIN32
