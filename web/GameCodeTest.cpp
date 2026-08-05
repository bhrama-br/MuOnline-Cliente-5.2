// Prova que o codigo de jogo migrado nao apenas compila e linka, mas EXECUTA em
// WebAssembly. Roda em Node (sem WebGL), comparando o resultado da matematica do
// proprio jogo contra valores conhecidos.
//
// Sem um teste assim, o link "fecha" mas o linker descarta a biblioteca inteira
// por falta de referencia, e nada e realmente exercitado.

#include <cstdio>
#include <cmath>

typedef float vec3_t[3];

extern "C" {
    void AngleMatrix(const vec3_t angles, float matrix[3][4]);
    void VectorRotate(const vec3_t in1, const float in2[3][4], vec3_t out);
    void AngleIMatrix(const vec3_t angles, float matrix[3][4]);
}

static int g_failures = 0;

static void Check(const char* label, float actual, float expected, float tolerance)
{
    if (std::fabs(actual - expected) > tolerance)
    {
        std::printf("  FALHA %-34s esperado %8.4f, obtido %8.4f\n", label, expected, actual);
        ++g_failures;
    }
}

int main()
{
    std::printf("Codigo de jogo executando em WebAssembly\n");
    std::printf("(ZzzMathLib.cpp compilado do fonte legado)\n\n");

    // Rotacao de 90 graus em Z: (10,0,0) deve virar (0,10,0).
    {
        std::printf("AngleMatrix + VectorRotate, 90 graus em Z:\n");
        vec3_t angles = { 0.f, 0.f, 90.f };
        float matrix[3][4];
        AngleMatrix(angles, matrix);

        vec3_t in = { 10.f, 0.f, 0.f };
        vec3_t out;
        VectorRotate(in, matrix, out);
        Check("x", out[0], 0.f, 1e-3f);
        Check("y", out[1], 10.f, 1e-3f);
        Check("z", out[2], 0.f, 1e-3f);
    }

    // Rotacao nula preserva o vetor.
    {
        std::printf("rotacao de 0 grau preserva o vetor:\n");
        vec3_t angles = { 0.f, 0.f, 0.f };
        float matrix[3][4];
        AngleMatrix(angles, matrix);

        vec3_t in = { 3.f, -7.f, 11.f };
        vec3_t out;
        VectorRotate(in, matrix, out);
        Check("x", out[0], 3.f, 1e-4f);
        Check("y", out[1], -7.f, 1e-4f);
        Check("z", out[2], 11.f, 1e-4f);
    }

    // AngleIMatrix e a transposta da parte rotacional de AngleMatrix.
    {
        std::printf("AngleIMatrix e a inversa de AngleMatrix:\n");
        vec3_t angles = { 15.f, 30.f, 45.f };
        float forward[3][4];
        float inverse[3][4];
        AngleMatrix(angles, forward);
        AngleIMatrix(angles, inverse);

        vec3_t in = { 1.f, 2.f, 3.f };
        vec3_t mid, back;
        VectorRotate(in, forward, mid);
        VectorRotate(mid, inverse, back);
        Check("ida e volta x", back[0], in[0], 1e-3f);
        Check("ida e volta y", back[1], in[1], 1e-3f);
        Check("ida e volta z", back[2], in[2], 1e-3f);
    }

    if (g_failures == 0)
        std::printf("\nTodas as verificacoes passaram.\n");
    else
        std::printf("\n%d verificacao(oes) falharam.\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
