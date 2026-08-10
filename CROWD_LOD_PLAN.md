# Crowd — LOD e culling de personagens (plano)

Alvo: fazer 100–200 personagens visíveis caberem no orçamento de frame nos **três**
alvos (PC, Web/WebGL2, Android), sem mudar o que o jogador precisa ler na tela.

Este documento é plano, não relatório. Nenhum número de ganho aparece aqui porque
nenhum foi medido ainda — e é exatamente isso que a Fase 0 existe para consertar.

## 1. O que já está medido, e o que não está

Da geração v14–v16 (ver README, seção *Medição*):

| fato | fonte |
| --- | --- |
| `us_characters` = 33,7% do frame em Lorencia — a maior fatia medida | CSV v14 |
| `us_characters` já se reparte em `us_char_pose` + `us_char_shadow` + `us_char_parts` | `ZzzScene.h:43` |
| `us_simulation` = 15,2% em Lorencia e a **maior** fatia em world 3, com `us_sim_chars` próprio | `ZzzScene.cpp:160` |
| instancing está ligado **por decisão, não por ganho**: 2–3 draws instanciados cobrindo 4–6 objetos | README, `LegacyRenderAdapter.cpp:59` |
| um player carrega ~15 malhas contra 1 de corpo | comentário em `ZzzCharacter.cpp:8455` |

O que **não** está medido: como o frame escala com o número de personagens. Todas as
capturas existentes foram feitas em cena de referência com punhado de personagens.
Sem essa curva, qualquer LOD é chute — e o projeto já retratou uma conclusão por
medir a coisa errada (commit `e3cb8b7`).

Estado atual do culling de personagem:

- `MoveCharactersClient` (`ZzzCharacter.cpp:6479`) faz `TestFrustrum2D(...,-20.f)` para
  os 400 slots, e `MoveCharacterClient` (`:6452`) **repete o mesmo teste** no mesmo
  objeto no laço seguinte. Dois testes por personagem por frame.
- O teste é **2D**: polígono do frustum no plano XY, sem plano far. Só a cena
  `WD_77NEW_LOGIN_SCENE` (`:6448`) tem corte por distância, com `3800.f` fixo.
- `RenderCharactersClient` (`:11228`) só olha `o->Live && o->Visible`. Não existe
  nenhum nível intermediário entre "desenha tudo" e "não desenha".
- `MAX_CHARACTERS_CLIENT` = 400 (`_define.h:460`).

Ou seja: hoje o custo por personagem visível é **constante** — pose completa, sombra,
6 `BodyPart` + armas + asas + pet + marca de guild + balão de nome, a qualquer
distância, em qualquer tamanho na tela.

## 2. Invariantes — não negociáveis

Estes limites definem o que o LOD **não pode** fazer. Violar um deles transforma
otimização em bug de jogabilidade, que é mais caro que o frame que ela economiza.

1. **`o->Visible` não muda de semântica.** Ele é lido por lógica de jogo, não só por
   render: `GMBattleCastle.cpp:225`, `GMHuntingGround.cpp:453`, `ZzzEffect.cpp:87`,
   `w_PetActionCollecter.cpp:299`. Apertar esse campo altera targeting e eventos. O
   LOD usa campo próprio (`CrowdLodLevel`), e `Visible` continua sendo o teste de
   frustum de sempre.
2. **O tempo de animação continua andando.** O LOD corta o *cálculo* da pose, nunca o
   avanço de `AnimationFrame`. Timing de ataque, spawn de efeito preso a bone e som
   de passo não podem depender de distância.
3. **Isenções obrigatórias de LOD** (sempre L0, custe o que custar):
   `Hero`; `SelectedCharacter` e `SelectedNpc`; membros de party; e **todo** o mapa
   quando `InChaosCastle()`, `InBloodCastle()`, `InBattleCastle()`, Duel Arena ou
   Castle Siege. Legibilidade competitiva não é negociável por FPS.
4. **Toda flag tem `compare`,** que alterna por frame contra o caminho antigo — a
   divergência aparece como cintilação. Mesma escada dos itens anteriores:
   `off` → `compare` nas cenas de referência → `on` em QA → `on` em produção.
5. **Nenhuma captura pode parecer melhor do que é.** O nível de LOD ativo e o
   tamanho da multidão sintética vão para colunas do CSV, mesma regra que já vale
   para `render_scale` e `wheel_trail_cap`.
6. **Instrumentar antes de otimizar.** Cada fase entrega os contadores que provam o
   efeito dela **antes** de a otimização ser ligada.

## 3. Fase 0 — Rig, canal de configuração e contadores

Nada de LOD aqui. Esta fase produz a curva "frame × número de personagens" e o
aparato para medir as fases seguintes.

### 0.1 Canal de configuração para os três alvos

Web e Android não têm linha de comando: hoje nenhuma flag é lida neles e o default
compilado é o que roda. Um sistema que precisa de calibração por dispositivo não pode
depender só de `argv`.

**Estado: feito.** Como ficou, e o que a implementação corrigiu no plano original:

- `CarregarMainInfo` (`source/Platform/LegacyClientGlobals.cpp:103`) passa cada chave
  não reconhecida para `CrowdLod::ApplyIniKey`: `CrowdLod`,
  `CrowdLodPixelsL1/L2/L3`, `CrowdMaxFull`, `CrowdSpawn`.
