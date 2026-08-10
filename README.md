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

**Cobertura ampliada em 2026-08-10, e ainda não verificada com `compare`.** O diagnóstico
`[GpuSkinReject]` (ver abaixo) mostrou que 824 sub-malhas por frame caíam na CPU numa
multidão de 50 players, todas por material: `RENDER_CHROME4` — com que as cinco peças de
armadura são desenhadas — não tinha efeito mapeado no caminho de GPU, e a condição de
blend mesh reprovava sub-malhas equivalentes ao ramo normal. As duas foram consertadas
(`CROWD_LOD_PLAN.md`, seções 0.10 e 0.11); a primeira rendeu **+15% de FPS**. Rode
`-gpuskinning=compare` antes de confiar no visual: divergência aparece como cintilação.

Ainda ficam na CPU, de propósito: `RENDER_CHROME3` (a UV depende de `LightVector`, que o
shader não recebe), `RENDER_CHROME5` e `RENDER_CHROME7` (o caminho legado não faz bind de
textura para elas — a malha herda a anterior, o que não é reproduzível num lote), malha com
UV animada e malha com alpha < 0,99.

Quando um material é reprovado, `-renderstatscsv` escreve no `MuError.log` uma linha por
combinação (modelo, motivo) nova, com teto de 64, e um resumo com a média por frame:

```
[GpuSkinReject] modelo 4708: sem-textura+renderflags (renderflags nao suportadas 0x1000)
[GpuSkinReject] media/frame em 600 frames: total 422  blendmesh 272  uv-animado 90 ...
```

É log e não coluna de CSV porque a pergunta é pontual: respondida, o trabalho é consertar
a condição, não acompanhar a métrica.

Antes de abrir o jogo, um verificador confere se as UV de chrome do shader batem com as da
CPU — pega sinal trocado, fator errado e termo esquecido em um segundo:

```powershell
python diagnostic\check_chrome_uv.py
```

Ele **não** cobre estado de GL (blend, depth, textura ligada) nem precisão de float32 no
hardware. Para isso não há substituto para `-gpuskinning=compare`.

- Desenvolvimento: `-gpuskinning=dev` ou `compare` alterna CPU/GPU por frame.
- QA: sem flag, GPU ativa para modelos elegíveis com fallback CPU.
- Produção gradual: `-gpuskinning=production -gpuskinning-models=ID1,ID2`.
- Produção padrão: `-gpuskinning=on`.

### Otimizações por fase

Cada uma aceita `off|on|compare`. `compare` alterna por frame contra o caminho
antigo, então qualquer divergência aparece como cintilação — é o jeito mais
rápido de achar uma regressão visual. A coluna **Padrão** vale para os três
alvos: no Web e no Android não há linha de comando, então nenhuma flag é lida e o
default é o que roda.

| Flag | Padrão | O que liga |
| --- | --- | --- |
| `-statictransformcache=on` | off | Adia o laço por vértice de `BMD::Transform` até alguém ler os arrays de transformação, e cacheia a pose em `BMD::Animation`. **Medido em 2026-08-10 sob multidão: −15% de frame com 100 monstros, −5% com 50 players.** A corretude depende de todo leitor dos arrays globais materializar antes de ler; isso foi auditado (57 leituras, 4 arquivos, todas guardadas) e virou teste — `python diagnostic\check_transform_cache_readers.py`. Falta só `compare` para o default virar `on`. |
| `-batching=on` | **on** | Cache espelhado de uniformes, submissão de vértices em bloco, fusão de comandos adjacentes na fila de opacos e redução dos pontos de flush. Em mundo 2 cena 5: draws de 4.366 → ~1.835 por frame, `flush_matrix` de 449 → 0, ~20.000 chamadas de uniforme evitadas por frame. `-batching=off` reverte. |
| `-instancing=on` | **on** | Agrupa instâncias da mesma malha com o mesmo estado num `glDrawElementsInstanced`, com a paleta de ossos em textura. Ligada por decisão de projeto, **não por ganho medido**: em `RenderPerformance_v16` rendeu 2–3 draws instanciados por frame cobrindo 4–6 objetos (`instance_batch_max = 2`), e em mundo 94 (sem personagens, ~55 draws) a captura ligada ficou 10% mais lenta. `-instancing=off` reverte. |
| `-cpumatrices=on` | **on** | Lê projeção e modelview do espelho em CPU em vez de `glGetFloatv`. Elimina os 6 pontos de sincronização por frame: `cpu_us` de 7.596 para 4.450 µs em Lorencia, −41%. Divergência contra o driver medida em 1,0e-6. |
| `-meshcache=on` | **on** | Deduplicação de cantos e índices de 16 bits na malha residente. Já estava em produção sem chave; o interruptor serve para desligar. |

