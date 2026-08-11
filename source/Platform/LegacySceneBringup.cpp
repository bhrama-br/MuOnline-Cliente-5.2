// Ver LegacySceneBringup.h. Este arquivo inclui os cabecalhos do CLIENTE (nao
// so a camada Platform) porque a subida da cena e feita com o codigo do proprio
// jogo -- BITMAP_TITLE e um valor de enum no meio de uma cadeia longa em
// _TextureIndex.h, e escrever o numero a mao quebraria em silencio se a cadeia
// mudasse.
//
// Nao compila no Windows: la o ponto de entrada e o `Winmain`, que faz tudo isto
// por conta propria.

#if !defined(_WIN32)

#include "../stdafx.h"
#include "../ZzzTexture.h"
#include "../UIMng.h"
#include "../_TextureIndex.h"
#include "../Input.h"
#include "../Widescreen.h"
#include "../ZzzOpenglUtil.h"
#include "../ZzzOpenData.h"
#include "../NewUISystem.h"
#include "../ZzzScene.h"
#include "../ZzzInterface.h"
#include "../Interfaces.h"
#include "../MapManager.h"
#include "../CreateFont.h"
#include "../WSclient.h"      // ProtocolCompiler, g_pChatRoomSocketList
#include "../UIWindows.h"     // CChatRoomSocketList
#include "LegacySceneBringup.h"

#include <cmath>
#include <stdio.h>

// Definidas em LegacyClientGlobals.cpp; declaradas em Winmain.h, que arrasta a
// cadeia Win32 do ponto de entrada.
extern int m_Resolution;
extern CChatRoomSocketList* g_pChatRoomSocketList;

// Definida em ZzzScene.cpp:2409 e declarada apenas dentro de Winmain.cpp (556), que
// nao compila fora do Windows.
void MainScene(HDC hDC);

// Globais de mouse sem cabecalho proprio (o dono no PC e o Winmain).
extern int g_iNoMouseTime;
extern int g_iMousePopPosition_x;
extern int g_iMousePopPosition_y;

// Caixas de texto compartilhadas; ver CriarCaixasDeTextoUnicas mais abaixo.
extern CUITextInputBox* g_pSingleTextInputBox;
extern CUITextInputBox* g_pSinglePasswdInputBox;

namespace
{
    Platform::LegacySceneLogger g_logger = 0;
    bool g_titleReady = false;
    int  g_currentWidth = 0;
    int  g_currentHeight  = 0;

    void Log(const char* message)
    {
        if (g_logger != 0 && message != 0) g_logger(message);
    }

    void Logf(const char* format, ...)
    {
        if (g_logger == 0) return;
        char line[320];
        va_list args;
        va_start(args, format);
        vsnprintf(line, sizeof(line), format, args);
        va_end(args);
        g_logger(line);
    }

