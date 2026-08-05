// Subsistema de texto multiplataforma: reimplementa, sobre stb_truetype, o
// subconjunto do GDI que o cliente usa para medir e desenhar texto.
//
// Por que reimplementar GDI em vez de trocar o codigo de UI: as metricas de
// texto atravessam praticamente toda a interface (mais de 200 pontos de chamada
// a _GetTextExtentPoint32, em ~40 arquivos), e todas passam por dois pontos
// unicos — GetTextExtentPoint32W e TextOutW. Reproduzir esses dois pontos custa
// muito menos e nao arrisca a aparencia da UI no Windows, onde nada muda.
//
// O caminho reproduzido e o de CUIRenderTextOriginal:
//
//   Create()  cria uma DIB de 24 bpp top-down e um DC de memoria;
//   RenderText() mede a string, pinta branco sobre preto na DIB via TextOutW,
//   WriteText()  le a DIB e trata pixel == 255 como "tem tinta".
//
// Como WriteText compara com 255 exato, a rasterizacao aqui e binaria (limiar),
// nao suavizada — que e justamente o NONANTIALIASED_QUALITY pedido em
// CreateFont.cpp. Suavizar produziria bordas descartadas e texto esgarcado.

#if !defined(_WIN32)

#include "WindowsCompat.h"
#include "LegacyFileAccess.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace
{
    // ---- Fonte -------------------------------------------------------------

    // Conteudo do TTF. stb_truetype nao copia o buffer: ele indexa direto, entao
    // este vetor precisa viver enquanto qualquer stbtt_fontinfo existir.
    std::vector<unsigned char> g_fontFile;
    stbtt_fontinfo g_fontInfo;
    bool g_fontReady = false;

    struct PlatformFont
    {
        int   pixelHeight;   // altura pedida, na convencao do GDI (celula inteira)
        float scale;         // fator do stb_truetype para essa altura
        int   ascent;        // ja escalado, em pixels
        int   descent;
        int   lineGap;
        bool  bold;
        bool  italic;
    };

    struct PlatformBitmap
    {
        std::vector<unsigned char> pixels; // 24 bpp, BGR, top-down
        int width;
        int height;
        int pitch;
    };

    struct PlatformDc
    {
        PlatformFont*   font;
        PlatformBitmap* bitmap;
        COLORREF textColor;
        COLORREF backColor;
    };

    // Recursivo de proposito: as funcoes publicas travam e depois chamam umas as
    // outras (TextOutW -> medicao, CreateFontA -> LoadTextFont). Com um mutex
    // simples a segunda trava do mesmo thread seria deadlock.
    std::recursive_mutex g_textMutex;

    // SelectObject/DeleteObject recebem um void* e precisam saber se e fonte ou
    // bitmap. O GDI resolve pelo tipo interno do handle; aqui cada objeto criado
    // se registra por tipo.
    const int kKindFont = 1;
    const int kKindBitmap = 2;

    std::map<void*, int>& ObjectKinds()
    {
        static std::map<void*, int> kinds;
        return kinds;
    }

    bool ReadWholeFile(const char* path, std::vector<unsigned char>& out)
    {
        FILE* file = Platform::LegacyFileOpen(path, "rb");
        if (file == NULL) return false;
        if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return false; }
        long size = ftell(file);
        if (size <= 0) { fclose(file); return false; }
        rewind(file);
        out.resize((size_t)size);
        size_t read = fread(&out[0], 1, (size_t)size, file);
        fclose(file);
        if (read != (size_t)size) { out.clear(); return false; }
        return true;
    }

    // Ordem de busca quando nenhum caminho e informado. Data/Fonts/GameFont.ttf e
    // o que o empacotador do Web inclui; os demais existem no Android.
    const char* const kFontCandidates[] = {
        "Data\\Fonts\\GameFont.ttf",
        "Data/Fonts/GameFont.ttf",
        "/system/fonts/Roboto-Regular.ttf",
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/DroidSans.ttf",
    };
}