Escada de rollout sugerida, a mesma já validada pelo GPU skinning: `off` →
`compare` nas cenas de referência → `on` em QA → `on` em produção.

`-cpumatrices` já percorreu essa escada: 50 amostras em 5 mundos (Lorencia,
Dungeon, Devias, Noria) e na seleção de personagem, todas com divergência de
1,0e-6 contra o driver. Está **ligada por padrão**; `-cpumatrices=off` reverte.

Vale saber: o espelho de matrizes alimenta a pilha em CPU **sempre**, com a flag
ligada ou não — ela controla apenas de onde as matrizes são *lidas*. Desligar não
remove a interceptação, só volta a ler do driver com `glGetFloatv`.

### Isolar uma regressão visual

Quatro mudanças de 2026-08-10 alteram o que é desenhado, e **cada uma tem sua própria
chave de desligar**. Se algo parecer errado na tela, não é preciso adivinhar: desligue uma
por vez, na ordem abaixo, e a primeira que resolver aponta a culpada.

| # | mudança | desliga com | o que ela afeta |
| --- | --- | --- | --- |
| 1 | `RENDER_CHROME4`/`CHROME6` no caminho de GPU | `-gpuskinning=off` | UV de chrome da armadura de player |
| 2 | condição de blend mesh removida do portão | `-gpuskinning=off` | sub-malhas não-blend de modelo com blend mesh |
| 3 | coletor de instâncias só descarrega quando a ordem importa | `-instancing=off` | ordem de emissão entre opacos |
| 4 | corte de arma/asa por teto de contagem | `-crowdlod=off` (**já é o default**) | arma e asa fora do teto |

As duas primeiras compartilham a chave: ambas só existem dentro do caminho de GPU skinning.
Para separá-las é preciso reverter no código — mas a matemática das UV do item 1 já está
verificada por `python diagnostic\check_chrome_uv.py`, então o suspeito preferencial é o 2.

**Cuidado com o significado de cintilação**, que muda por flag:

| modo | cintilação significa |
| --- | --- |
| `-gpuskinning=compare`, `-instancing=compare`, `-cpumatrices=compare`, `-statictransformcache=compare` | **bug** — os dois caminhos deveriam ser idênticos |
| `-crowdlod=preview` | **o efeito** — os dois caminhos são intencionalmente diferentes |

Ordem sugerida de verificação, do mais barato ao mais caro:

```powershell
.\Main.exe -crowd=50 -gpuskinning=compare            # itens 1 e 2
.\Main.exe -crowd=50 -instancing=compare             # item 3
.\Main.exe -crowd=50 -statictransformcache=compare   # libera o default do cache
```

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

### Crowd LOD (personagens)

Objetivo: fazer 100–200 personagens visíveis caberem no orçamento de frame nos três
alvos. Plano completo e invariantes em `CROWD_LOD_PLAN.md`.

**O que o LOD corta, e por que só isso:** com `-crowdlod=on` e um teto
(`-crowdmaxfull=N`), os personagens fora do teto param de ter **arma e asa** desenhadas.
É o único corte com prêmio grande que a medição encontrou — `us_char_link_draw` é 30% do
frame, e 81% dele é emissão de geometria, então nenhum cache resolve. Pose (0,5 ms) e
sombra (0,5 ms) de um frame de 18,7 não pagam corte.

**Sem `-crowdmaxfull` o LOD não corta nada**, e isso é de propósito: limiar por distância
não dispara nesta câmera — na captura de 2026-08-10, **175 de 175** personagens ficaram em
L0, porque MU mantém tudo grande na tela. A alavanca é contagem, não metros.

**Isto colide com uma invariante do plano** (arma e asa definem silhueta e identificação de
classe, e não deveriam sair). A medição deu o preço da invariante — **+38% de FPS**, 53,6 →
73,7 com 59 personagens — e pagar ou não é decisão de quem opera o jogo. É por isso que o
default é `off`.