- **PC: não tem canal de arquivo.** O plano dizia "linha de comando sobrescreve o
  `.ini`", e isso estava errado: `CarregarMainInfo` vive dentro de
  `#if !defined(_WIN32)`, e no PC o `MainInfo` vem de um struct binário cifrado
  (`CProtect::ReadMainFile`, `Data\Configs\Configs.xtm`), não de texto. O canal do PC
  é `argv` — `-crowdlod=off|on|compare` e `-crowd=N`. A precedência continua
  default compilado → `MainInfo.ini` → `argv`; no PC o degrau do meio não existe.
- **Web:** chaves em `web/MainInfo.web.ini`, que o CMake já pré-carrega como
  `/MainInfo.ini` (`web/CMakeLists.txt:187`).
- **Android: confirmado que faltava, e resolvido.** `android/app/build.gradle` copia
  subconjuntos de `Client/Data`, e `MainInfo.ini` vive em `Client/` — não estava no
  APK. Pior: `MainActivity.extractAssets("Data", …)` percorre só a subárvore `Data`,
  então um arquivo na raiz dos assets também não seria extraído. Correção:
  `android/app/src/main/assets/MainInfo.ini` versionado, com **só** a seção
  `[Render]`, e uma chamada explícita `extractAssets("MainInfo.ini", dataRoot)`.
  Empacotar o `MainInfo.ini` do PC resolveria o canal, mas mudaria de uma vez o
  regime de decifragem de Lua (`OnlyCryptedLua`, `PrivateCode`) e a versão de cliente
  do Android — decisão separada, com teste separado, sem relação com LOD.

### 0.2 Contadores e colunas (CSV v16 → **v17**)

Mudar o conjunto de colunas obriga a bump de versão: `kRenderCsvHeader`
(`ZzzScene.cpp:3379`) é comparado com o header do arquivo existente, e
`RotateRenderCsvIfHeaderDiffers` rotaciona em vez de anexar. Arquivar dados de v17
como `...v16.oldN.csv` seria exatamente o erro que aquele mecanismo existe para
impedir. Renomear a constante de caminho junto (`kRenderCsvPath`).

Colunas novas — 16, todas com valor real já na Fase 0:

| coluna | o que responde |
| --- | --- |
| `chars_live_avg`, `chars_visible_avg`, `chars_visible_max` | qual multidão a captura realmente viu |
| `chars_culled_frustum_avg` | quanto o teste de frustum atual corta |
| `chars_beyond_far_avg` | quantos estão além de `CameraViewFar` e **ainda são desenhados**. Se vier zerado, corte por distância (Fase 1) não paga |
| `chars_lod0_avg` … `chars_lod3_avg` | distribuição de níveis — se tudo cai em L0, o LOD é inerte |
| `chars_lod_forced_avg` | quantos foram forçados a L0 pelas isenções do item 3 |
| `chars_players_avg`, `chars_monsters_avg`, `chars_npcs_avg`, `chars_other_avg` | **composição.** 200 players e 200 monstros com o mesmo `chars_visible` são cargas diferentes; sem isso o tempo medido não se atribui a ninguém |
| `char_poses_avg` | poses calculadas por frame |
| `char_part_meshes_avg` | malhas de personagem emitidas por frame (as ~15 por player) |
| `char_shadows_avg` | sombras de personagem emitidas por frame |
| `crowd_lod`, `crowd_max_full` | regime do LOD na captura |
| `crowd_spawn`, `crowd_spawn_monsters`, `crowd_spawn_npcs` | multidão sintética **pedida** por tipo; `0` = orgânica. O que foi realmente criado está nas colunas de composição — pedir e conseguir são coisas diferentes |

O plano original previa pares `*_drawn` / `*_skipped` aqui. Eles saíram: na Fase 0
nada é cortado, então metade de cada par seria estruturalmente zero — coluna que só
existe para ser preenchida depois é ruído no arquivo. Os contadores de `skipped`
entram junto com o corte que os produz, nas Fases 2 e 3.

Três identidades exatas por construção, validadas por `read_render_csv.py` antes de
qualquer leitura: `chars_visible + chars_culled_frustum == chars_live`,
`chars_lod0..3 == chars_live` e `players + monsters + npcs + other == chars_live`. Se
uma não fecha, o laço contou um slot duas vezes ou deixou de contar — e nem a
composição nem a distribuição de LOD servem para decidir nada.

### 0.5 Player e monstro não são o mesmo problema

O escopo dos contadores, porque ler as três colunas de trabalho como simétricas dá
conclusão errada:

| | player | monstro / NPC |
| --- | --- | --- |
| `chars_*` (vivos, visíveis, níveis) | sim | sim — o laço é sobre o pool inteiro, sem filtro de `Kind` |
| `char_poses_avg` | `Calc_ObjectAnimation` | `RenderObject` — os dois contam |
| `char_part_meshes_avg` | sombra + 6 `BodyPart` + armas/asas | corpo (`:8547`). **Não vê** `b->RenderMesh` direto de `Draw_RenderObject` (patente, helper, dark spirit, chrome) |
| `char_shadows_avg` | sim | **não** — não há passe separado; `EnableShadow` envolve o próprio corpo (`:8546`) |
| isenção de LOD | Hero, alvo, party, mapa PvP | alvo (`SelectedNpc`) e mapa PvP |