namespace Platform
{
    bool LoadTextFont(const char* path)
    {
        std::lock_guard<std::recursive_mutex> guard(g_textMutex);
        if (g_fontReady) return true;

        std::vector<unsigned char> data;
        bool loaded = false;
        if (path != NULL && path[0] != '\0')
            loaded = ReadWholeFile(path, data);
        else
        {
            const size_t count = sizeof(kFontCandidates) / sizeof(kFontCandidates[0]);
            for (size_t i = 0; i < count && !loaded; ++i)
                loaded = ReadWholeFile(kFontCandidates[i], data);
        }
        if (!loaded)
        {
            fprintf(stderr, "[Platform] nenhuma fonte TTF encontrada; o texto ficara sem glifos\n");
            return false;
        }

        g_fontFile.swap(data);
        const int offset = stbtt_GetFontOffsetForIndex(&g_fontFile[0], 0);
        if (offset < 0 || !stbtt_InitFont(&g_fontInfo, &g_fontFile[0], offset))
        {
            fprintf(stderr, "[Platform] stbtt_InitFont falhou; arquivo de fonte invalido\n");
            g_fontFile.clear();
            return false;
        }
        g_fontReady = true;
        return true;
    }

    bool IsTextFontLoaded()
    {
        std::lock_guard<std::recursive_mutex> guard(g_textMutex);
        return g_fontReady;
    }
}

// ---- Objetos GDI -----------------------------------------------------------

HFONT CreateFontA(int height, int, int, int, int weight, DWORD italic, DWORD,
                  DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCSTR)
{
    // O nome da face e ignorado: fora do Windows nao ha enumeracao de fontes do
    // sistema por nome, e o cliente pede faces que so existem no Windows
    // ("Tahoma", "Gulim"). Todas as faces caem na fonte carregada; peso e
    // italico ainda sao respeitados por sintese.
    if (!Platform::IsTextFontLoaded() && !Platform::LoadTextFont(NULL))
        return NULL;

    std::lock_guard<std::recursive_mutex> guard(g_textMutex);
    PlatformFont* font = new PlatformFont();
    // O GDI interpreta altura negativa como altura de celula; o cliente passa
    // valores positivos, mas normalizar evita uma fonte de altura zero.
    font->pixelHeight = height < 0 ? -height : height;
    if (font->pixelHeight <= 0) font->pixelHeight = 12;
    font->bold = weight >= FW_BOLD;
    font->italic = italic != 0;

    font->scale = stbtt_ScaleForPixelHeight(&g_fontInfo, (float)font->pixelHeight);
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&g_fontInfo, &ascent, &descent, &lineGap);
    font->ascent  = (int)(ascent  * font->scale + 0.5f);
    font->descent = (int)(-descent * font->scale + 0.5f);
    font->lineGap = (int)(lineGap * font->scale + 0.5f);
    ObjectKinds()[font] = kKindFont;
    return (HFONT)font;
}

HDC CreateCompatibleDC(HDC)
{
    PlatformDc* dc = new PlatformDc();
    dc->font = NULL;
    dc->bitmap = NULL;
    dc->textColor = RGB(255, 255, 255);
    dc->backColor = RGB(0, 0, 0);
    return (HDC)dc;
}

HBITMAP CreateDIBSection(HDC, const BITMAPINFO* info, UINT, void** bits, HANDLE, DWORD)
{
    if (info == NULL) return NULL;
    std::lock_guard<std::recursive_mutex> guard(g_textMutex);
    PlatformBitmap* bitmap = new PlatformBitmap();
    bitmap->width = (int)info->bmiHeader.biWidth;
    // biHeight negativo = top-down, que e como CUIRenderTextOriginal cria a DIB.
    // O sinal so define a origem; o buffer aqui e sempre top-down.
    long declaredHeight = (long)info->bmiHeader.biHeight;
    bitmap->height = (int)(declaredHeight < 0 ? -declaredHeight : declaredHeight);
    if (bitmap->width <= 0 || bitmap->height <= 0)
    {
        delete bitmap;
        return NULL;
    }
    // Mesmo alinhamento de linha do GDI: multiplo de 4 bytes.
    bitmap->pitch = ((bitmap->width * 24 + 31) & ~31) >> 3;
    bitmap->pixels.assign((size_t)bitmap->pitch * (size_t)bitmap->height, 0);
    if (bits != NULL) *bits = &bitmap->pixels[0];
    ObjectKinds()[bitmap] = kKindBitmap;
    return (HBITMAP)bitmap;
}