> **Cuidado com o significado de `preview`/`compare` nesta flag.** Nas demais
> (`-gpuskinning`, `-cpumatrices`, `-statictransformcache`), `compare` alterna por frame
> **dois caminhos que deveriam ficar idênticos**, e cintilação significa **bug**. Aqui os
> dois caminhos são intencionalmente diferentes: cintilação é **o efeito do corte**, não
> defeito. Foi por isso que o modo ganhou o nome `preview`.
>
> Para julgar se o corte é aceitável, use **`-crowdlod=on`** e olhe o estado final. No modo
> alternado o cérebro reage ao movimento, não à ausência, e a avaliação sai errada. E uma
> captura em `preview` não mede nem o corte nem a ausência dele — mede a média dos dois.

| flag | efeito |
| --- | --- |
| `-crowdlod=off\|on\|preview` | modo do LOD. **Default `off`.** Registrado na coluna `crowd_lod`. `compare` é aceito como sinônimo de `preview` — leia o aviso abaixo antes de usar. |
| `-crowdmaxfull=N` | teto de personagens em qualidade plena. **Ausente = sem teto**, e sem teto o LOD **não corta nada** — ver abaixo. Registrado em `crowd_max_full`. |
| `-crowd=N` | N players sintéticos ao redor do herói. Registrado em `crowd_spawn`. |
| `-crowdmonsters=N` | N monstros sintéticos. Registrado em `crowd_spawn_monsters`. |
| `-crowdnpcs=N` | N NPCs sintéticos. Registrado em `crowd_spawn_npcs`. |

Ausente = 0 em todas. Os três são **independentes e combináveis** — `-crowd=100
-crowdmonsters=100` monta uma multidão mista. Separados porque os três custam coisas
diferentes: um player tem ~15 malhas de equipamento e passe de sombra próprio, um
monstro tem uma malha de corpo e nenhum dos dois. Um número único não permitiria
isolar qual deles move o frame.

O rig é **instrumento, não recurso**, pela mesma razão de `-renderscale`: servidor
povoado não é reproduzível, e sem carga repetível não há como saber se um corte de LOD
ganhou tempo ou se a cena mudou.

Como cada tipo é criado:

- **Player:** `CreateCharacter` + `SetCharacterClass`, então carrega o mesmo
  equipamento do jogador local — as ~15 malhas que `us_char_parts` mede. Um boneco
  pelado mediria a carga errada.
- **Monstro e NPC:** `CreateMonster`, o mesmo caminho da rede — é `Setting_Monster` que
  define o `Kind` pela tabela de tipo, não eu. O **tipo é clonado dos que já existem
  vivos no mapa**, e isso é essencial: um modelo de monstro que não pertence ao mapa
  atual pode não estar carregado, e `RenderCharacter` desiste em silêncio quando
  `Models[Type].NumActions == 0` (`ZzzCharacter.cpp:8357`) — o rig spawnaria 200
  monstros invisíveis de custo zero e a captura mostraria um ganho que não existe. Sem
  monstro (ou NPC) vivo no mapa, o rig **não cria nada** e diz isso no log de erro; a
  coluna `chars_monsters_avg` denuncia, e o leitor de CSV avisa.

Limites e efeitos colaterais, iguais para os três: o teto é do **total** (64 dos 400
slots ficam reservados para jogadores de verdade, então a soma dos três entrega no
máximo 336), a `Key` é negativa (o servidor só manda chaves positivas, então nada da
rede colide) e, como qualquer personagem vivo, eles **bloqueiam caminho** enquanto
existem. Monstro sintético é alvo clicável — o pacote de ataque sai para uma chave que
o servidor não conhece e é ignorado.

Configuração por arquivo, na seção `[Render]` do `MainInfo.ini`:
`CrowdLod`, `CrowdLodPixelsL1/L2/L3`, `CrowdMaxFull`, `CrowdSpawn` (players, também
aceita `CrowdSpawnPlayers`), `CrowdSpawnMonsters`, `CrowdSpawnNpcs`.
Precedência: default compilado → `MainInfo.ini` → linha de comando.