Consequência para as fases seguintes, já anotada no próprio código (`:8489-8491`):
monstro não tem equipamento, então para ele `us_char_pose` tende a ser quase todo o
`us_characters`. **A Fase 2 (pose) vale para os dois; a Fase 3 (partes) é
essencialmente player.**

Um mapa de horda e uma cidade cheia são cargas diferentes e precisam de **capturas
separadas** — daí os três pedidos independentes do rig (`-crowd`, `-crowdmonsters`,
`-crowdnpcs`). Uma captura mista mede a soma e não diz de quem é o tempo; ela serve
para uma pergunta só, e vale fazer: as duas curvas isoladas somam a mista, ou existe
custo que só aparece com os dois juntos?

`diagnostic/read_render_csv.py` precisa aprender as colunas novas — a lista de fases
está explícita em `linha 31` e o aninhamento em `linha 42`.

### 0.3 Rig de multidão reproduzível (`-crowd=N`)

Sem multidão não há medição, e servidor povoado não é reproduzível. O rig popula o pool
`CharactersClient` com sintéticos ao redor do Hero, **um pedido por tipo**:
`-crowd=N` (players), `-crowdmonsters=N`, `-crowdnpcs=N`.

**Estado: feito**, em `UpdateCrowdRig` (`source/ZzzCharacter.cpp`), chamado no topo de
`MoveCharactersClient`. Como ficou:

- **Três pedidos separados, não um.** Player, monstro e NPC custam coisas diferentes;
  um número único não permitiria isolar qual move o frame. O plano original só previa
  player — o escopo passou a ser os três por decisão explícita.
- **Player:** `CreateCharacter` + `SetCharacterClass` — o mesmo caminho da rede, e é
  `SetCharacterClass` que dá a eles o equipamento completo (e a animação de parado).
- **Monstro e NPC:** `CreateMonster`, também o caminho da rede. É `Setting_Monster`
  que define o `Kind` pela tabela de tipo — o rig não classifica nada à mão, senão rig
  e jogo poderiam discordar sobre o que é monstro e o que é NPC.
- **O tipo de monstro/NPC é clonado dos que já existem vivos no mapa**, e isso não é
  conveniência: um modelo que não pertence ao mapa atual pode não estar carregado, e
  `RenderCharacter` desiste em silêncio quando `Models[Type].NumActions == 0`
  (`:8357`). Uma tabela de tipos fixa spawnaria 200 monstros invisíveis de custo zero e
  a captura mostraria um ganho inexistente. Clonar garante modelo carregado e
  composição realista, sem tabela por mapa para manter. Sem nenhum vivo para clonar, o
  rig **não cria nada** e escreve o motivo no log — e `chars_monsters_avg` denuncia.
- `Key` negativa a partir de `-30000`. O servidor só manda chaves positivas, então
  nada da rede colide com o rig e `DeleteCharacter(Key)` da rede nunca acerta um
  sintético.
- **64 slots do pool ficam reservados, e o teto é do TOTAL.** O plano dizia "cauda do
  pool", o que não resolvia nada: `CreateCharacter` toma o primeiro slot livre e é o
  mesmo pool da rede. O que protege é a folga — sem ela `-crowd=200
  -crowdmonsters=200` encheria o pool e jogadores de verdade parariam de aparecer, ou
  seja, o instrumento passaria a alterar o que deveria só medir. A soma dos três
  entrega no máximo 336.
- Posições em espiral quadrada de tiles em volta do herói, pulando `TW_NOMOVE` e
  `TW_NOGROUND`. Grade determinística, não sorteio: duas execuções precisam ser
  comparáveis.
- Vale só em `MAIN_SCENE` e só com pedido > 0. Sai de cena, troca de mapa ou volta a
  zero e eles somem.
- Efeitos colaterais aceitos: como qualquer personagem vivo, marcam `TW_CHARACTER` e
  bloqueiam caminho enquanto existem; monstro sintético é alvo clicável, e o pacote de
  ataque sai para uma chave que o servidor não conhece e é ignorado.
- Os valores vão para `crowd_spawn`, `crowd_spawn_monsters` e `crowd_spawn_npcs`:
  captura sintética não pode ser lida como orgânica.

### 0.4 Baseline — a entrega da fase

Lorencia, mesma resolução, mesmo regime de vsync registrado, com
`-renderstatscsv -crowd=0|25|50|100|200`.

Perguntas que a captura tem de responder, nesta ordem:

1. `frame_total_us` e `fps_period` crescem **linearmente** com N, ou saturam?
2. O crescimento está em `us_char_pose`, `us_char_parts` ou `us_char_shadow`?
3. `us_sim_chars` cresce junto? (Se sim, LOD de render sozinho não fecha a conta.)
4. `us_present` cresce? (Se sim, há componente de GPU/fill rate — vale cruzar com
   `-renderscale=50`. Não use `gpu_us`: ele reproduz `frame_total_us`.)

**A ordem das Fases 2 e 3 é decidida por essa medição, não por este documento.** Se o
custo estiver em `us_char_parts`, a Fase 3 vem antes.

### 0.6 Resultado da captura de base (2026-08-10)

13 amostras, `RenderPerformance_v17.csv`, `transform_cache=off` (o default) em todas.