HGDIOBJ SelectObject(HDC dcHandle, HGDIOBJ object)
{
    if (dcHandle == NULL || object == NULL) return NULL;
    std::lock_guard<std::recursive_mutex> guard(g_textMutex);
    PlatformDc* dc = (PlatformDc*)dcHandle;
    std::map<void*, int>& kinds = ObjectKinds();
    std::map<void*, int>::const_iterator found = kinds.find(object);
    if (found == kinds.end()) return NULL;
    if (found->second == kKindFont)
    {
        HGDIOBJ previous = (HGDIOBJ)dc->font;
        dc->font = (PlatformFont*)object;
        return previous;
    }
    HGDIOBJ previous = (HGDIOBJ)dc->bitmap;
    dc->bitmap = (PlatformBitmap*)object;
    return previous;
}

BOOL DeleteObject(HGDIOBJ object)
{
    if (object == NULL) return FALSE;
    std::lock_guard<std::recursive_mutex> guard(g_textMutex);
    std::map<void*, int>& kinds = ObjectKinds();
    std::map<void*, int>::iterator found = kinds.find(object);
    if (found == kinds.end()) return FALSE;
    if (found->second == kKindFont) delete (PlatformFont*)object;
    else delete (PlatformBitmap*)object;
    kinds.erase(found);
    return TRUE;
}

BOOL DeleteDC(HDC dcHandle)
{
    if (dcHandle == NULL) return FALSE;
    delete (PlatformDc*)dcHandle;
    return TRUE;
}

COLORREF SetTextColor(HDC dcHandle, COLORREF color)
{
    if (dcHandle == NULL) return 0;
    PlatformDc* dc = (PlatformDc*)dcHandle;
    COLORREF previous = dc->textColor;
    dc->textColor = color;
    return previous;
}

COLORREF SetBkColor(HDC dcHandle, COLORREF color)
{
    if (dcHandle == NULL) return 0;
    PlatformDc* dc = (PlatformDc*)dcHandle;
    COLORREF previous = dc->backColor;
    dc->backColor = color;
    return previous;
}

// ---- Medicao e desenho -----------------------------------------------------

namespace
{
    // Avanco horizontal de um codepoint, ja com kerning em relacao ao anterior.
    int AdvanceFor(const PlatformFont& font, int codepoint, int previousCodepoint)
    {
        int advance = 0, leftBearing = 0;
        stbtt_GetCodepointHMetrics(&g_fontInfo, codepoint, &advance, &leftBearing);
        float total = advance * font.scale;
        if (previousCodepoint != 0)
            total += stbtt_GetCodepointKernAdvance(&g_fontInfo, previousCodepoint, codepoint) * font.scale;
        // Negrito sintetico alarga o glifo em um pixel, entao o avanco acompanha.
        if (font.bold) total += 1.0f;
        return (int)(total + 0.5f);
    }

    bool ComputeTextExtent(const PlatformDc* dc, const wchar_t* text, int length, LPSIZE size)
    {
        if (size == NULL) return FALSE;
        size->cx = 0;
        size->cy = 0;
        if (dc == NULL || dc->font == NULL || text == NULL || !Platform::IsTextFontLoaded())
            return FALSE;

        const PlatformFont& font = *dc->font;
        int width = 0;
        int previous = 0;
        for (int i = 0; i < length && text[i] != 0; ++i)
        {
            const int codepoint = (int)text[i];
            width += AdvanceFor(font, codepoint, previous);
            previous = codepoint;
        }
        size->cx = width;
        // Altura da linha na convencao do GDI: ascent + descent, sem lineGap.
        // E o que tmHeight devolve e o que o layout do cliente espera.
        size->cy = font.ascent + font.descent;
        return TRUE;
    }

