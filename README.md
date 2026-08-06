# RoxGaming 5.2 — cliente PC e Web

Este diretório contém o cliente legado do jogo, com dois alvos principais:

- **PC (Windows x86):** solução Visual Studio `Main.sln`.
- **Web (WebAssembly/WebGL 2):** projeto CMake em `web/`, compilado com Emscripten.

`sourceOLD/` é uma cópia de referência do código original. Ela serve somente para comparação: **não a modifique**.

## Estrutura relevante

| Caminho | Finalidade |
| --- | --- |
| `source/` | Código-fonte atual do cliente. |
| `sourceOLD/` | Código original de referência, somente leitura. |
| `Main.sln` | Solução do cliente Windows. |
| `web/` | Pontes Web, entrada CMake e configuração WebSocket. |
| `cmake/LegacySceneSources.cmake` | Lista compartilhada de fontes do cliente para Web/Android. |
| `Dependencies/` | Dependências incluídas no projeto. |
| `build-web/` | Diretório de saída do CMake/Ninja para Web. Pode ser recriado. |
| `Global Release/` | Saída da compilação Release do PC. |
| `../../Client/` | Pasta do cliente usada em execução; deve conter `Data/`. |

## Pré-requisitos

### PC

- Windows 10/11 x64.
- Visual Studio 2022 Community ou Build Tools 2022.
- Workload **Desktop development with C++**.
- MSVC v143 e Windows SDK instalados.

### Web

- Requisitos do PC.
- CMake 3.22 ou superior.
- Ninja.
- Emscripten SDK (emsdk), com `emcmake`, `em++` e `wasm-opt` disponíveis no terminal.
- Python 3 para a ponte WebSocket↔TCP e para servir os arquivos localmente.

O build Web usa dados de `../../Client/Data`. Essa pasta é obrigatória; ela é ligada como `build-web/Data` após a compilação.

## Compilar o cliente PC

Abra um **Developer PowerShell for VS 2022** no diretório `Source/Main` e execute:

```powershell
msbuild Main.sln /t:Build /p:Configuration=Release /p:Platform=x86 /m:1
```

O executável é gerado em:

```text
Global Release\Main.exe
```

Para instalar a versão recém-compilada na pasta do cliente:

```powershell
Copy-Item 'Global Release\Main.exe' '..\..\Client\Main.exe' -Force
Copy-Item 'Global Release\Main.pdb' '..\..\Client\Main.pdb' -Force
```

Também é possível abrir `Main.sln` no Visual Studio, selecionar **Global Release | Win32** e usar **Build Solution**.

## Compilar o cliente Web

No PowerShell, a partir de `Source/Main`:

```powershell
. 'C:\emsdk\emsdk_env.ps1'
emcmake cmake -S web -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --target mu_legacy_web --parallel 2
```

Após a primeira configuração, basta repetir o último comando para builds incrementais:

```powershell
cmake --build build-web --target mu_legacy_web --parallel 2
```

Artefatos produzidos:

```text
build-web\mu_legacy_web.html
build-web\mu_legacy_web.js
build-web\mu_legacy_web.wasm
build-web\mu_legacy_web.data
```

Não abra o `.html` com `file://`. Sirva a pasta por HTTP, por exemplo:

```powershell
python -m http.server 8080 --directory build-web
```

Depois abra `http://127.0.0.1:8080/mu_legacy_web.html`.

## Rede no Web

Navegadores não abrem TCP diretamente. O cliente Web usa `web/ws_tcp_bridge.py` para converter WebSocket em TCP para o servidor do jogo.

1. Revise `web/MainInfo.web.ini` e configure o endereço/porta da ponte.
2. Revise o destino TCP no início de `web/ws_tcp_bridge.py`.
3. Inicie a ponte em outro terminal:

```powershell
python web\ws_tcp_bridge.py
```

4. Inicie o servidor HTTP e abra a página Web.

## Limpeza de builds

Com o jogo e servidores locais fechados, é seguro remover arquivos temporários:

```text
source\objfix*
build-web\CMakeFiles
build-web\*.o / *.obj / *.a
```

Para uma reconfiguração Web completa, remova somente o conteúdo gerado de `build-web/` e execute novamente o comando `emcmake cmake` acima. Preserve `web/`, `source/`, `sourceOLD/`, `Dependencies/` e `Client/Data/`.

## Boas práticas

- Nunca altere `sourceOLD/`.
- Faça alterações em `source/` ou `web/` e compile o alvo afetado.
- Para mudanças compartilhadas de UI, rede ou inventário, teste PC e Web.
- Se o navegador mantiver uma versão anterior, faça recarregamento forçado (`Ctrl+F5`) e confirme a data dos arquivos em `build-web/`.