    // Traduz o ponteiro da plataforma para os globais de mouse do cliente.
    //
    // POR QUE ISTO EXISTE: o cliente tem DUAS fontes de mouse. A interface nova le
    // CInput (que o backend de plataforma alimenta), mas o codigo mais antigo -- e a
    // lista de servidores da tela de login e dele -- le os globais `MouseX`,
    // `MouseY`, `MouseLButtonPush` e companhia. E QUEM ESCREVE NESSES GLOBAIS e o
    // window proc do Win32 (Winmain.cpp:778-830), que nao existe aqui.
    //
    // O sintoma era enganoso: ligar CInput e os callbacks de clique do Emscripten nao
    // mudou nada, porque o HUD e a lista olham para os globais antigos. `MouseX`
    // valia `WindowWidth/2` por inicializacao (ZzzOpenglUtil.cpp:42) -- o "512 384"
    // que aparecia fixo na tela.
    //
    // As contas e as bordas de pressionar/soltar sao as mesmas do window proc,
    // inclusive a divisao dos DOIS eixos por g_fScreenRate_y (nao _x), que e como o
    // original faz.
    void UpdateLegacyMouse()
    {
        Platform::PointerSnapshot pointer;
        Platform::GetInputBackend().ReadPointer(pointer);

        const int previousX = MouseX;
        const int previousY = MouseY;

        // WM_MOUSEMOVE (Winmain.cpp:778-793).
        const float rate = (g_fScreenRate_y != 0.0f) ? g_fScreenRate_y : 1.0f;
        const int limitX = (int)(WindowWidth / rate);
        const int limitY = (int)GetWindowsY;
        int x = (int)((float)pointer.x / rate);
        int y = (int)((float)pointer.y / rate);
        if (x < 0) x = 0;
        if (x > limitX) x = limitX;
        if (y < 0) y = 0;
        if (y > limitY) y = limitY;
        MouseX = x;
        MouseY = y;

        // Preambulo do window proc (773-775): o duplo clique e por evento, e o "pop"
        // caduca se o cursor sair do lugar onde o botao foi solto.
        MouseLButtonDBClick = false;
        if (MouseLButtonPop && (g_iMousePopPosition_x != MouseX || g_iMousePopPosition_y != MouseY))
            MouseLButtonPop = false;

        // Bordas dos botoes. O window proc as deriva de mensagens; aqui saem da
        // comparacao com o estado do quadro anterior, que e o mesmo resultado.
        const bool leftNow = pointer.leftButtonDown;
        if (leftNow && !MouseLButton)          // WM_LBUTTONDOWN
        {
            g_iNoMouseTime = 0;
            MouseLButtonPop = false;
            MouseLButtonPush = true;
            MouseLButton = true;
        }
        else if (!leftNow && MouseLButton)     // WM_LBUTTONUP
        {
            g_iNoMouseTime = 0;
            MouseLButtonPush = false;
            MouseLButtonPop = true;
            MouseLButton = false;
            g_iMousePopPosition_x = MouseX;
            g_iMousePopPosition_y = MouseY;
        }

        const bool rightNow = pointer.rightButtonDown;
        if (rightNow && !MouseRButton)           // WM_RBUTTONDOWN
        {
            g_iNoMouseTime = 0;
            MouseRButtonPop = false;
            MouseRButtonPush = true;
            MouseRButton = true;
        }
        else if (!rightNow && MouseRButton)      // WM_RBUTTONUP
        {
            g_iNoMouseTime = 0;
            MouseRButtonPush = false;
            MouseRButtonPop = true;
            MouseRButton = false;
        }

        (void)previousX;
        (void)previousY;
    }

