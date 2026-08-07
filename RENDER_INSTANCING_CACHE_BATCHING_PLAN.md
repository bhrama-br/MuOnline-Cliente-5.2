# Plano — GPU Instancing (monstros), Cache de objetos estáticos e Batching

Continuação direta das fases já entregues (`39fa476` pipeline GL 3.3, `54a047a` opacos de
terreno, `f66dd52` GPU skinning, `74ccb16` deployment/whitelist). Alvo: **OpenGL 3.3 core**
e **WebGL 2 / GLES 3** com o mesmo código — isso elimina SSBO, `GL_TEXTURE_BUFFER` e
`glMultiDrawElementsIndirect` do desenho. `glDrawElementsInstanced` e
`glVertexAttribDivisor` são core nos dois, e são a base do plano.

---

## 0.0 CORREÇÃO IMPORTANTE — o instrumento de GPU estava quebrado

**`gpu_us` não media carga de GPU. Media o período do frame.**

| mundo | fps | frame_total_us | gpu_us | cpu_us | CPU / frame |
| --- | --- | --- | --- | --- | --- |
| 0 Lorencia | 172 | 6.083 | 6.074 | 5.041 | 83% |
| 3 | 149 | 7.121 | 7.101 | 4.150 | 58% |
| 4 Noria | 178 | 5.716 | 5.701 | 4.790 | 84% |
| 94 | 131 | 7.986 | 7.936 | 7.299 | 91% |

`gpu_us ≈ frame_total_us` em todas as linhas. `GL_TIME_ELAPSED` mede o tempo
decorrido na timeline da GPU entre as duas queries — **ocioso incluso** —, e as
queries envolviam o frame inteiro. Ele estava apenas remedindo o frame.

**Fica retratado tudo que foi concluído a partir dele**, em particular:

- ~~"o frame é limitado pela GPU"~~ — não há evidência disso.
- ~~"o piso é 6.597 µs de GPU"~~ — esse número era o período do frame.
- ~~"não é fill rate"~~ — a sonda comparou dois valores de `gpu_us`, que não
  mediam pixels. O teste de fill rate precisa ser refeito contra `frame_total_us`.

### O que os dados válidos dizem

- **O jogo roda a 130-178 FPS. Não há teto ativo** — o limitador não está atuando.
- **A região de CPU medida é 58-91% do período do frame.** Ela é o termo dominante,
  não um detalhe.
- Portanto os ganhos de CPU **se traduzem em frame**, e as sete rodadas de "ganho
  zero" precisam ser relidas: elas foram comparadas entre capturas com cena e
  câmera diferentes, e contra um `gpu_us` que não media nada.

**A métrica correta para qualquer comparação daqui em diante é `frame_total_us`
e `fps`, não `cpu_us` e nunca `gpu_us`.**

## 0. Placar final

| entrega | contador | tempo de frame | estado |
| --- | --- | --- | --- |
| Fase 1 — cache de malha | −50% upload, índices 16 bits | não isolado | `-meshcache`, **default on** |
| Fase 2 — cache de transform | skinning CPU −70% | **zero** | `-statictransformcache`, default off |
| Fase 3.1 — shadow state | −12.001 `glUniform`/frame | **zero** | `-batching`, default off |
| Fase 3.2 — submissão em bloco | −470k chamadas virtuais/frame | **zero** | `-batching`, default off |
| Fase 3.2 — fusão de comandos | não fundiu nada até a 3.3 | **zero** | `-batching`, default off |
| Fase 3.3 — redução de flushes | fila desfragmentada | não isolado | `-batching`, default off |
| Fase 4 — instancing | 2 draws/frame em Lorencia | −42% **pior**, depois neutro | `-instancing`, default off |
| **Espelho de matrizes em CPU** | **6 → 0 readbacks/frame** | **−41% (7.596 → 4.450 µs)** | `-cpumatrices`, **default on** |

**A única entrega com ganho de tempo reproduzível não estava no plano.** Ela saiu da
medição por fase, feita depois de a segunda otimização sem resultado.

### Ressalva importante sobre os "zero"

**Zero é zero *nesta máquina*, onde a GPU é o gargalo.** Numa máquina com CPU mais
fraca e a mesma GPU, o quadro inverte: o cache de transform (−70% de skinning) e a
submissão em bloco (−470k chamadas virtuais por frame) passariam a valer, porque
ali a CPU seria o polo mais longo.

Cliente de MU roda num leque grande de hardware. Chegou-se a recomendar apagar
essas fases por "não renderem nada" — **isso foi apressado e fica retratado aqui**.
A decisão certa exige medir num PC representativo do pior caso de CPU, não no
melhor. Até lá elas ficam desligadas por padrão e disponíveis por flag.

Correções de bugs pré-existentes encontradas no caminho:

- `Platform/RenderPipeline.cpp` nunca esteve no `web/CMakeLists.txt`: o cliente Web
  abortava em runtime ao renderizar objetos.
- `uShadowMap` vazava de `DrawStaticMesh` para o caminho imediato, achatando geometria.
- `polygon == 4` alocava 6 vértices e escrevia 4, enviando o resto não inicializado.
- Timer de CPU em `GetTickCount` (granularidade 15,6 ms) não conseguia medir um frame de 5 ms.

## 0. Progresso

- [x] **Fase 0 — Instrumentação e baseline**
  - [x] `LegacyRenderFrameStats` estendido (instancing, cache de malha, cache de pose, uniformes poupados)
  - [x] `RecordCpuTransformWork` na interface + implementação no adaptador GLSL
  - [x] `RenderFeature` / `RenderFeatureMode` com modo `compare` por paridade de frame
  - [x] `RenderPerformance_v6.csv` com 15 colunas novas
  - [x] Flags `-instancing=`, `-statictransformcache=`, `-batching=` (`off|on|compare`)
  - [x] Build PC x86 Release limpo
