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


## Flags de renderização

### GPU skinning

- Desenvolvimento: `-gpuskinning=dev` ou `compare` alterna CPU/GPU por frame.
- QA: sem flag, GPU ativa para modelos elegíveis com fallback CPU.
- Produção gradual: `-gpuskinning=production -gpuskinning-models=ID1,ID2`.
- Produção padrão: `-gpuskinning=on`.

### Otimizações por fase

Cada uma aceita `off|on|compare` e **vem desligada por padrão**. `compare` alterna
por frame contra o caminho antigo, então qualquer divergência aparece como
cintilação — é o jeito mais rápido de achar uma regressão visual.

| Flag | O que liga |
| --- | --- |
| `-statictransformcache=on` | Adia o laço por vértice de `BMD::Transform` até alguém ler os arrays de transformação, e cacheia a pose em `BMD::Animation`. |
| `-batching=on` | Cache espelhado de uniformes, submissão de vértices em bloco, fusão de comandos adjacentes na fila de opacos e redução dos pontos de flush. |
| `-instancing=on` | Agrupa instâncias da mesma malha com o mesmo estado num `glDrawElementsInstanced`, com a paleta de ossos em textura. |
| `-cpumatrices=on` | **A única com ganho medido.** Lê projeção e modelview do espelho em CPU em vez de `glGetFloatv`. Elimina os 6 pontos de sincronização por frame: `cpu_us` de 7.596 para 4.450 µs em Lorencia, −41%. Divergência contra o driver medida em 1,0e-6. |
| `-meshcache=on` | Deduplicação de cantos e índices de 16 bits na malha residente. **Nasce ligada** — já estava em produção sem chave; o interruptor serve para desligar. |

Escada de rollout sugerida, a mesma já validada pelo GPU skinning: `off` →
`compare` nas cenas de referência → `on` em QA → `on` em produção.

`-cpumatrices` já percorreu essa escada: 50 amostras em 5 mundos (Lorencia,
Dungeon, Devias, Noria) e na seleção de personagem, todas com divergência de
1,0e-6 contra o driver. Está **ligada por padrão**; `-cpumatrices=off` reverte.

Vale saber: o espelho de matrizes alimenta a pilha em CPU **sempre**, com a flag
ligada ou não — ela controla apenas de onde as matrizes são *lidas*. Desligar não
remove a interceptação, só volta a ler do driver com `glGetFloatv`.

### Diagnóstico de fill rate

`-renderscale=N` (N em 1..100) escala **apenas o viewport 3D**, mantendo
geometria, draws, câmera e UI idênticos. Serve para uma pergunta só: o frame
escala com a área de pixel?

```
.\Main.exe -renderstatscsv -renderscale=100
.\Main.exe -renderstatscsv -renderscale=50    # 1/4 dos pixels
```

Se `gpu_us` cair perto de 4× entre os dois, o custo de GPU é **fill rate** —
overdraw, shading, textura. Se quase não mudar, é **vértice/draw call**, e o
caminho é outro.

A cena passa a ocupar um canto da janela e o clique sai do lugar. É esperado:
`OpenglWindowWidth/Height` mantêm o valor real de propósito, para que projeção e
frustum não mudem e só a contagem de pixels varie. **Não é recurso, é
instrumento** — a coluna `render_scale` no CSV registra o valor para que uma
captura a 50% não pareça um ganho mágico.

### Medição

`-renderstatscsv` grava `RenderPerformance_v9.csv` a cada 120 frames, após 180 de
aquecimento. Além dos contadores de draw/vértice/upload, o arquivo traz a
repartição do frame em microssegundos: `us_terrain`, `us_objects`,
`us_characters`, `us_effects`, `us_sprites`, `us_simulation`, `us_select`,
`us_setup`, `us_misc` e `us_unmeasured`.

Comece por essas colunas antes de otimizar qualquer coisa. Ver
`RENDER_INSTANCING_CACHE_BATCHING_PLAN.md` para o histórico de medições —
inclusive as otimizações que os contadores confirmaram e que **não** moveram o
tempo de frame.