| cena | visíveis | frame | FPS | `us_characters` | `us_char_parts` | `us_char_pose` |
| --- | --- | --- | --- | --- | --- | --- |
| Lorencia orgânica | 7–14 | 5,5–6,4 ms | 157–181 | 1,4–2,6 ms | 1,2–2,1 ms | 0,2–0,5 ms |
| Lorencia `-crowd=50` | 52–60 | 24–26 ms | 38–41 | 21–22,7 ms | **20–21,6 ms** | 0,4–0,6 ms |
| Devias `-crowdmonsters=100` | 139 | 26,2 ms | 38 | 22,2 ms | **18,5 ms** | 3,7 ms |

O frame escala linearmente com personagens: 180 FPS → 38 FPS. E o custo está
**quase todo em `us_char_parts`** (83–95% de `us_characters`), que é uma coluna de
subtração — ou seja, o instrumento apontou o bloco e não sabe o que tem dentro. Mas
duas colunas de contexto identificam:

```
us_char_parts ~= 70 ns x cpu_skinning_vertices
```

| cena | `cpu_skinning_vertices` | 70 ns × isso | `us_char_parts` medido | sobra |
| --- | --- | --- | --- | --- |
| Devias, 139 monstros | 261 k | 18,30 ms | 18,46 ms | +0,15 ms |
| Devias, 91 monstros | 192 k | 13,41 ms | 13,38 ms | −0,03 ms |
| Lorencia, 60 players | 134 k | 9,37 ms | 21,46 ms | **+12,1 ms** |

Para monstro o laço por vértice em CPU explica `us_char_parts` inteiro, com erro de
1%. Para player ele explica menos da metade: sobram ~12 ms distribuídos em 339 malhas,
**36 µs por malha** — e esse valor se repete nas três amostras (36, 38, 36).

`transforms_skipped_avg = 0` em **todas** as linhas: com `-statictransformcache=off`,
`TransformVertices` roda para todo personagem todo frame, inclusive quando a malha vai
ser desenhada pelo caminho GPU depois.

O que a captura derrubou do plano:

1. **LOD por distância não dispararia.** `chars_lod0` é 172 de 174 (monstros) e 53 de
   60 (players). Com os limiares de 64/32/12 px, praticamente nada é rebaixado — a
   câmera de MU mantém tudo grande na tela. A Fase 1 e a Fase 2, como escritas, teriam
   ganho perto de zero nesse cenário. O que resta como alavanca é o **teto por
   contagem** (`CrowdMaxFull`), não a distância.
2. **Fase 2 (pose) é o menor prêmio.** `us_char_pose` é 0,6 ms (players) e 3,7 ms
   (monstros) de um frame de 26 ms. Cortar metade rende ~2 ms no melhor caso.
   **A Fase 3 passa na frente da Fase 2** — mas as duas ficam atrás dos dois itens
   abaixo.
3. **Sombra é irrelevante:** `us_char_shadow` = 10 µs. Não há o que cortar.
4. **A GPU está ociosa:** `us_present` = 45–48 µs. O frame é 100% CPU. Isso explica
   retroativamente por que instancing e batching não moveram tempo: eles reduzem draw
   calls, e draw call não é o gargalo. Com 139 monstros quase idênticos —
   o caso ideal de instancing — `instanced_draws` continua **2** e `instances` **4**.

Prioridade que sai daí, em ordem de valor medido:

1. **Medir `-statictransformcache=on` sob multidão.** Ataca exatamente o bloco
   dominante: até 18,3 ms dos 26,2 ms em Devias. A flag já existe, está `off` por
   default e nunca passou pela escada — porque não havia multidão para medir. Agora há.
   Ressalva: ela só ganha se ninguém ler os arrays transformados; se o caminho legado
   ler, `EnsureVerticesTransformed` roda o laço e o ganho é zero. A coluna
   `transforms_skipped_avg` diz qual dos dois aconteceu.
2. **Repartir `us_char_parts`** em transformação e submissão. É o que falta para
   atacar os 36 µs/malha do caso player (12 ms), e hoje é subtração, não medida.
   **Feito:** coluna `us_char_transform` (CSV com 124 colunas), cronometrando
   `BMD::TransformVertices` dentro da fase Characters. O leitor imprime
   `us_char_transform` e `submissao+extras` (a subtração) sob `us_char_parts`, com o
   custo por malha. Nenhuma captura existe ainda com ela — os 70 ns/vértice são conta
   de guardanapo, e é exatamente isso que a coluna substitui.
3. **Instancing: investigado, e a conclusão é não mexer agora.** Os modelos
   qualificam — `ShouldInstanceModel` libera tudo com a whitelist vazia, e o teto de
   200 ossos por paleta (`kMaxInstanceBones`) não é alcançado por monstro nenhum. O
   que impede é o **descarregamento entre personagens**:
   `FlushOpaqueWorldQueueForImmediateDraw` (`ZzzBMD.cpp:268`) esvazia a fila de opacos
   — e com ela o coletor de instâncias — sempre que uma malha compõe com o
   framebuffer, isto é, `alpha < 0.99`, qualquer flag de CHROME/METAL/OIL/LIGHTMAP/
   NODEPTH, ou malha com script de textura. Numa multidão essas malhas vêm
   intercaladas, então o balde raramente passa de 2 instâncias — exatamente o
   `instance_batch_max = 2` que o README registrou.
   **Mas o prêmio é zero:** `us_present` = 48 µs, a GPU está ociosa, e instancing
   reduz draw calls. Agrupar personagens por modelo antes de emitir resolveria o
   flush, e não moveria o tempo de frame. Fica registrado, não priorizado.