    void DrawGlyph(PlatformBitmap& bitmap, const PlatformFont& font, int codepoint,
                   int penX, int baselineY, COLORREF color)
    {
        int glyphWidth = 0, glyphHeight = 0, offsetX = 0, offsetY = 0;
        unsigned char* coverage = stbtt_GetCodepointBitmap(
            &g_fontInfo, 0, font.scale, codepoint,
            &glyphWidth, &glyphHeight, &offsetX, &offsetY);
        if (coverage == NULL) return;

        const unsigned char blue  = GetBValue(color);
        const unsigned char green = GetGValue(color);
        const unsigned char red   = GetRValue(color);

        // Negrito sintetico: mesma cobertura desenhada tambem um pixel a direita.
        const int passes = font.bold ? 2 : 1;
        for (int pass = 0; pass < passes; ++pass)
        {
            for (int y = 0; y < glyphHeight; ++y)
            {
                const int targetY = baselineY + offsetY + y;
                if (targetY < 0 || targetY >= bitmap.height) continue;
                for (int x = 0; x < glyphWidth; ++x)
                {
                    // Limiar em vez de mistura: WriteText so aceita 255 exato.
                    if (coverage[y * glyphWidth + x] < 128) continue;
                    int targetX = penX + offsetX + x + pass;
                    // Italico sintetico: inclinacao de ~12 graus em relacao a base.
                    if (font.italic)
                        targetX += (int)((float)(font.ascent - (offsetY + y)) * 0.2f);
                    if (targetX < 0 || targetX >= bitmap.width) continue;
                    unsigned char* pixel = &bitmap.pixels[(size_t)targetY * bitmap.pitch + (size_t)targetX * 3];
                    pixel[0] = blue;
                    pixel[1] = green;
                    pixel[2] = red;
                }
            }
        }
        stbtt_FreeBitmap(coverage, 0);
    }

    BOOL RasterizeText(PlatformDc* dc, int x, int y, const wchar_t* text, int length)
    {
        if (dc == NULL || dc->font == NULL || dc->bitmap == NULL || text == NULL)
            return FALSE;
        if (!Platform::IsTextFontLoaded()) return FALSE;

        const PlatformFont& font = *dc->font;
        PlatformBitmap& bitmap = *dc->bitmap;

        // O GDI pinta o retangulo de fundo antes dos glifos (modo OPAQUE, que e o
        // padrao). CUIRenderTextOriginal conta com isso: a area precisa ficar
        // limpa, senao sobra o texto do quadro anterior.
        SIZE measured = { 0, 0 };
        ComputeTextExtent(dc, text, length, &measured);
        const unsigned char backBlue  = GetBValue(dc->backColor);
        const unsigned char backGreen = GetGValue(dc->backColor);
        const unsigned char backRed   = GetRValue(dc->backColor);
        for (int row = 0; row < measured.cy; ++row)
        {
            const int targetY = y + row;
            if (targetY < 0 || targetY >= bitmap.height) continue;
            for (int column = 0; column < measured.cx; ++column)
            {
                const int targetX = x + column;
                if (targetX < 0 || targetX >= bitmap.width) continue;
                unsigned char* pixel = &bitmap.pixels[(size_t)targetY * bitmap.pitch + (size_t)targetX * 3];
                pixel[0] = backBlue;
                pixel[1] = backGreen;
                pixel[2] = backRed;
            }
        }

        int penX = x;
        int previous = 0;
        const int baseline = y + font.ascent;
        for (int i = 0; i < length && text[i] != 0; ++i)
        {
            const int codepoint = (int)text[i];
            if (previous != 0)
                penX += (int)(stbtt_GetCodepointKernAdvance(&g_fontInfo, previous, codepoint) * font.scale + 0.5f);
            DrawGlyph(bitmap, font, codepoint, penX, baseline, dc->textColor);
            int advance = 0, leftBearing = 0;
            stbtt_GetCodepointHMetrics(&g_fontInfo, codepoint, &advance, &leftBearing);
            penX += (int)(advance * font.scale + (font.bold ? 1.0f : 0.0f) + 0.5f);
            previous = codepoint;
        }
        return TRUE;
    }

    // Converte a forma estreita para wchar_t antes de medir/desenhar, do mesmo
    // jeito que CMultiLanguage faz no Windows.
    std::wstring WidenLatin(const char* text, int length)
    {
        std::wstring wide;
        if (text == NULL) return wide;
        for (int i = 0; i < length && text[i] != '\0'; ++i)
            wide.push_back((wchar_t)(unsigned char)text[i]);
        return wide;
    }
}

BOOL TextOutW(HDC dcHandle, int x, int y, LPCWSTR text, int length)
{
    std::lock_guard<std::recursive_mutex> guard(g_textMutex);
    return RasterizeText((PlatformDc*)dcHandle, x, y, text, length);
}

BOOL TextOutA(HDC dcHandle, int x, int y, LPCSTR text, int length)
{
    std::wstring wide = WidenLatin(text, length);
    std::lock_guard<std::recursive_mutex> guard(g_textMutex);
    return RasterizeText((PlatformDc*)dcHandle, x, y, wide.c_str(), (int)wide.size());
}