- [x] **Fase 1 — Cache de malha estática real**
  - [x] Indexação real em `PrepareStaticGpuMesh` (dedup por `(vertex, normal, texCoord)`)
  - [x] Índices de 16 bits quando os vértices únicos couberem (conversão no backend)
  - [x] Handle O(1) com geração de contexto, no lugar de `std::map<const void*, …>`
  - [x] `Mesh_t::GpuMeshHandle` + `IsStaticMeshResident` (auto-recuperação após perda de contexto)
  - [x] Staging de CPU liberado após o upload (`ReleaseStaticGpuMeshStaging`)
  - [x] Corrigido de passagem: `polygon == 4` alocava 6 vértices e escrevia 4 — o resto ia
        para a GPU não inicializado. Sem efeito prático (todo BMD do cliente é triângulo),
        mas era um buffer parcialmente indefinido.
  - [ ] *Adiado* — 1.2 (um VBO por modelo em vez de por malha). Ganho real, mas a Fase 4
        usa `meshIndex` na chave de lote e não depende disso. Reavaliar com os números da
        Fase 3 em mãos.
  - [x] Build PC x86 Release limpo
- [x] **Fase 2 — Cache de objetos estáticos** *(atrás de `-statictransformcache=on`, default off)*
  - [x] `BMD::Transform` dividido: parte barata (paleta GPU, luz, OBB) sempre; laço por
        vértice movido para `BMD::TransformVertices`, adiado até a primeira leitura
  - [x] `BMD::EnsureVerticesTransformed()` com token de dono (os arrays são globais)
  - [x] Snapshot da paleta ao adiar — `BoneTransform` é global e o próximo objeto a animar
        já a sobrescreveu quando a leitura adiada chega
  - [x] Todos os leitores cobertos: `RenderMesh` (após o retorno do caminho GPU),
        `RenderMeshAlternative`, `RenderMeshTranslate`, `RenderMeshEffect`,
        `CollisionDetectLineToMesh`, `CreateLightMapSurface`, `AddToCoinHeap`,
        `AddMeshShadowTriangles`, `CSideHair`, `CPhysicsClothMesh` (leitura **e** escrita),
        `GMNewTown`, `GM_Raklion`
  - [x] Cache de pose em `BMD::Animation` com comparação exata (não hash) — colapsa
        fileiras de cenário idêntico que hoje recalculam a mesma pose objeto a objeto
  - [x] Build PC x86 Release limpo
  - [n/a] 2.4 (bbox persistente) — em Release com `EditFlag != 2` o OBB já vem do
        bounding box pré-calculado do modelo; não havia trabalho por frame para cortar
  - [ ] *Não feito* — 2.5 (corte por distância para objetos pequenos). É mudança de
        política visual, não de desempenho puro; precisa de decisão sobre alcance.
- [x] **Timer de CPU corrigido** *(fora do plano original — o instrumento estava quebrado)*
  - [x] `GetTickCount` (granularidade ~15,6 ms) → `QueryPerformanceCounter` em microssegundos.
        Todas as linhas do v6 traziam `cpu_ms_p95 = 16` em qualquer modo: era o tick do
        relógio, não o frame. Um frame de 5 ms não era mensurável.
  - [x] CSV vira **v7**, com `cpu_us_avg` / `cpu_us_p95`
- [ ] **Fase 3 — Batching** *(atrás de `-batching=on`, default off)*
  - [x] 3.1 Shadow state completo dos uniformes de modelo/skinning, usado pelos dois
        caminhos (imediato e `DrawStaticMesh`)
  - [x] 3.1 `InvalidarCacheDeEstado()` removido do fim de `DrawStaticMesh` — só o VAO é
        invalidado. Antes, cada malha do *mesmo* modelo reenviava as ~20 chamadas
  - [x] 3.1 `Flush` deixa de reenviar 8 uniformes neutros por lote de UI
  - [x] 3.1 `uWorldTime` só é enviado quando `wave` ou efeito de material o usa
  - [x] 3.1 **Correção de vazamento**: `Flush` agora zera `uShadowMap`. `DrawStaticMesh`
        podia deixá-lo em 1 e o caminho imediato nunca o zerava — a geometria seguinte
        saía achatada em `z = 5`. Esta correção vale nos dois modos do flag.
  - [x] 3.2 **Submissão em bloco** — `DrawVertices` (AoS) e `DrawVertexArrays` (SoA) na
        interface, com implementação padrão idêntica ao caminho por vértice. O backend GLSL
        especializa com append em memória. Convertidos os dois emissores mais quentes:
        `DrawLegacyVertexArray` (BMD legado) e o replay da fila de opacos.
        Efeito colateral do caminho antigo preservado: cor/UV/normal do último vértice
        continuam virando estado corrente (`HerdarUltimoVertice`), e a saturação de cor
        do `glColor*f` é reproduzida.
  - [x] 3.2 **Fusão de comandos adjacentes** de mesmo passe/material/topologia em
        `RenderQueue::Execute`. O terreno emite **um comando por tile** (4 vértices); sem
        fusão, um mapa inteiro vira milhares de draws de um quad cada. Fusão só entre
        comandos adjacentes na ordem final, então preserva composição — inclusive de
        transparentes.
  - [x] `static_assert` de layout entre `RenderVertex` e `LegacyBulkVertex` (o
        `reinterpret_cast` da fila depende disso)
  - [ ] 3.3 Redução dos pontos de flush
  - [ ] 3.4 Ring buffer persistente para UI/texto
- [x] **Medição por fase** *(fora do plano original — o plano estava errado sobre onde está o custo)*
  - [x] `ScopedRenderPhase` em terreno, objetos, personagens, efeitos e sprites
  - [x] CSV vira **v8** com `us_terrain`, `us_objects`, `us_characters`, `us_effects`, `us_sprites`
  - [x] Paridade formato/argumentos do `fprintf` verificada (55 × 55)

---

## 0.1 Resultados medidos — e o que eles derrubaram do plano

Lorencia, 1360x768, `glsl` + `gpuskinning=on`, 120 frames por amostra.

### Fase 2 (cache de transform), v6

| modo | cpu | draws | cpu_skin_vtx | tf_exec | tf_skip |
| --- | --- | --- | --- | --- | --- |
| off | 5,51 ms | 895 | **22.529** | 165,5 | 0 |
| on | 5,47 ms | 1009 | **6.798** | 27,1 | **147,4** |

Skinning de CPU **−70%**, 84% dos `Transform` adiados. **Ganho de tempo: zero.**

O cache de pose do `Animation` acerta 7% (8,8 de 123,4) — como previsto depois da correção
da chave, ele não paga.