4. Só então Fase 3, e por último Fase 2.

### 0.7 `-statictransformcache=on` sob multidão (2026-08-10, segunda captura)

Devias, `-crowdmonsters=100`. **1 amostra `off`, 2 amostras `on`.**

| coluna | off | on | Δ |
| --- | --- | --- | --- |
| `frame_total_us` | 28.922 | 16.872 | −42% |
| `fps_period` | 34,6 | 59 | +72% |
| `fps_min` | 25,2 | 37 | +47% |
| `us_characters` | 23.507 | 12.167 | −48% |
| `us_char_parts` | 19.415 | 10.937 | −44% |
| `us_char_transform` | 4.760 | 1.341 | −72% |
| `us_char_pose` | 4.082 | 1.218 | −70% |
| `cpu_skinning_vertices` | 261 k | 69 k | −74% |
| `transforms_exec` | 396 | 147 | −63% |
| `transforms_skipped` | **0** | **361** | — |
| `draws_avg` | 2.340 | 1.113 | −52% |

**A flag funciona, e o mecanismo está provado:** `transforms_skipped` saiu de zero,
`transforms_exec` caiu 63%, e o laço por vértice caiu 72%. Isso não depende de
interpretação.

**Mas o Δ de frame estava contaminado.** A amostra `off` foi capturada com **27 rastros
de Twisting Slash** vivos em média (pico 53) e 32 efeitos, contra **zero** rastros e 6
efeitos nas duas amostras `on`, e com 4% mais personagens visíveis. Estimei na época
que ~9,7 dos 12 ms sobrariam para a flag, ou seja −34%.

**A recaptura limpa refutou também essa estimativa.** Com `-wheeltrail=0` nos dois
lados e contagem de personagens casada (135 vs 136):

| coluna | off | on | Δ |
| --- | --- | --- | --- |
| `chars_visible` | 135 | 136 | +1% |
| `frame_total_us` | 29.065 | 24.585 | **−15%** |
| `fps_period` | 34,4 | **40,7** | **+18%** |
| `us_characters` | 25.768 | 22.165 | −14% |
| `us_char_transform` | 4.634 | 2.511 | −46% |
| `us_char_mesh` | 18.802 | 17.301 | −8% |
| `transforms_skipped` | 0 | 394 | — |

O ganho real de `-statictransformcache=on` é **−15% de frame / +18% de FPS**, não −42%
nem −34%. As duas estimativas anteriores vieram de comparar amostras de cenas
diferentes. Continua sendo um ganho que vale ligar — de graça, sem cortar detalhe — mas
uma ordem de grandeza abaixo do que o primeiro par de amostras sugeriu.

**Regra que sai daí:** comparar duas configurações exige `chars_visible` casado e
`-wheeltrail=0` nos dois lados. Sem isso o Δ de frame não é do que se está medindo.

#### Retratação: a conta de 70 ns/vértice estava errada

A seção 0.6 afirmou que `70 ns × cpu_skinning_vertices` explicava `us_char_parts`
inteiro no caso monstro, com erro de 1%. A medição direta refuta: o laço custa
**18–20 ns por vértice**, e `us_char_transform` era 4.760 µs de 19.415 — **24%**, não
100%. A concordância de 0,6 ms foi coincidência entre duas grandezas que crescem
juntas com o número de personagens.

O erro não mudou a prioridade (o item 1 continua sendo o certo, por outro motivo: o
trabalho é jogado fora), mas mudou o alvo seguinte — e é por isso que a coluna existe.
Correlação em planilha não é medição; foi o mesmo tipo de raciocínio que produziu a
retratação do commit `e3cb8b7`.

#### O que a repartição de `us_char_parts` revelou

`us_char_mesh` (tempo dentro de `RenderPartObject`) é o bloco real:

| cena | visíveis | malhas | `us_char_mesh` | por malha | por personagem | extras por personagem |
| --- | --- | --- | --- | --- | --- | --- |
| 100 monstros, off | 135 | 140 | 18.802 | 135 µs | 139 µs | 21 µs |
| 100 monstros, on | 136 | 141 | 17.301 | 123 µs | 127 µs | 21 µs |
| 50 players + 73 monstros, off | 86 | 341 | 13.494 | 40 µs | 157 µs | 113 µs |
| 50 players + 73 monstros, on | 88 | 343 | 13.635 | 40 µs | 155 µs | 109 µs |

Três coisas:

1. **`us_char_mesh` é 70% do frame** no caso monstro (17,3 ms de 24,6 ms). É aqui que o
   tempo está, e não nos extras por personagem — que são 21 µs no monstro.
2. **O custo por malha não é constante** (40 µs no player, 123–135 µs no monstro), mas o
   custo **por personagem** é (127–157 µs nos quatro casos). Malha de monstro é grande e
   única; parte de player é pequena e vem em ~4. Então o que escala não é "quantas
   malhas" — é o volume de cada uma.
3. **Player cobra 109–113 µs de extras por personagem contra 21 µs do monstro.** Cinco
   vezes mais. É o que se esperaria de nome, barra de vida, marca de guild, pet e
   ganchos que só existem para player.
4. O cache de transformação quase não move `us_char_mesh` (−8% no monstro, 0% no
   player): o que ele corta é o laço por vértice ansioso, que era 4,6 ms.