namespace Platform
{
    // Pinta a DIB inteira com a cor de fundo do DC.
    //
    // Serve ao WM_ERASEBKGND do controle EDIT sintetico (PlatformEditControl.cpp).
    // RasterizeText limpa apenas o retangulo da propria string, entao quando o texto
    // ENCURTA -- uma tecla de apagar -- sobrariam os glifos do quadro anterior a
    // direita do novo fim. O EDIT do Windows apaga o fundo antes de pintar; isto
    // reproduz esse passo.
    void LimparSuperficieDoDc(HDC dcHandle)
    {
        std::lock_guard<std::recursive_mutex> guard(g_textMutex);
        PlatformDc* dc = (PlatformDc*)dcHandle;
        if (dc == NULL || dc->bitmap == NULL) return;
        PlatformBitmap& bitmap = *dc->bitmap;
        const unsigned char canais[3] = { GetBValue(dc->backColor),
                                          GetGValue(dc->backColor),
                                          GetRValue(dc->backColor) };
        // Uma cor cinzenta pediria preenchimento por canal; as cores usadas aqui
        // (preto/branco) tem os tres canais iguais, entao o caso comum e um memset.
        if (canais[0] == canais[1] && canais[1] == canais[2])
        {
            if (!bitmap.pixels.empty())
                memset(&bitmap.pixels[0], canais[0], bitmap.pixels.size());
            return;
        }
        for (int y = 0; y < bitmap.height; ++y)
            for (int x = 0; x < bitmap.width; ++x)
            {
                unsigned char* pixel = &bitmap.pixels[(size_t)y * bitmap.pitch + (size_t)x * 3];
                pixel[0] = canais[0];
                pixel[1] = canais[1];
                pixel[2] = canais[2];
            }
    }
}

BOOL GetTextExtentPoint32W(HDC dcHandle, LPCWSTR text, int length, LPSIZE size)
{
    std::lock_guard<std::recursive_mutex> guard(g_textMutex);
    return ComputeTextExtent((const PlatformDc*)dcHandle, text, length, size);
}

BOOL GetTextExtentPoint32A(HDC dcHandle, LPCSTR text, int length, LPSIZE size)
{
    std::wstring wide = WidenLatin(text, length);
    std::lock_guard<std::recursive_mutex> guard(g_textMutex);
    return ComputeTextExtent((const PlatformDc*)dcHandle, wide.c_str(), (int)wide.size(), size);
}

BOOL GetTextExtentPointA(HDC dcHandle, LPCSTR text, int length, LPSIZE size)
{
    return GetTextExtentPoint32A(dcHandle, text, length, size);
}

// ---- Conversao de code page ------------------------------------------------

namespace
{
    // Decodifica um caractere UTF-8. Devolve o numero de bytes consumidos, ou 0
    // quando a sequencia e invalida.
    int DecodeUtf8(const unsigned char* source, int available, unsigned int& codepoint)
    {
        if (available <= 0) return 0;
        const unsigned char first = source[0];
        if (first < 0x80) { codepoint = first; return 1; }
        if ((first & 0xE0) == 0xC0 && available >= 2 && (source[1] & 0xC0) == 0x80)
        {
            codepoint = ((unsigned int)(first & 0x1F) << 6) | (source[1] & 0x3F);
            return 2;
        }
        if ((first & 0xF0) == 0xE0 && available >= 3 &&
            (source[1] & 0xC0) == 0x80 && (source[2] & 0xC0) == 0x80)
        {
            codepoint = ((unsigned int)(first & 0x0F) << 12) |
                        ((unsigned int)(source[1] & 0x3F) << 6) | (source[2] & 0x3F);
            return 3;
        }
        if ((first & 0xF8) == 0xF0 && available >= 4 &&
            (source[1] & 0xC0) == 0x80 && (source[2] & 0xC0) == 0x80 && (source[3] & 0xC0) == 0x80)
        {
            codepoint = ((unsigned int)(first & 0x07) << 18) |
                        ((unsigned int)(source[1] & 0x3F) << 12) |
                        ((unsigned int)(source[2] & 0x3F) << 6) | (source[3] & 0x3F);
            return 4;
        }
        return 0;
    }
}

