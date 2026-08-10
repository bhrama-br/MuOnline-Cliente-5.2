cmake_minimum_required(VERSION 3.22)
project(mu_legacy_web LANGUAGES C CXX)

# Lua padrao 5.3.5 compilado do fonte. LuaJIT nao tem backend WebAssembly, entao
# o projeto usa Lua padrao, que compila para wasm sem alteracao.
file(GLOB LUA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/src/lua-5.3.5/*.c)
add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/src/lua-5.3.5)
target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)

# libjpeg 9a do IJG, compilada do fonte. O repositorio so tinha o .lib do Windows;
# os assets .OZJ do MU sao JPEG com cabecalho proprio, entao o decodificador
# precisa existir tambem em wasm.
file(GLOB JPEG_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/src/jpeg-9a/*.c)
add_library(jpeg_static STATIC ${JPEG_SOURCES})
target_include_directories(jpeg_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/src/jpeg-9a)

# Modulos graficos do jogo que ja compilam sob Emscripten. Alvo separado para
# medir a superficie de link sem quebrar o smoke test.
set(GAME_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../source)
# A lista de modulos vive em cmake/LegacySceneSources.cmake, compartilhada com o
# build Android para as duas plataformas nao divergirem.
set(MU_GAME_SRC ${GAME_SRC})
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/LegacySceneSources.cmake)
add_library(mu_legacy_scene STATIC ${MU_LEGACY_SCENE_SOURCES})
target_include_directories(mu_legacy_scene PUBLIC
    ${GAME_SRC}
    ${GAME_SRC}/Platform
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/src/stb
    # jpeg-9a ANTES de Dependencies/include, que tem um Jpeglib.h da versao 9.1
    # com struct de 456 bytes contra 488 da 9a. GlobalBitmap.cpp compilava com o
    # cabecalho errado e o decodificador abortava com "JPEG parameter struct
    # mismatch" na primeira textura .OZJ.
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/src/jpeg-9a
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/include
)
target_compile_features(mu_legacy_scene PRIVATE cxx_std_17)
target_compile_definitions(mu_legacy_scene PRIVATE LEGACY_GLES_RENDERER=1)
# GlobalBitmap.cpp usa setjmp/longjmp no tratador de erro da libjpeg. A
# transformacao de SjLj do Emscripten e feita na COMPILACAO, nao so no link;
# sem isto a chamada indireta quebra com "function signature mismatch".
# Os fontes legados sao normalizados para UTF-8. Os textos estreitos legados
# usam escapes de byte quando necessario, portanto nao dependem de charset de
# execucao especifico do compilador.
target_compile_options(mu_legacy_scene PRIVATE
    "-finput-charset=UTF-8"
    "-sSUPPORT_LONGJMP=emscripten")
target_link_libraries(mu_legacy_scene PUBLIC lua_static)

add_executable(mu_legacy_web
    WebGameGlobals.cpp
    WebPlatform.cpp
    ../source/Platform/LegacyRenderAdapter.cpp
    ../source/Platform/GlslLegacyRenderAdapter.cpp
    # Sem isto o link do Web fica com BeginOpaqueWorldRenderQueue e companhia
    # indefinidos. Nao e erro de link no Emscripten: vira um aviso e a chamada
    # aborta em runtime, e RenderObjects() chama a fila incondicionalmente.
    ../source/Platform/RenderPipeline.cpp
    ../source/Platform/LegacyMatrixStack.cpp
    WebInputBackend.cpp
    WebAudioBackend.cpp
    WebTitleScene.cpp
    WebLazyAssets.cpp
    ../source/Platform/LegacySceneMatrices.h
)
# jpeg-9a antes de Dependencies/include: la ha um Jpeglib.h da versao 9.1, com
# struct de tamanho diferente, que causa "JPEG parameter struct mismatch".
target_include_directories(mu_legacy_web PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/src/jpeg-9a
    ${GAME_SRC} ${GAME_SRC}/Platform
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/include)
set_target_properties(mu_legacy_web PROPERTIES SUFFIX ".html")
target_compile_features(mu_legacy_web PRIVATE cxx_std_17)
target_compile_definitions(mu_legacy_web PRIVATE LEGACY_GLES_RENDERER=1)
target_compile_options(mu_legacy_web PRIVATE "-finput-charset=UTF-8")
target_link_libraries(mu_legacy_web PRIVATE mu_legacy_scene jpeg_static)

# Teste de asset: empacota um mapa real (World10, ~0,8 MB) no sistema de arquivos
# virtual do Emscripten e le o mapa de altura pelo mesmo caminho que o jogo usa.
# A pasta Data completa tem ~965 MB, entao so um subconjunto e empacotado.
set(CLIENT_DATA ${CMAKE_CURRENT_SOURCE_DIR}/../../../Client/Data)
add_executable(mu_asset_test AssetTest.cpp WebGameGlobals.cpp)
# A pasta Dependencies/include tem um Jpeglib.h da versao 9.1, incompativel com a
# 9a compilada do fonte (struct de 456 contra 488 bytes). O diretorio da 9a vem
# primeiro para vencer a resolucao.
target_include_directories(mu_asset_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/src/jpeg-9a
    ${GAME_SRC} ${GAME_SRC}/Platform
    ${CMAKE_CURRENT_SOURCE_DIR}/../Dependencies/include)
target_compile_features(mu_asset_test PRIVATE cxx_std_17)
# Linka contra o jogo: o teste chama OpenTerrainHeight de ZzzLodTerrain.cpp.
target_link_libraries(mu_asset_test PRIVATE mu_legacy_scene jpeg_static)
set_target_properties(mu_asset_test PROPERTIES SUFFIX ".js")
target_link_options(mu_asset_test PRIVATE
    "--preload-file=${CLIENT_DATA}/World10@/Data/World10"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    "-sINITIAL_MEMORY=268435456"
)

# Teste rodavel em Node: mede e desenha texto com o mini-GDI sobre stb_truetype.
add_executable(mu_text_test TextTest.cpp)
target_include_directories(mu_text_test PRIVATE ${GAME_SRC} ${GAME_SRC}/Platform)
target_compile_features(mu_text_test PRIVATE cxx_std_17)
target_link_libraries(mu_text_test PRIVATE mu_legacy_scene)
set_target_properties(mu_text_test PROPERTIES SUFFIX ".js")
target_link_options(mu_text_test PRIVATE
    "--preload-file=${CLIENT_DATA}/Fonts@/Data/Fonts"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    "-sINITIAL_MEMORY=268435456"
)

# Teste rodavel em Node: le .wav reais do cliente pelo leitor da camada Platform.
add_executable(mu_audio_test AudioTest.cpp)
target_include_directories(mu_audio_test PRIVATE ${GAME_SRC} ${GAME_SRC}/Platform)
target_compile_features(mu_audio_test PRIVATE cxx_std_17)
target_link_libraries(mu_audio_test PRIVATE mu_legacy_scene)
set_target_properties(mu_audio_test PROPERTIES SUFFIX ".js")
target_link_options(mu_audio_test PRIVATE
    "--preload-file=${CLIENT_DATA}/Sound/beep.wav@/Data/Sound/beep.wav"
    "--preload-file=${CLIENT_DATA}/Sound/mSpider1.wav@/Data/Sound/mSpider1.wav"
    "--preload-file=${CLIENT_DATA}/Sound/iRepair.wav@/Data/Sound/iRepair.wav"
    "--preload-file=${CLIENT_DATA}/World10@/Data/World10"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    "-sINITIAL_MEMORY=268435456"
)

# Teste rodavel em Node: exercita a matematica do proprio jogo dentro do wasm.
add_executable(mu_gamecode_test GameCodeTest.cpp)
target_link_libraries(mu_gamecode_test PRIVATE mu_legacy_scene)
target_compile_features(mu_gamecode_test PRIVATE cxx_std_17)
set_target_properties(mu_gamecode_test PROPERTIES SUFFIX ".js")
target_link_options(mu_gamecode_test PRIVATE
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    "-sALLOW_MEMORY_GROWTH=1"
)

target_link_options(mu_legacy_web PRIVATE
    # Os subsistemas ainda nao portados (rede, audio, protecao, crash handler)
    # deixam simbolos indefinidos. Em vez de gerar centenas de stubs para nomes
    # C++ decorados, o link e fechado e cada simbolo vira erro de runtime com o
    # nome legivel — o que mostra o que e de fato ALCANCADO, nao apenas
    # referenciado. Deve voltar a 1 quando os subsistemas existirem.
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    # Sem limite de erros do linker. ERROR_ON_UNDEFINED_SYMBOLS=0 cobre FUNCOES,
    # nao dados: cada global que o Winmain define e o Web nao vira erro de link.
    # Com o limite padrao o linker para em 3, e descobri-los de tres em tres
    # custa um build inteiro por rodada.
    # Dois tracos: com um so, o wasm-ld le "-e rror-limit=0" e trata como simbolo
    # de entrada.
    "-Wl,--error-limit=0"
    "-sUSE_WEBGL2=1"
    "-sMIN_WEBGL_VERSION=2"
    "-sMAX_WEBGL_VERSION=2"
    # Nao usar -sGL_ASSERTIONS=1 aqui: em 2026-08-10 a camada de validacao atrasou
    # tanto o carregamento que o cliente saiu antes do primeiro quadro, num
    # OpenTexture Failed de Data\Skill\ston1.jpg. O erro de draw foi instrumentado
    # dentro de GlslLegacyRenderAdapter em vez disso.
    # Memoria fixa em vez de crescivel: com ALLOW_MEMORY_GROWTH o Emscripten cria
    # um ArrayBuffer redimensionavel, e o Chrome recente recusa
    # TextDecoder.decode() sobre esse tipo de buffer — a pagina quebra antes de
    # desenhar. 256 MB cobrem o smoke test e os assets iniciais.
    "--preload-file=${CLIENT_DATA}/World10@/Data/World10"
    # Fonte do subsistema de texto: sem ela CreateFontA nao acha glifo nenhum e
    # as metricas voltam a zero, que era o estado anterior a PlatformText.cpp.
    "--preload-file=${CLIENT_DATA}/Fonts@/Data/Fonts"
    # Data/Local: .bmd de texto/itens lidos por inicializadores estaticos. Sem
    # eles o cliente exibe "File not exist" e chama ExitProcess ANTES do main.
    "--preload-file=${CLIENT_DATA}/Local@/Data/Local"
    # Data/Interface NAO e mais empacotada. Os 35 MB (~400 arquivos) faziam a
    # pagina esperar o download inteiro antes de desenhar, e a tela de titulo usa
    # 14 deles. Agora a arvore e apenas COPIADA para junto da pagina (regra
    # copiar_interface abaixo) e cada arquivo e buscado no primeiro fopen que
    # falhar, pelo gancho de WebLazyAssets.cpp.
    # Som de verificacao do backend Web Audio.
    "--preload-file=${CLIENT_DATA}/Sound/mSpider1.wav@/Data/Sound/mSpider1.wav"
    # MainInfo.ini: endereco do servidor e demais campos que no PC vem do arquivo
    # principal cifrado. Precisa estar no MEMFS desde o inicio -- a busca sob demanda
    # nao serve, porque ela procura no diretorio da pagina e este arquivo fica na
    # RAIZ do cliente, fora de Data. Fica na raiz do MEMFS, que e o diretorio
    # corrente do processo.
    #
    # Usa a variante do Web (MainInfo.web.ini), nao a do cliente: o navegador nao
    # abre socket TCP e precisa apontar para a ponte WebSocket. O motivo esta
    # comentado dentro do proprio arquivo.
    # Data/Configs: 45 arquivos, 364 KB -- os scripts Lua de configuracao.
    #
    # Empacotados em vez de buscados sob demanda porque OpenFolder ENUMERA a pasta
    # (FindFirstFile "*.lua"), e nao ha como enumerar o que ainda nao foi baixado: a
    # busca sob demanda so sabe trazer um caminho que alguem pediu pelo nome.
    "--preload-file=${CLIENT_DATA}/Configs@/Data/Configs"
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/MainInfo.web.ini@/MainInfo.ini"
    # 2 GB, fixos.
    #
    # Historico: 256 MB cobriam a tela de titulo, mas OpenBasicData decodifica centenas
    # de texturas e modelos de uma vez e esgotava a heap por volta dos 2700 arquivos. O
    # sintoma NAO foi um erro de alocacao: o cliente nao confere o retorno de malloc,
    # entao escrevia atraves de um ponteiro nulo, e o que aparecia era "Stack cookie has
    # been overwritten at 0x00000004" -- o endereco 4 e onde fica o cookie, nao onde
    # estava o bug. Subiu para 512 MB.
    #
    # 512 MB chegaram ate a cena de personagem, e caiu ao ENTRAR NO MAPA:
    #
    #   Aborted(Cannot enlarge memory arrays to size 1294204928 bytes (OOM))
    #
    # ou seja, o mapa pediu ~1,21 GB de uma vez (terreno, objetos e todas as texturas
    # de Lorencia). 2 GB dao margem para o mapa e para o que vier depois.
    #
    # POR QUE NAO ALLOW_MEMORY_GROWTH: com ele o Emscripten cria um ArrayBuffer
    # redimensionavel, e o Chrome recente recusa TextDecoder.decode() sobre esse tipo de
    # buffer -- a pagina quebra antes de desenhar. Memoria fixa e a opcao que funciona.
    "-sINITIAL_MEMORY=2147483648"
    # A pilha padrao do Emscripten e 64 KB, contra 1 MB do executavel Windows.
    # OpenBasicData estourou: e uma funcao de 500 linhas com centenas de buffers
    # locais, chamando cadeias fundas de carregamento. Com ASSERTIONS o estouro
    # aparece como "Stack cookie has been overwritten"; SEM elas seria corrupcao
    # silenciosa de memoria, entao vale manter as assercoes neste build.
    "-sSTACK_SIZE=8388608"
    "-sASSERTIONS=1"
    # Nomes de funcao no wasm. Sem isto um erro de runtime aparece como
    # "wasm-function[12345]" e nao ha onde ocorreu; com isto a pilha da excecao
    # traz os nomes C++. Foi o que localizou o fclose duplo em OpenFilterFile,
    # depois de Runtime.exceptionThrown e o depurador nao darem pilha nenhuma.
    # Custa tamanho de arquivo, nao velocidade -- vale manter enquanto a
    # migracao estiver descobrindo falhas dentro do codigo do jogo.
    "-g"
    "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
)

# A arvore Data inteira precisa ficar acessivel ao lado da pagina, porque o
# gancho de busca sob demanda usa o MESMO texto como caminho virtual e como URL.
# Se os dois divergirem, a busca da 404 em silencio e o sintoma so aparece muito
# depois, como textura faltando.
#
# Um LINK, nao uma copia: Client/Data tem 0,94 GB, e copiar isso a cada build
# custaria minutos e o dobro do disco. A busca sob demanda torna a copia inutil
# de qualquer forma -- so o que o jogo toca desce pela rede.
# CMAKE_HOST_WIN32, nao WIN32: sob a toolchain do Emscripten, WIN32 descreve o
# ALVO (que e wasm) e sai falso mesmo compilando no Windows. Com WIN32 este
# bloco caia no ramo do symlink, que falha por falta de privilegio.
if(CMAKE_HOST_WIN32)
    # Junction em vez de create_symlink: no Windows o symlink de diretorio exige
    # modo de desenvolvedor ou privilegio de administrador; junction nao exige.
    add_custom_command(TARGET mu_legacy_web POST_BUILD
        COMMAND if not exist "$<SHELL_PATH:$<TARGET_FILE_DIR:mu_legacy_web>/Data>"
                mklink /J "$<SHELL_PATH:$<TARGET_FILE_DIR:mu_legacy_web>/Data>"
                          "$<SHELL_PATH:${CLIENT_DATA}>"
        COMMENT "Ligando Data para busca sob demanda"
        VERBATIM
    )
else()
    add_custom_command(TARGET mu_legacy_web POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${CLIENT_DATA} $<TARGET_FILE_DIR:mu_legacy_web>/Data
        COMMENT "Ligando Data para busca sob demanda"
    )
endif()