E o custo por malha **não é efeito de multidão**: na amostra de 1 personagem visível
(mundo 74) `us_char_mesh` deu 347 µs para ~4 malhas, ~87 µs cada. O motor cobra ~100 µs
para emitir uma malha de personagem em qualquer regime. Com 140 malhas isso é 17 ms, e
é o frame inteiro.

Por isso a próxima coluna é `us_char_draw`, cronometrando `RenderPartObjectEffect` — que
apesar do nome é o desenho de verdade (`b->RenderBodyShadow`, `RenderBody` e companhia).
`RenderPartObject` é setup + `b->Transform` + cloth + esse desenho, e `us_char_mesh`
menos `us_char_draw` isola o setup e a matriz de osso.

### 0.8 O caso player, e um bug na minha própria instrumentação

Lorencia, `-crowd=50`, `-wheeltrail=0`, `transform_cache=off`. 59 visíveis (51 players
sintéticos + 12 NPCs, **nenhum monstro** — o caso player finalmente limpo):

| | valor |
| --- | --- |
| `frame_total_us` | 29.195 (34,3 FPS) |
| `us_characters` | 25.858 (89% do frame) |
| `us_char_parts` | 24.614 |
| `us_char_mesh` | 13.413 — 41 µs/malha, **227 µs por personagem** |
| `us_char_transform` | 2.488 |
| `us_char_parts − us_char_mesh` | 11.201 — **190 µs por personagem** |

Um player custa ~2,5× um monstro: 51 players dão os mesmos 34 FPS que 135 monstros.

#### O bug: `us_char_draw` saiu com 125–147% de `us_char_mesh`

Impossível se um aninha no outro. Causa: `RenderPartObjectEffect` tem **dois**
chamadores, e só um está dentro de `RenderPartObject`. O outro é `RenderLinkObject`
(`ZzzCharacter.cpp:7222`) — armas, asas e partes ligadas — que tem `PlayAnimation`,
`Animation` e `Transform` próprios e **não passa** por `RenderPartObject`. A coluna
media os dois, então `mesh − draw` dava negativo.

**O aninhamento estava afirmado num comentário meu e não era verificado.** Consequência
direta: na seção anterior eu atribuí os 113–190 µs de "extras por personagem" a nome,
barra de vida, marca de guild e pet. Errado — **o desenho de arma sozinho já era ≥6,3
ms** daquele bloco.

Correções:

1. `us_char_draw` só conta quando aninhado em `RenderPartObject` (imposto no código,
   não em comentário). Agora `us_char_mesh − us_char_draw` é setup + matriz de osso +
   cloth, e a conta fecha.
2. Coluna nova `us_char_link` para `RenderLinkObject` — irmã de `us_char_mesh` dentro de
   `us_char_parts`, não filha. CSV com 127 colunas.
3. `read_render_csv.py` passou a validar o aninhamento **declarado** (tabela `NESTING`)
   em vez de só comparar com o avô. Rodado contra a captura bugada, ele agora reprova
   com `us_char_draw (19706 us) excede us_char_mesh (13413 us)`.

O terceiro item é o que importa a longo prazo: era exatamente esse tipo de afirmação
não verificada que produziu a retratação de 70 ns/vértice duas seções acima. Duas vezes
seguidas o erro foi eu **declarar** uma relação em vez de medi-la.

### 0.9 A repartição fechada, e o alvo real

Lorencia, `-crowd=50`, `-wheeltrail=0`, 59 → 58 visíveis (51 players + 12 NPCs). Agora
todas as identidades fecham e `us_char_parts` = `mesh` + `link` + `miúdo`, exatamente.

| bloco | off | on | por personagem (off) |
| --- | --- | --- | --- |
| `frame_total_us` | 28.508 | 26.965 (−5%) | — |
| `us_char_mesh` | 12.952 | 12.552 | 220 µs |
| ├ `us_char_draw` | 11.603 | 12.282 | 197 µs |
| └ setup + ossos | 1.349 | 270 (**−80%**) | 23 µs |
| `us_char_link` (arma/asa) | 10.355 | 9.710 | **176 µs** |
| `us_char_transform` | 2.434 | 2.032 | 41 µs |
| miúdo por char | 606 | 518 | **10 µs** |

Cinco conclusões, todas medidas:

1. **`us_char_draw` é 90% de `us_char_mesh`.** O custo é emitir geometria, não o setup
   em volta — setup+ossos é 10%, e o cache de transformação o derruba 80%.
2. **`us_char_link` é 43% de `us_char_parts` e 36% do frame inteiro.** Arma e asa custam
   176 µs por personagem, quase o mesmo que o corpo e todo o equipamento juntos.
3. **O "miúdo" é 10 µs por personagem.** Nome, barra de vida, marca de guild, pet e
   ganchos de mapa são irrelevantes — o oposto do que eu afirmei na seção 0.8 antes de
   medir.
4. **O cache de transformação rende pouco para player: −5%** (contra −15% no monstro).
   Coerente: o custo do player não é o laço por vértice.
5. **Um player custa ~2,5× um monstro.** 51 players e 135 monstros dão os mesmos ~35 FPS.

#### O alvo real: o GPU skinning nunca engata

`gpu_skinning_material_fallbacks` = **824 sub-malhas por frame**, contra 0 de geometria e
0 de recurso. Com `-gpuskinning=on`. E `us_present` = 51 µs — **a GPU está ociosa**.

