// Valida o carregamento de assets reais do MU dentro do WebAssembly:
//
//   1. o caminho no estilo Windows ("Data\\World10\\...") e normalizado;
//   2. o arquivo empacotado no sistema virtual do Emscripten e encontrado;
//   3. o conteudo binario e lido e interpretado como o jogo faz.
//
// Roda em Node, sem WebGL.

#include "LegacyFileAccess.h"

#include <cstdio>
#include <cstring>
#include <cstdint>

extern "C" {
#include <stdio.h>
#include "jpeglib.h"
}

// Do modulo de terreno do jogo (source/ZzzLodTerrain.cpp).
extern bool OpenTerrainHeight(char* filename);
extern float BackTerrainHeight[256 * 256];

namespace
{
    int g_failures = 0;

    void Fail(const char* message)
    {
        std::printf("  FALHA %s\n", message);
        ++g_failures;
    }

    // Mesmo layout que o loader do terreno espera.
#pragma pack(push, 1)
    struct BitmapFileHeader
    {
        uint16_t type;
        uint32_t size;
        uint16_t reserved1;
        uint16_t reserved2;
        uint32_t offBits;
    };
    struct BitmapInfoHeader
    {
        uint32_t size;
        int32_t  width;
        int32_t  height;
        uint16_t planes;
        uint16_t bitCount;
        uint32_t compression;
        uint32_t sizeImage;
        int32_t  xPelsPerMeter;
        int32_t  yPelsPerMeter;
        uint32_t clrUsed;
        uint32_t clrImportant;
    };
#pragma pack(pop)
}

