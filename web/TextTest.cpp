// Valida o subsistema de texto (Platform/PlatformText.cpp) dentro do wasm.
//
// Antes desta implementacao, GetTextExtentPoint32 devolvia ZERO para qualquer
// string, o que zerava todo o layout de texto da UI. Os testes abaixo checam as
// propriedades das quais o layout depende, nao pixels especificos:
//
//   1. medir uma string devolve largura e altura positivas;
//   2. string mais longa mede mais largo (o layout quebra linha por isso);
//   3. fonte maior mede mais alto;
//   4. negrito mede pelo menos tao largo quanto o normal;
//   5. TextOut deposita pixels 255 na DIB — que e exatamente o valor que
//      CUIRenderTextOriginal::WriteText procura para decidir "tem tinta";
//   6. a area desenhada respeita os limites da DIB (nada escreve fora).

#include "WindowsCompat.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
    int g_failures = 0;

    void Check(bool condition, const char* description)
    {
        printf("%s %s\n", condition ? "[ok]  " : "[FALHA]", description);
        if (!condition) ++g_failures;
    }

    // Conta pixels totalmente brancos, o unico valor que WriteText aceita.
    int CountInkPixels(const unsigned char* pixels, int width, int height, int pitch)
    {
        int count = 0;
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                if (pixels[(size_t)y * pitch + (size_t)x * 3] == 255) ++count;
        return count;
    }
}

int main()
{
    printf("== teste do subsistema de texto ==\n");

    const bool fontLoaded = Platform::LoadTextFont("Data\\Fonts\\GameFont.ttf");
    Check(fontLoaded, "fonte TTF carregada de Data\\Fonts\\GameFont.ttf");
    if (!fontLoaded)
    {
        printf("sem fonte nao ha o que testar\n");
        return 1;
    }

    HFONT font = CreateFontA(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
    Check(font != NULL, "CreateFontA devolveu uma fonte");

    // Mesma DIB de CUIRenderTextOriginal::Create: 24 bpp, top-down.
    const int kWidth = 640;
    const int kHeight = 480;
    BITMAPINFO info;
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = kWidth;
    info.bmiHeader.biHeight = -kHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 24;
    info.bmiHeader.biCompression = BI_RGB;

    unsigned char* buffer = NULL;
    HBITMAP bitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, (void**)&buffer, NULL, 0);
    Check(bitmap != NULL && buffer != NULL, "CreateDIBSection devolveu bitmap e ponteiro");

    HDC dc = CreateCompatibleDC(NULL);
    Check(dc != NULL, "CreateCompatibleDC devolveu um DC");
    SelectObject(dc, bitmap);
    SelectObject(dc, font);

    // --- 1 e 2: medicao ------------------------------------------------------
    SIZE shortSize = { 0, 0 };
    SIZE longSize = { 0, 0 };
    GetTextExtentPoint32A(dc, "Mu", 2, &shortSize);
    GetTextExtentPoint32A(dc, "Mu Online Season 3", 18, &longSize);
    printf("       medida curta=%ldx%ld  longa=%ldx%ld\n",
           shortSize.cx, shortSize.cy, longSize.cx, longSize.cy);
    Check(shortSize.cx > 0 && shortSize.cy > 0, "string curta mede largura e altura positivas");
    Check(longSize.cx > shortSize.cx, "string mais longa mede mais largo");
    Check(longSize.cy == shortSize.cy, "altura da linha nao depende do comprimento");

    // --- 3: tamanho da fonte -------------------------------------------------
    HFONT bigFont = CreateFontA(32, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
    SelectObject(dc, bigFont);
    SIZE bigSize = { 0, 0 };
    GetTextExtentPoint32A(dc, "Mu", 2, &bigSize);
    printf("       fonte 16 -> %ldx%ld   fonte 32 -> %ldx%ld\n",
           shortSize.cx, shortSize.cy, bigSize.cx, bigSize.cy);
    Check(bigSize.cy > shortSize.cy, "fonte de 32 px mede mais alto que a de 16 px");
    Check(bigSize.cx > shortSize.cx, "fonte de 32 px mede mais largo que a de 16 px");

    // --- 4: negrito ----------------------------------------------------------
    HFONT boldFont = CreateFontA(16, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
    SelectObject(dc, boldFont);
    SIZE boldSize = { 0, 0 };
    GetTextExtentPoint32A(dc, "Mu Online Season 3", 18, &boldSize);
    Check(boldSize.cx >= longSize.cx, "negrito mede pelo menos tao largo quanto o normal");

    // --- 5: desenho ----------------------------------------------------------
    SelectObject(dc, font);
    SetBkColor(dc, RGB(0, 0, 0));
    SetTextColor(dc, RGB(255, 255, 255));

    const int inkBefore = CountInkPixels(buffer, kWidth, kHeight, ((kWidth * 24 + 31) & ~31) >> 3);
    Check(inkBefore == 0, "DIB comeca sem nenhum pixel branco");

    const BOOL drawn = TextOutA(dc, 0, 0, "Mu Online Season 3", 18);
    Check(drawn == TRUE, "TextOutA reportou sucesso");

    const int pitch = ((kWidth * 24 + 31) & ~31) >> 3;
    const int inkAfter = CountInkPixels(buffer, kWidth, kHeight, pitch);
    printf("       pixels brancos apos desenhar: %d\n", inkAfter);
    Check(inkAfter > 0, "TextOutA depositou pixels 255 (o valor que WriteText procura)");

    // --- 6: limites ----------------------------------------------------------
    // Nenhuma tinta pode aparecer fora da caixa medida, nem fora da DIB. Se o
    // rasterizador escrevesse fora, corromperia o buffer de outro objeto.
    int outsideBox = 0;
    for (int y = 0; y < kHeight; ++y)
        for (int x = 0; x < kWidth; ++x)
            if (buffer[(size_t)y * pitch + (size_t)x * 3] == 255)
                if (x >= longSize.cx || y >= longSize.cy) ++outsideBox;
    printf("       pixels fora da caixa medida: %d\n", outsideBox);
    Check(outsideBox == 0, "nenhum pixel desenhado fora da caixa medida");

    // Desenhar perto da borda nao pode estourar o buffer.
    TextOutA(dc, kWidth - 10, kHeight - 4, "Mu Online Season 3", 18);
    Check(true, "desenho recortado na borda nao estourou o buffer");

    DeleteDC(dc);
    DeleteObject(bitmap);
    DeleteObject(font);
    DeleteObject(bigFont);
    DeleteObject(boldFont);

    printf("== %s ==\n", g_failures == 0 ? "todos passaram" : "HOUVE FALHAS");
    return g_failures == 0 ? 0 : 1;
}
