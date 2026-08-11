// Ligacao do Web com a subida de cena compartilhada.
//
// A orquestracao em si (texturas do titulo, OpenBasicData,
// LoadMainSceneInterface, entrada na cena de login, laco de quadro) vive em
// source/Platform/LegacySceneBringup.cpp, para o Android usar exatamente a mesma.
// Aqui fica so o que e do navegador: o tamanho do canvas e o destino do log.

#include "../source/Platform/LegacySceneBringup.h"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace
{
    void LogWeb(const char* message)
    {
        emscripten_log(EM_LOG_ERROR, "%s", message);
    }

    void GetCanvasSize(int& width, int& height)
    {
        width = 0;
        height = 0;
        emscripten_get_canvas_element_size("#canvas", &width, &height);
    }
}

namespace Platform
{
    bool CreateLegacyTitleScene(int screenWidth, int screenHeight)
    {
        SetLegacySceneLogger(&LogWeb);
        return CreateTitleScene(screenWidth, screenHeight);
    }

    bool IsLegacyTitleSceneReady()
    {
        return IsTitleSceneReady();
    }

    void RenderLegacyTitleScene()
    {
        // O canvas so ganha o tamanho real depois do primeiro quadro, e muda
        // quando a janela e redimensionada -- por isso e consultado a cada quadro
        // em vez de uma vez na inicializacao.
        int width = 0, height = 0;
        GetCanvasSize(width, height);
        DrawLegacyFrame(width, height);
    }
}