### Fase 3.1 (shadow state de uniformes), v7

| batching | cpu_us | p95 | draws | uniform_saved |
| --- | --- | --- | --- | --- |
| off | 5041 | 5915 | 1151 | 0 |
| on | 5063 | 5776 | 1184 | **12.001** |

**12 mil `glUniform` a menos por frame. Ganho de tempo: zero.**

### Conclusão

Duas otimizações grandes, ambas confirmadas pelos contadores, ambas sem efeito no tempo de
frame. O custo não está no skinning de CPU nem na submissão de uniformes. O que sobra, pelos
contadores do mesmo CSV:

- **117.405 vértices/frame** ainda passam pelo caminho imediato, emitidos **um a um** por
  chamada virtual (`Color4f` + `TexCoord2f` + `Normal3f` + `Vertex3fv`) — da ordem de
  **470 mil chamadas virtuais por frame**
- **1.079 `glBufferSubData` + 999 flushes** por frame, 968 KB em uploads picados
- o caminho GPU residente cobre só 184 draws / 96k índices — a maior parte da cena **não**
  passa por ele

O plano original tratava instancing como o item de maior retorno. Os números dizem que o
gargalo é anterior a isso: é o *caminho imediato por vértice*. Instanciar 30 monstros não
ajuda quando 117 mil vértices por frame continuam sendo empurrados de um em um.

**Reordenação decidida:** medir por fase primeiro (feito), depois atacar o caminho imediato
(3.2/3.4 promovidos a prioridade), e só então reavaliar a Fase 4.

### Fase 3.2 (submissão em bloco + fusão de comandos), v8

| batching | cpu_us | draws | flushes | bufSubData | vertices |
| --- | --- | --- | --- | --- | --- |
| off | 7186 | 1721 | 1537 | 1621 | 115.642 |
| on | 7444 | 1764 | 1581 | 1665 | 118.696 |

**A fusão não fundiu nada.** `draws` e `flushes` não se moveram. Causa: o item 3.3, que não
foi feito — `FlushOpaqueWorldRenderQueue()` é chamado de 12 lugares, então a fila nunca
acumula comandos suficientes para haver o que fundir. A fusão está correta, mas chega tarde
demais no pipeline.

### A medição que muda tudo — repartição por fase (v8, Lorencia)

| fase | µs/frame | % do frame |
| --- | --- | --- |
| terreno | 277 | 3,7% |
| objetos | 200 | 2,7% |
| personagens | 1007 | 13,5% |
| efeitos | 249 | 3,3% |
| sprites | 56 | 0,8% |
| **não medido** | **5655** | **76%** |

Somadas, as cinco fases dão 1,8 ms de 7,4 ms. **Quatro rodadas de otimização foram gastas
dentro de 24% do frame.**

O plano inteiro assumiu que o caminho de render era o gargalo. A medição não sustenta isso.
O maior bloco *medido* é `RenderCharactersClient` com 1 ms — 13,5%.

**Próxima instrumentação (v9):** `us_simulation` (`UpdateSceneState` → `MoveMainScene`, a
simulação do jogo), `us_select` (`SelectObjects`), `us_setup` (`BeginOpengl` +
`CreateFrustrum`), `us_misc` (itens/peixes/insetos/folhas/pets/boids) e uma coluna
`us_unmeasured` explícita. Nenhuma otimização nova até esses números existirem.
- [x] **Verificação do build Web** *(promessa do plano que eu não estava cumprindo)*
  - [x] Achado 1: o timer novo usava `QueryPerformanceCounter` — **não compila no Emscripten**.
        Trocado por `std::chrono::steady_clock`, portável, com microssegundos em todo o caminho.
  - [x] Achado 2 (pré-existente, não meu): `Platform/RenderPipeline.cpp` nunca foi adicionado
        a `web/CMakeLists.txt`. O link do Web saía com `BeginOpaqueWorldRenderQueue`,
        `SubmitOpaqueWorldRenderCommand` e mais 3 **indefinidos**. No Emscripten isso é aviso,
        não erro: o `.wasm` linka e a chamada aborta em runtime — e `RenderObjects()` chama a
        fila incondicionalmente. O cliente Web abortaria ao renderizar objetos.
  - [x] `libmu_legacy_scene.a` e `mu_legacy_web` compilam e linkam com zero indefinidos
- [x] **3.3 — Redução dos pontos de flush** *(atrás de `-batching=on`)*
  - [x] Regra correta identificada: a fila guarda opacos com depth-test, e geometria opaca
        com depth-test é **independente de ordem** contra outra igual. Só quem **compõe**
        com o framebuffer (blend, alpha < 1, sem depth-test) precisa da barreira.
        Alpha-test escreve profundidade e não mistura — não compõe.
  - [x] `RenderMesh`, `RenderMeshAlternative`, `RenderMeshTranslate`: flush condicional
  - [x] `RenderFace` (terreno): o caminho de fallthrough sempre termina em
        `EnableAlphaTest()` ou `DisableAlphaBlend()` e **nunca liga blend** — aquele flush
        nunca foi necessário. Era a maior fonte de fragmentação da fila, e a razão de a
        fusão de comandos da 3.2 não ter nada para fundir.
  - [x] Mantidas as barreiras legítimas: `glReadPixels`, leitura de profundidade,
        `BeginOpengl`/`BeginSprite`/`BeginBitmap`, e o stencil de `RenderBodyShadow`
- [x] **Fase 4 — GPU Instancing de monstros** *(atrás de `-instancing=on`)*
  - [x] Paleta de ossos em textura `RGBA32F` com `texelFetch`, 3 texels por osso, uma
        linha por instância. A UBO de 64 KB limitaria o lote a ~6 instâncias; SSBO e
        `samplerBuffer` não existem no WebGL 2.
  - [x] Atributos por instância com divisor 1; `wave`, efeito de material e `shadowMap`
        ficam **fora** da chave de lote, que era exatamente o que os fragmentaria
  - [x] Um shader só — `uInstanced` escolhe entre atributo e uniforme
  - [x] Coletor com estado capturado **por valor** e reaplicado no flush, em vez de herdado
        do GL; flush registrado nas mesmas barreiras da fila de opacos
  - [x] Fallback para `DrawStaticMesh` por instância (lote de 1, backend sem
        `glVertexAttribDivisor`/`glDrawElementsInstanced`, paleta grande demais)
  - [x] Divisor e atributos desligados após cada draw instanciado — senão o VAO da malha
        manteria os atributos ligados e o próximo draw não instanciado leria lixo
  - [x] PC e Web compilando
