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

Leia o resultado em **`us_present` e `frame_total_us`**, nesta ordem:

| observação | conclusão |
| --- | --- |
| `us_present` cai muito (perto de 4×) | o custo de GPU é **fill rate** — overdraw, shading, textura |
| `us_present` quase não muda, mas é alto | GPU limitada por **vértice/draw call**, não por pixel |
| `us_present` já era baixo nos dois | o frame **não** espera a GPU; olhe as colunas de `cpu_us` |
| `frame_total_us` não muda e `us_present` também não | o gargalo não é GPU nenhuma — pare de procurar aqui |

**Não use `gpu_us` nesta comparação.** A versão anterior deste README mandava
compará-lo, e isso estava errado: `GL_TIME_ELAPSED` mede o intervalo na timeline
da GPU incluindo o ocioso, e na prática reproduz `frame_total_us`. A sonda de fill
rate original comparou dois valores de `gpu_us` e por isso sua conclusão foi
retratada (commit `e3cb8b7`). `us_present` mede o bloqueio real em `SwapBuffers` e
existe desde a v15 — é a coluna que essa pergunta sempre precisou.

A cena passa a ocupar um canto da janela e o clique sai do lugar. É esperado:
`OpenglWindowWidth/Height` mantêm o valor real de propósito, para que projeção e
frustum não mudem e só a contagem de pixels varie. **Não é recurso, é
instrumento** — a coluna `render_scale` no CSV registra o valor para que uma
captura a 50% não pareça um ganho mágico.

### Rastros da Twisting Slash

`-wheeltrail=N` limita quantos rastros da skill Wheel podem existir ao mesmo
tempo. **Ausente = ilimitado**, o comportamento histórico.

Cada rastro (`MODEL_SKILL_WHEEL2`) **não é um sprite**: `RenderWheelWeapon`
(`ZzzEffect.cpp:18009`) desenha uma cópia completa do modelo da arma do jogador,
com `b->Animation`, `RequestTerrainLight` e `RenderPartObject` com alpha — por
rastro, por frame. Um rastro vive ~1 segundo (`LifeTime = 25`, decaindo a 25/s) e
a skill gera ~5 por golpe, então o uso contínuo sustenta dezenas de modelos de
arma animados simultâneos.

As colunas `wheel_trails_avg` / `wheel_trails_max` medem quantos existem, e
`wheel_trail_cap` registra o limite usado — uma captura limitada não pode parecer
ganho mágico, mesma regra do `render_scale`.

```
.\Main.exe -renderstatscsv                  # baseline, sem limite
.\Main.exe -renderstatscsv -wheeltrail=4    # com limite, para comparar
```

Compare por `fps_min` e `frame_total_us_max`, não pela média: o sintoma é um
mergulho transitório e a média de 120 frames o dilui.

### Teto de FPS e vsync

| flag | efeito |
| --- | --- |
| `-fpslimit=N` | teto de N FPS (10..1000). Ausente = ilimitado. Até a v14 o teto **nunca funcionava**: `SetTargetFps` sobrescrevia o próprio argumento, então `WaitForNextActivity()` era código morto. Vale para máquina que aquece: rodar a 170 FPS derruba o FPS *sustentado* por throttling. O sleep resultante aparece em **`us_limiter`**, não em `us_frame_gap` — espera deliberada não pode parecer trabalho não instrumentado. Valor fora da faixa vai para o log de erro. |
| `-vsync=on\|off` | swap interval explícito. **Ausente não chama nada** — fica no default do driver, o mesmo regime de todas as capturas anteriores. A coluna `vsync` do CSV registra o valor efetivo (`-1` default do driver, `0` off, `1` on). |

Sem `-vsync`, um vsync forçado no painel do driver trava o FPS em 60 e **esconde
qualquer otimização**. Registre em que regime a captura foi feita.

### Medição

`-renderstatscsv` grava `RenderPerformance_v16.csv` a cada 120 frames, após 180 de
aquecimento. Além dos contadores de draw/vértice/upload, o arquivo reparte o frame
inteiro em microssegundos, em duas camadas que fecham por construção:

```
cpu_us         = us_terrain + us_objects + us_characters + us_effects + us_sprites
               + us_simulation + us_select + us_setup_gl + us_frustum + us_misc
               + us_water + us_ui + us_framebegin + us_unmeasured
frame_total_us = cpu_us + us_overlay + us_present + us_protocol + us_pump
               + us_limiter + us_frame_gap
```

Se o arquivo já existir com um header **diferente**, ele é renomeado para
`RenderPerformance_v16.oldN.csv` antes da escrita. Se a rotação falhar (o caso
comum é o arquivo estar aberto no Excel), a amostra é **descartada** em vez de
anexada: perder 120 frames custa uma re-execução, gravar linha de outra largura
custa o arquivo inteiro e produz número que parece válido. Foi assim que a
geração v15 se corrompeu.

As três cenas (`MAIN_SCENE`, `CHARACTER_SCENE`, `LOG_IN_SCENE`) usam as **mesmas**
fases, então são comparáveis coluna a coluna entre si. Antes da v15 as duas
últimas não tinham nenhuma instrumentação: o `cpu_us` delas era integralmente
`us_unmeasured` — 87% na seleção de personagem, que é o pior caso de CPU medido
(7,3 ms para 58 draw calls).

Há uma **terceira** família de colunas, de detalhe. Elas **aninham dentro de** uma
coluna da primeira família e somam exatamente ela:

```
us_char_pose + us_char_shadow + us_char_parts              == us_characters
us_sim_ui + us_sim_objects + us_sim_chars
          + us_sim_effects + us_sim_rest                   == us_simulation
```

São sobreposição, não partição — **não** as inclua em nenhuma soma do frame, ou o
mesmo tempo é contado duas vezes. `us_char_parts` e `us_sim_rest` são derivadas
por subtração.

Elas existem porque `us_characters` (33,7% do frame em Lorencia) e `us_simulation`
(15,2%, e o maior bloco de todos em world 3) eram as duas maiores fatias medidas e
nenhuma tinha detalhamento. As perguntas que respondem: no caso de personagens, se
o custo está no corpo/pose ou nas ~15 malhas de equipamento que um player carrega;
no caso da simulação, se está nos objetos do mundo, nos personagens, ou no update
de UI — que roda a cada frame mesmo a 170 FPS.

As quatro colunas da camada externa (`us_overlay`, `us_present`, `us_protocol`,
`us_pump`, `us_limiter`) medem o que acontece **depois** da leitura de `cpu_us`: o overlay de
debug, `SwapBuffers`, o protocolo e o pump de mensagens do laço principal. Elas
chegam com **um frame de atraso** — são acumuladas após a escrita do CSV e
transferidas no topo do frame seguinte. Numa média de 120 frames isso não muda
nada, mas não compare uma linha dessas contra um frame específico.

`us_present` é o número que diz se o frame espera a GPU. Antes da v15 esse custo
não aparecia em nenhuma coluna: ficava no gap entre `cpu_us` e `frame_total_us`,
que a v14 mediu entre 0,4 e 4,1 ms.

`us_water` só é diferente de zero em mapa com água — ele mede o **segundo passe
completo** (terreno de água, joints, efeitos, blurs e todos os sprites outra vez).
Não compare um mapa com água contra um sem olhando só o total.

**Compare sempre por `frame_total_us` e `fps_period`.** A coluna `fps` é média
aritmética de `1/dt` instantâneo e **superestima** o FPS real em até 11% nas
amostras rápidas — ela fica só para comparação com as capturas antigas.
`fps_period` é `1e6 / média(frame_total_us)`, que é o FPS de verdade.
`cpu_us` mede só a região instrumentada, e `gpu_us` **não é confiável**:
`GL_TIME_ELAPSED` inclui o tempo ocioso, e na prática ele reproduz
`frame_total_us` em vez de medir carga de GPU. A coluna fica no arquivo como
registro do problema, não como métrica.

Comece por essas colunas antes de otimizar qualquer coisa. Ver
`FPS_DIAGNOSTICO_E_PLANO.md` para a leitura da captura v14 — a repartição real do
frame, o que ela derrubou, e como interpretar cada coluna nova da v15. Ver
`RENDER_INSTANCING_CACHE_BATCHING_PLAN.md` para o histórico de medições —
inclusive as otimizações que os contadores confirmaram e que **não** moveram o
tempo de frame.