    // Publica a resolucao nos globais do cliente e refaz o que depende dela.
    //
    // O `Winmain` faz isso ao criar a janela. Sem ele, WindowWidth/WindowHeight
    // ficam com o tamanho PADRAO da area de desenho -- e BeginBitmap chama
    // glViewport(0,0,WindowWidth,WindowHeight), entao a interface inteira sai
    // espremida num retangulo no canto.
    void ApplyResolution(int width, int height)
    {
        WindowWidth  = (unsigned int)width;
        WindowHeight = (unsigned int)height;

        // m_Resolution e o INDICE na tabela de resolucoes do Winmain (1209-1220), e
        // varios pontos decidem por ele -- nao e um dado morto:
        //
        //   ZzzScene.cpp:2158  BeginOpengl(0,0, (m_Resolution > 2 ? GetWindowsX : Width), ...)
        //   Winmain.cpp:1222   m_Resolution > 3  ->  g_fScreenRate fixo em 1.6
        //   ZzzScene.cpp:1465/1476, PersonalShopTitleImp.cpp:316
        //
        // Ficava em ZERO (o Winmain e quem o preencheria), e com isso a linha 2158
        // escolhia `Width`, que FrameBeginOpengl devolve como 640 fixo. A cena 3D
        // saia com 640*1.6 = 1024 de largura numa tela de 1280 ou 2424 -- a faixa
        // com barra preta lateral que aparecia no Web E no Android.
        //
        // Em 4:3 o defeito nao aparecia porque a tabela e consistente: 640 vezes a
        // taxa da altura da exatamente a largura cheia (800x600 -> 640*1.25 = 800).
        // So em widescreen as duas contas divergem.
        static const struct { int width, height; } kResolucoes[] = {
            { 640, 480}, { 800, 600}, {1024, 768}, {1280, 960},   // 0..3: 4:3
            {1360, 768}, {1440, 900}, {1600, 900}, {1680,1050},   // 4..8: widescreen
            {1920,1080}
        };
        const int kTotalResolucoes = (int)(sizeof(kResolucoes) / sizeof(kResolucoes[0]));

        // Escolhe pela PROPORCAO primeiro e pela altura como desempate: a proporcao
        // e o que decide se o cliente esta no caminho widescreen, e e o que estava
        // errado. Alturas nao listadas (720, 1080 em 2424 de largura) sao normais
        // aqui, ao contrario do PC, onde o usuario escolhe da lista.
        const float proporcao = (float)width / (float)(height > 0 ? height : 1);
        int melhor = 0;
        float melhorErro = 1e9f;
        for (int i = 0; i < kTotalResolucoes; ++i)
        {
            const float proporcaoI = (float)kResolucoes[i].width / (float)kResolucoes[i].height;
            const float erro = fabsf(proporcaoI - proporcao) * 1000.0f
                             + fabsf((float)(kResolucoes[i].height - height));
            if (erro < melhorErro) { melhorErro = erro; melhor = i; }
        }
        m_Resolution = melhor;

        // Regra do Winmain, sem aproximacao: indice > 3 (widescreen) fixa 1.6.
        if (m_Resolution > 3)
        {
            g_fScreenRate_x = 1.6f;
            g_fScreenRate_y = 1.6f;
        }
        else
        {
            g_fScreenRate_x = (float)height / 480.0f;
            g_fScreenRate_y = (float)height / 480.0f;
        }
        GWidescreen.Init();

        // Fontes. O Winmain (1702-1711) deriva os tamanhos da ALTURA e chama
        // gCreateFont.SetFont, que e quem preenche g_hFont e as variantes. Sem
        // isso os handles ficam nulos e NENHUM texto do jogo aparece. Fica aqui,
        // e nao numa inicializacao unica, porque depende da resolucao.
        // FontHeight e um GLOBAL do cliente (ZzzInterface.cpp:136), nao uma variavel
        // local: varias janelas derivam altura de linha dele. Ele so era escrito no
        // Winmain (1772), entao fora do PC ficava em ZERO e todo leitor recebia 0:
        //
        //   NewUIMoveCommandWindow.cpp:137  altura de linha = 0/rate + 2 = 2
        //     -> a janela de Move desenhava todas as linhas empilhadas em 2 pixels,
        //        que e o texto embolado relatado no Web.
        //   NewUICharacterInfoWindow.cpp:222, PersonalShopTitleImp.cpp:542/607-617,
        //   ZzzInterface.cpp:779, LuaGlobal.cpp:80  -> alturas zeradas.
        //
        // A expressao e a MESMA do Winmain, de proposito: as duas plataformas tem de
        // derivar o mesmo tamanho da mesma altura de tela.
        FontHeight = (int)ceilf(12.f + ((float)height - 480.f) / 200.f);
        const int fontHeight = FontHeight;
        const int fixedHeight  = (height <= 600) ? 14 : 15;
        gCreateFont.SetFont(fontHeight - 1, fontHeight, fixedHeight - 1, fixedHeight);

        // CreateTitleSceneUI dimensiona os sprites por CInput::GetScreenWidth/
        // Height, e so CInput::Create os define. Ele recusa handle nulo, mas os
        // backends de input de Web e Android ignoram o handle (os eventos vem da
        // superficie), entao um valor nao-nulo qualquer passa pela validacao.
        CInput::Instance().Create((Platform::NativeWindowHandle)1, width, height);

        g_currentWidth = width;
        g_currentHeight  = height;
    }