- [x] **Fase 5 — Rollout e documentação**
  - [x] README com a tabela das três flags, a escada de rollout e as colunas de medição
  - [ ] *Não feito, com justificativa* — terreno residente em VBO e água pelo caminho
        `uWave`. `us_terrain` mede **277 µs de 7.400 µs (3,7%)**, e a 3.3 já resolveu a
        fragmentação de lote do terreno. Reescrever a residência de terreno é trabalho
        grande com teto medido de 3,7%, e há um alvo de 76% do frame ainda não
        caracterizado. Fazer isto antes do v9 seria repetir o erro das quatro
        primeiras rodadas.

---

## 0.2 A medição final — o frame explicado

Lorencia, 5 amostras com `gpu_timer_state = 2` (GL timer query funcionando).

| | µs/frame |
| --- | --- |
| CPU (região medida) | 9.253 |
| — destes, esperando em `glGetFloatv` | 4.017 |
| **CPU, trabalho real** | **5.236** |
| **GPU (timer query)** | **6.597** |

O frame é **limitado pela GPU**, e a CPU passa 4 ms bloqueada em seis pontos de
sincronização descobrindo isso.

Com CPU e GPU sobrepostas, o frame tenderia a `max(5.236, 6.597) = 6.597 µs`.
Ele custa 9.253 porque os seis `glGetFloatv` serializam os dois lados. **A perda
por serialização é de ~2.650 µs, ou 29%.**

Isso explica as sete rodadas anteriores de uma vez: cortar trabalho de CPU não
podia ajudar, porque a CPU não era o polo mais longo. E remover **um** ponto de
sincronização piorou o frame porque os outros cinco continuaram serializando —
só se ganha removendo **todos**.

### O que isso implica

1. **Piso de 6.597 µs** enquanto a carga de GPU não mudar. Nada de CPU passa disso.
2. **~2.650 µs recuperáveis** eliminando os seis readbacks. Exige espelhar a pilha
   de matrizes na CPU em todos os sítios, não só em `BeginOpengl` — os demais
   `SyncLegacyRenderMatrices` vêm depois de manipulações arbitrárias em código de
   efeito e objeto.
3. **Abaixo de 6.597 µs** só reduzindo trabalho de GPU: overdraw, fill rate,
   quantidade e tamanho de textura. **Nenhuma das cinco fases deste plano toca
   nisso.**

## 0.3 O resultado — espelho de matrizes em CPU

Lorencia, mesmas cenas.

| `-cpumatrices` | n | cpu_us | gpu_us | us_readback | chamadas | divergência rel. |
| --- | --- | --- | --- | --- | --- | --- |
| off | 18 | 7.596 | 3.521 | 3.183 | 6,0 | — |
| compare | 7 | 7.519 | 5.581 | 3.731 | 6,0 | **1,0e-6** |
| **on** | 3 | **4.450** | 5.651 | **0** | **0,0** | — |

**Os seis `glGetFloatv` foram a zero. `cpu_us` caiu 41%: 7.596 → 4.450 µs.**

A divergência de 1,0e-6 é epsilon de float, medida agora sobre *todas* as
manipulações de matriz do jogo — efeitos e objetos inclusos —, não só sobre
`BeginOpengl`.

O sinal de que o mecanismo é o previsto: com `on`, `cpu_us` (4.450) ficou
**abaixo** de `gpu_us` (5.651). A CPU passou a terminar antes da GPU e a correr à
frente, em vez de bloquear seis vezes por frame esperando por ela. O frame agora
tende ao maior dos dois, como projetado na seção 0.2.

Ressalva honesta: são 3 amostras no modo `on`. A magnitude precisa de mais
capturas; o mecanismo não, porque `matrix_readback_calls = 0` é binário.

**Esta foi a única mudança da sessão com ganho de tempo reproduzível** — e ela
não estava no plano original. Veio da medição por fase, depois que sete rodadas
de otimização de CPU renderam zero.

---

## 1. Diagnóstico — onde o tempo está indo hoje

### 1.1 CPU: o skinning nunca foi removido

`Calc_RenderObject` (`source/ZzzObject.cpp:249`) chama **incondicionalmente**
`b->Animation(...)` e depois `b->Transform(...)` (linhas 296-301 e 370-377). `BMD::Transform`
(`source/ZzzBMD.cpp:366`) percorre *todas* as malhas × *todos* os vértices e normais,
gravando `VertexTransform` / `NormalTransform` / `IntensityTransform`
(`MAX_MESH 50 × MAX_VERTICES 15000`).

Isso roda **mesmo quando o desenho vai pelo caminho GPU** — `RenderMesh` só decide usar
`DrawStaticMesh` em `source/ZzzBMD.cpp:1224`, muito depois do `Transform` já ter feito o
trabalho. Ou seja: hoje o GPU skinning **adiciona** custo de GPU sem **remover** custo de CPU.
O contador `cpuSkinningVertices` no CSV confirma isso — ele continua subindo com
`-gpuskinning=on`.

O mesmo vale para objetos de cenário: uma árvore com `NumActions == 1` e um keyframe
recalcula quaternions, matrizes de osso e todos os vértices a cada frame, em cada um dos
256 blocos visitados por `RenderObjects` (`source/ZzzObject.cpp:3334`).

### 1.2 GPU: o cache de malha estática é indexado só no nome

`PrepareStaticGpuMesh` (`source/ZzzBMD.cpp:72`) expande cada polígono em cantos e escreve
`indices[outputIndex] = outputIndex` (linha 121). O IBO é a sequência `0,1,2,3,…`: **zero
reuso de vértice**, `indexCount == vertexCount`. Um modelo com 3.000 triângulos ocupa 9.000
vértices de 60 bytes = 540 KB de VRAM onde caberiam ~200 KB, e o pós-transform cache da GPU
nunca acerta.