Ou seja: o skinning de personagem roda inteiro na CPU, e é isso que `us_char_draw` e
`us_char_link` estão medindo. Juntos são 22 ms de um frame de 28,5 ms — **77%** — de
trabalho que existe hardware parado para fazer.

Isso reordena tudo o que restava: nenhum corte de LOD chega perto de 77% do frame, e o
corte não precisaria existir se o trabalho fosse para onde deveria ir.

`staticGpuCandidate` (`ZzzBMD.cpp:1606`) tem doze condições e o contador as agrega numa
só. Foi adicionado um diagnóstico que escreve no `MuError.log` uma linha por combinação
(modelo, motivo) nova, com teto de 64 e só sob `-renderstatscsv`:

```
[GpuSkinReject] modelo 1: renderflags+alpha<0.99 (renderflags nao suportadas 0x...)
```

Log e não coluna porque a pergunta é pontual: respondida, o trabalho é consertar a
condição, não acompanhar a métrica.

#### O bloco que passou a dominar

Com a flag ligada, `us_char_parts − us_char_transform` = **9,6 ms** de um frame de
16,9 ms, para ~132 personagens visíveis com **uma malha cada**: ~73 µs por
personagem. Não é volume de geometria, é custo por personagem. Daí a coluna
`us_char_mesh` (CSV com 125 colunas), cronometrando `RenderPartObject`:
`us_char_parts − us_char_mesh` isola os extras por personagem — luz de terreno,
ganchos de `RenderMonsterVisual` de cada mapa, nome, barra de vida, pet, marca de
guild.

**Achado que reforça o item 1.** `transforms_exec_avg` = 366 no caso de 139 monstros,
contra apenas 21 fallbacks de GPU skinning: 366 modelos passam pelo laço por vértice
enquanto quase todos são depois desenhados pelo caminho GPU, **que não lê
`VertexTransform`**. O laço roda porque `BMD::Transform` o chama de forma ansiosa
(`ZzzBMD.cpp:719-722`) quando o cache está desligado, antes de existir qualquer decisão
sobre GPU. Ou seja: os ~18 ms não são custo necessário mal distribuído, são trabalho
jogado fora. É por isso que o item 1 é o primeiro.

## 4. Fase 1 — Culling correto (sem custo visual)

O corte mais barato é o que não desenha nada. Nada aqui degrada imagem.

- **Um teste de visibilidade por personagem por frame.** Remover a duplicação
  `:6479` / `:6452`, guardando o resultado.
- **Corte por distância de verdade.** O teste 2D atual não tem plano far: um
  personagem muito além do alcance da câmera passa o polígono e é desenhado. Corte
  derivado de `CameraViewFar` (não constante mágica — `CameraViewFar` varia por cena:
  `Widescreen.cpp:120` usa 33000, `ZzzLodTerrain.cpp:1993` usa 8500).
- **Distância ao quadrado, uma vez por frame,** guardada junto do nível de LOD para
  as fases seguintes. Sem `sqrtf` e sem recomputar por consumidor.
- **Teto de personagens em qualidade plena** (`CrowdMaxFull`): passado o teto, os mais
  distantes descem de nível em vez de o frame afundar. Impede o pior caso (200 players
  numa esquina de Lorencia) de degradar sem limite.

Aceite: `chars_culled_far_avg > 0` na captura de 200; `compare` sem cintilação;
`frame_total_us` menor ou igual ao baseline. Nenhuma mudança em `o->Visible`.

## 5. Fase 2 — LOD de pose

Alvo: `us_char_pose`. Hook natural: o cache que já existe. `-statictransformcache`
adia o laço por vértice de `BMD::Transform` e cacheia a pose em `BMD::Animation`.

Níveis por **tamanho na tela**, não por distância crua — `CameraZoom`, `CameraFOV` e
a resolução mudam a relação, e `TestFrustrum2D` já compensa zoom à mão (`:2194`):

```
px ≈ alturaViewport * alturaModelo / (2 * d * tan(FOV/2))
```

| nível | limiar inicial | pose |
| --- | --- | --- |
| L0 | > 64 px | todo frame |
| L1 | 32–64 px | recalcula a cada 2 frames, reusa a pose cacheada no intervalo |
| L2 | 12–32 px | recalcula a cada 3–4 frames |
| L3 | < 12 px | não desenha |

Limiares são **ponto de partida para calibrar em `compare`**, não valores medidos.

Pontos de atenção:

- `Calc_ObjectAnimation` (`ZzzCharacter.cpp:8461`) faz avanço de frame **e** cálculo de
  pose. O LOD corta só a segunda parte (invariante 2). Se a função não separa as duas
  coisas hoje, separá-la é parte desta fase.
- Escalonar as fases de recálculo por índice de personagem: sem isso, todos os L1
  recalculam no mesmo frame e o custo economizado volta como pico. `frame_total_us_max`
  e `fps_min` são as colunas que enxergam isso — a média de 120 frames dilui.
- **Aqui é onde o instancing pode finalmente pagar.** Personagens no mesmo LOD com
  mesma malha e mesmo estado são candidatos naturais a lote. Esperar
  `instance_batches_avg` e `instance_batch_max` subirem; se não subirem, o instancing
  continua inerte e isso precisa ser dito no relatório.