int MultiByteToWideChar(UINT codePage, DWORD, LPCSTR source, int sourceLength,
                        LPWSTR destination, int destinationLength)
{
    if (source == NULL) return 0;
    // sourceLength == -1 significa "string terminada em nulo, inclua o nulo".
    const bool nullTerminated = (sourceLength < 0);
    const int available = nullTerminated ? (int)strlen(source) : sourceLength;

    std::vector<wchar_t> converted;
    const unsigned char* bytes = (const unsigned char*)source;
    int index = 0;
    while (index < available)
    {
        unsigned int codepoint = 0;
        int consumed = 0;
        if (codePage == CP_UTF8)
            consumed = DecodeUtf8(bytes + index, available - index, codepoint);
        if (consumed == 0)
        {
            // Latin-1, e tambem a saida para UTF-8 malformado: byte vira
            // codepoint. Nunca perde dado nem trava o laco.
            codepoint = bytes[index];
            consumed = 1;
        }
        converted.push_back((wchar_t)codepoint);
        index += consumed;
    }
    if (nullTerminated) converted.push_back(L'\0');

    const int needed = (int)converted.size();
    if (destination == NULL || destinationLength == 0) return needed;
    if (destinationLength < needed) return 0;
    for (int i = 0; i < needed; ++i) destination[i] = converted[i];
    return needed;
}

int WideCharToMultiByte(UINT, DWORD, LPCWSTR source, int sourceLength,
                        LPSTR destination, int destinationLength, LPCSTR, BOOL*)
{
    if (source == NULL) return 0;
    const bool nullTerminated = (sourceLength < 0);
    int available = sourceLength;
    if (nullTerminated)
    {
        available = 0;
        while (source[available] != 0) ++available;
    }

    // Sempre emite UTF-8: e o que o restante do cliente ja trata (IsCharUTF8).
    std::string converted;
    for (int i = 0; i < available; ++i)
    {
        const unsigned int codepoint = (unsigned int)source[i];
        if (codepoint < 0x80)
            converted.push_back((char)codepoint);
        else if (codepoint < 0x800)
        {
            converted.push_back((char)(0xC0 | (codepoint >> 6)));
            converted.push_back((char)(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint < 0x10000)
        {
            converted.push_back((char)(0xE0 | (codepoint >> 12)));
            converted.push_back((char)(0x80 | ((codepoint >> 6) & 0x3F)));
            converted.push_back((char)(0x80 | (codepoint & 0x3F)));
        }
        else
        {
            converted.push_back((char)(0xF0 | (codepoint >> 18)));
            converted.push_back((char)(0x80 | ((codepoint >> 12) & 0x3F)));
            converted.push_back((char)(0x80 | ((codepoint >> 6) & 0x3F)));
            converted.push_back((char)(0x80 | (codepoint & 0x3F)));
        }
    }
    if (nullTerminated) converted.push_back('\0');

    const int needed = (int)converted.size();
    if (destination == NULL || destinationLength == 0) return needed;
    if (destinationLength < needed) return 0;
    memcpy(destination, converted.data(), (size_t)needed);
    return needed;
}

// ---- Secao critica ---------------------------------------------------------

void InitializeCriticalSection(LPCRITICAL_SECTION section)
{
    if (section == NULL) return;
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    // Recursivo: a secao critica do Win32 permite que a mesma thread entre varias
    // vezes, e o codigo legado conta com isso.
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&section->mutex, &attributes);
    pthread_mutexattr_destroy(&attributes);
    section->initialized = 1;
}

void DeleteCriticalSection(LPCRITICAL_SECTION section)
{
    if (section == NULL || section->initialized == 0) return;
    section->initialized = 0;
    // Destruir um mutex que alguem ainda segura ABORTA no bionic
    // ("destroying mutex with owner or contenders"), enquanto o Win32 apenas
    // ignora. O codigo legado copia objetos que contem CRITICAL_SECTION, entao
    // isso acontece de verdade. O trylock confirma que esta livre antes de
    // destruir; se nao estiver, a struct e abandonada — como ela vive embutida no
    // objeto, nao ha vazamento de heap.
    if (pthread_mutex_trylock(&section->mutex) == 0)
    {
        pthread_mutex_unlock(&section->mutex);
        pthread_mutex_destroy(&section->mutex);
    }
}

void EnterCriticalSection(LPCRITICAL_SECTION section)
{
    if (section == NULL || section->initialized == 0) return;
    pthread_mutex_lock(&section->mutex);
}

void LeaveCriticalSection(LPCRITICAL_SECTION section)
{
    if (section == NULL || section->initialized == 0) return;
    pthread_mutex_unlock(&section->mutex);
}

#endif // !_WIN32