    // As duas caixas de texto COMPARTILHADAS do cliente, criadas em Winmain.cpp
    // 1782-1818 dentro de `if (g_iChatInputType == 1)`.
    //
    // Sao usadas de longe: o nome do personagem em CharMakeWin, a senha do baú em
    // MsgWin, o nome de guilda em UIGuildMaster. E ClearInput -- chamado na PRIMEIRA
    // cena, antes de qualquer UI -- ja faz `g_pSingleTextInputBox->SetText(NULL)` sem
    // testar nulo, o que abortava o cliente com "memory access out of bounds" dentro
    // de ClearInput assim que g_iChatInputType passou a valer 1.
    //
    // Fica aqui, depois de gCreateFont.SetFont, porque SetFont recusa handle nulo e
    // sem fonte a caixa nao mede nem desenha texto.
    void CreateUniqueTextBoxes()
    {
        if (g_pSingleTextInputBox != NULL) return;   // idempotente

        g_pSingleTextInputBox = new CUITextInputBox;
        g_pSinglePasswdInputBox = new CUITextInputBox;

        // Mesmas medidas do Winmain: 200x20, e a senha com limite 9 e mascara.
        g_pSingleTextInputBox->Init((HWND)1, 200, 20);
        g_pSinglePasswdInputBox->Init((HWND)1, 200, 20, 9, TRUE);
        g_pSingleTextInputBox->SetState(UISTATE_HIDE);
        g_pSinglePasswdInputBox->SetState(UISTATE_HIDE);
        g_pSingleTextInputBox->SetFont(g_hFont);
        g_pSinglePasswdInputBox->SetFont(g_hFont);

        // g_pMercenaryInputBox nao entra: nenhum modulo compilado fora do Windows o
        // referencia (so o proprio Winmain).
    }
}

namespace Platform
{
    void SetLegacySceneLogger(LegacySceneLogger logger)
    {
        g_logger = logger;
    }

    bool IsTitleSceneReady()
    {
        return g_titleReady;
    }

    bool CreateTitleScene(int screenWidth, int screenHeight)
    {
        // Mesma lista de WebzenScene, na mesma ordem de slots. O ramo de fundo
        // tem duas variantes escolhidas por rand(); aqui a primeira, para o
        // resultado ser reproduzivel entre execucoes.
        struct Target { const char* file; GLuint wrap; };
        static const Target targets[] = {
            { "Interface\\New_lo_back_01.jpg",     GL_CLAMP_TO_EDGE },
            { "Interface\\New_lo_back_02.jpg",     GL_CLAMP_TO_EDGE },
            { "Interface\\MU_TITLE.tga",           GL_CLAMP_TO_EDGE },
            { "Interface\\lo_121518.tga",          GL_CLAMP_TO_EDGE },
            { "Interface\\New_lo_webzen_logo.tga", GL_CLAMP_TO_EDGE },
            { "Interface\\lo_lo.jpg",              GL_REPEAT        },
            { "Interface\\lo_back_s5_03.jpg",      GL_CLAMP_TO_EDGE },
            { "Interface\\lo_back_s5_04.jpg",      GL_CLAMP_TO_EDGE },
            { "Interface\\lo_back_im01.jpg",       GL_CLAMP_TO_EDGE },
            { "Interface\\lo_back_im02.jpg",       GL_CLAMP_TO_EDGE },
            { "Interface\\lo_back_im03.jpg",       GL_CLAMP_TO_EDGE },
            { "Interface\\lo_back_im04.jpg",       GL_CLAMP_TO_EDGE },
            { "Interface\\lo_back_im05.jpg",       GL_CLAMP_TO_EDGE },
            { "Interface\\lo_back_im06.jpg",       GL_CLAMP_TO_EDGE },
        };
        const int total = (int)(sizeof(targets) / sizeof(targets[0]));

        // Vetores globais que o Winmain aloca (GateAttribute, CharactersClient,
        // Hero, g_MapProcess...). Fica AQUI, e nao no ponto de entrada de cada
        // plataforma, porque so o Web tinha essa chamada: no Android `Hero` ficava
        // NULO e o primeiro uso quebrava dentro de OpenPlayers, em
        // AngleMatrix(m_oOwner->Angle) -- endereco 0x4d8, que e o offset de Angle
        // a partir do zero. E idempotente.
        AllocateClientGlobals();

        // ANTES de AplicarResolucao: e ela que chama gCreateFont.SetFont, e as faces
        // que SetFont usa vem de Font.lua, carregado por gCreateFont.Init() aqui.
        InicializarSubsistemasDeScript();

        ApplyResolution(screenWidth, screenHeight);

        // As duas primeiras linhas de WebzenScene. OpenFont carrega as texturas
        // de fonte e cria o renderizador de texto.
        OpenFont();
        CreateUniqueTextBoxes();
        ClearInput();

        while (glGetError() != GL_NO_ERROR) {}   // drena erros anteriores

        int ok = 0;
        for (int i = 0; i < total; ++i)
        {
            // bCheck=false desliga PopUpErrorCheckMsgBox, que depende da UI que
            // ainda nao existe neste ponto.
            if (LoadBitmap(targets[i].file, (GLuint)(BITMAP_TITLE + i),
                           GL_LINEAR, targets[i].wrap, false))
                ++ok;
        }
        Logf("texturas do titulo: %d de %d (GL 0x%04X)", ok, total, glGetError());
        if (ok != total) return false;

        CUIMng::Instance().CreateTitleSceneUI();

        // Estas duas linhas vem de WebzenScene, entre CreateTitleSceneUI e o
        // primeiro RenderTitleSceneUI. Sem EnableAlphaTest o GL_BLEND fica
        // desligado (AlphaBlendType comeca em 0), e as texturas com alfa
        // desenhavam o retangulo inteiro, fundo opaco incluso: RenderTitleSceneUI
        // nao liga blending por conta propria, depende do estado do chamador.
        FogEnable = false;
        ::EnableAlphaTest();

        g_titleReady = true;
        Log("UI do titulo criada pelo codigo do jogo");
        return true;
    }