Os arrays CPU (`m->GpuStaticVertices`, `m->GpuStaticIndices`) ficam vivos até
`BMD::Release` (`source/ZzzBMD.cpp:3098`) — cópia permanente na RAM de algo que já está
residente na GPU.

Cada malha tem seu próprio VAO/VBO/IBO (`GlslLegacyRenderAdapter.cpp:702-717`). Um modelo de
personagem com 12 malhas = 12 VAOs, 12 binds, 12 draw calls.

### 1.3 GPU: `DrawStaticMesh` reenvia o mundo inteiro a cada chamada

`GlslLegacyRenderAdapter::DrawStaticMesh` (`Platform/GlslLegacyRenderAdapter.cpp:727`) faz,
**por malha desenhada**:

- `std::map<const void*, StaticMesh>::find` (busca em árvore, linha 732);
- `glUseProgram` + ~20 `glUniform*` (linhas 737-771), incluindo projeção e modelView que
  praticamente nunca mudam entre malhas do mesmo frame;
- `glBindBufferBase` da UBO de ossos;
- `glBindTexture`, `glBindVertexArray`, `glDrawElements`;
- e no fim **`InvalidarCacheDeEstado()`** (linha 809), que joga fora todo o cache de estado
  do adaptador — garantindo que a próxima malha do *mesmo modelo, mesma pose, mesma textura*
  refaça tudo.

O `memcmp` da paleta (linha 781) é a única otimização real presente, e é boa: várias malhas
do mesmo BMD compartilham a pose e não reenviam a UBO.

### 1.4 A fila de opacos não faz batching de verdade

`RenderQueue::Submit` (`Platform/RenderPipeline.cpp:56`) **copia todos os vértices** para um
`std::vector<RenderVertex>`, e `OpenGL33RenderBackend::Draw` (linha 101) os **re-emite um a
um** por `Color4f`/`TexCoord2f`/`Normal3f`/`Vertex3fv`. É um round-trip completo de CPU por
vértice, mais um `std::stable_sort` com comparador de 8 campos a cada flush.

E a fila é esvaziada o tempo todo: `FlushOpaqueWorldRenderQueue()` aparece em
`ZzzBMD.cpp:1294`, `ZzzBMD.cpp:2507`, `ZzzBMD.cpp:2893`, `ZzzLodTerrain.cpp:1361`,
`ZzzObject.cpp:2821`, `ShadowVolume.cpp:28/49/74`, `ZzzOpenglUtil.cpp:189/355/838/1397/1600`.
Qualquer malha que não passe no filtro estreito de `queueOpaqueWorldMesh`
(`ZzzBMD.cpp:1283`: exatamente `RENDER_TEXTURE`, sem blend, sem script, `Components == 3`)
derruba o lote inteiro. Na prática o lote médio é pequeno.

### 1.5 Nenhum instancing

Não existe `glVertexAttribDivisor` no código. 40 stone golems iguais = 40 × 12 draw calls,
cada uma com sua tempestade de uniformes. O ponto positivo: `Models[]` é global e **todas as
instâncias de um mesmo tipo já compartilham o mesmo `Mesh_t*`** — que é justamente a chave do
cache (`UploadStaticMesh(m, …)`). O VBO compartilhado para instancing **já existe**; falta o
lado das instâncias.

---

## 2. Estratégia

Três eixos, mas com uma dependência forte: **instancing só paga se o estado por instância
sair dos uniformes**. Por isso a ordem é cache → batching → instancing, e não a ordem em que
foram pedidos.

| Fase | Entrega | Ganho esperado | Risco |
| --- | --- | --- | --- |
| 0 | Instrumentação e baseline | — | nenhum |
| 1 | Cache de malha estática de verdade | −60% VRAM de modelo, −40% draws | baixo |
| 2 | Cache de objetos estáticos (pular Animation/Transform) | **maior ganho de CPU** | médio |
| 3 | Batching: state cache + fila sem replay | −50% chamadas GL | médio |
| 4 | GPU Instancing de monstros | −80% draws em cena povoada | alto |
| 5 | Terreno, UI e rollout | consolidação | baixo |

---

## Fase 0 — Instrumentação e baseline

Sem isso não há como provar regressão visual nem ganho.

1. **Estender `LegacyRenderFrameStats`** (`Platform/LegacyRenderAdapter.h:29`):
   `instancedDrawCalls`, `instancesSubmitted`, `instanceBatchesFlushed`, `largestInstanceBatch`,
   `staticMeshCacheHits`, `staticMeshCacheMisses`, `transformsSkipped`, `transformsExecuted`,
   `animationsSkipped`, `uniformCallsSaved`.
2. **CSV v6**: `CaptureRenderStatsCsv` (`ZzzScene.cpp:2940`) — novo arquivo
   `RenderPerformance_v6.csv` com as colunas novas + coluna `instancing`. Não misturar com o v5.
3. **Flags de linha de comando**, espelhando `-gpuskinning=` (`ZzzScene.cpp:3039-3063`):
   - `-instancing=off|on|compare`
   - `-statictransformcache=off|on`
   - `-batching=off|on`
   Todas com `off` como caminho de fuga em produção.
4. **Cenas de referência** para medir sempre nas mesmas condições: Lorencia (muitos objetos
   estáticos), Devias/Dungeon com spawn denso (muitos monstros iguais), Kalima (efeitos), e
   Loren Market lotado (muitos players — pior caso de malha única por instância).

**Critério de saída:** 120 frames capturados por cena, em `fixed`, `glsl+skinning off` e
`glsl+skinning on`, arquivados como baseline.

---

## Fase 1 — Cache de malha estática de verdade

Arquivos: `source/ZzzBMD.cpp`, `source/ZzzBMD.h`, `Platform/GlslLegacyRenderAdapter.cpp`,
`Platform/LegacyRenderAdapter.h`.

### 1.1 Indexação real em `PrepareStaticGpuMesh` (`ZzzBMD.cpp:72`)

Deduplicar por chave `(VertexIndex, NormalIndex, TexCoordIndex)` num hash map local durante a
expansão. Costuras de UV/normal continuam preservadas — só cantos **idênticos nos três
índices** colapsam. Esperado: 2,5–3× menos vértices em modelos orgânicos.