| alvo | canal |
| --- | --- |
| PC | **linha de comando.** Não há canal de arquivo: o `MainInfo` do PC é um struct binário cifrado (`CProtect::ReadMainFile`, `Data\Configs\Configs.xtm`), não texto. |
| Web | `web/MainInfo.web.ini`, pré-carregado pelo CMake como `/MainInfo.ini`. |
| Android | `android/app/src/main/assets/MainInfo.ini`, extraído por `MainActivity`. Contém **só** a seção `[Render]`: empacotar o `MainInfo.ini` do PC mudaria de uma vez o regime de decifragem de Lua e a versão de cliente do alvo, o que não tem relação com LOD. Um arquivo já presente na raiz de dados (via `adb push`) não é sobrescrito. |

Os limiares são em **pixels de altura na tela**, não em distância crua: `CameraFOV`,
`CameraZoom` e a resolução mudam a relação entre distância e tamanho aparente — o
próprio `TestFrustrum2D` compensa zoom à mão. A conta usa `WindowHeight`, e **não** o
viewport escalado, para que `-renderscale` continue variando só a contagem de pixels.

Captura de base da Fase 0 — é ela que decide a ordem das fases seguintes:

```
.\Main.exe -renderstatscsv -crowd=0
.\Main.exe -renderstatscsv -crowd=50
.\Main.exe -renderstatscsv -crowd=100
.\Main.exe -renderstatscsv -crowd=200
```

Player e monstro precisam de curvas **separadas** — o custo por personagem não é o
mesmo, e uma captura mista não diz de quem é o tempo. Em mapa com monstro (Lorencia
serve, Dungeon melhor):

```
.\Main.exe -renderstatscsv -crowdmonsters=50
.\Main.exe -renderstatscsv -crowdmonsters=100
.\Main.exe -renderstatscsv -crowdmonsters=200
.\Main.exe -renderstatscsv -crowd=100 -crowdmonsters=100   # mista, para conferir se soma
```

Perguntas, nesta ordem: `frame_total_us` cresce linearmente com N ou satura? o
crescimento está em `us_char_pose`, `us_char_parts` ou `us_char_shadow`? `us_sim_chars`
cresce junto (aí LOD de render sozinho não fecha a conta)? `us_present` cresce (aí há
componente de GPU — cruze com `-renderscale=50`)?

### Teto de FPS e vsync

| flag | efeito |
| --- | --- |
| `-fpslimit=N` | teto de N FPS (10..1000). Ausente = ilimitado. Até a v14 o teto **nunca funcionava**: `SetTargetFps` sobrescrevia o próprio argumento, então `WaitForNextActivity()` era código morto. Vale para máquina que aquece: rodar a 170 FPS derruba o FPS *sustentado* por throttling. O sleep resultante aparece em **`us_limiter`**, não em `us_frame_gap` — espera deliberada não pode parecer trabalho não instrumentado. Valor fora da faixa vai para o log de erro. |
| `-vsync=on\|off` | swap interval explícito. **Ausente não chama nada** — fica no default do driver, o mesmo regime de todas as capturas anteriores. A coluna `vsync` do CSV registra o valor efetivo (`-1` default do driver, `0` off, `1` on). |

Sem `-vsync`, um vsync forçado no painel do driver trava o FPS em 60 e **esconde
qualquer otimização**. Registre em que regime a captura foi feita.

### Medição

