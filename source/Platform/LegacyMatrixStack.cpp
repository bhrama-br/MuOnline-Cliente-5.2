#include "LegacyMatrixStack.h"
#include "LegacyRenderAdapter.h"

#include <cmath>
#include <cstring>
#include <vector>

// Matrizes no layout coluna-maior do OpenGL: matrix[coluna * 4 + linha].
// A ordem de composicao replica o pos-multiplicar do pipeline fixo: cada
// glTranslatef/glRotatef/glScalef equivale a corrente = corrente * transformacao.

namespace
{
    const float kPi = 3.14159265358979323846f;

    struct MatrixState
    {
        float current[16];
        std::vector<float> saved;   // pilha achatada, 16 floats por nivel
    };

    MatrixState g_state[2];
    Platform::LegacyMatrixMode g_mode = Platform::LegacyMatrixModelView;
    bool g_initialized = false;

    void SetIdentity(float* matrix)
    {
        std::memset(matrix, 0, sizeof(float) * 16);
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.f;
    }

    void EnsureInitialized()
    {
        if (g_initialized) return;
        g_initialized = true;
        SetIdentity(g_state[0].current);
        SetIdentity(g_state[1].current);
    }

    MatrixState& Current()
    {
        EnsureInitialized();
        return g_state[g_mode];
    }

    // out = left * right
    void Multiply(const float* left, const float* right, float* out)
    {
        float result[16];
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                float sum = 0.f;
                for (int k = 0; k < 4; ++k)
                    sum += left[k * 4 + row] * right[column * 4 + k];
                result[column * 4 + row] = sum;
            }
        }
        std::memcpy(out, result, sizeof(result));
    }

    void ApplyToCurrent(const float* transform)
    {
        MatrixState& state = Current();
        Multiply(state.current, transform, state.current);
    }
}

namespace Platform
{
    void LegacySetMatrixMode(LegacyMatrixMode mode)
    {
        EnsureInitialized();
        g_mode = mode;
    }

    LegacyMatrixMode LegacyGetMatrixMode() { return g_mode; }

    void LegacyLoadIdentity() { SetIdentity(Current().current); }

    void LegacyLoadMatrix(const float* matrix)
    {
        if (matrix == NULL) return;
        std::memcpy(Current().current, matrix, sizeof(float) * 16);
    }

    void LegacyMultMatrix(const float* matrix)
    {
        if (matrix == NULL) return;
        ApplyToCurrent(matrix);
    }

    void LegacyPushMatrix()
    {
        MatrixState& state = Current();
        state.saved.insert(state.saved.end(), state.current, state.current + 16);
    }

    void LegacyPopMatrix()
    {
        MatrixState& state = Current();
        if (state.saved.size() < 16) return;   // pop sem push: mantem a corrente
        std::memcpy(state.current, &state.saved[state.saved.size() - 16], sizeof(float) * 16);
        state.saved.resize(state.saved.size() - 16);
    }

    void LegacyTranslate(float x, float y, float z)
    {
        float transform[16];
        SetIdentity(transform);
        transform[12] = x;
        transform[13] = y;
        transform[14] = z;
        ApplyToCurrent(transform);
    }

    void LegacyRotate(float degrees, float x, float y, float z)
    {
        const float length = std::sqrt(x * x + y * y + z * z);
        if (length <= 0.f) return;
        x /= length; y /= length; z /= length;

        const float radians = degrees * kPi / 180.f;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        const float t = 1.f - c;

        // Rotacao de eixo arbitrario (mesma formula de glRotatef).
        float transform[16];
        SetIdentity(transform);
        transform[0]  = t * x * x + c;
        transform[1]  = t * x * y + s * z;
        transform[2]  = t * x * z - s * y;
        transform[4]  = t * x * y - s * z;
        transform[5]  = t * y * y + c;
        transform[6]  = t * y * z + s * x;
        transform[8]  = t * x * z + s * y;
        transform[9]  = t * y * z - s * x;
        transform[10] = t * z * z + c;
        ApplyToCurrent(transform);
    }

    void LegacyScale(float x, float y, float z)
    {
        float transform[16];
        SetIdentity(transform);
        transform[0] = x;
        transform[5] = y;
        transform[10] = z;
        ApplyToCurrent(transform);
    }

    void LegacyPerspective(float fieldOfViewDegrees, float aspect, float nearPlane, float farPlane)
    {
        if (aspect == 0.f || nearPlane == farPlane) return;
        const float focal = 1.f / std::tan(fieldOfViewDegrees * kPi / 180.f * 0.5f);

        float transform[16];
        std::memset(transform, 0, sizeof(transform));
        transform[0]  = focal / aspect;
        transform[5]  = focal;
        transform[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
        transform[11] = -1.f;
        transform[14] = (2.f * farPlane * nearPlane) / (nearPlane - farPlane);
        ApplyToCurrent(transform);
    }

    void LegacyOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    {
        if (left == right || bottom == top || nearPlane == farPlane) return;

        float transform[16];
        SetIdentity(transform);
        transform[0]  =  2.f / (right - left);
        transform[5]  =  2.f / (top - bottom);
        transform[10] = -2.f / (farPlane - nearPlane);
        transform[12] = -(right + left) / (right - left);
        transform[13] = -(top + bottom) / (top - bottom);
        transform[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        ApplyToCurrent(transform);
    }

    const float* LegacyGetMatrix(LegacyMatrixMode mode)
    {
        EnsureInitialized();
        return g_state[mode].current;
    }

    void LegacyApplyMatricesToAdapter()
    {
        EnsureInitialized();
        GetLegacyRenderAdapter().SetMatrices(
            g_state[LegacyMatrixProjection].current,
            g_state[LegacyMatrixModelView].current);
    }
}