Aceite: `poses_reused_avg > 0`; queda em `us_char_pose` proporcional à distribuição
de L1/L2; `frame_total_us_max` **não** piora; `compare` sem travamento visível de
animação nos personagens próximos.

## 6. Fase 3 — LOD de partes e adornos

Alvo: `us_char_parts`, a fatia das ~15 malhas.

O laço de equipamento é `for(int i=MAX_BODYPART-1; i>=0; i--)` em
`ZzzCharacter.cpp:9435`, com `MAX_BODYPART` = 6 (`_define.h:132-138`).

| nível | corta |
| --- | --- |
| L1 | sombra (`MODEL_SHADOW_BODY`, `:8481`), pet (`giPetManager::RenderPet`), marca de guild (`RenderGuild` / `RenderProtectGuildMark`), balão de nome |
| L2 | `BODYPART_GLOVES`, `BODYPART_BOOTS`, `BODYPART_HELM` |
| L3 | nada é desenhado |

Nunca cortados acima de L3: `BODYPART_ARMOR`, `BODYPART_PANTS`, `BODYPART_HEAD`,
armas e asas — são o que define silhueta e identificação de classe. Uma multidão em
que não se distingue um DL de um SM a 40 px é bug de leitura, não otimização.

Ordenação: agrupar os personagens opacos por modelo antes de emitir aumenta a chance
de fusão que `-batching` já faz. **Só os opacos** — reordenar quem tem alpha muda o
resultado do blend.

Aceite: `char_parts_skipped_avg > 0`; queda em `us_char_parts`; `draws_avg` cai;
`compare` sem pop visível ao personagem se aproximar (se houver, os limiares estão
apertados demais, ou o nível precisa de histerese).

## 7. Fase 4 — Imposters (condicional)

**Só se as Fases 1–3 não fecharem o orçamento.** Custo alto, e a decisão precisa vir
de medição, não de gosto.

A câmera de MU é isométrica de ângulo fixo, o que torna imposter mais viável aqui do
que num jogo de câmera livre: um snapshot por ~8 faixas de yaw cobre tudo, sem
variação de pitch. Cache indexado por (modelo, hash de equipamento, ação, faixa de
direção), desenhado como quad orientado à câmera, aplicado só a L2/L3.

Riscos que precisam de teto explícito: memória do atlas; número de refreshes de
render-to-texture por frame (no Web isso é caro e não pode ser ilimitado); e
invalidação quando o jogador troca equipamento.

## 8. Fase 5 — Rollout e defaults por alvo

Escada de sempre: `off` → `compare` nas cenas de referência → `on` em QA → `on` em
produção. Cenas de referência para esta rodada: Lorencia com `-crowd=100` e
`-crowd=200`, um mapa com água (por causa do segundo passe em `us_water`), e um mapa
de isenção total (Chaos Castle) para provar que o LOD fica desligado lá.

Default por alvo é **decisão separada por alvo**, cada uma com captura própria: Web e
Android são os alvos fracos e provavelmente querem `on` antes do PC, mas nenhum ganha
default novo sem `compare` rodado naquele alvo.

## 9. Arquivos tocados

| arquivo | o quê |
| --- | --- |
| `source/CrowdLod.h/.cpp` (novo) | níveis, limiares, isenções, contadores |
| `cmake/LegacySceneSources.cmake` | **obrigatório** para o módulo novo: Web e Android compartilham essa lista, e fora dela o arquivo não existe nesses alvos |
| `Main.vcxproj` + `.filters` | mesmo módulo, alvo PC |
| `source/ZzzCharacter.cpp` | `:6437`, `:6462` (visibilidade e distância), `:8349` (pose `:8461`, sombra `:8465`, partes `:9435`), `:11228` (laço de render) |
| `source/ZzzScene.cpp` | parsing de flag (`:3710`), header e escrita do CSV (`:3379`, `CaptureRenderStatsCsv`), fases e contadores |
| `source/ZzzScene.h` | API `Record*` dos contadores novos |
| `source/Platform/LegacyClientGlobals.cpp` | `[Render]` em `CarregarMainInfo` (`:103`) |
| `web/MainInfo.web.ini` | chaves do Web |
| `android/app/build.gradle` | só se `MainInfo.ini` precisar entrar nos assets (ver 0.1) |
| `diagnostic/read_render_csv.py` | colunas v17 (`:31`, `:42`) |
| `README.md` | tabela de flags, colunas novas, bump v16 → v17 |

## 10. Riscos

| risco | mitigação |
| --- | --- |
| LOD altera jogabilidade via `o->Visible` | campo separado; `Visible` intocado (invariante 1) |
| animação travando/pipocando em PvP | isenções da invariante 3, aplicadas antes de qualquer cálculo de nível |
| pop visível ao trocar de nível | histerese nos limiares; validar em `compare`, não em captura agregada |
| picos por recálculo sincronizado | escalonar fase por índice; olhar `fps_min` e `frame_total_us_max` |
| CSV corrompido pela mudança de colunas | bump para v17 em `kRenderCsvPath` **e** `kRenderCsvHeader` juntos |
| ganho aparente vindo do rig sintético | coluna `crowd_spawn` em toda linha |
| módulo novo quebrando Web/Android | entrar em `LegacySceneSources.cmake` no mesmo commit |
| Fase 2/3 medirem nada porque o custo é de GPU | Fase 0.4 pergunta isso antes, cruzando `us_present` com `-renderscale` |
