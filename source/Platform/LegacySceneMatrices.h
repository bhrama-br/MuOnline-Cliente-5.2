#pragma once

#include <cmath>

namespace Platform
{
    // Matrizes no layout coluna-maior do OpenGL: matrix[coluna * 4 + linha].

    inline void BuildLegacyPerspective(float fieldOfViewDegrees, float aspect, float nearPlane, float farPlane, float* matrix)
    {
        const float radians = fieldOfViewDegrees * 3.14159265358979323846f / 180.0f;
        const float focal = 1.0f / std::tan(radians * 0.5f);
        for (int i = 0; i < 16; ++i) matrix[i] = 0.0f;
        matrix[0] = focal / aspect;
        matrix[5] = focal;
        matrix[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
        matrix[11] = -1.0f;
        matrix[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    }

    inline void BuildLegacyIdentity(float* matrix)
    {
        for (int i = 0; i < 16; ++i) matrix[i] = 0.0f;
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
    }

    inline void BuildLegacyView(float cameraX, float cameraY, float cameraZ, float* matrix)
    {
        BuildLegacyIdentity(matrix);
        matrix[12] = -cameraX;
        matrix[13] = -cameraY;
        matrix[14] = -cameraZ;
    }

    // out = left * right, equivalente ao pos-multiplicar do glRotatef/glTranslatef.
    inline void MultiplyLegacyMatrix(const float* left, const float* right, float* out)
    {
        float result[16];
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k)
                    sum += left[k * 4 + row] * right[column * 4 + k];
                result[column * 4 + row] = sum;
            }
        }
        for (int i = 0; i < 16; ++i) out[i] = result[i];
    }

    inline void BuildLegacyRotationX(float degrees, float* matrix)
    {
        const float radians = degrees * 3.14159265358979323846f / 180.0f;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        BuildLegacyIdentity(matrix);
        matrix[5] = c;  matrix[9] = -s;
        matrix[6] = s;  matrix[10] = c;
    }

    inline void BuildLegacyRotationY(float degrees, float* matrix)
    {
        const float radians = degrees * 3.14159265358979323846f / 180.0f;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        BuildLegacyIdentity(matrix);
        matrix[0] = c;   matrix[8] = s;
        matrix[2] = -s;  matrix[10] = c;
    }

    inline void BuildLegacyRotationZ(float degrees, float* matrix)
    {
        const float radians = degrees * 3.14159265358979323846f / 180.0f;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        BuildLegacyIdentity(matrix);
        matrix[0] = c;  matrix[4] = -s;
        matrix[1] = s;  matrix[5] = c;
    }

    // Parametros de camera do fluxo legado (ZzzOpenglUtil.cpp / ZzzScene.cpp).
    struct LegacySceneCamera
    {
        float fieldOfViewDegrees;   // CameraFOV
        float viewNear;             // CameraViewNear
        float viewFar;              // CameraViewFar (BeginOpengl aplica *1.4f)
        float angles[3];            // CameraAngle
        float position[3];          // CameraPosition
        bool  topViewEnable;        // CameraTopViewEnable
    };

    // Valores iniciais do legado: CameraViewNear/Far/FOV vem de ZzzOpenglUtil.cpp e
    // os angulos da cena de gameplay em ZzzScene.cpp (CameraAngle[0] = -48.5f,
    // CameraAngle[2] = -45.f). CameraPosition depende do heroi em tempo de execucao
    // e fica zerada aqui; usar BuildLegacyCameraPosition para enquadrar um alvo.
    inline LegacySceneCamera GetLegacySceneCameraDefaults()
    {
        LegacySceneCamera camera;
        camera.fieldOfViewDegrees = 55.0f;
        camera.viewNear = 20.0f;
        camera.viewFar = 2000.0f;
        camera.angles[0] = -48.5f;
        camera.angles[1] = 0.0f;
        camera.angles[2] = -45.0f;
        camera.position[0] = 0.0f;
        camera.position[1] = 0.0f;
        camera.position[2] = 0.0f;
        camera.topViewEnable = false;
        return camera;
    }

    // Rotacoes de BeginOpengl, na mesma ordem: Ry(CameraAngle[1]) * Rx(CameraAngle[0])
    // * Rz(CameraAngle[2]). Rx e ignorada quando CameraTopViewEnable esta ativo.
    inline void BuildLegacyCameraRotation(const float* angles, bool topViewEnable, float* matrix)
    {
        float rotation[16];
        float axis[16];
        BuildLegacyRotationY(angles[1], rotation);
        if (!topViewEnable)
        {
            BuildLegacyRotationX(angles[0], axis);
            MultiplyLegacyMatrix(rotation, axis, rotation);
        }
        BuildLegacyRotationZ(angles[2], axis);
        MultiplyLegacyMatrix(rotation, axis, matrix);
    }

    // Modelview equivalente ao bloco GL_MODELVIEW de BeginOpengl:
    // glRotatef(Y) -> glRotatef(X) -> glRotatef(Z) -> glTranslatef(-CameraPosition).
    inline void BuildLegacyCameraView(const LegacySceneCamera& camera, float* matrix)
    {
        float rotation[16];
        float translation[16];
        BuildLegacyCameraRotation(camera.angles, camera.topViewEnable, rotation);
        BuildLegacyIdentity(translation);
        translation[12] = -camera.position[0];
        translation[13] = -camera.position[1];
        translation[14] = -camera.position[2];
        MultiplyLegacyMatrix(rotation, translation, matrix);
    }

    // Projecao equivalente a gluPerspective2(CameraFOV, aspect, CameraViewNear,
    // CameraViewFar * 1.4f) usada por BeginOpengl.
    inline void BuildLegacySceneProjection(const LegacySceneCamera& camera, float aspect, float* matrix)
    {
        BuildLegacyPerspective(camera.fieldOfViewDegrees, aspect, camera.viewNear, camera.viewFar * 1.4f, matrix);
    }

    // Posiciona a camera a uma distancia do alvo mantendo os angulos legados, do
    // mesmo modo que o gameplay orbita o heroi por CameraDistance. Resolve
    // position = target + transposta(rotacao) * (0, 0, distance), de forma que o
    // alvo caia no centro do espaco de visao.
    inline void BuildLegacyCameraPosition(const float* angles, bool topViewEnable, const float* target, float distance, float* position)
    {
        float rotation[16];
        BuildLegacyCameraRotation(angles, topViewEnable, rotation);
        position[0] = target[0] + distance * rotation[2];
        position[1] = target[1] + distance * rotation[6];
        position[2] = target[2] + distance * rotation[10];
    }
}