`-renderstatscsv` grava `RenderPerformance_v17.csv` a cada 120 frames, após 180 de
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
`RenderPerformance_v17.oldN.csv` antes da escrita. Se a rotação falhar (o caso
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

Há um **quarto** nível, com uma coluna só: `us_char_transform` aninha dentro de
`us_char_parts` e mede o laço por vértice de `BMD::TransformVertices`. Ele existe
porque a captura de multidão de 2026-08-10 mostrou `us_char_parts` com 83–95% de
`us_characters` — e `us_char_parts` é subtração, então apontava o bloco dominante sem
dizer o que tinha dentro. `us_char_parts − us_char_transform` é submissão de malha mais
os extras por personagem (luz de terreno, ganchos de `RenderMonsterVisual`, nome,
barra, pet, marca de guild). O leitor imprime os dois, e a divisão entre eles é o que
decide o próximo corte.

Elas existem porque `us_characters` (33,7% do frame em Lorencia) e `us_simulation`
(15,2%, e o maior bloco de todos em world 3) eram as duas maiores fatias medidas e
nenhuma tinha detalhamento. As perguntas que respondem: no caso de personagens, se
o custo está no corpo/pose ou nas ~15 malhas de equipamento que um player carrega;
no caso da simulação, se está nos objetos do mundo, nos personagens, ou no update
de UI — que roda a cada frame mesmo a 170 FPS.

A **v17** acrescenta a família de multidão. Ela não mede tempo: conta o que o frame
fez, para que o tempo já medido possa ser dividido por algo.

| coluna | conteúdo |
| --- | --- |
| `chars_live_avg`, `chars_visible_avg`, `chars_visible_max` | slots vivos, dentro do frustum, e o pico de visíveis (a multidão de pior caso é o que define o orçamento; a média a esconde) |
| `chars_culled_frustum_avg` | vivos fora do frustum |
| `chars_beyond_far_avg` | vivos além do plano de recorte (`CameraViewFar * 1.4`, o valor que `BeginOpengl` passa à projeção) que **ainda são desenhados** — o teste de visibilidade de personagem é 2D e não tem plano far |
| `chars_lod0_avg` … `chars_lod3_avg` | distribuição de níveis. Tudo em L0 significa que nenhum corte teria efeito naquela captura |
| `chars_lod_forced_avg` | isentos de LOD: herói, alvo selecionado, party e mapas PvP |
| `chars_players_avg`, `chars_monsters_avg`, `chars_npcs_avg`, `chars_other_avg` | composição da multidão. `other` é trap/pet/tmp/edit, e existe para a soma fechar com `chars_live` |
| `char_poses_avg`, `char_part_meshes_avg`, `char_shadows_avg` | poses calculadas, malhas de personagem e sombras emitidas por frame |
| `char_batch_accum_avg`, `char_batch_breaks_avg`, `char_batch_run_max` | o coletor de instâncias: malhas que entraram num lote, malhas que forçaram descarga, e o maior lote do frame. **`accum / breaks` é o comprimento médio de sequência**, e é ele que decide se reordenar a emissão formaria lote ou não. Exige `-instancing=on` (o default) — com `off` nada acumula por construção e a medida é inválida. O leitor imprime a conclusão. |

Os contadores de `chars_*` cobrem o pool inteiro — **players, monstros e NPCs**. Os
três de trabalho não são simétricos, e ler como se fossem dá conclusão errada:

- `char_poses_avg` conta os dois: `RecordCharPoseUs` embrulha `Calc_ObjectAnimation`
  (player) e `RenderObject` (monstro/NPC).
- `char_shadows_avg` é **só player**. O passe de `MODEL_SHADOW_BODY`
  (`ZzzCharacter.cpp:8465`) é guardado por `o->Type==MODEL_PLAYER`; monstro não tem
  passe separado — o `EnableShadow` dele envolve o próprio corpo (`:8546`). Mesma
  assimetria que `us_char_shadow` já tinha.
- `char_part_meshes_avg` conta chamadas de `RenderPartObject`: corpo do monstro
  (`:8547`), sombra e equipamento do player. **Não vê** os modelos que desenham direto
  por `b->RenderMesh` dentro de `Draw_RenderObject` (patente, helper, dark spirit,
  caminho chrome). Irrelevante em Lorencia; não em mapa cheio de helpers.

E `-crowd=N` spawna **só players**, porque o player é o caso caro (~15 malhas contra 1
do monstro). Multidão de monstro — Blood Castle, Kalima — precisa de captura orgânica
ou de um spawner próprio.
| `crowd_lod`, `crowd_max_full`, `crowd_spawn`, `crowd_spawn_monsters`, `crowd_spawn_npcs` | regime da captura. As três de `spawn` são o que foi **pedido**; o que o rig conseguiu criar está em `chars_*_avg` |

Três identidades exatas por construção, validadas por `read_render_csv.py`:
`chars_visible + chars_culled_frustum == chars_live`, `chars_lod0..3 == chars_live` e
`players + monsters + npcs + other == chars_live`. Se uma delas não fecha, o laço de
personagens contou um slot duas vezes ou deixou de contar, e nem a composição nem a
distribuição de LOD servem.

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
tempo de frame. Ver `CROWD_LOD_PLAN.md` para o plano de LOD e culling de
personagens: as invariantes que o LOD não pode violar, e por que a Fase 0 mede
antes de cortar.

Para ler uma captura:

```powershell
python diagnostic\read_render_csv.py ..\..\Client\RenderPerformance_v17.csv
```

Ele valida as identidades do CSV **antes** de imprimir a repartição. Se alguma não
fecha, a repartição está errada e qualquer conclusão tirada dela também.