Usar `GL_UNSIGNED_SHORT` quando o número de vértices únicos couber em 16 bits (a esmagadora
maioria dos BMD) — metade da banda de índice. Guardar o tipo no `StaticMesh`.

### 1.2 Um VBO por modelo, não por malha

Concatenar as `NumMeshs` malhas de um BMD num único par VBO/IBO, guardando por malha
`{firstIndex, indexCount, baseVertex}`. Passa de 12 VAOs + 12 binds para **1 VAO e 1 bind**
por modelo. `glDrawElements` com offset em bytes; `baseVertex` via
`glDrawElementsBaseVertex` (core em GL 3.2+) e, no WebGL 2, aplicando o offset na
construção do IBO (não há `baseVertex` em GLES 3.0).

Chave do cache passa a ser o `BMD*`, não o `Mesh_t*`. `ReleaseStaticMesh` (`ZzzBMD.cpp:3097`)
sai do loop de malhas e vira uma chamada única em `BMD::Release`.

### 1.3 Handle direto, sem `std::map`

Substituir `std::map<const void*, StaticMesh>` (`GlslLegacyRenderAdapter.cpp:1144`) por um
vetor de slots + um `unsigned int GpuMeshHandle` gravado no próprio `BMD`. Lookup O(1) sem
comparação de ponteiros. Handle `0` = não residente. Geração no handle (16 bits de índice +
16 de geração) para detectar uso após `InvalidateGraphicsResources`.

### 1.4 Liberar a cópia de CPU

Após `UploadStaticMesh` bem-sucedido, `delete[]` em `GpuStaticVertices`/`GpuStaticIndices` e
marcar `GpuStaticUploaded = true`. Guardar apenas o suficiente para reconstruir após perda de
contexto — na prática, nada: `PrepareStaticGpuMesh` reconstrói a partir do BMD original, que
já está em memória.

**Critério de saída:** `gpu_mesh_upload_kb_avg` cai ≥50%; `gpu_mesh_draws_avg` cai
proporcionalmente ao número médio de malhas por modelo; captura de tela idêntica pixel a pixel
nas 4 cenas de referência.

---

## Fase 2 — Cache de objetos estáticos

O maior ganho de CPU do plano. Arquivos: `source/ZzzBMD.cpp`, `source/ZzzBMD.h`,
`source/ZzzObject.cpp`, `source/ZzzCharacter.cpp`.

### 2.1 Separar `BMD::Transform` em duas metades

Hoje `Transform` (`ZzzBMD.cpp:366`) faz três coisas acopladas:
1. preparar a paleta GPU (`PrepareGpuRender`, linha 368) — **barato, sempre necessário**;
2. calcular bounding box e OBB — necessário para picking/colisão;
3. transformar todos os vértices/normais/intensidades — **só necessário no fallback CPU**.

Dividir em:

```
void BMD::TransformBones(...)     // (1) + (2), sempre
void BMD::TransformVertices(...)  // (3), sob demanda
```

O bounding box em `TransformBones` sai de **AABBs por osso pré-calculados na bind pose**
(computados uma vez em `BMD::CreateBoundingBox`), transformados pelas matrizes de osso — 
`NumBones` transformações em vez de `NumVertices`.

`RenderMesh` chama `TransformVertices` preguiçosamente quando cai no caminho legado (a partir
de `ZzzBMD.cpp:1280`), com um flag `VerticesTransformedThisFrame` para não repetir entre
malhas do mesmo modelo.

**Este é o passo que finalmente faz o GPU skinning render lucro de CPU.**

### 2.2 Chave de pose e skip de `Animation`

Em `OBJECT`, adicionar `PoseKey` (hash de `Type`, `CurrentAction`, `PriorAction`,
`AnimationFrame` quantizado a 1/256, `Angle`, `HeadAngle`, `Scale`, `BoneScale`,
`EnableBoneMatrix`) e `PoseKeyValid`.

Em `Calc_RenderObject` (`ZzzObject.cpp:249`), se `PoseKey` não mudou e o objeto tem
`BoneTransform` próprio (`o->EnableBoneMatrix`), pular `b->Animation` e reaproveitar as
matrizes. Objetos sem `EnableBoneMatrix` usam o `BoneTransform` global compartilhado e
**precisam** ser recalculados — parte da Fase 2.4 é migrá-los.

### 2.3 Objetos verdadeiramente estáticos

Um BMD com `NumActions <= 1` e `Actions[0].NumAnimationKeys <= 1` não anima. Detectar isso
em `BMD::Open`, marcar `IsStaticModel = true`, calcular a paleta **uma vez** e reusá-la em
todos os frames e todas as instâncias. `GpuStaticMatrixValid` (`ZzzBMD.cpp:306`) já cobre o
caso de 1 osso; generalizar para N ossos sem animação.

Cobre a grande maioria dos `MAX_WORLD_OBJECTS` (paredes, casas, pedras, cercas).

### 2.4 Bounding box e OBB persistentes

`o->BoundingBoxMin/Max` e `o->OBB` só mudam quando `PoseKey` ou `o->Position` mudam. Guardar
`BoundsKey` separado e pular a recomputação — os objetos de cenário nunca se movem.

### 2.5 Frustum culling antes do trabalho, não depois

`RenderObjects` (`ZzzObject.cpp:3504-3534`) já testa `TestFrustrum2D`, mas o teste acontece
**depois** de o objeto ter entrado no laço. Bom o suficiente — o problema real é que
`RenderObjectVisual` e efeitos rodam sem teste de distância. Adicionar um corte por distância
configurável para objetos pequenos (`o->CollisionRange` baixo), com histerese para não piscar.

**Critério de saída:** `cpu_skinning_vertices_avg` cai ≥80% com `-gpuskinning=on`;
`cpu_ms_avg` em Lorencia cai ≥25%; `transformsSkipped/transformsExecuted` > 5 em cena de
cidade; picking e colisão inalterados (teste manual de clique em 20 objetos por cena).

---

## Fase 3 — Batching

Arquivos: `Platform/GlslLegacyRenderAdapter.cpp`, `Platform/RenderPipeline.cpp/.h`.

### 3.1 Matar a tempestade de uniformes

