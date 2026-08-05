cmake_minimum_required(VERSION 3.22.1)
project(mu_legacy_platform LANGUAGES C CXX)

# Raizes do projeto. O CMakeLists vive cinco niveis abaixo de Source/Main.
set(MU_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)
set(MU_GAME_SRC ${MU_ROOT}/source)
set(MU_DEPS ${MU_ROOT}/Dependencies)

# Lua 5.3.5 do fonte, igual ao build Web. O codigo de jogo referencia Lua no
# link, entao ela precisa existir tambem no Android.
file(GLOB LUA_SOURCES ${MU_DEPS}/src/lua-5.3.5/*.c)
add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC ${MU_DEPS}/src/lua-5.3.5)
target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)

# libjpeg 9a do IJG: decodifica os .OZJ do MU.
file(GLOB JPEG_SOURCES ${MU_DEPS}/src/jpeg-9a/*.c)
add_library(jpeg_static STATIC ${JPEG_SOURCES})
target_include_directories(jpeg_static PUBLIC ${MU_DEPS}/src/jpeg-9a)

# Mesma lista de modulos do build Web — ver cmake/LegacySceneSources.cmake.
# Ate esta rodada o Android compilava apenas a camada Platform/.
include(${MU_ROOT}/cmake/LegacySceneSources.cmake)
add_library(mu_legacy_scene STATIC ${MU_LEGACY_SCENE_SOURCES})
target_include_directories(mu_legacy_scene PUBLIC
    ${MU_GAME_SRC}
    ${MU_GAME_SRC}/Platform
    ${MU_DEPS}/src/stb
    # jpeg-9a antes de Dependencies/include: la ha um Jpeglib.h da versao 9.1,
    # com struct de tamanho diferente, e o decodificador aborta em runtime.
    ${MU_DEPS}/src/jpeg-9a
    ${MU_DEPS}/include
)
target_compile_features(mu_legacy_scene PRIVATE cxx_std_17)
target_compile_definitions(mu_legacy_scene PRIVATE LEGACY_GLES_RENDERER=1)
# Mesma regra do Web: fonte UTF-8; strings estreitas legadas usam escapes de
# byte quando necessario e nao dependem do charset de execucao do compilador.
target_compile_options(mu_legacy_scene PRIVATE "-finput-charset=UTF-8")
target_link_libraries(mu_legacy_scene PUBLIC lua_static)

add_library(mu_legacy_platform SHARED
    AndroidPlatform.cpp
    AndroidInputBackend.cpp
    AndroidGameGlobals.cpp
    AndroidAudioBackend.cpp
    AndroidMissingSymbols.cpp
    ${MU_GAME_SRC}/Platform/LegacyRenderAdapter.cpp
    ${MU_GAME_SRC}/Platform/GlslLegacyRenderAdapter.cpp
    ${MU_GAME_SRC}/Platform/LegacyMatrixStack.cpp
    ${MU_GAME_SRC}/Platform/LegacySceneMatrices.h
    ${MU_GAME_SRC}/Platform/PlatformTypes.h
)

target_compile_features(mu_legacy_platform PRIVATE cxx_std_17)
target_compile_definitions(mu_legacy_platform PRIVATE LEGACY_GLES_RENDERER=1)
target_include_directories(mu_legacy_platform PRIVATE
    ${MU_GAME_SRC}
    ${MU_GAME_SRC}/Platform
    ${MU_DEPS}/src/jpeg-9a
    ${MU_DEPS}/src/stb
    ${MU_DEPS}/include
)
# Os subsistemas ainda nao portados (rede, audio, protecao) deixam simbolos
# indefinidos. --allow-shlib-undefined + -z undefs adiam a resolucao para o
# carregamento, do mesmo jeito que -sERROR_ON_UNDEFINED_SYMBOLS=0 faz no Web:
# o que nao for alcancado em runtime nao chega a quebrar.
target_link_options(mu_legacy_platform PRIVATE
    "-Wl,--allow-shlib-undefined"
    "-Wl,-z,undefs"
    # Android 15+ roda com paginas de 16 KB em alguns aparelhos. Sem isto o
    # sistema abre um dialogo de compatibilidade por cima do app e o executa em
    # modo de compatibilidade de pagina.
    "-Wl,-z,max-page-size=16384"
)
# Sem --whole-archive de proposito.
#
# Forcar o arquivo inteiro coloca os 333 modulos na .so (52 MB, 32 mil simbolos),
# mas o APK deixa de ABRIR: o linker dinamico do Android resolve tudo no dlopen
# (BIND_NOW), e os ~916 simbolos dos subsistemas ainda nao portados (rede, audio,
# protecao, globais de Winmain) nao existem — dlopen falha no primeiro deles.
# Isso difere do Web, onde -sERROR_ON_UNDEFINED_SYMBOLS=0 adia a falha ate a
# chamada. Entao o Android puxa do arquivo apenas o que a camada Platform de fato
# CHAMA, como WebPlatform.cpp faz.
target_link_libraries(mu_legacy_platform PRIVATE
    mu_legacy_scene jpeg_static android log EGL GLESv3 OpenSLES)