    bool LoadBasicData()
    {
        if (!g_titleReady) return false;

        while (glGetError() != GL_NO_ERROR) {}

        // Winmain.cpp:1722 chama isto ANTES do laco de cena, logo antes de
        // OpenBasicData. E o que aloca CNewUIManager e as janelas base, das quais
        // LoadMainSceneInterface depende: sem ele aquele metodo devolvia false na
        // primeira sub-criacao, sem sequer tentar abrir um arquivo.
        if (!g_pNewUISystem->Create())
            Log("CNewUISystem::Create FALHOU");

        OpenBasicData(NULL);
        Logf("OpenBasicData concluiu (GL 0x%04X)", glGetError());
        return true;
    }

    bool LoadMainInterface()
    {
        if (!g_titleReady) return false;

        while (glGetError() != GL_NO_ERROR) {}

        const bool ok = g_pNewUISystem->LoadMainSceneInterface();
        Logf("LoadMainSceneInterface: %s (GL 0x%04X)", ok ? "ok" : "FALHOU", glGetError());
        return ok;
    }

    bool EnterLoginScene()
    {
        if (!g_titleReady) return false;

        CUIMng::Instance().ReleaseTitleSceneUI();
        for (int i = 0; i < 14; ++i)
            DeleteBitmap((GLuint)(BITMAP_TITLE + i));

        SceneFlag = LOG_IN_SCENE;

        // Gancho de fim de carregamento do lado Lua. Se o script nao existir,
        // Generic_Call apenas registra o erro.
        gInterface.hdc = NULL;
        gInterface.m_Lua.Generic_Call("FinalBoot", ">");

        Log("entrou na cena de login");
        return true;
    }

