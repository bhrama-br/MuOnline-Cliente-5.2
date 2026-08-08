#pragma once

// Subida da cena do cliente, compartilhada entre Web e Android.
//
// Reproduz `ZzzScene::WebzenScene` mais as partes da inicializacao do `Winmain`
// das quais ela depende. Vivia em `web/WebTitleScene.cpp`, que so entrava no
// build Web -- por isso o Android continuava no terreno de teste enquanto o Web
// chegava a cena de login.
//
// O que fica de fora daqui, por ser de plataforma: descobrir o tamanho da area de
// desenho e para onde vao as mensagens. As duas coisas entram por parametro.

namespace Platform
{
    // Destino das mensagens de progresso. Cada plataforma passa a sua
    // (emscripten_log no Web, __android_log_print no Android).
    typedef void (*LegacySceneLogger)(const char* mensagem);
    void SetLegacySceneLogger(LegacySceneLogger logger);

    // Etapas, na ordem em que `WebzenScene` as executa. Cada uma devolve false se
    // a anterior nao tiver concluido, entao chamar fora de ordem falha alto em vez
    // de desenhar errado.
    // Definida em LegacyGlobalAllocations.cpp; chamada por CriarCenaDeTitulo.
    void AlocarGlobaisDoCliente();

    // Preenche gProtect->m_MainInfo a partir de um MainInfo.ini em texto puro.
    //
    // No PC esses campos vem de um arquivo principal CIFRADO, lido por
    // CProtect::ReadMainFile -- que nesta arvore nao e chamada por ninguem (quem a
    // chamaria e o empacotador de protecao). Sem isso `szServerIpAddress` fica NULO
    // e a conexao sai com "ip address = (null)".
    //
    // Le texto puro de proposito: as chaves sao as mesmas de
    // MuServer/Tools/GetMainInfo/MainInfo.ini, entao trocar de servidor e editar um
    // arquivo em vez de recompilar -- e no Web isso importa mais, porque o pacote
    // ja esta montado quando se descobre o endereco.
    //
    // Definida em LegacyClientGlobals.cpp, junto do proprio gProtect.
    void CarregarMainInfo();

    // Os 28 `Init()` de subsistemas de script que o Winmain chama em 1494-1548. Sem
    // eles cada lua_State fica vazio e toda chamada a um script devolve
    // "attempt to call a nil value" -- entre elas o RenderProc da lista de personagens
    // da Season 13. Definida em LegacyLuaSubsystems.cpp; idempotente.
    void InicializarSubsistemasDeScript();

    bool CreateTitleScene(int screenWidth, int screenHeight);
    bool CenaDeTituloPronta();
    bool LoadBasicData();
    bool CarregarInterfacePrincipal();
    bool EntrarNaCenaDeLogin();

    // Um quadro. Recebe o tamanho ATUAL da area de desenho: quando ele muda, a
    // resolucao do cliente e as fontes sao refeitas.
    //
    // Cuidado ao mexer: o acompanhamento de tamanho tem que valer para TODAS as
    // cenas. Quando esse bloco vivia dentro do ramo da tela de titulo, entrar na
    // cena de login pulava a correcao e a resolucao voltava ao padrao da area de
    // desenho -- o mesmo defeito apareceu duas vezes, com sintomas diferentes.
    void DrawLegacyFrame(int screenWidth, int screenHeight);
}