Introduzir em `GlslLegacyRenderAdapter` um **shadow state do programa de malha estática**,
separado do cache do caminho imediato: cada `glUniform*` só é emitido se o valor mudou.
Projeção e modelView passam a ser enviadas **uma vez por frame** (ou por mudança de câmera),
não por malha.

Remover o `InvalidarCacheDeEstado()` do fim de `DrawStaticMesh`
(`GlslLegacyRenderAdapter.cpp:809`). Ele existe porque a função mexe em binds que o caminho
imediato assume; a correção certa é o caminho imediato reafirmar **seus** binds no `Flush`,
não o caminho estático destruir o cache global. Fazer essa inversão explicitamente e cobrir
com o modo `compare`.

Ganho isolado esperado: ~20 chamadas GL → ~3 por malha.

### 3.2 Fila de opacos sem replay de vértice

Reescrever `RenderQueue` (`Platform/RenderPipeline.cpp`):

- **Chave de ordenação empacotada em `uint64`** (`pass:3 | transparent:1 | shader:8 |
  blend:4 | depth/alpha:4 | texture:20 | sequence:24`) — sort de inteiros, não comparador de
  8 campos.
- Vértices ficam num buffer único da fila; no `Execute`, **um** `glBufferData` de orphaning +
  `glBufferSubData`, e depois `glDrawArrays`/`glDrawElements` por sub-range. Sem
  `Color4f`/`Vertex3fv` por vértice.
- Comandos adjacentes com a mesma chave de material **fundem** em um único draw
  (`vertexCount` somado) antes de emitir.

### 3.3 Reduzir os pontos de flush

Auditar os 12 `FlushOpaqueWorldRenderQueue()` existentes. Trocar "flush incondicional" por
"flush se a fila contém comando conflitante":

- `ShadowVolume.cpp:28/49/74` — leitura de framebuffer/stencil: flush obrigatório, manter.
- `ZzzOpenglUtil.cpp:189/355/838/1397/1600` — mudança de matriz/estado: substituir por
  invalidação de chave; a fila já ordena por estado.
- `ZzzBMD.cpp:1294` — o predicado `queueOpaqueWorldMesh` (linha 1283) é estreito demais.
  Alargar para aceitar alpha-test (`Components == 4`) com depth-test ligado, que é seguro
  para reordenação, e materiais chrome/metal (que só mudam UV no shader).

### 3.4 UI e texto

`BeginBatch`/`EndBatch` já existem. Trocar o `glBufferData` por flush por um **ring buffer
persistente** com `glBufferSubData` em região órfã, e `glDrawElements` sobre o IBO de quads já
mantido (`m_quadIndexBuffer`). Baixo risco, ganho direto no HUD.

**Critério de saída:** `draws_avg` cai ≥40%; `flush_matrix_avg` + `flush_texture_avg` caem
≥70%; nenhuma diferença visual em `-batching=compare`.

---

## Fase 4 — GPU Instancing de monstros

Depende das Fases 1 e 3. Arquivos: `Platform/GlslLegacyRenderAdapter.cpp`,
`Platform/LegacyRenderAdapter.h`, `source/ZzzBMD.cpp`, `source/ZzzCharacter.cpp`,
`source/ZzzObject.cpp`, novo `Platform/InstanceBatcher.{h,cpp}`.

### 4.1 A decisão crítica: onde mora a paleta de ossos

A UBO atual tem 600 `vec4` = 200 ossos (`GlslLegacyRenderAdapter.cpp:204`). O limite de UBO é
64 KB → **6 instâncias** se mantivermos 200 ossos por instância. Inviável.

Opções avaliadas:

| Opção | GL 3.3 | WebGL 2 | Instâncias/lote | Veredito |
| --- | --- | --- | --- | --- |
| UBO array com stride fixo de 200 ossos | sim | sim | ~6 | insuficiente |
| UBO array com stride = ossos reais (~32) | sim | sim | ~40 | frágil, stride por modelo |
| SSBO | 4.3+ | **não** | ilimitado | quebra o alvo Web |
| Texture Buffer (`samplerBuffer`) | sim | **não** | ilimitado | quebra o alvo Web |
| **Textura 2D `RGBA32F` + `texelFetch`** | **sim** | **sim** | ilimitado | **escolhido** |

**Escolha: paleta em textura `RGBA32F`.** Cada osso = 3 texels (3 linhas de `mat3x4`). A
textura é `width = 3 × maxBones` (arredondado), `height = número de instâncias do lote`.
`texelFetch` não exige filtragem, então `RGBA32F` amostrável é suficiente — core em GLES 3.0,
sem extensão. Upload por lote com um único `glTexSubImage2D`.

Fallback: se `RGBA32F` amostrável falhar na sonda de capacidade, o batcher degrada para o
caminho atual de uma UBO por draw (sem instancing), registrando
`GpuSkinningFallbackResource`.

### 4.2 Atributos por instância (`divisor = 1`)

Novos atributos, locations 7-11, num VBO de instância separado (o VBO de geometria continua
`GL_STATIC_DRAW`, o de instância é `GL_STREAM_DRAW` com orphaning):

```
layout(location=7)  in vec4 iColor;              // BodyLight.rgb + alpha
layout(location=8)  in vec4 iPostTransScale;     // GpuPostTranslation.xyz + GpuBodyScale
layout(location=9)  in vec4 iLightPos;           // GpuLightPosition.xyz + lightingFlag
layout(location=10) in vec4 iBodyOriginBone;     // BodyOrigin.xyz + BoneScale
layout(location=11) in vec4 iParams;             // paletteRow, waveSeedBase, materialEffect, flags
```

`uSkinning`, `uWave`, `uShadowMap`, `uMaterialEffect` migram de uniforme para bits empacotados
em `iParams.w` — assim variações desses flags **não quebram o lote**. `uProjection`,
`uModelView`, `uFog*`, `uAlphaTest*` continuam uniformes: são estado de passe, não de
instância.

O vertex shader passa a ler a linha da paleta em
`texelFetch(uBonePalette, ivec2(boneIndex * 3 + row, int(iParams.x)), 0)`.

O caminho não-instanciado continua existindo: com `instanceCount == 1` e divisor ativo, o
mesmo shader serve os dois casos — **um shader só**, sem `#ifdef` nem variante.

### 4.3 O coletor (`InstanceBatcher`)