    void DrawLegacyFrame(int screenWidth, int screenHeight)
    {
        if (!g_titleReady) return;

        // Drenar a fila de pacotes, como o laco do Winmain faz (1924).
        //
        // Sem isto a rede parecia funcionar e nao funcionava: a conexao TCP subia, o
        // servidor mandava o pacote de saudacao, nRecv o recebia e empilhava com
        // PushPacket -- e ninguem nunca chamava GetReadMsg. O cliente ficava em
        // CurrentProtocolState 0 indefinidamente, sem lista de servidores e sem
        // enviar nada, com a conexao viva no netstat. Tres medicoes foram necessarias
        // para chegar aqui (trafego na ponte, retorno de nRecv, e finalmente quem
        // consome a fila), porque cada etapa isolada parecia correta.
        //
        // ProtocolCompiler tem argumentos padrao (WSclient.h:3686) e usa o
        // SocketClient global, igual ao Winmain.
        ProtocolCompiler();

        // g_pChatRoomSocketList e NULO neste alvo (salas de chat nao portadas); o
        // Winmain chama sem testar porque la ele mesmo aloca o objeto.
        if (g_pChatRoomSocketList != NULL)
            g_pChatRoomSocketList->ProtocolCompile();

        // Antes de qualquer ramo: ver LegacySceneBringup.h sobre por que isto NAO
        // pode viver dentro do ramo de uma cena.
        if (screenWidth > 0 && screenHeight > 0 &&
            (screenWidth != g_currentWidth || screenHeight != g_currentHeight))
        {
            ApplyResolution(screenWidth, screenHeight);

            // A UI de titulo assa a escala nos sprites, entao so ela e remontada,
            // e so enquanto ainda estivermos nela.
            if (SceneFlag == WEBZEN_SCENE)
                CUIMng::Instance().CreateTitleSceneUI();

            Logf("resolucao %dx%d (rate %.3f)", screenWidth, screenHeight, g_fScreenRate_x);
        }

        if (SceneFlag != WEBZEN_SCENE)
        {
            // Atualizacao de input e de UI, antes de desenhar.
            //
            // MainScene() e quem roda CInput::Instance().Update() e
            // CUIMng::Instance().Update(delta) -- ou seja, e ela que move o cursor do
            // cliente e transforma pressionar/soltar em cliques. Sem isso o cliente
            // reagia ao mouse em NADA: o HUD mostrava "MousePos : 512 384" fixo no
            // centro, e dava para ver "Valhalla" na lista de servidores sem conseguir
            // escolher.
            //
            // Observacao que vale registrar: nesta arvore MainScene esta ORFA. O laco
            // do Winmain chama Platform::RenderLegacySceneFrame, que so faz
            // RenderScene -- a chamada de MainScene parece ter ficado para tras na
            // conversao. Nao a acrescentei ao caminho do PC porque nao posso verificar
            // aqui se o mouse do PC funciona por outra via; se estiver quebrado la
            // tambem, o lugar da correcao e LegacySceneRunner.cpp.
            // Os globais antigos de mouse ANTES do MainScene: a lista de servidores e
            // o resto da UI legada leem deles no mesmo quadro.
            UpdateLegacyMouse();
            MainScene(NULL);

            // Quem desenha e o proprio jogo. RenderScene chama UpdateSceneState
            // por conta propria, no inicio.
            RenderScene(NULL);

            // Uma medicao, no primeiro quadro da cena: sem ela nao da para
            // distinguir "o mundo nao carregou" de "carregou e o enquadramento
            // esta errado" -- os dois dao tela preta.
            static bool relatado = false;
            if (!relatado)
            {
                relatado = true;
                GLint vp[4] = { 0, 0, 0, 0 };
                glGetIntegerv(GL_VIEWPORT, vp);
                Logf("login: mundo=%d Window=%ux%u viewport=%d,%d,%dx%d "
                     "cena3D_largura=%d camera=(%.0f,%.0f,%.0f) rate=%.2f",
                     gMapManager.WorldActive, WindowWidth, WindowHeight,
                     vp[0], vp[1], vp[2], vp[3], OpenglWindowWidth,
                     CameraPosition[0], CameraPosition[1], CameraPosition[2],
                     g_fScreenRate_x);
            }
            return;
        }

        // Os dois ultimos parametros sao o progresso da barra de carregamento.
        CUIMng::Instance().RenderTitleSceneUI(NULL, 11, 11);
    }
}

#endif  // !_WIN32
