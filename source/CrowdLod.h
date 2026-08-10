#pragma once

// Crowd — LOD e culling de personagens. Ver CROWD_LOD_PLAN.md.
//
// FASE 0: este modulo CLASSIFICA e CONTA. Ele nao corta nada. Nenhum caminho de
// render consulta o nivel ainda, e o default de modo e `off`.
//
// A razao de existir antes de qualquer otimizacao: nao existe nenhuma captura do
// projeto com multidao. Sem a curva "frame x numero de personagens" nao se sabe
// nem se o custo esta em pose, em partes ou em sombra -- e o projeto ja retratou
// uma conclusao inteira por medir a coisa errada (commit e3cb8b7). Os contadores
// daqui viram colunas no CSV v17 e sao o que decide a ordem das fases seguintes.
//
// Este arquivo NAO inclui header de jogo de proposito. Ele compila igual no PC,
// no Web (Emscripten) e no Android; quem conhece Hero, camera e mapa e o chamador,
// que passa numeros prontos.

namespace CrowdLod
{
	// Mesma escada das outras otimizacoes do projeto: off -> compare nas cenas de
	// referencia -> on em QA -> on em producao. `compare` alterna por frame contra
	// o caminho antigo, entao divergencia aparece como cintilacao.
	enum Mode
	{
		ModeOff = 0,
		ModeOn,
		ModeCompare
	};

	// Nivel de detalhe de um personagem. LevelFull e o comportamento historico:
	// pose todo frame, sombra, todas as partes de equipamento.
	enum Level
	{
		LevelFull = 0,
		LevelReduced,
		LevelMinimal,
		LevelHidden,
		LevelCount
	};

	// Player, monstro e NPC nao sao o mesmo problema de custo: o player carrega ~15
	// malhas de equipamento e tem passe de sombra proprio; o monstro tem uma malha de
	// corpo e nenhum dos dois. Sem saber a COMPOSICAO da multidao nao se atribui o
	// tempo medido a ninguem -- 200 monstros e 200 players com o mesmo
	// `chars_visible` sao cargas diferentes.
	//
	// `KindOther` junta trap, pet, tmp e edit. Existe para a soma dos tipos fechar
	// com `chars_live`: sem ele a identidade nao seria verificavel.
	enum Kind
	{
		KindPlayer = 0,
		KindMonster,
		KindNpc,
		KindOther,
		KindCount
	};

	// Traduz o KIND_* do legado (bitmask em _define.h:144) para os buckets acima.
	Kind KindFromLegacyKind(int legacyKind);

	// --- Configuracao -------------------------------------------------------
	// Ordem de precedencia, montada pelo chamador: default compilado ->
	// MainInfo.ini ([Render]) -> linha de comando (so o PC tem argv).

	void        SetMode(Mode mode);
	Mode        GetMode();
	const char* GetModeName();

	// Limiares em PIXELS de altura na tela, nao em distancia crua: CameraFOV,
	// CameraZoom e a resolucao mudam a relacao entre distancia e tamanho aparente
	// (TestFrustrum2D chega a compensar zoom a mao). Um limiar em pixels significa
	// a mesma coisa em 1280x720 e em 2560x1440.
	void  SetPixelThresholds(float reduced, float minimal, float hidden);
	float GetPixelThreshold(Level level);

	// Teto de personagens em qualidade plena. <= 0 desliga o teto. So a Fase 1
	// passa a consumir isso; aqui ele existe para o CSV registrar o regime.
	void SetMaxFull(int maxFull);
	int  GetMaxFull();

	// Rig de medicao: multidao sintetica reproduzivel, um pedido por TIPO. 0 =
	// desligado. Separados de proposito: uma multidao de player e uma de monstro
	// custam coisas diferentes, e um numero unico nao permitiria isolar qual.
	//
	// Vao para as colunas crowd_spawn* porque captura sintetica nao pode ser lida
	// como organica -- mesma regra de render_scale e wheel_trail_cap.
	void SetSpawnRequest(Kind kind, int count);
	int  GetSpawnRequest(Kind kind);

	// Uma chave da secao [Render] do MainInfo.ini. Retorna true se consumiu a
	// chave. Existe porque Web e Android nao tem linha de comando: sem canal de
	// arquivo, esses dois alvos ficariam presos ao default compilado.
	bool ApplyIniKey(const char* key, const char* value);

	// Overrides de linha de comando: -crowdlod=off|on|compare e -crowd=N.
	void ApplyCommandLine(const char* commandLine);

	// --- Classificacao ------------------------------------------------------
	// px ~= alturaViewport * alturaModelo / (2 * distancia * tan(fov/2))
	// gluPerspective recebe CameraFOV como abertura VERTICAL em graus
	// (ZzzOpenglUtil.cpp:1712), entao a conta usa a altura do viewport.
	float ScreenPixels(float distance, float viewportHeight, float fieldOfViewDegrees);
	Level ClassifyByScreenPixels(float pixels);

	// --- Contadores do frame ------------------------------------------------
	struct FrameCounters
	{
		unsigned int live;           // slots vivos no pool
		unsigned int visible;        // vivos e dentro do frustum
		unsigned int culledFrustum;  // vivos e fora do frustum
		unsigned int beyondFar;      // vivos alem de CameraViewFar -- HOJE ainda
		                             // desenhados: o teste de frustum de
		                             // personagem e 2D e nao tem plano far
		unsigned int forced;         // isentos de LOD (Hero, alvo, party, mapa PvP)
		unsigned int kinds[KindCount];
		unsigned int levels[LevelCount];
		unsigned int posesComputed;  // corpos que passaram pelo calculo de pose
		unsigned int partMeshes;     // malhas de personagem emitidas
		unsigned int shadows;        // sombras de personagem emitidas
	};

	void                 BeginFrame();
	const FrameCounters& GetFrameCounters();

	void CountCharacter(Kind kind, bool visible, bool beyondFar, bool forced, Level level);
	void CountPose();
	void CountPartMesh();
	void CountShadow();
}