Chave de lote:

```
{ bmdHandle, meshIndex, textureId, blendMode, depthTest, depthMask, alphaTest, alphaRef, fogState }
```

`materialEffect`, `wave` e `shadowMap` **não** entram na chave (viraram atributos).

Fluxo:

1. `RenderMesh` (`ZzzBMD.cpp:1224`) — onde hoje decide `staticGpuCandidate` — passa a
   **submeter uma instância** ao batcher em vez de chamar `DrawStaticMesh`, desde que a malha
   seja opaca segundo `IsTransparentBodyMesh` (`ZzzBMD.cpp:135`, predicado já existente e
   conservador).
2. O batcher acumula: `{chave → vetor de registros de instância + linhas de paleta}`.
3. Flush no fim da fase opaca — `ExecuteOpaqueWorldRenderQueue` (`ZzzObject.cpp:3576`) e o
   ponto equivalente após `RenderCharactersClient` (`ZzzCharacter.cpp:11214`). Um
   `glTexSubImage2D` da paleta + um `glBufferSubData` dos atributos + um
   `glDrawElementsInstanced` por chave.
4. Qualquer emissão transparente/legada força `FlushInstanceBatches()` antes, exatamente como
   a fila de opacos já faz.

### 4.4 Dedup de paleta

Monstros parados na mesma ação e no mesmo frame de animação produzem paletas idênticas.
Hashear a paleta (FNV-1a sobre `boneCount × 12` floats) e reutilizar a linha da textura em
caso de acerto. Em spawn denso de mobs idle isso corta a maior parte do upload de paleta.
Contabilizar em `paletteDedupHits`. **Opcional na primeira entrega** — medir antes de fazer.

### 4.5 Escopo: por que "monstros" e não "tudo"

- **Monstros** (`MODEL_MONSTER01+`): muitos duplicados, paleta simples, materiais uniformes.
  Alvo primário, maior retorno.
- **Players**: cada um tem partes/itens diferentes → malhas diferentes → lotes de tamanho 1.
  Ganham pela Fase 3, não pela 4. Habilitar só se a medição mostrar duplicatas reais (Loren
  Market com muitos personagens de classe igual).
- **Objetos de cenário**: instanciáveis e com paleta constante (Fase 2.3) — a paleta pode ser
  uma linha única compartilhada. **Segunda onda**, depois de monstros validados.
- **Efeitos/transparentes**: fora de escopo. Dependem de ordem de emissão.

### 4.6 Reaproveitar o rollout já existente

`GpuSkinningDeployment` (`LegacyRenderAdapter.h:88`) e `SetGpuSkinningModelWhitelist`
(`ZzzScene.cpp:3035`) já implementam whitelist por ID de modelo. **Reutilizar o mesmo
mecanismo** para instancing (`-instancing-models=`), em vez de criar um segundo sistema.

**Critério de saída:** em cena com ≥30 monstros do mesmo tipo, `draws_avg` cai ≥70%;
`largestInstanceBatch` ≥ 20; `cpu_ms_p95` cai ≥30%; comparação pixel a pixel em
`-instancing=compare` sem diferença acima do ruído de precisão de float.

---

## Fase 5 — Terreno, consolidação e rollout

1. **Terreno residente**: `ZzzLodTerrain.cpp:2632` já abre a fila de opacos. Promover os
   blocos de terreno a VBOs persistentes por `(bloco 16×16, textura)`, reconstruídos só
   quando lightmap/atributo muda. Terreno é o caso mais estático de todos e hoje é reenviado
   integralmente todo frame.
2. **Água e `CSWaterTerrain`**: deslocamento por tempo já é candidato natural ao caminho
   `uWave` do shader — remover a animação de vértice em CPU.
3. **Rollout**: `-instancing=off` como default → QA → whitelist de modelos → default ligado,
   mesma escada já usada e validada pelo GPU skinning.
4. **README**: documentar as flags novas junto das de `-gpuskinning=`.

---

## 3. Riscos e como cada um é contido

| Risco | Contenção |
| --- | --- |
| Reordenação quebra composição de blend | Só malhas que passam em `IsTransparentBodyMesh` (`ZzzBMD.cpp:135`) entram no batcher. Transparentes nunca reordenam — a regra já está em `IsOpaqueBefore` (`RenderPipeline.cpp:20`). |
| `RGBA32F` amostrável indisponível | Sonda de capacidade no bring-up; fallback para UBO por draw com `GpuSkinningFallbackResource`. |
| Pular `Transform` quebra picking/colisão | `TransformBones` continua produzindo bbox/OBB. Teste manual de clique obrigatório no critério de saída da Fase 2. |
| Cache de pose "gruda" numa animação | `PoseKey` inclui `AnimationFrame` quantizado; quantização de 1/256 é mais fina que qualquer keyframe do BMD. |
| Divergência PC × Web | Todo o plano usa só o subconjunto GL 3.3 ∩ GLES 3. `build-web` roda em cada fase, não no fim. |
| Perda de contexto (Web/alt-tab) | `InvalidateGraphicsResources` (`LegacyRenderAdapter.h:135`) já existe; estender para textura de paleta, VBO de instância e handles com geração. |
| Regressão silenciosa | Modo `compare` por fase + CSV v6 + capturas das 4 cenas de referência. |

---

## 4. Sequência de trabalho sugerida

```
Fase 0  ──►  Fase 1  ──►  Fase 2  ──┐
                    └──►  Fase 3  ──┴──►  Fase 4  ──►  Fase 5
```

Fases 2 e 3 são independentes entre si e podem ir em paralelo. A Fase 4 exige as duas
concluídas: sem a Fase 1 não há VBO compartilhado por modelo, sem a Fase 3 o batcher continua
pagando a tempestade de uniformes que ele existe para eliminar.

Ordem de valor por esforço, se for preciso cortar escopo:

1. **Fase 2.1** (dividir `Transform`) — maior ganho isolado, risco contido, ~1 arquivo.
2. **Fase 3.1** (matar uniformes redundantes) — ganho grande, mudança local.
3. **Fase 1.1** (indexação real) — ganho de VRAM e vertex shading, mudança local.
4. **Fase 4** — o maior ganho absoluto, mas só depois das três acima.
