#pragma once

// Espelho em CPU das operacoes de matriz do pipeline fixo, para o alvo Windows.
//
// POR QUE ISTO EXISTE
//
// O cliente monta projecao e modelview com glMatrixMode/glPushMatrix/glRotatef e
// depois LE AS MATRIZES DE VOLTA com glGetFloatv, para entregar ao adapter GLSL.
// glGetFloatv e uma consulta sincrona: o driver precisa drenar o pipeline antes
// de responder. A medicao (RENDER_INSTANCING_CACHE_BATCHING_PLAN.md, secao 0.2)
// atribuiu 4.017 us por frame a seis dessas leituras, num frame de 9.253 us cujo
// trabalho de GPU e 6.597 us. Sem os pontos de sincronizacao, CPU e GPU se
// sobrepoem e o frame tende ao maior dos dois — cerca de 2.650 us a menos.
//
// COMO
//
// Cada operacao de matriz passa a atualizar TAMBEM uma pilha em CPU, que e a
// mesma ja usada em producao pelo alvo Web (Platform/LegacyMatrixStack). O GL
// continua recebendo a chamada real, para o caminho de funcao fixa seguir
// valido quando o backend GLSL esta desligado. As leituras entao vem da pilha em
// CPU em vez do driver.
//
// As macros sao objeto, nao funcao, e o codigo nao usa nenhuma destas com
// qualificacao `::gl...` nem toma o endereco delas — verificado antes de
// introduzi-las.
//
// ATENCAO: este cabecalho e incluido pelo stdafx.h, logo chega em TODO arquivo
// do projeto, inclusive em LegacyMatrixMirror.cpp. La as macros sao desfeitas
// com #undef; sem isso cada wrapper chamaria a si mesmo. Qualquer outro arquivo
// que precise da funcao real do GL tem que fazer o mesmo.

#ifdef _WIN32

namespace Platform
{
    void MirrorMatrixMode(unsigned int mode);
    void MirrorPushMatrix();
    void MirrorPopMatrix();
    void MirrorLoadIdentity();
    void MirrorLoadMatrixf(const float* matrix);
    void MirrorMultMatrixf(const float* matrix);
    void MirrorTranslatef(float x, float y, float z);
    void MirrorRotatef(float degrees, float x, float y, float z);
    void MirrorScalef(float x, float y, float z);
    void MirrorPerspective(float fieldOfViewDegrees, float aspect, float nearPlane, float farPlane);
    // gluOrtho2D instala a projecao da UI em BeginBitmap. Sem espelha-la, o
    // espelho ficaria com a perspectiva enquanto o GL usa a ortografica.
    void MirrorOrtho2D(float left, float right, float bottom, float top);

    // Projecao e modelview correntes segundo o espelho, no layout coluna-maior
    // do GL. Substituem o par de glGetFloatv.
    const float* MirrorGetProjection();
    const float* MirrorGetModelView();
    // Verdadeiro quando o espelho recebeu pelo menos uma operacao e portanto
    // descreve um estado real. Antes disso o leitor precisa usar o driver.
    bool MirrorIsPrimed();
}

#define glMatrixMode   Platform::MirrorMatrixMode
#define glPushMatrix   Platform::MirrorPushMatrix
#define glPopMatrix    Platform::MirrorPopMatrix
#define glLoadIdentity Platform::MirrorLoadIdentity
#define glLoadMatrixf  Platform::MirrorLoadMatrixf
#define glMultMatrixf  Platform::MirrorMultMatrixf
#define glTranslatef   Platform::MirrorTranslatef
#define glRotatef      Platform::MirrorRotatef
#define glScalef       Platform::MirrorScalef
#define gluPerspective Platform::MirrorPerspective
#define gluOrtho2D     Platform::MirrorOrtho2D

#endif // _WIN32