int main()
{
    std::printf("Carregamento de asset do MU em WebAssembly\n\n");

    // Caminho exatamente como o codigo legado monta: separador do Windows.
    const char* legacyPath = "Data\\World10\\TerrainHeight.OZB";
    std::printf("caminho legado : %s\n", legacyPath);

    char normalized[512];
    Platform::NormalizeLegacyPath(legacyPath, normalized, sizeof(normalized));
    std::printf("normalizado    : %s\n\n", normalized);

    FILE* file = Platform::LegacyFileOpen(legacyPath, "rb");
    if (file == NULL)
    {
        Fail("arquivo nao encontrado no sistema de arquivos virtual");
        std::printf("\n%d verificacao(oes) falharam.\n", g_failures);
        return 1;
    }
    std::printf("arquivo aberto com sucesso\n");

    std::fseek(file, 0, SEEK_END);
    long bytes = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    std::printf("tamanho        : %ld bytes\n", bytes);

    if (bytes < 4 + (long)sizeof(BitmapFileHeader) + (long)sizeof(BitmapInfoHeader))
        Fail("arquivo menor que os cabecalhos esperados");

    // O loader do terreno pula 4 bytes antes dos cabecalhos BMP.
    unsigned char skip[4];
    if (std::fread(skip, 1, 4, file) != 4) Fail("nao leu os 4 bytes iniciais");

    BitmapFileHeader fileHeader;
    BitmapInfoHeader infoHeader;
    if (std::fread(&fileHeader, sizeof(fileHeader), 1, file) != 1) Fail("nao leu BITMAPFILEHEADER");
    if (std::fread(&infoHeader, sizeof(infoHeader), 1, file) != 1) Fail("nao leu BITMAPINFOHEADER");
    std::fclose(file);

    std::printf("dimensoes      : %d x %d, %d bpp\n",
                infoHeader.width, infoHeader.height, infoHeader.bitCount);

    // O mapa de altura do MU e sempre 256x256 (TERRAIN_SIZE). A profundidade
    // varia: ZzzLodTerrain tem dois carregadores, escolhidos pelo flag bNew de
    // CreateTerrain — OpenTerrainHeightNew le 3 bytes por pixel (24 bpp) e
    // OpenTerrainHeight le o formato antigo de 8 bpp com paleta.
    if (infoHeader.width != 256)  Fail("largura diferente de 256");
    if (infoHeader.height != 256) Fail("altura diferente de 256");
    if (infoHeader.bitCount != 8 && infoHeader.bitCount != 24)
        Fail("profundidade nao e 8 nem 24 bpp");

    // Le as alturas e confere que os valores sao plausiveis, para provar que o
    // conteudo — e nao so o cabecalho — chegou intacto.
    const long headerBytes = 4 + (long)sizeof(BitmapFileHeader) + (long)sizeof(BitmapInfoHeader);
    const long paletteBytes = (infoHeader.bitCount == 8) ? 1024 : 0;
    file = Platform::LegacyFileOpen(legacyPath, "rb");
    if (file != NULL)
    {
        // Varre o mapa inteiro, nao uma linha: o World10 e quase todo plano
        // (um unico valor cobre ~95% da area) e qualquer amostra local daria
        // "sem variacao" mesmo com a leitura correta.
        const int bytesPerPixel = (infoHeader.bitCount == 8) ? 1 : 3;
        const long pixelStart = headerBytes + paletteBytes;
        std::fseek(file, pixelStart, SEEK_SET);

        static unsigned char heights[256 * 256];
        const size_t wanted = (size_t)256 * 256 * bytesPerPixel;
        size_t read = std::fread(heights, 1,
                                 wanted < sizeof(heights) ? wanted : sizeof(heights), file);
        std::fclose(file);

        if (read < (size_t)256 * 256)
        {
            Fail("nao leu o mapa de altura completo");
        }
        else
        {
            int distinct = 0;
            bool seen[256] = { false };
            unsigned lowest = 255, highest = 0;
            for (size_t i = 0; i < read; i += bytesPerPixel)
            {
                unsigned v = heights[i];
                if (!seen[v]) { seen[v] = true; ++distinct; }
                if (v < lowest) lowest = v;
                if (v > highest) highest = v;
            }
            std::printf("relevo         : %d alturas distintas (min=%u max=%u)\n",
                        distinct, lowest, highest);
            if (highest == 0) Fail("mapa de altura todo zero — leitura vazia");
            if (distinct < 2) Fail("mapa sem nenhuma variacao de relevo");
        }
    }

    // Ate aqui a leitura foi feita por codigo deste teste. Agora o carregador do
    // PROPRIO jogo: OpenTerrainHeight de ZzzLodTerrain.cpp, que preenche o array
    // global BackTerrainHeight usado pelo render do terreno.
    std::printf("\n--- carregador do jogo (ZzzLodTerrain) ---\n");
    if (!OpenTerrainHeight((char*)"World10/TerrainHeight."))
    {
        Fail("OpenTerrainHeight devolveu false");
    }
    else
    {
        int distinct = 0;
        bool seen[512] = { false };
        float lowest = 1e9f, highest = -1e9f;
        for (int i = 0; i < 256 * 256; ++i)
        {
            float h = BackTerrainHeight[i];
            if (h < lowest) lowest = h;
            if (h > highest) highest = h;
            int bucket = (int)h & 511;
            if (!seen[bucket]) { seen[bucket] = true; ++distinct; }
        }
        std::printf("BackTerrainHeight: min=%.1f max=%.1f, %d faixas distintas\n",
                    lowest, highest, distinct);
        if (highest <= lowest) Fail("terreno carregado sem variacao de altura");
        if (distinct < 2) Fail("BackTerrainHeight sem relevo");
    }

    // --- Textura .OZJ ---------------------------------------------------------
    // OZJ e um JPEG precedido de 24 bytes de cabecalho proprio do MU. Aqui o
    // decodificador (libjpeg 9a compilada para wasm) e exercitado no asset real.
    std::printf("\n--- textura OZJ (libjpeg em wasm) ---\n");
    {
        const char* texturePath = "Data\\World10\\TileGrass01.OZJ";
        FILE* texture = Platform::LegacyFileOpen(texturePath, "rb");
        if (texture == NULL)
        {
            Fail("TileGrass01.OZJ nao encontrada");
        }
        else
        {
            std::fseek(texture, 24, SEEK_SET);   // pula o cabecalho do MU

            jpeg_decompress_struct cinfo;
            jpeg_error_mgr jerr;
            cinfo.err = jpeg_std_error(&jerr);
            jpeg_create_decompress(&cinfo);
            jpeg_stdio_src(&cinfo, texture);
            jpeg_read_header(&cinfo, TRUE);
            jpeg_start_decompress(&cinfo);

            std::printf("textura        : %u x %u, %d componentes\n",
                        cinfo.output_width, cinfo.output_height, cinfo.output_components);

            const int stride = cinfo.output_width * cinfo.output_components;
            unsigned char* row = new unsigned char[stride];
            unsigned long long sum = 0;
            unsigned lines = 0;
            while (cinfo.output_scanline < cinfo.output_height)
            {
                unsigned char* rows[1] = { row };
                jpeg_read_scanlines(&cinfo, rows, 1);
                for (int i = 0; i < stride; ++i) sum += row[i];
                ++lines;
            }
            jpeg_finish_decompress(&cinfo);
            jpeg_destroy_decompress(&cinfo);
            delete[] row;
            std::fclose(texture);

            const unsigned long long samples = (unsigned long long)lines * stride;
            const double average = samples ? (double)sum / (double)samples : 0.0;
            std::printf("decodificada   : %u linhas, brilho medio %.1f\n", lines, average);

            if (lines != cinfo.output_height) Fail("nao decodificou todas as linhas");
            if (cinfo.output_width == 0) Fail("largura zero");
            // Imagem toda preta indicaria decodificacao vazia.
            if (average < 1.0) Fail("textura decodificada totalmente preta");
        }
    }

    if (g_failures == 0)
        std::printf("\nAsset real do MU lido pelo carregador do proprio jogo.\n");
    else
        std::printf("\n%d verificacao(oes) falharam.\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
