///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Platform/LegacyRenderAdapter.h"
#include "UIManager.h"
#include "GuildCache.h"
#include "ZzzOpenglUtil.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzLodTerrain.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "ZzzTexture.h"
#include "ZzzOpenData.h"
#include "ZzzScene.h"
#include "CrowdLod.h"
#include "ZzzEffect.h"
#include "ZzzAI.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"
#include "SMD.h"
#include "Local.h"
#include "MatchEvent.h"
#include "PhysicsManager.h"
#include "./Utilities/Log/ErrorReport.h"
#include "CSQuest.h"
#include "PersonalShopTitleImp.h"
#include "uicontrols.h"
#include "GOBoid.h"
#include "GMHellas.h"
#include "CSItemOption.h"
#include "GMBattleCastle.h"
#include "GMHuntingGround.h"
#include "GMAida.h"
#include "GMCrywolf1st.h"
#include "npcBreeder.h"
#include "CSPetSystem.h"
#include "GIPetManager.h"
#include "CComGem.h"
#include "UIMapName.h"	// rozy
#include "./Time/Timer.h"
#include "Input.h"
#include "UIMng.h"
#include "LoadingScene.h"
#include "CDirection.h"
#include "GM_Kanturu_3rd.h"
#ifdef MOVIE_DIRECTSHOW
	#include <dshow.h>
	#include "MovieScene.h"
#endif // MOVIE_DIRECTSHOW
#include "Event.h"
#include "./Utilities/Log/muConsoleDebug.h"
#include "MixMgr.h"
#include "GameCensorship.h"
#include "GM3rdChangeUp.h"
#include "NewUISystem.h"
#include "NewUICommonMessageBox.h"
#include "PartyManager.h"
#include "w_CursedTemple.h"
#include "CameraMove.h"
#include "w_MapHeaders.h"
#include "w_PetProcess.h"
#include "PortalMgr.h"
#include "ServerListManager.h"
#include "ProtocolSend.h"
#include "MapManager.h"
#include <thread>
#include <chrono>
#include <stdio.h>

// GetTickCount tem granularidade de ~15,6 ms: um frame de 5 ms era medido como
// 0 ou 16, e a media dizia mais sobre o tick do relogio do que sobre o frame.
//
// steady_clock em vez de QueryPerformanceCounter: o alvo Web (Emscripten) nao
// tem a API do Windows, e o plano se comprometeu a manter os dois alvos
// compilando. As unidades sao microssegundos em todo o caminho.
static long long g_renderStatsStart = 0;
static void CaptureRenderStatsCsv(const Platform::LegacyRenderFrameStats& stats, DWORD renderCpuUs);

static long long RenderStatsNowMicroseconds()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

static DWORD RenderStatsElapsedMicroseconds()
{
	if (g_renderStatsStart == 0) return 0;
	const long long elapsed = RenderStatsNowMicroseconds() - g_renderStatsStart;
	return elapsed > 0 ? static_cast<DWORD>(elapsed) : 0;
}

// Reparticao do frame. Duas otimizacoes grandes (70% menos skinning de CPU e 12
// mil glUniform a menos por frame) nao moveram o total, o que so pode significar
// que o custo esta em outro lugar. Medir por bloco e mais barato que continuar
// adivinhando.
enum RenderPhaseId
{
	RenderPhaseTerrain,
	RenderPhaseObjects,
	RenderPhaseCharacters,
	RenderPhaseEffects,
	RenderPhaseSprites,
	// A primeira medicao mostrou 76% do frame fora dos cinco blocos acima.
	// Estes cobrem o resto: simulacao, selecao, preparo de frame e o miudo.
	RenderPhaseSimulation,
	RenderPhaseSelect,
	// setup deu 41% do frame na v9. Nada dentro dele custa isso em CPU — sao
	// glViewport, gluPerspective e umas rotacoes de matriz. Separar as duas
	// funcoes diz se e trabalho (CreateFrustrum, matematica pura) ou espera de
	// driver (BeginOpengl, que fala com o GL).
	RenderPhaseSetup,
	RenderPhaseFrustum,
	RenderPhaseMisc,

	// A v14 deixou ~640 us tipicos em us_unmeasured, e a leitura do codigo mostrou
	// exatamente o que estava la: o segundo passe de agua (que refaz efeitos e
	// sprites inteiros), o bloco de UI/2D, e o preparo de frame. Estes tres levam
	// us_unmeasured para perto de zero, que e a condicao para parar de adivinhar.
	RenderPhaseWater,
	RenderPhaseUi,
	RenderPhaseFrameBegin,

	// Marcador: tudo acima acontece DENTRO da janela de cpu_us e soma exatamente
	// cpu_us. Somente estas entram na subtracao de us_unmeasured.
	RenderPhaseInsideCount,

	// A v14 mostrou frame_total_us - cpu_us entre 0,4 e 4,1 ms, e em world 3 o
	// frame cresceu 50% com cpu_us constante: toda a variacao estava aqui. Estes
	// quatro blocos cobrem o intervalo entre a leitura do CSV e o topo do frame
	// seguinte. Ficam FORA de cpu_us de proposito — mover a janela invalidaria a
	// comparacao com todas as capturas anteriores.
	RenderPhaseOverlay = RenderPhaseInsideCount,
	RenderPhasePresent,
	RenderPhaseProtocol,
	RenderPhasePump,
	// Sleep do limitador de FPS. Sem coluna propria ele cairia em us_frame_gap, que
	// a documentacao manda ler como "sobrou caminho sem instrumento" -- entao um
	// -fpslimit=60 numa maquina de 170 FPS produziria ~10 ms de gap que sao espera
	// deliberada, e mandaria o leitor cacar trabalho que nao existe.
	RenderPhaseLimiter,
	RenderPhaseOutsideCount,

	// Terceira regiao: DETALHE. Estas ANINHAM dentro de us_characters, logo
	// sobrepoem valores ja contados. Nao entram em nenhuma das duas somas — sao
	// informativas. us_characters e 33,7% do frame e a maior fatia medida; a
	// Fase 2 provou que nao e o laco por vertice, entao a pergunta que sobra e
	// "corpo ou equipamento?".
	RenderPhaseCharPose = RenderPhaseOutsideCount,
	RenderPhaseCharShadow,

	// Detalhe DE us_char_parts, um nivel mais fundo: o laco por vertice de
	// BMD::TransformVertices. Existe porque a captura de multidao de 2026-08-10
	// mostrou us_char_parts com 83-95% de us_characters, e us_char_parts e coluna
	// de SUBTRACAO -- ela aponta o bloco sem dizer o que tem dentro.
	//
	// A conta de guardanapo (70 ns x cpu_skinning_vertices) explicou 100% do caso
	// monstro e menos da metade do caso player. Guardanapo nao e medicao: esta
	// coluna e que decide se o alvo e o laco por vertice ou a submissao de malha.
	RenderPhaseCharTransform,

	// Emissao de malha (RenderPartObject). Tambem dentro de us_char_parts, e ela
	// CONTEM RenderPhaseCharTransform no regime diferido. A captura de 2026-08-10
	// justificou: com `-statictransformcache=on` o laco por vertice caiu para 1,3 ms
	// e sobraram 9,6 ms de um frame de 16,9 ms em "submissao mais extras". Sem
	// separar os dois, o proximo corte seria escolhido por palpite.
	RenderPhaseCharMesh,

	// Desenho (RenderPartObjectEffect), dentro de RenderPhaseCharMesh. Fecha a
	// pergunta que a captura limpa deixou: dos ~127 us por personagem gastos em
	// RenderPartObject, quanto e emitir geometria e quanto e o setup em volta.
	RenderPhaseCharDraw,

	// RenderLinkObject: arma, asa e parte ligada. Irma de RenderPhaseCharMesh, nao
	// filha -- este caminho nao passa por RenderPartObject. Sem ela, o tempo de desenho
	// de arma caia no bucket de subtracao e eu o atribui a nome/barra/pet.
	RenderPhaseCharLink,

	// O desenho dentro de RenderLinkObject, aninhado em RenderPhaseCharLink. Existe
	// porque us_char_link virou o maior bloco INDIVISO do frame depois dos consertos de
	// GPU skinning, e ele mistura preparo de matriz de osso com emissao de geometria --
	// duas coisas com consertos completamente diferentes.
	RenderPhaseCharLinkDraw,

	// Detalhe de us_simulation, o segundo maior bloco em Lorencia (15,2%) e o
	// MAIOR de todos em world 3 (1.440 us, acima de us_characters). Nenhuma das
	// cinco fases do plano original olhou para ele: nao e render, e logica de
	// jogo. Tambem aninham — o resto sai por subtracao.
	RenderPhaseSimUi,
	RenderPhaseSimObjects,
	RenderPhaseSimChars,
	RenderPhaseSimEffects,
	RenderPhaseCount
};

static long long g_renderPhaseUs[RenderPhaseCount] = { 0 };

// As fases pos-cpu_us acontecem depois de CaptureRenderStatsCsv ter lido o
// array, e antes do memset do frame seguinte. Sem este acumulador separado elas
// seriam zeradas antes de qualquer leitura. O topo de RenderScene transfere
// pending -> phase, entao o CSV le sempre o frame anterior nessas quatro
// colunas. Um frame de atraso nao muda uma media de 120.
static long long g_pendingPhaseUs[RenderPhaseCount] = { 0 };

// Tempo gasto nas leituras sincronas de matriz (glGetFloatv) em ZzzOpenglUtil.
// Elas drenam o pipeline do driver e sao o principal suspeito do custo de setup.
extern unsigned long long g_matrixReadbackUs;
extern unsigned long long g_matrixReadbackCalls;
// Maior diferenca observada entre a matriz calculada na CPU e a lida do
// driver, em -cpumatrices=compare. Deve ficar na casa do epsilon de float.
extern float g_cpuMatrixMaxDivergence;
// Escala do viewport 3D em porcento (diagnostico de fill rate).
extern int g_renderScalePercent;
// Efeitos vivos no frame (ZzzEffect.cpp). Pool de 200; cada um e malha com alpha.
extern int g_liveEffects;
// Rastros da Twisting Slash vivos, e o limite opcional (-wheeltrail=N, -1 = sem
// limite). Cada rastro custa um modelo de arma animado por frame.
extern int g_liveWheelTrails;
extern int g_wheelTrailCap;
// Periodo real do frame, medido de topo a topo de RenderScene: inclui
// SwapBuffers E o sleep do limitador. Sem ele nao da para saber se o jogo
// esta no teto de FPS — e, se estiver, nenhuma otimizacao muda o FPS aqui.
static long long g_frameTopPrevUs = 0;
static long long g_frameTotalUs = 0;

// Swap interval efetivamente aplicado: -1 = default do driver (nao configurado),
// 0 = vsync off, 1 = vsync on. Ate a v14 o codigo nunca chamava
// wglSwapIntervalEXT, entao o teto de FPS de cada maquina vinha do painel do
// driver e a medicao nao era comparavel entre PCs. Registrado no CSV.
int g_vsyncInterval = -1;

// RenderCharacter tambem e chamado de CharMakeWin.cpp:444 (criacao de personagem) e
// UIWindows.cpp:1830, os dois FORA da fase Characters. Sem este guarda, o pose/shadow
// desses caminhos entraria nas colunas de detalhe sem contribuir para us_characters, e
// us_char_parts — que sai por subtracao — ficaria negativo e seria truncado em zero.
// Corrupcao silenciosa exatamente na coluna que a rodada existe para produzir.
static int g_charactersPhaseDepth = 0;

// As 13 fases de dentro de cpu_us tem que ser mutuamente exclusivas: se duas se
// aninharem, o mesmo tempo entra em duas colunas e us_unmeasured — que sai por
// subtracao — afunda ou trava em zero, sem nenhum sinal. As fronteiras foram
// escolhidas a mao em tres funcoes diferentes, entao "eu conferi" nao e garantia.
// Este contador transforma o risco em coluna: phase_nesting != 0 significa que a
// reparticao de cpu_us daquela captura NAO e confiavel.
static int g_insidePhaseDepth = 0;
static long long g_phaseNestingViolations = 0;

struct ScopedRenderPhase
{
	explicit ScopedRenderPhase(RenderPhaseId id) : m_id(id), m_start(RenderStatsNowMicroseconds())
	{
		if (id == RenderPhaseCharacters) ++g_charactersPhaseDepth;
		if (id < RenderPhaseInsideCount)
		{
			if (g_insidePhaseDepth > 0) ++g_phaseNestingViolations;
			++g_insidePhaseDepth;
		}
	}
	~ScopedRenderPhase()
	{
		if (m_id == RenderPhaseCharacters) --g_charactersPhaseDepth;
		if (m_id < RenderPhaseInsideCount) --g_insidePhaseDepth;
		const long long elapsed = RenderStatsNowMicroseconds() - m_start;
		// Somente a regiao pos-cpu_us precisa do pending: ela e medida depois de o
		// CSV ler o array. As fases de dentro e as de detalhe acontecem antes da
		// leitura, entao acumulam direto. Ver o comentario do enum.
		if (m_id >= RenderPhaseInsideCount && m_id < RenderPhaseOutsideCount)
			g_pendingPhaseUs[m_id] += elapsed;
		else
			g_renderPhaseUs[m_id] += elapsed;
	}
	RenderPhaseId m_id;
	long long m_start;
};

// Winmain.cpp mede o pump de mensagens e o protocolo, que ficam fora de
// RenderScene mas dentro do periodo do frame. O enum e o acumulador sao locais
// deste arquivo, entao a contribuicao entra por funcao nomeada — sem indice
// magico atravessando a fronteira de traducao.
long long FrameLoopNowMicroseconds()
{
	return RenderStatsNowMicroseconds();
}

void RecordFrameProtocolUs(long long microseconds)
{
	if (microseconds > 0) g_pendingPhaseUs[RenderPhaseProtocol] += microseconds;
}

void RecordFramePumpUs(long long microseconds)
{
	if (microseconds > 0) g_pendingPhaseUs[RenderPhasePump] += microseconds;
}

void RecordFrameLimiterUs(long long microseconds)
{
	if (microseconds > 0) g_pendingPhaseUs[RenderPhaseLimiter] += microseconds;
}

// Detalhamento de us_characters, alimentado por ZzzCharacter.cpp. Aninha dentro de
// us_characters: e sobreposicao, nao particao. `us_char_parts` nao tem funcao
// propria — sai por subtracao no CSV, para nao precisar instrumentar as 2.400
// linhas de equipamento de RenderCharacter uma a uma.
// A contagem sai da MESMA guarda que o tempo: RenderCharacter tambem e chamado da
// criacao de personagem e da UI de item, fora da fase Characters. Sem a guarda,
// char_poses_avg contaria bonecos de janela como multidao.
//
// O incremento fica FORA do `microseconds > 0`: uma pose que custa menos de 1 us
// ainda e uma pose calculada, e o objetivo aqui e contar quantas o frame faz.
void RecordCharPoseUs(long long microseconds)
{
	if (g_charactersPhaseDepth <= 0)
		return;
	CrowdLod::CountPose();
	if (microseconds > 0)
		g_renderPhaseUs[RenderPhaseCharPose] += microseconds;
}

void RecordCharShadowUs(long long microseconds)
{
	if (g_charactersPhaseDepth <= 0)
		return;
	CrowdLod::CountShadow();
	if (microseconds > 0)
		g_renderPhaseUs[RenderPhaseCharShadow] += microseconds;
}

// Laco por vertice (BMD::TransformVertices) gasto dentro da fase Characters.
//
// ANINHA DENTRO de us_char_parts, nao de us_characters: as duas chamadas de
// TransformVertices que importam saem de RenderPartObject, nao do bloco de pose. Logo
// `us_char_parts - us_char_transform` e o custo de submissao mais os extras por
// personagem (luz de terreno, ganchos de RenderMonsterVisual, nome, barra, pet, marca).
//
// Custo do proprio instrumento: duas leituras de relogio por chamada, e a captura de
// multidao mostrou 366-679 chamadas por frame -- ordem de 20-40 us num frame de 26 ms.
// Sequencias que o coletor de instancias consegue acumular, dentro da fase Characters.
//
// A PERGUNTA: `instanced_draws` e 2 com 59 personagens, e o diagnostico diz que o coletor e
// esvaziado por malha que compoe com o framebuffer. Reordenar a emissao para agrupar as
// reordenaveis renderia lote grande -- ou as reordenaveis vem uma-a-uma intercaladas e nao
// ha lote possivel? Comprimento medio de sequencia responde, e sem refatorar nada.
//
// `accum` conta malha que entrou num balde; `breaks` conta malha que forcou descarga. O
// comprimento medio e accum/breaks, e o pico diz se existe caso bom escondido na media.
//
// So mede com -instancing=on: com off, ShouldInstanceModel reprova tudo e nada acumula.
static unsigned long long g_charBatchAccum = 0;
static unsigned long long g_charBatchBreaks = 0;
static unsigned long long g_charBatchRunMax = 0;
static unsigned long long g_charBatchRunCurrent = 0;

void RecordCharBatchAccum()
{
	if (g_charactersPhaseDepth <= 0) return;
	++g_charBatchAccum;
	++g_charBatchRunCurrent;
	if (g_charBatchRunCurrent > g_charBatchRunMax) g_charBatchRunMax = g_charBatchRunCurrent;
}

void RecordCharBatchBreak()
{
	if (g_charactersPhaseDepth <= 0) return;
	++g_charBatchBreaks;
	g_charBatchRunCurrent = 0;
}

void RecordCharTransformUs(long long microseconds)
{
	if (microseconds > 0 && g_charactersPhaseDepth > 0)
		g_renderPhaseUs[RenderPhaseCharTransform] += microseconds;
}

// Emissao de malha de personagem. A reentrancia ja e resolvida no chamador
// (ZzzObject.cpp), entao aqui basta a guarda de fase.
void RecordCharMeshUs(long long microseconds)
{
	if (microseconds > 0 && g_charactersPhaseDepth > 0)
		g_renderPhaseUs[RenderPhaseCharMesh] += microseconds;
}

void RecordCharDrawUs(long long microseconds)
{
	if (microseconds > 0 && g_charactersPhaseDepth > 0)
		g_renderPhaseUs[RenderPhaseCharDraw] += microseconds;
}

void RecordCharLinkUs(long long microseconds)
{
	if (microseconds > 0 && g_charactersPhaseDepth > 0)
		g_renderPhaseUs[RenderPhaseCharLink] += microseconds;
}

void RecordCharLinkDrawUs(long long microseconds)
{
	if (microseconds > 0 && g_charactersPhaseDepth > 0)
		g_renderPhaseUs[RenderPhaseCharLinkDraw] += microseconds;
}

// Malhas de personagem emitidas no frame. Instrumentado em RenderPartObject, um
// ponto so, em vez de nas ~2.400 linhas de equipamento de RenderCharacter: assim a
// conta inclui corpo, sombra, asas, pet e equipamento sem 20 chamadas espalhadas.
// A guarda de fase e o que mantem objetos de mundo e efeitos fora da conta.
void RecordCharPartMesh()
{
	if (g_charactersPhaseDepth > 0)
		CrowdLod::CountPartMesh();
}

// `-<feature>=off|on|compare`. Ausente mantem o default compilado da fase, que
// e Disabled ate a otimizacao ter passado pelas cenas de referencia.
static void ParseRenderFeatureFlag(const char* commandLine, const char* prefix, Platform::RenderFeature feature)
{
	const char* argument = ::strstr(commandLine, prefix);
	if (argument == NULL)
		return;
	argument += strlen(prefix);
	if (::strncmp(argument, "off", 3) == 0)
		Platform::SetRenderFeatureMode(feature, Platform::RenderFeatureDisabled);
	else if (::strncmp(argument, "compare", 7) == 0)
		Platform::SetRenderFeatureMode(feature, Platform::RenderFeatureCompare);
	else if (::strncmp(argument, "on", 2) == 0)
		Platform::SetRenderFeatureMode(feature, Platform::RenderFeatureEnabled);
}

// `-<prefixo>=ID1,ID2,...`. Ausente limpa a lista, para que remover a flag da
// linha de comando volte ao comportamento padrao em vez de manter a ultima.
static void ParseModelListFlag(const char* commandLine, const char* prefix, void (*setter)(const char*))
{
	const char* argument = ::strstr(commandLine, prefix);
	if (argument == NULL) { setter(NULL); return; }
	argument += strlen(prefix);
	char list[512];
	size_t length = 0;
	while (argument[length] != 0 && argument[length] != ' ' && argument[length] != '\t' &&
		length + 1 < sizeof(list))
		++length;
	memcpy(list, argument, length);
	list[length] = 0;
	setter(list);
}
#include "Interfaces.h"
#include "Camera3D.h"
#include "CharacterList.h"
#ifdef NEW_MUHELPER_ON
#include "CAIController.h"
#endif
#include "WindowTray.h"

extern CUITextInputBox * g_pSingleTextInputBox;
extern CUITextInputBox * g_pSinglePasswdInputBox;
extern int g_iChatInputType;
extern BOOL g_bUseChatListBox;
extern DWORD g_dwMouseUseUIID;
extern DWORD g_dwActiveUIID;
extern DWORD g_dwKeyFocusUIID;	
extern CUIMapName* g_pUIMapName;
extern bool HighLight;
extern CTimer*	g_pTimer;

#ifdef MOVIE_DIRECTSHOW
	extern CMovieScene* g_pMovieScene;
#endif // MOVIE_DIRECTSHOW
	

bool	g_bTimeCheck = false;
int 	g_iBackupTime = 0;

float	g_fMULogoAlpha = 0;

   // extern CGuildCache g_GuildCache;

extern float g_fSpecialHeight;

short   g_shCameraLevel = 0;


/*#ifdef _DEBUG
bool EnableEdit    = true;
#else
bool EnableEdit    = false;
#endif*/

int g_iLengthAuthorityCode = 20;

char *szServerIpAddress;
//char *szServerIpAddress = "210.181.89.215";
WORD g_ServerPort = 44405;

#ifdef MOVIE_DIRECTSHOW
int  SceneFlag = MOVIE_SCENE;
#else // MOVIE_DIRECTSHOW
int  SceneFlag = WEBZEN_SCENE;
#endif // MOVIE_DIRECTSHOW

int  MoveSceneFrame = 0;

extern int g_iKeyPadEnable;


CPhysicsManager g_PhysicsManager;


char *g_lpszMp3[NUM_MUSIC] =
{
	"data\\music\\Pub.mp3",
	"data\\music\\Mutheme.mp3",
	"data\\music\\Church.mp3",
	"data\\music\\Devias.mp3",
	"data\\music\\Noria.mp3",
	"data\\music\\Dungeon.mp3",
	"data\\music\\atlans.mp3",
	"data\\music\\icarus.mp3",
	"data\\music\\tarkan.mp3",
	"data\\music\\lost_tower_a.mp3",
	"data\\music\\lost_tower_b.mp3",
	"data\\music\\kalima.mp3",
    "data\\music\\castle.mp3",
    "data\\music\\charge.mp3",
    "data\\music\\lastend.mp3",
	"data\\music\\huntingground.mp3",
	"data\\music\\Aida.mp3",
	"data\\music\\crywolf1st.mp3",
	"data\\music\\crywolf_ready-02.ogg",
	"data\\music\\crywolf_before-01.ogg",
	"data\\music\\crywolf_back-03.ogg",
	"data\\music\\main_theme.mp3",
	"data\\music\\kanturu_1st.mp3",
	"data\\music\\kanturu_2nd.mp3",
	"data\\music\\KanturuMayaBattle.mp3",
	"data\\music\\KanturuNightmareBattle.mp3",
	"data\\music\\KanturuTower.mp3",
	"data\\music\\BalgasBarrack.mp3",
	"data\\music\\BalgasRefuge.mp3",
	"data\\music\\cursedtemplewait.mp3",
	"data\\music\\cursedtempleplay.mp3",
	"data\\music\\elbeland.mp3",
	"data\\music\\login_theme.mp3",
	"data\\music\\SwampOfCalmness.mp3",
	"data\\music\\Raklion.mp3",
	"data\\music\\Raklion_Hatchery.mp3",
	"data\\music\\Santa_Village.mp3",
	"data\\music\\DuelArena.mp3",
	"data\\music\\PK_Field.mp3",
	"data\\music\\ImperialGuardianFort.mp3",
	"data\\music\\ImperialGuardianFort.mp3",
	"data\\music\\ImperialGuardianFort.mp3",
	"data\\music\\ImperialGuardianFort.mp3",
	"data\\music\\iDoppelganger.mp3",
	"data\\music\\iDoppelganger.mp3",
#ifdef ASG_ADD_MAP_KARUTAN
	"data\\music\\Karutan_A.mp3",
	"data\\music\\Karutan_B.mp3",
#endif	// ASG_ADD_MAP_KARUTAN
};

extern char Mp3FileName[256];

#define MAX_LENGTH_CMB	( 38)

DWORD   g_dwWaitingStartTick;
int     g_iRequestCount;

int     g_iMessageTextStart     = 0;
char    g_cMessageTextCurrNum   = 0;
char    g_cMessageTextNum       = 0;
int     g_iNumLineMessageBoxCustom;
char    g_lpszMessageBoxCustom[NUM_LINE_CMB][MAX_LENGTH_CMB];
int     g_iCustomMessageBoxButton[NUM_BUTTON_CMB][NUM_PAR_BUTTON_CMB];

int     g_iCustomMessageBoxButton_Cancel[NUM_PAR_BUTTON_CMB];

int		g_iCancelSkillTarget	= 0;

#define NUM_LINE_DA		( 1)
int g_iCurrentDialogScript = -1;
int g_iNumAnswer = 0;
char g_lpszDialogAnswer[MAX_ANSWER_FOR_DIALOG][NUM_LINE_DA][MAX_LENGTH_CMB];

DWORD GenerateCheckSum2( BYTE *pbyBuffer, DWORD dwSize, WORD wKey);

void StopMusic()
{
	for ( int i = 0; i < NUM_MUSIC; ++i)
	{
		StopMp3( g_lpszMp3[i]);
	}
}

bool CheckAbuseFilter(char *Text, bool bCheckSlash)
{
	if (bCheckSlash == true)
	{
		if ( Text[0] == '/')
		{
			return false;
		}
	}

	int icntText = 0;
	char TmpText[2048];
	for( int i=0; i<(int)strlen(Text); ++i )
	{
		if ( Text[i]!=32 )
		{
			TmpText[icntText] = Text[i];
			icntText++;
		}
	}
	TmpText[icntText] = 0;

	for(int i=0;i<AbuseFilterNumber;i++)
	{
        if(FindText(TmpText,AbuseFilter[i]))
		{
			return true;
		}
	}
	return false;
}

bool CheckAbuseNameFilter(char *Text)
{
	int icntText = 0;
	char TmpText[256];
	for( int i=0; i<(int)strlen(Text); ++i )
	{
		if ( Text[i]!=32 )
		{
			TmpText[icntText] = Text[i];
			icntText++;
		}
	}
	TmpText[icntText] = 0;

	for(int i=0;i<AbuseNameFilterNumber;i++)
	{
		if(FindText(TmpText,AbuseNameFilter[i]))
		{
			return true;
		}
	}
	return false;
}

bool CheckName()
{
    if( CheckAbuseNameFilter(InputText[0]) || CheckAbuseFilter(InputText[0]) ||
		FindText(InputText[0]," ") || FindText(InputText[0],"\xA1\xA1") ||
		FindText(InputText[0],".") || FindText(InputText[0],"\xB7") || FindText(InputText[0],"\xA1\xAD") ||
		FindText(InputText[0],"Webzen") || FindText(InputText[0],"WebZen") || FindText(InputText[0],"webzen") ||  FindText(InputText[0],"WEBZEN") ||
		FindText(InputText[0],GlobalText[457]) || FindText(InputText[0],GlobalText[458]))
		return true;
	return false;
}

#ifdef MOVIE_DIRECTSHOW
void MovieScene(HDC hDC)
{
	if(g_pMovieScene->GetPlayNum() == 0)
	{
		g_pMovieScene->InitOpenGLClear(hDC);
		
		g_pMovieScene->Initialize_DirectShow(g_hWnd, MOVIE_FILE_WMV);
		
		if(g_pMovieScene->IsFile() == FALSE || g_pMovieScene->IsFailDirectShow() == TRUE)
		{
			g_pMovieScene->Destroy();
			SAFE_DELETE(g_pMovieScene);
			SceneFlag = WEBZEN_SCENE;
			return;	
		}

		g_pMovieScene->PlayMovie();

		if(g_pMovieScene->IsEndMovie())
		{
			g_pMovieScene->Destroy();
			SAFE_DELETE(g_pMovieScene);
			SceneFlag = WEBZEN_SCENE;
			return;
		}
		else
		{
			if(HIBYTE(GetAsyncKeyState(VK_ESCAPE))==128 || HIBYTE(GetAsyncKeyState(VK_RETURN))==128)
			{
				g_pMovieScene->Destroy();
				SAFE_DELETE(g_pMovieScene);
				SceneFlag = WEBZEN_SCENE;
				return;
			}
		}
	}
	else
	{
		g_pMovieScene->Destroy();
		SAFE_DELETE(g_pMovieScene);
		SceneFlag = WEBZEN_SCENE;
		return;
	}
}
#endif // MOVIE_DIRECTSHOW

bool EnableMainRender = false;
extern int HeroKey;

void WebzenScene(HDC hDC)
{
	CUIMng& rUIMng = CUIMng::Instance();

	g_ErrorReport.Write("[BootTrace] Loading: begin.\r\n");
	OpenFont();
	g_ErrorReport.Write("[BootTrace] Loading: font ready.\r\n");
	ClearInput();

	LoadBitmap("Interface\\New_lo_back_01.jpg", BITMAP_TITLE, GL_LINEAR);
	LoadBitmap("Interface\\New_lo_back_02.jpg", BITMAP_TITLE+1, GL_LINEAR);
	LoadBitmap("Interface\\MU_TITLE.tga", BITMAP_TITLE+2, GL_LINEAR);
	LoadBitmap("Interface\\lo_121518.tga", BITMAP_TITLE+3, GL_LINEAR);
	LoadBitmap("Interface\\New_lo_webzen_logo.tga", BITMAP_TITLE+4, GL_LINEAR);
	LoadBitmap("Interface\\lo_lo.jpg", BITMAP_TITLE+5, GL_LINEAR, GL_REPEAT);
	LoadBitmap("Interface\\lo_back_s5_03.jpg", BITMAP_TITLE+6, GL_LINEAR);
	LoadBitmap("Interface\\lo_back_s5_04.jpg", BITMAP_TITLE+7, GL_LINEAR);
	if(rand()%100 <= 70)
	{
		LoadBitmap("Interface\\lo_back_im01.jpg", BITMAP_TITLE+8, GL_LINEAR);
		LoadBitmap("Interface\\lo_back_im02.jpg", BITMAP_TITLE+9, GL_LINEAR);	
		LoadBitmap("Interface\\lo_back_im03.jpg", BITMAP_TITLE+10, GL_LINEAR);
		LoadBitmap("Interface\\lo_back_im04.jpg", BITMAP_TITLE+11, GL_LINEAR);
		LoadBitmap("Interface\\lo_back_im05.jpg", BITMAP_TITLE+12, GL_LINEAR);
		LoadBitmap("Interface\\lo_back_im06.jpg", BITMAP_TITLE+13, GL_LINEAR);
	}
	else
	{
		LoadBitmap("Interface\\lo_back_s5_im01.jpg", BITMAP_TITLE+8, GL_LINEAR);
		LoadBitmap("Interface\\lo_back_s5_im02.jpg", BITMAP_TITLE+9, GL_LINEAR);	
		LoadBitmap("Interface\\lo_back_s5_im03.jpg", BITMAP_TITLE+10, GL_LINEAR);
		LoadBitmap("Interface\\lo_back_s5_im04.jpg", BITMAP_TITLE+11, GL_LINEAR);
		LoadBitmap("Interface\\lo_back_s5_im05.jpg", BITMAP_TITLE+12, GL_LINEAR);
		LoadBitmap("Interface\\lo_back_s5_im06.jpg", BITMAP_TITLE+13, GL_LINEAR);
	}

	g_ErrorReport.Write("[BootTrace] Loading: title textures ready.\r\n");
	rUIMng.CreateTitleSceneUI();
	g_ErrorReport.Write("[BootTrace] Loading: title UI ready.\r\n");
	
	FogEnable = false;
	
	::EnableAlphaTest();
	g_ErrorReport.Write("[BootTrace] Loading: basic data begin.\r\n");
	OpenBasicData(hDC);
	g_ErrorReport.Write("[BootTrace] Loading: basic data ready.\r\n");

	g_ErrorReport.Write("[BootTrace] Loading: main UI begin.\r\n");
	g_pNewUISystem->LoadMainSceneInterface();
	g_ErrorReport.Write("[BootTrace] Loading: main UI ready.\r\n");

	CUIMng::Instance().RenderTitleSceneUI(hDC, 11, 11);
	g_ErrorReport.Write("[BootTrace] Loading: title UI rendered.\r\n");

	rUIMng.ReleaseTitleSceneUI();
	g_ErrorReport.Write("[BootTrace] Loading: title UI released.\r\n");
		DeleteBitmap(BITMAP_TITLE);
   	DeleteBitmap(BITMAP_TITLE+1);
	DeleteBitmap(BITMAP_TITLE+2);
   	DeleteBitmap(BITMAP_TITLE+3);
	DeleteBitmap(BITMAP_TITLE+4);
   	DeleteBitmap(BITMAP_TITLE+5);

	for(int i=6; i<14; ++i)
		DeleteBitmap(BITMAP_TITLE+i);

	g_ErrorReport.Write( "> Loading ok.\r\n");

	SceneFlag = LOG_IN_SCENE;	//

	gInterface.hdc = hDC;

	gInterface.m_Lua.Generic_Call("FinalBoot", ">");
}

int MenuStateCurrent = MENU_SERVER_LIST;
int MenuStateNext    = MENU_SERVER_LIST;
int DeleteGuildIndex = -1;

void DeleteCharacter()
{
	SelectedHero = -1;
	if (g_iChatInputType == 1)
	{
		g_pSinglePasswdInputBox->GetText(InputText[0]);
		g_pSinglePasswdInputBox->SetText(NULL);
		g_pSinglePasswdInputBox->SetState(UISTATE_HIDE);
	}
	SendRequestDeleteCharacter(CharactersClient[SelectedHero].ID,InputText[0]);

	MenuStateCurrent = MENU_DELETE_LEFT;
	MenuStateNext    = MENU_NEW_DOWN;
	PlayBuffer(SOUND_MENU01);
	
	ClearInput();
	InputEnable = false;
}
int  ErrorMessage = NULL;
int	 ErrorMessageNext = NULL;
extern bool g_bEnterPressed;

bool IsEnterPressed() {
	return g_bEnterPressed;
}

void SetEnterPressed( bool enterpressed ) {
	g_bEnterPressed = enterpressed;
}

BOOL CheckOptionMouseClick(int iOptionPos_y, BOOL bPlayClickSound = TRUE)
{
	if (CheckMouseIn((640-120)/2, 30+iOptionPos_y, 120, 22) && MouseLButtonPush)
	{
		MouseLButtonPush = false;
		MouseUpdateTime = 0;
		MouseUpdateTimeMax = 6;
		if (bPlayClickSound == TRUE) PlayBuffer(SOUND_CLICK01);
		return TRUE;
	}
	return FALSE;
}

int SeparateTextIntoLines( const char *lpszText, char *lpszSeparated, int iMaxLine, int iLineSize)
{
	int iLine = 0;
	const char *lpLineStart = lpszText;
	char *lpDst = lpszSeparated;
	const char *lpSpace = NULL;
	int iMbclen = 0;
	for ( const char* lpSeek = lpszText; *lpSeek; lpSeek += iMbclen, lpDst += iMbclen)
	{
		iMbclen = _mbclen( ( unsigned char*)lpSeek);
		if ( iMbclen + ( int)( lpSeek - lpLineStart) >= iLineSize)
		{
			if ( lpSpace && ( int)( lpSeek - lpSpace) < min( 10, iLineSize / 2))
			{
				lpDst -= ( lpSeek - lpSpace - 1);
				lpSeek = lpSpace + 1;
			}

			lpLineStart = lpSeek;
			*lpDst = '\0';
			if ( iLine >= iMaxLine - 1)
			{
				break;
			}
			++iLine;
			lpDst = lpszSeparated + iLine * iLineSize;
			lpSpace = NULL;
		}

		memcpy( lpDst, lpSeek, iMbclen);
		if ( *lpSeek == ' ')
		{
			lpSpace = lpSeek;
		}
	}
	*lpDst = '\0';

	return ( iLine + 1);
}

void SetEffectVolumeLevel ( int level )
{
	if(level > 9)
		level = 9;
	if(level < 0)
		level = 0;

	if(level == 0)
	{
		SetMasterVolume(-10000);
	}
	else
	{
		long vol = -2000*log10(10.f/float(level));
		SetMasterVolume(vol);
	}
}

void SetViewPortLevel ( int Wheel )
{
    if ( (HIBYTE( GetAsyncKeyState(VK_CONTROL))==128) )
    {
        if ( Wheel>0 )
            g_shCameraLevel--;
        else if ( Wheel<0 )
            g_shCameraLevel++;

        MouseWheel = 0;

	    if ( g_shCameraLevel>4 )
		    g_shCameraLevel = 4;
	    if ( g_shCameraLevel<0 )
		    g_shCameraLevel = 0;
    }
}

void RenderInfomation3D()
{
	bool Success = false;

	if ( ( ( ErrorMessage==MESSAGE_TRADE_CHECK || ErrorMessage==MESSAGE_CHECK ) && AskYesOrNo==1 ) 
		|| ErrorMessage==MESSAGE_USE_STATE 
		|| ErrorMessage==MESSAGE_USE_STATE2) 
	{
		Success = true;
	}

    if ( ErrorMessage==MESSAGE_TRADE_CHECK && AskYesOrNo==5 )
	{
		Success = true;
	}
	if ( ErrorMessage==MESSAGE_PERSONALSHOP_WARNING ) 
	{
		Success = true;
	}

	if ( Success )
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glViewport2(0,0,WindowWidth,WindowHeight);
        gluPerspective2(1.f,(float)(WindowWidth)/(float)(WindowHeight),CameraViewNear,CameraViewFar);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        GetOpenGLMatrix(CameraMatrix);
        EnableDepthTest();
        EnableDepthMask();

        float Width, Height;
        float x = (640-150)/2;
        float y;
        if ( ErrorMessage==MESSAGE_TRADE_CHECK )
        {
            y = 60+55;
        }
        else
        {
            y = 60+55;
        }

	    Width=40.f;Height=60.f;
		int iRenderType = ErrorMessage;
		if(AskYesOrNo == 5)
			iRenderType = MESSAGE_USE_STATE;
		switch( iRenderType )
		{
		case MESSAGE_USE_STATE :
		case MESSAGE_USE_STATE2 :
        case MESSAGE_PERSONALSHOP_WARNING :
            RenderItem3D(x,y,Width,Height,TargetItem.Type,TargetItem.Level,TargetItem.Option1,TargetItem.ExtOption,true);
			break;

		default :
            RenderItem3D(x,y,Width,Height,PickItem.Type,PickItem.Level,PickItem.Option1,PickItem.ExtOption,true);
			break;
		}

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		UpdateMousePositionn();
    }
}

void RenderInfomation()
{
	if (SceneFlag == MAIN_SCENE)
	{
		gInterface.m_Lua.Generic_Call("MainProc", ">");
	}

	gCharacterList.RenderCharacterList();

	RenderNotices();
    
	CUIMng::Instance().Render();

	if(SceneFlag == LOG_IN_SCENE || SceneFlag == CHARACTER_SCENE)
	{
		RenderCursor();
	}
	
    RenderInfomation3D();
}

BOOL ShowCheckBox( int num, int index, int message )
{
	if ( message==MESSAGE_USE_STATE || message==MESSAGE_USE_STATE2)
    {
        char Name[50] = { 0, };
        if ( TargetItem.Type==ITEM_HELPER+15 )
        {
            switch ( (TargetItem.Level>>3)&15 )
            {
            case 0:sprintf(Name,"%s", GlobalText[168] );break;
            case 1:sprintf(Name,"%s", GlobalText[169] );break;
            case 2:sprintf(Name,"%s", GlobalText[167] );break;
            case 3:sprintf(Name,"%s", GlobalText[166] );break;
			case 4:sprintf(Name,"%s", GlobalText[1900] );break;
            }
        }

		if (message==MESSAGE_USE_STATE2)
			sprintf ( g_lpszMessageBoxCustom[0], "( %s%s )", Name, GlobalText[1901] );
		else
			sprintf ( g_lpszMessageBoxCustom[0], "( %s )", Name );
		
        num++;
        for ( int i=1; i<num; ++i )
        {
	        sprintf ( g_lpszMessageBoxCustom[i], "%s", GlobalText[index] );
        }
        g_iNumLineMessageBoxCustom = num;
    }
	else if ( message==MESSAGE_PERSONALSHOP_WARNING )
	{
		char szGold[256];
		ConvertGold(InputGold,szGold);
	    sprintf ( g_lpszMessageBoxCustom[0], GlobalText[index], szGold );

        for ( int i=1; i<num; ++i )
        {
	        sprintf ( g_lpszMessageBoxCustom[i], "%s", GlobalText[index+i] );
        }
        g_iNumLineMessageBoxCustom = num;
	}
    else if ( message==MESSAGE_CHAOS_CASTLE_CHECK )
    {
	    g_iNumLineMessageBoxCustom = 0;
        for ( int i=0; i<num; ++i )
        {
	        g_iNumLineMessageBoxCustom += SeparateTextIntoLines( GlobalText[index+i], g_lpszMessageBoxCustom[g_iNumLineMessageBoxCustom], NUM_LINE_CMB, MAX_LENGTH_CMB);
        }
    }
	else if ( message==MESSAGE_GEM_INTEGRATION3)
	{
		char tBuf[MAX_GLOBAL_TEXT_STRING];
		char tLines[2][30];
		for(int t = 0; t < 2; ++t) memset(tLines[t], 0, 20);
		g_iNumLineMessageBoxCustom = 0;
		if(COMGEM::isComMode())
		{
			if(COMGEM::m_cGemType == 0) sprintf(tBuf, GlobalText[1809], GlobalText[1806], COMGEM::m_cCount);
			else sprintf(tBuf, GlobalText[1809], GlobalText[1807], COMGEM::m_cCount);

			g_iNumLineMessageBoxCustom += SeparateTextIntoLines( tBuf, 
				tLines[g_iNumLineMessageBoxCustom], 2, 30);

			for(int t = 0; t < 2; ++t) strcpy(g_lpszMessageBoxCustom[t], tLines[t]);

			sprintf(g_lpszMessageBoxCustom[g_iNumLineMessageBoxCustom], GlobalText[1810], COMGEM::m_iValue);
			++g_iNumLineMessageBoxCustom;

		}
		else
		{
			int t_GemLevel = COMGEM::GetUnMixGemLevel()+1;
			if(COMGEM::m_cGemType == 0) sprintf(tBuf, GlobalText[1813], GlobalText[1806], t_GemLevel);
			else sprintf(tBuf, GlobalText[1813], GlobalText[1807], t_GemLevel);

			g_iNumLineMessageBoxCustom += SeparateTextIntoLines( tBuf, 
				tLines[g_iNumLineMessageBoxCustom], 2, 30);

			for(int t = 0; t < 2; ++t) strcpy(g_lpszMessageBoxCustom[t], tLines[t]);

			sprintf(g_lpszMessageBoxCustom[g_iNumLineMessageBoxCustom], GlobalText[1814], COMGEM::m_iValue);
			++g_iNumLineMessageBoxCustom;
		}
	}
	else if(message == MESSAGE_CANCEL_SKILL)
	{
		char tBuf[MAX_GLOBAL_TEXT_STRING];	
		sprintf(tBuf, "%s%s", SkillAttribute[index].Name, GlobalText[2046]);
		g_iNumLineMessageBoxCustom = SeparateTextIntoLines(tBuf, g_lpszMessageBoxCustom[0], 2, MAX_LENGTH_CMB);
		g_iCancelSkillTarget = index;
	}
    else
    {
        for ( int i=0; i<num; ++i )
        {
	        strcpy ( g_lpszMessageBoxCustom[i], GlobalText[index+i]);
        }
        g_iNumLineMessageBoxCustom = num;
    }

	ZeroMemory( g_iCustomMessageBoxButton, NUM_BUTTON_CMB * NUM_PAR_BUTTON_CMB * sizeof ( int) );

    int iOkButton[5]     = { 1,  21, 90, 70, 21};
    int iCancelButton[5] = { 3, 120, 90, 70, 21};

	if(message == MESSAGE_USE_STATE2)
	{
		iOkButton[1]	 = 22;	
		iOkButton[2]	 = 92;	// y
		iOkButton[3]     = 49;
		iOkButton[4]     = 16;

		iCancelButton[1] = 82;
		iCancelButton[2] = 92;	// y
		iCancelButton[3] = 49;
		iCancelButton[4] = 16;

		g_iCustomMessageBoxButton_Cancel[0] = 5;
		g_iCustomMessageBoxButton_Cancel[1] = 142;	// x
		g_iCustomMessageBoxButton_Cancel[2] = 92;	// y
		g_iCustomMessageBoxButton_Cancel[3] = 49;	// width
		g_iCustomMessageBoxButton_Cancel[4] = 16;	// height
	}

    if ( message==MESSAGE_CHAOS_CASTLE_CHECK )
    {
        iOkButton[2]     = 120;
        iCancelButton[2] = 120;
    }
	
    memcpy( g_iCustomMessageBoxButton[0], iOkButton, 5 * sizeof ( int));
    memcpy( g_iCustomMessageBoxButton[1], iCancelButton, 5 * sizeof ( int));

	return true;
}

int    CameraWalkCut;
int    CurrentCameraCount    = -1;
int    CurrentCameraWalkType = 0;
int    CurrentCameraNumber   = 0;
vec3_t CurrentCameraPosition;
vec3_t CurrentCameraAngle;
float  CurrentCameraWalkDelta[6]; 
float  CameraWalk[] = 
{
	 0.f,-1000.f,500.f,-80.f,0.f, 0.f,
	 0.f,-1100.f,500.f,-80.f,0.f, 0.f,
	 0.f,-1100.f,500.f,-80.f,0.f, 0.f,
     0.f,-1100.f,500.f,-80.f,0.f, 0.f,
	 0.f,-1100.f,500.f,-80.f,0.f, 0.f,
	200.f,-800.f,250.f,-87.f,0.f, -10.f,
};

void MoveCharacterCamera(vec3_t Origin,vec3_t Position,vec3_t Angle)
{
	vec3_t TransformPosition;
	CameraAngle[0] = 0.f;
	CameraAngle[1] = 0.f;
	CameraAngle[2] = Angle[2];
	float Matrix[3][4];
	AngleMatrix(CameraAngle,Matrix);
	VectorIRotate(Position,Matrix,TransformPosition);
	VectorAdd(Origin,TransformPosition,CameraPosition);
	CameraAngle[0] = Angle[0];
}

void MoveCamera()
{
	if (CCameraMove::GetInstancePtr()->IsTourMode())
	{
		return;
	}

    if(CurrentCameraCount == -1)
	{
		for(int i=0;i<3;i++)
		{
			CurrentCameraPosition[i] = CameraWalk[i  ];
			CurrentCameraAngle   [i] = CameraWalk[i+3];
		}
		CurrentCameraNumber = 1;
		CurrentCameraWalkType = 1;

		for(int i=0;i<3;i++)
		{
			CurrentCameraWalkDelta[i  ] = (CameraWalk[CurrentCameraNumber*6+i  ]-CurrentCameraPosition[i])/128;
			CurrentCameraWalkDelta[i+3] = (CameraWalk[CurrentCameraNumber*6+i+3]-CurrentCameraAngle   [i])/128;
		}
	}
	CurrentCameraCount++;
    if((CameraWalkCut==0 && CurrentCameraCount>=40) || (CameraWalkCut>0 && CurrentCameraCount>=128))
	{
        CurrentCameraCount = 0;
		if(CameraWalkCut==0)
		{
			CameraWalkCut = 1;
		}
		else
		{
            if(SceneFlag == LOG_IN_SCENE)
			{
				CurrentCameraNumber = rand()%4+1;
				CurrentCameraWalkType = rand()%2;
			}
			else
			{
      			CurrentCameraNumber = 5;
				CurrentCameraWalkType = 0;
			}
		}
		for(int i=0;i<3;i++)
		{
			CurrentCameraWalkDelta[i  ] = (CameraWalk[CurrentCameraNumber*6+i  ]-CurrentCameraPosition[i])/128;
			CurrentCameraWalkDelta[i+3] = (CameraWalk[CurrentCameraNumber*6+i+3]-CurrentCameraAngle   [i])/128;
		}
	}
	if(CurrentCameraWalkType==0)
	{
    	for(int i=0;i<3;i++)
		{
			CurrentCameraPosition[i] += (CameraWalk[CurrentCameraNumber*6+i  ]-CurrentCameraPosition[i])/6;
			CurrentCameraAngle   [i] += (CameraWalk[CurrentCameraNumber*6+i+3]-CurrentCameraAngle   [i])/6;
		}
	}
	else
	{
    	for(int i=0;i<2;i++)
		{
			CurrentCameraPosition[i] += CurrentCameraWalkDelta[i  ];
		}
	}
    CameraFOV = 45.f;
	vec3_t Position;
	Vector(0.f,0.f,0.f,Position);
    MoveCharacterCamera(Position,CurrentCameraPosition,CurrentCameraAngle);
}

bool MenuCancel         = true;
bool EnableSocket       = false;
bool InitLogIn          = false;
bool InitLoading        = false;
bool InitCharacterScene = false;
bool InitMainScene      = false;
int  MenuY = 480;
int  MenuX = -200;
extern char LogInID[MAX_ID_SIZE+1];
extern char m_ExeVersion[11];

BOOL Util_CheckOption( char *lpszCommandLine, unsigned char cOption, char *lpszString);

extern DWORD g_dwBKConv;
extern DWORD g_dwBKSent;
extern BOOL g_bIMEBlock;

int SelectedHero = -1;
bool MoveMainCamera();

void StartGame()
{
	{
		if (CTLCODE_01BLOCKCHAR & CharactersClient[SelectedHero].CtlCode)
			CUIMng::Instance().PopUpMsgWin(MESSAGE_BLOCKED_CHARACTER);
		else
		{
			CharacterAttribute->Level = CharactersClient[SelectedHero].Level;
			CharacterAttribute->Class = CharactersClient[SelectedHero].Class;
			CharacterAttribute->Skin  = CharactersClient[SelectedHero].Skin;
			::strcpy(CharacterAttribute->Name, CharactersClient[SelectedHero].ID);

			::ReleaseCharacterSceneData();
			InitLoading = false;
			SceneFlag = LOADING_SCENE;
		}
	}
}

void CreateCharacterScene()
{
	g_pNewUIMng->ResetActiveUIObj();

	EnableMainRender = true;
	MouseOnWindow = false;
	ErrorMessage = NULL;

#ifdef PJH_NEW_SERVER_SELECT_MAP
	gMapManager.WorldActive = WD_74NEW_CHARACTER_SCENE;
#else //PJH_NEW_SERVER_SELECT_MAP
	gMapManager.WorldActive = WD_78NEW_CHARACTER_SCENE;
#endif //PJH_NEW_SERVER_SELECT_MAP

	gMapManager.LoadWorld(gMapManager.WorldActive);
    OpenCharacterSceneData();

    CreateCharacterPointer(&CharacterView,MODEL_FACE+1,0,0);
	CharacterView.Class = 1;
	CharacterView.Object.Kind = 0;
	
	SelectedHero = -1;
	CUIMng::Instance().CreateCharacterScene();

    ClearInventory();
    CharacterAttribute->SkillNumber = 0;

	for(int i=0;i<MAX_MAGIC;i++)
		CharacterAttribute->Skill[i] = 0;

	for(int i=EQUIPMENT_WEAPON_RIGHT;i<EQUIPMENT_HELPER;i++)
		CharacterMachine->Equipment[i].Level = 0;

	g_pNewUISystem->HideAll();

	g_iKeyPadEnable = 0;
	GuildInputEnable = false;
	TabInputEnable   = false;
	GoldInputEnable  = false;
	InputEnable      = true;
    ClearInput();
	InputIndex = 0;
    InputTextWidth = 90;
    InputNumber = 1;

	for(int i=0;i<MAX_WHISPER;i++)
	{
		g_pChatListBox->AddText("", "", SEASON3B::TYPE_WHISPER_MESSAGE);
	}

	HIMC hIMC = ImmGetContext(g_hWnd);
    DWORD Conversion, Sentence;

	Conversion = IME_CMODE_NATIVE;
	Sentence = IME_SMODE_NONE;

	g_bIMEBlock = FALSE;
	RestoreIMEStatus();
	ImmSetConversionStatus(hIMC, Conversion, Sentence);
	ImmGetConversionStatus(hIMC, &g_dwBKConv, &g_dwBKSent);
	SaveIMEStatus();
	ImmReleaseContext(g_hWnd, hIMC);
	g_bIMEBlock = TRUE;

    g_ErrorReport.Write( "> Character scene init success.\r\n");
}

void NewMoveCharacterScene()
{
	if (CurrentProtocolState < RECEIVE_CHARACTERS_LIST)
	{
		return;
	}


	if (!InitCharacterScene)
	{
		InitCharacterScene = true;
		CreateCharacterScene();
	}

	gCharacterList.MoveCharacterList();

    InitTerrainLight();
    MoveObjects();
	MoveBugs();
    MoveCharactersClient();
    MoveCharacterClient(&CharacterView);

	MoveEffects();
    MoveJoints();
    MoveParticles();
	MoveBoids();

	ThePetProcess().UpdatePets();

    MoveCamera();

#if defined _DEBUG || defined FOR_WORK
	char lpszTemp[256];
	if (::Util_CheckOption(::GetCommandLine(), 'c', lpszTemp))
	{
		SelectedHero = ::atoi(lpszTemp);
		::StartGame();
	}
#endif

	CInput& rInput = CInput::Instance();
	CUIMng& rUIMng = CUIMng::Instance();

	if (rInput.IsKeyDown(VK_RETURN))
	{
		if (!(rUIMng.m_MsgWin.IsShow() || rUIMng.m_CharMakeWin.IsShow()
			|| rUIMng.m_SysMenuWin.IsShow() || rUIMng.m_OptionWin.IsShow())
			&& SelectedHero > -1 && SelectedHero < gCharacterList.MaxCharacters)
		{
			::PlayBuffer(SOUND_CLICK01);

			if(SelectedCharacter >= 0)
				SelectedHero = SelectedCharacter;

			::StartGame();
		}
	}
	else if (rInput.IsKeyDown(VK_ESCAPE))
	{
		if (!(rUIMng.m_MsgWin.IsShow() || rUIMng.m_CharMakeWin.IsShow()
			|| rUIMng.m_SysMenuWin.IsShow() || rUIMng.m_OptionWin.IsShow()
			)
			&& rUIMng.IsSysMenuWinShow() )
		{
			::PlayBuffer(SOUND_CLICK01);
			rUIMng.ShowWin(&rUIMng.m_SysMenuWin);
		}
	}

	if (rUIMng.IsCursorOnUI())
	{
		return;
	}

	if (rInput.IsLBtnDbl() && rUIMng.m_CharSelMainWin.IsShow())
	{
		if (SelectedCharacter < 0 || SelectedCharacter >(gCharacterList.MaxCharacters - 1))
		{
			return;
		}

		SelectedHero = SelectedCharacter;
		::StartGame();
	}
	else if(rInput.IsLBtnDn())
	{
		if (SelectedCharacter < 0 || SelectedCharacter >(gCharacterList.MaxCharacters - 1))
			SelectedHero = -1;
		else
			SelectedHero = SelectedCharacter;
		rUIMng.m_CharSelMainWin.UpdateDisplay();
	}

	g_ConsoleDebug->UpdateMainScene();
}

bool NewRenderCharacterScene(HDC hDC)
{
	if(!InitCharacterScene) 
	{
		return false;
	}
	if(CurrentProtocolState < RECEIVE_CHARACTERS_LIST) 
	{
		return false;
	}

    FogEnable = false;
	vec3_t pos;
	Vector(9758.0f, 18913.0f, 675.0f, pos);

    GWidescreen.SceneLogin();

	int Width,Height;

	SetLegacyColor3f(1.f,1.f,1.f);
#ifndef PJH_NEW_SERVER_SELECT_MAP
	BeginBitmap();
		Width = 320;
		Height = 320;
		RenderBitmap(BITMAP_LOG_IN+9,  (float)0,(float)25,(float)Width,(float)Height,0.f,0.f);
		RenderBitmap(BITMAP_LOG_IN+10,(float)320,(float)25,(float)Width,(float)Height,0.f,0.f);
	EndBitmap();
#endif //PJH_NEW_SERVER_SELECT_MAP
	Height = 480;
	{
		ScopedRenderPhase phase(RenderPhaseFrameBegin);
		Width = FrameBeginOpengl();
		glClearColor(0.f,0.f,0.f,1.f);
	}
	// A v14 mediu 7,3 ms de CPU nesta cena para 58 draw calls — mais CPU que
	// Lorencia inteira — com 87% em us_unmeasured, porque aqui nao havia uma unica
	// ScopedRenderPhase. As fases reusadas sao as mesmas de MainScene de proposito:
	// assim a cena de personagem fica comparavel coluna a coluna com o gameplay.
	{
		ScopedRenderPhase phase(RenderPhaseSetup);
		BeginOpengl(0, 25, GetWindowsX, GetWindowsY - 50);
	}

	{
		ScopedRenderPhase phase(RenderPhaseFrustum);
		CreateFrustrum((float)Width/(float)640, pos);
	}

	OBJECT *o = &CharactersClient[SelectedHero].Object;

	CreateScreenVector(MouseX,MouseY,MouseTarget);
	for (int i = 0; i < gCharacterList.MaxCharacters; i++)
	{
		CharactersClient[i].Object.Position[2] = 163.0f;
		Vector ( 0.0f, 0.0f, 0.0f, CharactersClient[i].Object.Light );
	}

	if(SelectedHero!=-1 && o->Live)
	{
		EnableAlphaBlend();
		vec3_t Light;
		Vector ( 1.0f, 1.0f, 1.0f, Light );
		Vector ( 1.0f, 1.0f, 1.0f, o->Light );
		AddTerrainLight(o->Position[0],o->Position[1],Light,1,PrimaryTerrainLight);
		DisableAlphaBlend();
	}

	CHARACTER* pCha = NULL;
	OBJECT* pObj = NULL;

	for (int i = 0; i < gCharacterList.MaxCharacters; ++i)
	{
		pCha = &CharactersClient[i];
		pObj = &pCha->Object;
		if (pCha->Helper.Type == MODEL_HELPER + 3 || gHelperSystem.CheckHelperType(pCha->Helper.Type, 4) == 1)
		{
#ifdef PJH_NEW_SERVER_SELECT_MAP
			pObj->Position[2] = 194.5f;
#else //PJH_NEW_SERVER_SELECT_MAP
			pObj->Position[2] = 55.0f;
#endif //PJH_NEW_SERVER_SELECT_MAP
		}
		else
		{
#ifdef PJH_NEW_SERVER_SELECT_MAP
			pObj->Position[2] = 169.5f;
#else //PJH_NEW_SERVER_SELECT_MAP
			pObj->Position[2] = 30.0f;
#endif //PJH_NEW_SERVER_SELECT_MAP
		}
	}

	{
		ScopedRenderPhase phase(RenderPhaseTerrain);
		RenderTerrain(false);
	}
	{
		ScopedRenderPhase phase(RenderPhaseObjects);
		RenderObjects();
	}
	{
		ScopedRenderPhase phase(RenderPhaseCharacters);
		RenderCharactersClient();
	}

	{
		ScopedRenderPhase phase(RenderPhaseSelect);
		if(!CUIMng::Instance().IsCursorOnUI())
			SelectObjects();
	}

	{
		ScopedRenderPhase phase(RenderPhaseMisc);
		RenderBugs();
	}
	{
		ScopedRenderPhase phase(RenderPhaseEffects);
		RenderBlurs();
		RenderJoints();
		RenderEffects();
	}
	{
		ScopedRenderPhase phase(RenderPhaseMisc);
		ThePetProcess().RenderPets();
		RenderBoids();
	}
	{
		ScopedRenderPhase phase(RenderPhaseObjects);
		RenderObjects_AfterCharacter();
	}
	{
		ScopedRenderPhase phase(RenderPhaseSprites);
		CheckSprites();
	}

	if(SelectedHero!=-1 && o->Live)
	{
		vec3_t vLight;
		
		Vector ( 1.0f, 1.0f, 1.f, vLight );
		float fLumi = sinf ( WorldTime*0.0015f )*0.3f+0.5f;
		Vector ( fLumi*vLight[0], fLumi*vLight[1], fLumi*vLight[2], vLight );

		float Rotation = (int)WorldTime%3600/(float)10.f;
		Vector ( 0.15f, 0.15f, 0.15f, o->Light );
		
		EnableAlphaBlend();
		Rotation = (int)WorldTime % 3600;
		RenderCircle(BITMAP_MAGIC + 2, o->Position, 50.0, 70.0, 200.0, Rotation, 0.0, 0.0);
		RenderCircle(BITMAP_MAGIC + 2, o->Position, 50.0, 70.0, 200.0, Rotation, 0.0, 0.0);
		
		g_csMapServer.SetHeroID ( (char *)CharactersClient[SelectedHero].ID );
	}

	{
		ScopedRenderPhase phase(RenderPhaseSprites);
		BeginSprite();
		RenderSprites();
		RenderParticles();
		RenderPoints();
		EndSprite();
	}
	// Sem isto us_ui lia 0 na cena de personagem e o custo caia em us_unmeasured --
	// justamente o bloco que era o maior suspeito dos 87% nao medidos aqui.
	ScopedRenderPhase uiPhase(RenderPhaseUi);
	BeginBitmap();
	RenderInfomation();

	if ((GetTickCount() - gTrayMode.LastPress) > 200) {
		if (GetKeyState(VK_F12) & 0x8000) {
			gTrayMode.SwitchState();
			gTrayMode.LastPress = GetTickCount();
		}
	}

#ifdef ENABLE_EDIT
	RenderDebugWindow();
#endif //ENABLE_EDIT

	EndBitmap();

	EndOpengl();

	return true;
}

void CreateLogInScene()
{
	EnableMainRender = true;
#ifdef PJH_NEW_SERVER_SELECT_MAP
	gMapManager.WorldActive = 94;
#else
	World = WD_77NEW_LOGIN_SCENE;
#endif //PJH_NEW_SERVER_SELECT_MAP
	gMapManager.LoadWorld(gMapManager.WorldActive);

	OpenLogoSceneData();

	CUIMng::Instance().CreateLoginScene();

	CurrentProtocolState = REQUEST_JOIN_SERVER;
    CreateSocket(szServerIpAddress,g_ServerPort);
    EnableSocket = true;

	GuildInputEnable = false;
	TabInputEnable   = false;
	GoldInputEnable  = false;
	InputEnable      = true;
	ClearInput();

	if (g_iChatInputType == 0)
	{
		strcpy(InputText[0],m_ID);
		InputLength[0] = strlen(InputText[0]);
		InputTextMax[0] = MAX_ID_SIZE;
		if(InputLength[0] == 0)	InputIndex = 0;
		else InputIndex = 1;
	}
	InputNumber = 2;
    InputTextHide[1] = 1;

	CCameraMove::GetInstancePtr()->PlayCameraWalk(Hero->Object.Position, 1000);
#ifdef PJH_NEW_SERVER_SELECT_MAP
	CCameraMove::GetInstancePtr()->SetTourMode(TRUE, FALSE, 1);
#else //PJH_NEW_SERVER_SELECT_MAP
	CCameraMove::GetInstancePtr()->SetTourMode(TRUE, TRUE);
#endif //PJH_NEW_SERVER_SELECT_MAP
	
	GWidescreen.SceneLogin();

	g_fMULogoAlpha = 0;
	
	::PlayMp3(g_lpszMp3[MUSIC_LOGIN_THEME]);

	g_ErrorReport.Write( "> Login Scene init success.\r\n");
}

void NewMoveLogInScene()
{
	if(!InitLogIn)
	{
		InitLogIn = true;
		CreateLogInScene();
	}

#ifdef MOVIE_DIRECTSHOW
	if(CUIMng::Instance().IsMoving() == true)
	{
		return;
	}
#endif // MOVIE_DIRECTSHOW
	if (!CUIMng::Instance().m_CreditWin.IsShow())
	{
		InitTerrainLight();
		MoveObjects();
		MoveBugs();
		MoveLeaves();
		MoveCharactersClient();
		MoveEffects();
		MoveJoints();
		MoveParticles();
		MoveBoids();
		ThePetProcess().UpdatePets();
		MoveCamera();
	}

	if (CInput::Instance().IsKeyDown(VK_ESCAPE))
	{
		CUIMng& rUIMng = CUIMng::Instance();
		if (!(rUIMng.m_MsgWin.IsShow() || rUIMng.m_LoginWin.IsShow()
			|| rUIMng.m_SysMenuWin.IsShow() || rUIMng.m_OptionWin.IsShow()
			|| rUIMng.m_CreditWin.IsShow()
			)
			&& rUIMng.m_LoginMainWin.IsShow() && rUIMng.m_ServerSelWin.IsShow()
			&& rUIMng.IsSysMenuWinShow())
		{
			::PlayBuffer(SOUND_CLICK01);
			rUIMng.ShowWin(&rUIMng.m_SysMenuWin);
		}
	}
	if (RECEIVE_LOG_IN_SUCCESS == CurrentProtocolState)
	{
		g_ErrorReport.Write( "> Request Character list\r\n");

		CCameraMove::GetInstancePtr()->SetTourMode(FALSE);

		SceneFlag = CHARACTER_SCENE;

		#ifdef NEW_PROTOCOL_SYSTEM
			gProtocolSend.SendRequestCharactersListNew();
		#else
			SendRequestCharactersList(g_pMultiLanguage->GetLanguage());
		#endif

        ReleaseLogoSceneData();

		ClearCharacters();
	}

	g_ConsoleDebug->UpdateMainScene();
}

bool NewRenderLogInScene(HDC hDC)
{
	if(!InitLogIn) return false;


	Console.Write(1, "CurrentProtocolState: %d / %d", CurrentProtocolState, InitLogIn);

	FogEnable = false;
// 	extern GLfloat FogColor[4];
// 	FogColor[0] = 178.f/256.f; FogColor[1] = 178.f/256.f; FogColor[2] = 178.f/256.f; FogColor[3] = 0.f;
// 	glFogf(GL_FOG_START, 3700.0f);
// 	glFogf(GL_FOG_END, 4000.0f);

#ifdef MOVIE_DIRECTSHOW
	if(CUIMng::Instance().IsMoving() == true)
	{
		g_pMovieScene->PlayMovie();

		if(g_pMovieScene->IsEndMovie())
		{
			g_pMovieScene->Destroy();
			SAFE_DELETE(g_pMovieScene);
			CUIMng::Instance().SetMoving(false);
			::PlayMp3(g_lpszMp3[MUSIC_MAIN_THEME]);
		}
		else
		{
			if(HIBYTE(GetAsyncKeyState(VK_ESCAPE))==128 || HIBYTE(GetAsyncKeyState(VK_RETURN))==128)
			{
				g_pMovieScene->Destroy();
				SAFE_DELETE(g_pMovieScene);
				CUIMng::Instance().SetMoving(false);
				::PlayMp3(g_lpszMp3[MUSIC_MAIN_THEME]);
			}
		}
		return true;
	}
#endif // MOVIE_DIRECTSHOW

	vec3_t pos;
	if(CCameraMove::GetInstancePtr()->IsCameraMove()) 
	{
		VectorCopy(CameraPosition, pos);
	}

	GWidescreen.SceneLogin();

	int Width,Height;

	SetLegacyColor3f(1.f,1.f,1.f);
#ifndef PJH_NEW_SERVER_SELECT_MAP
	BeginBitmap();
	Width = 320;
	Height = 320;
	RenderBitmap(BITMAP_LOG_IN+9,  (float)0,(float)25,(float)Width,(float)Height,0.f,0.f);
	RenderBitmap(BITMAP_LOG_IN+10,(float)320,(float)25,(float)Width,(float)Height,0.f,0.f);
	EndBitmap();
#endif //PJH_NEW_SERVER_SELECT_MAP

	Height = 480;
	// Ultimo bloco 100% invisivel do cliente: a tela de login nao tinha uma unica
	// ScopedRenderPhase, entao seu cpu_us era integralmente us_unmeasured. Mesmas
	// fases de MainScene, pelo mesmo motivo da cena de personagem — comparabilidade.
	{
		ScopedRenderPhase phase(RenderPhaseFrameBegin);
		Width = FrameBeginOpengl();
		glClearColor(0.f,0.f,0.f,1.f);
	}

	{
		ScopedRenderPhase phase(RenderPhaseSetup);
		BeginOpengl(0, 0, GetWindowsX, GetWindowsY);
	}
	{
		ScopedRenderPhase phase(RenderPhaseFrustum);
		CreateFrustrum((float)Width/(float)640, pos);
	}

	if (!CUIMng::Instance().m_CreditWin.IsShow())
	{
		CameraViewFar = 330.f * CCameraMove::GetInstancePtr()->GetCurrentCameraDistanceLevel();
#ifndef PJH_NEW_SERVER_SELECT_MAP
		{
			ScopedRenderPhase phase(RenderPhaseSetup);
			BeginOpengl();
		}
#endif //PJH_NEW_SERVER_SELECT_MAP
		{
			ScopedRenderPhase phase(RenderPhaseTerrain);
			RenderTerrain(false);
		}
		CameraViewFar = 7000.f;
		{
			ScopedRenderPhase phase(RenderPhaseCharacters);
			RenderCharactersClient();
		}
		{
			ScopedRenderPhase phase(RenderPhaseMisc);
			RenderBugs();
		}
		{
			ScopedRenderPhase phase(RenderPhaseObjects);
			RenderObjects();
		}
		{
			ScopedRenderPhase phase(RenderPhaseEffects);
			RenderJoints();
			RenderEffects();
		}
		{
			ScopedRenderPhase phase(RenderPhaseSprites);
			CheckSprites();
		}
		{
			ScopedRenderPhase phase(RenderPhaseMisc);
			RenderLeaves();
			RenderBoids();
		}
		{
			ScopedRenderPhase phase(RenderPhaseObjects);
			RenderObjects_AfterCharacter();
		}
		{
			ScopedRenderPhase phase(RenderPhaseMisc);
			ThePetProcess().RenderPets();
		}
	}

	{
		ScopedRenderPhase phase(RenderPhaseSprites);
		BeginSprite();
		RenderSprites();
		RenderParticles();
		EndSprite();
	}
	// Toda a cauda 2D da tela de login -- logo, tour mode, campos de entrada,
	// janelas. Sao centenas de linhas que ficavam fora de qualquer fase, entao
	// us_ui lia 0 aqui e o custo caia inteiro em us_unmeasured.
	ScopedRenderPhase uiPhase(RenderPhaseUi);
	BeginBitmap();

	if (CCameraMove::GetInstancePtr()->IsTourMode())
	{
#ifndef PJH_NEW_SERVER_SELECT_MAP
		// È­¸é Èå¸®±â
		EnableAlphaBlend4();
		SetLegacyColor4f(0.7f,0.7f,0.7f,1.0f);
		float fScale = (sinf(WorldTime*0.0005f) + 1.f) * 0.00011f;
		//RenderBitmap(BITMAP_CHROME+3, 0.0f,0.0f, 640.0f,480.0f, 800.f,600.f, (800.f)/1024.f,(600.f)/1024.f);
		RenderBitmap(BITMAP_CHROME+3, 0.0f,0.0f, 640.0f,480.0f, 800.f*fScale,600.f*fScale, (800.f)/1024.f-800.f*fScale*2,(600.f)/1024.f-600.f*fScale*2);
		float fAngle = WorldTime * 0.00018f;
		float fLumi = 1.0f - (sinf(WorldTime*0.0015f) + 1.f) * 0.25f;
		SetLegacyColor4f(fLumi*0.3f,fLumi*0.3f,fLumi*0.7f,fLumi);
		fScale = (sinf(WorldTime*0.0015f) + 1.f) * 0.00021f;
		RenderBitmapLocalRotate(BITMAP_CHROME+4,320.0f,240.0f, 1150.0f, 1150.0f, fAngle, fScale*512.f,fScale*512.f, (512.f)/512.f-fScale*2*512.f,(512.f)/512.f-fScale*2*512.f);

		// À§¾Æ·¡ ÀÚ¸£±â
		EnableAlphaTest();
		SetLegacyColor4f(0.0f,0.0f,0.0f,1.0f);
		RenderColor(0, 0, 640, 25);
		RenderColor(0, 480-25, 640, 25);

		// È­¸éÄ¥
		SetLegacyColor4f(0.0f,0.0f,0.0f,0.2f);
		RenderColor(0, 25, 640, 430);
#endif //PJH_NEW_SERVER_SELECT_MAP
		// ¹Â·Î°í
		g_fMULogoAlpha += 0.02f;
		if (g_fMULogoAlpha > 10.0f) g_fMULogoAlpha = 10.0f;
		
		EnableAlphaBlend();
		SetLegacyColor4f(g_fMULogoAlpha-0.3f,g_fMULogoAlpha-0.3f,g_fMULogoAlpha-0.3f,g_fMULogoAlpha-0.3f);
#ifdef PBG_ADD_MUBLUE_LOGO
		BITMAP_t *pImage =NULL;
		pImage = &Bitmaps[BITMAP_LOG_IN+17];
		RenderBitmap(BITMAP_LOG_IN+17, 320.0f-432*0.4f*0.5f,25.0f, 432*0.4f,384*0.4f,0,0,(432-0.5f)/pImage->Width,(384-0.5f)/pImage->Height);
#else //PBG_ADD_MUBLUE_LOGO
		RenderBitmap(BITMAP_LOG_IN+17, setPosMidRight(320.0f-128.0f*0.8f),25.0f, 256.0f*0.8f,128.0f*0.8f);
#endif //PBG_ADD_MUBLUE_LOGO
		EnableAlphaTest();
		SetLegacyColor4f(g_fMULogoAlpha,g_fMULogoAlpha,g_fMULogoAlpha,g_fMULogoAlpha);
#ifdef PBG_ADD_MUBLUE_LOGO
		pImage = &Bitmaps[BITMAP_LOG_IN+16];
		RenderBitmap(BITMAP_LOG_IN+16, 320.0f-432*0.4f*0.5f,25.0f, 432*0.4f,384*0.4f,0,0,432/pImage->Width,384/pImage->Height);
#else //PBG_ADD_MUBLUE_LOGO
		RenderBitmap(BITMAP_LOG_IN+16, setPosMidRight(320.0f-128.0f*0.8f),25.0f, 256.0f*0.8f,128.0f*0.8f);
#endif //PBG_ADD_MUBLUE_LOGO
	}

	SIZE Size;
	char Text[100];

	int Y = 0;
	
	//	- Caixa de criação de conta (dev)

	g_pRenderText->SetFont(g_hFont);

	InputTextWidth = 256;
	SetLegacyColor3f(0.8f,0.7f,0.6f);
	g_pRenderText->SetTextColor(255, 255, 255, 255);
	g_pRenderText->SetBgColor(0, 0, 0, 128);
	
	if (m_Resolution > 1)
	{
		sprintf(Text, "%s%s", GlobalText[454], GlobalText[455]);
	}
	else
	{
		sprintf(Text, "%s", GlobalText[454]);
	}
	g_pMultiLanguage->_GetTextExtentPoint32(g_pRenderText->GetFontDC(), Text, lstrlen(Text), &Size);
	g_pRenderText->RenderText(setPosMidRight(335) - Size.cx * 640 / WindowWidth, GetWindowsY - Size.cy * 640 / WindowWidth - 1, Text);

	if (m_Resolution < 2)
	{
		strcpy(Text, GlobalText[455]);

		g_pMultiLanguage->_GetTextExtentPoint32(g_pRenderText->GetFontDC(), Text, lstrlen(Text), &Size);
		g_pRenderText->RenderText(setPosMidRight(335), GetWindowsY - Size.cy * 640 / WindowWidth - 1, Text);
	}

	sprintf(Text,GlobalText[456],m_ExeVersion);
	
	g_pMultiLanguage->_GetTextExtentPoint32(g_pRenderText->GetFontDC(), Text, lstrlen(Text), &Size);
	g_pRenderText->RenderText(0, GetWindowsY - Size.cy * 640 / WindowWidth - 1, Text);

    RenderInfomation();
	
#ifdef ENABLE_EDIT
	RenderDebugWindow();
#endif //ENABLE_EDIT

	EndBitmap();

	EndOpengl();

	return true;
}

void RenderInterfaceEdge()
{
	int Width,Height;
	int WindowX,WindowY;
   	EnableAlphaTest();
	SetLegacyColor3f(1.f,1.f,1.f);
	//interface edge
	Width = 192;Height = 37;WindowX = 448;WindowY = 0;
	RenderBitmap(BITMAP_LOG_IN,(float)WindowX,(float)WindowY,(float)Width,(float)Height,0.f,0.f,Width/256.f,Height/64.f);
	Width = 192;Height = 37;WindowX = 0;WindowY = 0;
	RenderBitmap(BITMAP_LOG_IN,(float)WindowX,(float)WindowY,(float)Width,(float)Height,Width/256.f,0.f,-Width/256.f,Height/64.f);
	Width = 106;Height = 256;WindowX = 534;WindowY = 3;
	RenderBitmap(BITMAP_LOG_IN+1,(float)WindowX,(float)WindowY,(float)Width,(float)Height,0.f,0.f,Width/128.f,Height/256.f);
	Width = 106;Height = 256;WindowX = 0;WindowY = 3;
	RenderBitmap(BITMAP_LOG_IN+1,(float)WindowX,(float)WindowY,(float)Width,(float)Height,Width/128.f,0.f,-Width/128.f,Height/256.f);
	Width = 106;Height = 222;WindowX = 534;WindowY = 259;
	RenderBitmap(BITMAP_LOG_IN+2,(float)WindowX,(float)WindowY,(float)Width,(float)Height,0.f,0.f,Width/128.f,Height/256.f);
	Width = 106;Height = 222;WindowX = 0;WindowY = 259;
	RenderBitmap(BITMAP_LOG_IN+2,(float)WindowX,(float)WindowY,(float)Width,(float)Height,Width/128.f,0.f,-Width/128.f,Height/256.f);
	Width = 256;Height = 70;WindowX = 192;WindowY = 0;
	RenderBitmap(BITMAP_LOG_IN+3,(float)WindowX,(float)WindowY,(float)Width,(float)Height,0.f,0.f,Width/256.f,Height/128.f);
}

void LoadingScene(HDC hDC)
{
	g_ConsoleDebug->Write(MCD_NORMAL, "LoadingScene_Start");

	CUIMng& rUIMng = CUIMng::Instance();
	if (!InitLoading)
	{
		LoadingWorld = 9999999;

		InitLoading = true;
		
		LoadBitmap("Interface\\LSBg01.JPG", BITMAP_TITLE, GL_LINEAR);
		LoadBitmap("Interface\\LSBg02.JPG", BITMAP_TITLE+1, GL_LINEAR);
		LoadBitmap("Interface\\LSBg03.JPG", BITMAP_TITLE+2, GL_LINEAR);
		LoadBitmap("Interface\\LSBg04.JPG", BITMAP_TITLE+3, GL_LINEAR);

		::StopMp3(g_lpszMp3[MUSIC_LOGIN_THEME]);

		rUIMng.m_pLoadingScene = new CLoadingScene;
		rUIMng.m_pLoadingScene->Create();
	}

    FogEnable = false;
	::BeginOpengl();
	::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	::BeginBitmap();

	rUIMng.m_pLoadingScene->Render();

	::EndBitmap();
	::EndOpengl();
	::SwapBuffers(hDC);

	SAFE_DELETE(rUIMng.m_pLoadingScene);

	SceneFlag = MAIN_SCENE;
	for (int i = 0; i < 4; ++i)
		::DeleteBitmap(BITMAP_TITLE+i);

	::ClearInput();

	g_ConsoleDebug->Write(MCD_NORMAL, "LoadingScene_End");
}

float CameraDistanceTarget = 1000.f;
float CameraDistance = CameraDistanceTarget;

bool MoveMainCamera()
{
    bool bLockCamera = false;

	if (
#ifdef PJH_NEW_SERVER_SELECT_MAP
		gMapManager.WorldActive == 94
#else
		gMapManager.WorldActive == WD_77NEW_LOGIN_SCENE
#endif //PJH_NEW_SERVER_SELECT_MAP
		&& CCameraMove::GetInstancePtr()->IsTourMode())
	{
#ifdef PJH_NEW_SERVER_SELECT_MAP
		CameraFOV = 65.0f;
#else //PJH_NEW_SERVER_SELECT_MAP
		CameraFOV = 61.0f;
#endif //PJH_NEW_SERVER_SELECT_MAP
	}
	else
	{
		if (gMapManager.WorldActive == WD_74NEW_CHARACTER_SCENE || gMapManager.WorldActive == WD_73NEW_LOGIN_SCENE) // Fix for opening scene and character selection
		{
			CameraZoom = 0;
			AngleY3D = 0;
		}
		CameraFOV = 37.f + CameraZoom; //3D CAMERA

		if (!g_pUIManager->IsInputEnable())
		{
			gCamera.Update();
		}
	}

#ifdef ENABLE_EDIT2
	{
		bool EditMove = false;
		if( !g_pUIManager->IsInputEnable() )
		{
			if(HIBYTE(GetAsyncKeyState(VK_INSERT))==128)
				CameraAngle[2] += 15;
			if(HIBYTE(GetAsyncKeyState(VK_DELETE))==128)
				CameraAngle[2] -= 15;

			vec3_t p1,p2;
			Vector(0.f,0.f,0.f,p1);
			FLOAT Velocity = sqrtf(TERRAIN_SCALE*TERRAIN_SCALE)*1.25f;

			if(HIBYTE(GetAsyncKeyState(VK_LEFT ))==128)// || (MouseX<=0 && MouseY>=100))
			{
				Vector(-Velocity, -Velocity, 0.f, p1);
				EditMove = true;
			}
			if(HIBYTE(GetAsyncKeyState(VK_RIGHT))==128)// || (MouseX>=639 && MouseY>=100))
			{
				Vector(Velocity, Velocity, 0.f, p1);
				EditMove = true;
			}
			if(HIBYTE(GetAsyncKeyState(VK_UP   ))==128)// || (MouseY<=0 && MouseX>=100 && MouseX<540))
			{
				Vector(-Velocity, Velocity, 0.f, p1);
				EditMove = true;
			}
			if(HIBYTE(GetAsyncKeyState(VK_DOWN ))==128)// || (MouseY>=479))
			{
				Vector(Velocity, -Velocity, 0.f, p1);
				EditMove = true;
			}

			glPushMatrix();
			glLoadIdentity();
			glRotatef(-CameraAngle[2],0.f,0.f,1.f);
			float Matrix[3][4];
			GetOpenGLMatrix(Matrix);
			glPopMatrix();
			VectorRotate(p1,Matrix,p2);
			VectorAdd(Hero->Object.Position, p2, Hero->Object.Position);
		}

        if ( gMapManager.InChaosCastle()==false || !Hero->Object.m_bActionStart	)
        {
			if(gMapManager.WorldActive == WD_39KANTURU_3RD && Hero->Object.m_bActionStart)
			{}
			else
            if ( gMapManager.WorldActive==-1 || Hero->Helper.Type != MODEL_HELPER+3 || Hero->SafeZone )
            {
				Hero->Object.Position[2] = RequestTerrainHeight(Hero->Object.Position[0],Hero->Object.Position[1]);
            }
            else
            {
                if ( gMapManager.WorldActive==WD_8TARKAN || gMapManager.WorldActive==WD_10HEAVEN )
                    Hero->Object.Position[2] = RequestTerrainHeight(Hero->Object.Position[0],Hero->Object.Position[1])+90.f;
                else
                    Hero->Object.Position[2] = RequestTerrainHeight(Hero->Object.Position[0],Hero->Object.Position[1])+30.f;
            }
        }

		if(EditMove)
		{
			BYTE PathX[1];
			BYTE PathY[1];
			PathX[0] = (BYTE)(Hero->Object.Position[0]/TERRAIN_SCALE);
			PathY[0] = (BYTE)(Hero->Object.Position[1]/TERRAIN_SCALE);

			#ifdef NEW_PROTOCOL_SYSTEM
				gProtocolSend.SendCharacterMoveNew(Hero->Key,Hero->Object.Angle[2],1,PathX,PathY,PathX[0],PathY[0]);
			#else
				SendCharacterMove(Hero->Key,Hero->Object.Angle[2],1,PathX,PathY,PathX[0],PathY[0]);
			#endif

            Hero->Path.PathNum = 0;
		}
	}
#endif //ENABLE_EDIT2

	CameraAngle[0] = 0.f;
	CameraAngle[1] = 0.f;

	if(CameraTopViewEnable)
	{
		CameraViewFar = 3200.f;
		//CameraViewFar = 60000.f;
		CameraPosition[0] = Hero->Object.Position[0];
		CameraPosition[1] = Hero->Object.Position[1];
		CameraPosition[2] = CameraViewFar;
	}
	else
	{
		int iIndex = TERRAIN_INDEX((Hero->PositionX),(Hero->PositionY));
		vec3_t Position,TransformPosition;
		float Matrix[3][4];

        if ( battleCastle::InBattleCastle2( Hero->Object.Position ) )
        {
            CameraViewFar = 3000.f;
        }
        else if ( gMapManager.InBattleCastle() && SceneFlag == MAIN_SCENE)
        {
            CameraViewFar = 2500.f;
        }
		else if (gMapManager.WorldActive == WD_51HOME_6TH_CHAR)
		{
			CameraViewFar = 2800.f * 1.15f;
		}
 		else if(gMapManager.IsPKField()	|| IsDoppelGanger2())
		{
			CameraViewFar = 3700.0f;
 		}
        else
        {
            switch ( g_shCameraLevel )
            {
            case 0:
				if(SceneFlag == LOG_IN_SCENE)
				{
				}
				else if(SceneFlag == CHARACTER_SCENE)
				{
#ifdef PJH_NEW_SERVER_SELECT_MAP
					CameraViewFar = 3500.f;
#else //PJH_NEW_SERVER_SELECT_MAP
					CameraViewFar = 10000.f;
#endif //PJH_NEW_SERVER_SELECT_MAP
				}
				else if (g_Direction.m_CKanturu.IsMayaScene())
				{
					CameraViewFar = 2000.f * 10.0f * 0.115f;
				}
				else
				{
					CameraViewFar = 2000.f; 
				}
				break;
            case 1: CameraViewFar = 2500.f; break;
            case 2: CameraViewFar = 2600.f; break;
            case 3: CameraViewFar = 2950.f; break;
			case 5:
            case 4: CameraViewFar = 3200.f; break;
            }
        }

		Vector(0.f,-CameraDistance,0.f,Position);//-750
		AngleMatrix(CameraAngle,Matrix);
		VectorIRotate(Position,Matrix,TransformPosition);

		if(SceneFlag == MAIN_SCENE)
		{
			g_pCatapultWindow->GetCameraPos(Position);
		}
		else if (CCameraMove::GetInstancePtr()->IsTourMode())
		{
			CCameraMove::GetInstancePtr()->UpdateTourWayPoint();
			CCameraMove::GetInstancePtr()->GetCurrentCameraPos(Position);
			CameraViewFar = 390.f * CCameraMove::GetInstancePtr()->GetCurrentCameraDistanceLevel();
		}

		if(g_Direction.IsDirection() && !g_Direction.m_bDownHero)
		{
			Hero->Object.Position[2] = 300.0f;
			g_shCameraLevel = g_Direction.GetCameraPosition(Position);
		}
 		else if(gMapManager.IsPKField()	|| IsDoppelGanger2())
		{
 			g_shCameraLevel =5;
 		}
		else if (IsDoppelGanger1())
		{
 			g_shCameraLevel =5;
		}
		else g_shCameraLevel =0;

#ifdef PJH_NEW_SERVER_SELECT_MAP
		if(CCameraMove::GetInstancePtr()->IsTourMode())
		{
			vec3_t temp = {0.0f,0.0f,-100.0f};
			VectorAdd(TransformPosition, temp, TransformPosition);
		}
#endif //PJH_NEW_SERVER_SELECT_MAP

		VectorAdd ( Position,TransformPosition,CameraPosition);

        if ( gMapManager.InBattleCastle()==true )
        {
            CameraPosition[2] = 255.f;//700
        }
		else if (CCameraMove::GetInstancePtr()->IsTourMode());
        else
        {
            CameraPosition[2] = Hero->Object.Position[2];//700
        }
		
		if ( (TerrainWall[iIndex]&TW_HEIGHT)==TW_HEIGHT )
		{
			CameraPosition[2] = g_fSpecialHeight = 1200.f+1;
		}            
		CameraPosition[2] += CameraDistance-150.f;//700

		if (CCameraMove::GetInstancePtr()->IsTourMode())
		{
#ifdef PJH_NEW_SERVER_SELECT_MAP
			CCameraMove::GetInstancePtr()->SetAngleFrustum(-112.5f);
			CameraAngle[0] = CCameraMove::GetInstancePtr()->GetAngleFrustum();
#else	// PJH_NEW_SERVER_SELECT_MAP
			CameraAngle[0] = -78.5f;
#endif	// PJH_NEW_SERVER_SELECT_MAP
			CameraAngle[1] = 0.0f;
			CameraAngle[2] = CCameraMove::GetInstancePtr()->GetCameraAngle();
		}
		else if(SceneFlag == CHARACTER_SCENE)
		{
#ifdef PJH_NEW_SERVER_SELECT_MAP
			CameraAngle[0] = -84.5f;
			CameraAngle[1] = 0.0f;
			CameraAngle[2] = -75.0f;
 			CameraPosition[0] = 9758.93f;
 			CameraPosition[1] = 18913.11f;
 			CameraPosition[2] = 675.5f;
#else //PJH_NEW_SERVER_SELECT_MAP
			CameraAngle[0] = -84.5f;
			CameraAngle[1] = 0.0f;
			CameraAngle[2] = -30.0f;
			CameraPosition[0] = 23566.75f;
			CameraPosition[1] = 14085.51f;
			CameraPosition[2] = 395.0f;
#endif //PJH_NEW_SERVER_SELECT_MAP
		}
		else
		{
			CameraAngle[0] = -48.5f;
			CameraAngle[0] += AngleY3D;
			CameraPosition[2] += AngleZ3D;
		}

		CameraAngle[0] += EarthQuake;

        if ( ( TerrainWall[iIndex]&TW_CAMERA_UP )==TW_CAMERA_UP )
        {
            if ( g_fCameraCustomDistance<=CUSTOM_CAMERA_DISTANCE1 )
            {
                g_fCameraCustomDistance+=10;
            }
        }
        else
        {
            if ( g_fCameraCustomDistance>0 )
            {
                g_fCameraCustomDistance-=10;
            }
        }

        if ( g_fCameraCustomDistance>0 )
        {
            vec3_t angle = { 0.f, 0.f, -45.f };
		    Vector ( 0.f, g_fCameraCustomDistance, 0.f, Position );
		    AngleMatrix ( angle, Matrix );
		    VectorIRotate ( Position, Matrix, TransformPosition );
		    VectorAdd ( CameraPosition, TransformPosition, CameraPosition );
        }
        else if ( g_fCameraCustomDistance<0 )
        {
            vec3_t angle = { 0.f, 0.f, -45.f };
		    Vector ( 0.f, g_fCameraCustomDistance, 0.f, Position );
		    AngleMatrix ( angle, Matrix );
		    VectorIRotate ( Position, Matrix, TransformPosition );
		    VectorAdd ( CameraPosition, TransformPosition, CameraPosition );
        }
	}
	if(gMapManager.WorldActive==5)
	{
		CameraAngle[0] += sinf(WorldTime*0.0005f)*2.f;
		CameraAngle[1] += sinf(WorldTime*0.0008f)*2.5f;
	}
    else if (CCameraMove::GetInstancePtr()->IsTourMode())
	{
		CameraDistanceTarget = 1100.f * CCameraMove::GetInstancePtr()->GetCurrentCameraDistanceLevel() * 0.1f;
		CameraDistance = CameraDistanceTarget;
	}
	else
    {
        if ( gMapManager.InBattleCastle() )
        {
            CameraDistanceTarget = 1100.f;
            CameraDistance = CameraDistanceTarget;
        }
        else
        {
            switch ( g_shCameraLevel )
            {
            case 0: CameraDistanceTarget = 1000.f; CameraDistance += (CameraDistanceTarget-CameraDistance)/3; break;
            case 1: CameraDistanceTarget = 1100.f; CameraDistance += (CameraDistanceTarget-CameraDistance)/3; break;
            case 2: CameraDistanceTarget = 1200.f; CameraDistance += (CameraDistanceTarget-CameraDistance)/3; break;
            case 3: CameraDistanceTarget = 1300.f; CameraDistance += (CameraDistanceTarget-CameraDistance)/3; break;
            case 4: CameraDistanceTarget = 1400.f; CameraDistance += (CameraDistanceTarget-CameraDistance)/3; break;
			case 5: CameraDistanceTarget = g_Direction.m_fCameraViewFar; CameraDistance += (CameraDistanceTarget-CameraDistance)/3; break;
            }
        }
    }
	// 3d camera
	// Field of vision

	if (CameraZoom > 0)
		CameraViewFar += 458.33f + (458.33f * CameraZoom);
	else
		CameraViewFar += 1458.33f;

    return bLockCamera;
}

void MoveMainScene()
{
	if(!InitMainScene)
	{
		g_pMainFrame->ResetSkillHotKey();
		
		g_ConsoleDebug->Write( MCD_NORMAL, "Join the game with the following character: %s", CharactersClient[SelectedHero].ID);

		g_ErrorReport.Write( "> Character selected <%d> \"%s\"\r\n", SelectedHero+1, CharactersClient[SelectedHero].ID);

        InitMainScene = true;
		
		g_ConsoleDebug->Write( MCD_SEND, "SendRequestJoinMapServer");

	    SendRequestJoinMapServer(CharactersClient[SelectedHero].ID);

		CUIMng::Instance().CreateMainScene();

		CameraAngle[2] = -45.f;

		ClearInput();
		InputEnable     = false;
		TabInputEnable  = false;
		InputTextWidth  = 256;
		InputTextMax[0] = 42;
		InputTextMax[1] = 10;
		InputNumber     = 2;
		for(int i=0;i<MAX_WHISPER;i++)
		{
			g_pChatListBox->AddText("", "", SEASON3B::TYPE_WHISPER_MESSAGE);
		}

		g_GuildNotice[0][0] = '\0';
		g_GuildNotice[1][0] = '\0';
	
		g_pPartyManager->Create();

		g_pChatListBox->ClearAll();

		g_pSlideHelpMgr->Init();		
		g_pUIMapName->Init();

		g_GuildCache.Reset();

		g_PortalMgr.Reset();

		ClearAllObjectBlurs();
		
		SetFocus(g_hWnd);

		g_ErrorReport.Write( "> Main Scene init success. ");
		g_ErrorReport.WriteCurrentTime();

		g_ConsoleDebug->Write(MCD_NORMAL, "MainScene Init Success");
	}
	
	if(CurrentProtocolState == RECEIVE_JOIN_MAP_SERVER)
	{
		EnableMainRender = true;
	}
	if(EnableMainRender == false)
	{
		return;
	}
	//init
	EarthQuake *= 0.2f;

	InitTerrainLight();

#ifdef NEW_MUHELPER_ON
	if (pAIController->IsRunning())
	{
		pAIController->WhatToDoNext();
	}
#endif

	CheckInventory = NULL;
	CheckSkill = -1;
	MouseOnWindow = false;


	if(!CameraTopViewEnable	&& LoadingWorld < 30 )
	{
		if(MouseY>=(int)(GetWindowsY - 51))
			MouseOnWindow = true;

		ScopedRenderPhase phase(RenderPhaseSimUi);
		g_pPartyManager->Update();
		g_pNewUISystem->Update();

		if (MouseLButton == true && false == g_pNewUISystem->CheckMouseUse() && g_dwMouseUseUIID == 0 && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHATINPUTBOX) == false )
		{
			g_pWindowMgr->SetWindowsEnable(FALSE);
			g_pFriendMenu->HideMenu();
			g_dwKeyFocusUIID = 0;
			if(GetFocus() != g_hWnd)
			{
				SaveIMEStatus();
				SetFocus(g_hWnd);
			}
		}
		MoveInterface();
		MoveTournamentInterface();
		if( ErrorMessage != MESSAGE_LOG_OUT )
			g_pUIManager->UpdateInput();
	}

	if(ErrorMessage != NULL)
		MouseOnWindow = true;

	{
		// Percorre os objetos de cenario do mundo. Suspeito principal do custo de
		// simulacao: e o laco mais largo daqui.
		ScopedRenderPhase phase(RenderPhaseSimObjects);
		MoveObjects();
	}
    if(!CameraTopViewEnable)
    	MoveItems();
	if ( ( gMapManager.WorldActive==WD_0LORENCIA && HeroTile!=4 ) || 
         ( gMapManager.WorldActive==WD_2DEVIAS && HeroTile!=3 && HeroTile<10 ) 
		 || gMapManager.WorldActive==WD_3NORIA 
		 || gMapManager.WorldActive==WD_7ATLANSE 
		 || gMapManager.InDevilSquare() == true
		 || gMapManager.WorldActive==WD_10HEAVEN 
         || gMapManager.InChaosCastle()==true 
         || gMapManager.InBattleCastle()==true
		 || M31HuntingGround::IsInHuntingGround()==true
		 || M33Aida::IsInAida()==true
		 || M34CryWolf1st::IsCyrWolf1st()==true
		|| gMapManager.WorldActive == WD_42CHANGEUP3RD_2ND
		|| IsIceCity()
		|| IsSantaTown()
		|| gMapManager.IsPKField()
		|| IsDoppelGanger2()
		|| gMapManager.IsEmpireGuardian1() 
		|| gMapManager.IsEmpireGuardian2()
		|| gMapManager.IsEmpireGuardian3()
		|| gMapManager.IsEmpireGuardian4()
		|| IsUnitedMarketPlace()
	 )
	{
        MoveLeaves();
	}
	
	MoveBoids();
   	MoveFishs();
	MoveBugs();
	MoveChat();
	UpdatePersonalShopTitleImp();
	{
		ScopedRenderPhase phase(RenderPhaseSimChars);
		MoveHero();
		MoveCharactersClient();
	}
	ThePetProcess().UpdatePets();
	{
		ScopedRenderPhase phase(RenderPhaseSimEffects);
		MovePoints();
		MovePlanes();
		MoveEffects();
		MoveJoints();
		MoveParticles();
		MovePointers();
	}

	g_Direction.CheckDirection();
    
#ifdef ENABLE_EDIT
    EditObjects();
#endif //ENABLE_EDIT

	g_GameCensorship->Update();

	g_ConsoleDebug->UpdateMainScene();
}

bool RenderMainScene()
{
	if(EnableMainRender == false)  
	{
		return false;
	}

    if(( LoadingWorld) > 30)
	{
		return false;
	}

    FogEnable = false;

    vec3_t pos;

    if(MoveMainCamera() == true)
    {
        VectorCopy ( Hero->Object.StartPosition, pos );
    }
    else
    {
		g_pCatapultWindow->GetCameraPos(pos);
		
		if(g_Direction.IsDirection() && g_Direction.m_bDownHero == false)
		{
			g_Direction.GetCameraPosition(pos);
		}
    }

	int Width,Height;

    BYTE byWaterMap = 0;

	if(CameraTopViewEnable == false)
	{
		Height = 480;	//480-48;
	}
	else
	{
		Height = 480;
	}

	// FrameBeginOpengl e a escolha de clear color. Barato em teoria, mas nunca foi
	// medido — e no PC o FrameBeginOpengl fala com o GL, entao pode bloquear.
	long long frameBeginStartUs = RenderStatsNowMicroseconds();
    Width = FrameBeginOpengl();
    if(gMapManager.WorldActive == WD_0LORENCIA)
	{
		glClearColor(10/256.f,20/256.f,14/256.f,1.f);
	}
    else if(gMapManager.WorldActive == WD_2DEVIAS)
	{
		glClearColor(0.f/256.f,0.f/256.f,10.f/256.f,1.f);
	}
    else if(gMapManager.WorldActive == WD_10HEAVEN)
	{
		glClearColor(3.f/256.f,25.f/256.f,44.f/256.f,1.f);
	}
    else if(gMapManager.InChaosCastle() == true)
	{
		glClearColor(0/256.f,0/256.f,0/256.f,1.f);
	}
	else if(gMapManager.WorldActive >= WD_45CURSEDTEMPLE_LV1 && gMapManager.WorldActive <= WD_45CURSEDTEMPLE_LV6) 
	{
		glClearColor(9.f/256.f,8.f/256.f,33.f/256.f,1.f);
	}
    else if(gMapManager.InHellas() == true)
    {
        byWaterMap = 1;
        glClearColor(0.f/256.f,0.f/256.f,0.f/256.f,1.f);
    }
    else    
	{
		glClearColor(0/256.f,0/256.f,0/256.f,1.f);
	}

	g_renderPhaseUs[RenderPhaseFrameBegin] += RenderStatsNowMicroseconds() - frameBeginStartUs;

	{
		ScopedRenderPhase phase(RenderPhaseSetup);
		BeginOpengl(0, 0, (m_Resolution > 2 ? GetWindowsX : Width), GetWindowsY);
	}
	{
		ScopedRenderPhase phase(RenderPhaseFrustum);
		CreateFrustrum((float)Width/(float)640, pos);
	}

    if ( gMapManager.InBattleCastle() )
    {
        if ( battleCastle::InBattleCastle2( Hero->Object.Position ) )
        {
            vec3_t Color = { 0.f, 0.f, 0.f };
            battleCastle::StartFog ( Color );
        }
        else
        {
            glDisable ( GL_FOG );
        }
    }

	CreateScreenVector(MouseX,MouseY,MouseTarget);

    if ( IsWaterTerrain()==false )
    {
		if(gMapManager.WorldActive==WD_39KANTURU_3RD)
		{
			if(!g_Direction.m_CKanturu.IsMayaScene())
				RenderTerrain(false);
		}
		else
        if(gMapManager.WorldActive!=WD_10HEAVEN && gMapManager.WorldActive != -1)
        {
			if(gMapManager.IsPKField() || IsDoppelGanger2())
			{
				ScopedRenderPhase phase(RenderPhaseObjects);
				RenderObjects();
			}
			{
				ScopedRenderPhase phase(RenderPhaseTerrain);
				RenderTerrain(false);
			}
        }
    }

	if(!gMapManager.IsPKField()	&& !IsDoppelGanger2())
	{
		ScopedRenderPhase phase(RenderPhaseObjects);
		RenderObjects();
	}

	{
		ScopedRenderPhase phase(RenderPhaseEffects);
		RenderEffectShadows();
	}
	{
		// O RenderBoids(true) mais adiante ja contava em Misc; este nao contava em
		// nada. Um resto conhecido mas nao embrulhado faz us_unmeasured != 0 parecer
		// custo desconhecido.
		ScopedRenderPhase phase(RenderPhaseMisc);
		RenderBoids();
	}

	{
		ScopedRenderPhase phase(RenderPhaseCharacters);
		RenderCharactersClient();
	}

	if(EditFlag!=EDIT_NONE)
	{
		ScopedRenderPhase phase(RenderPhaseTerrain);
		RenderTerrain(true);
    }
	{
		ScopedRenderPhase phase(RenderPhaseMisc);
		if(!CameraTopViewEnable)
			RenderItems();

		RenderFishs();
		RenderBugs();
		RenderLeaves();

		if (!gMapManager.InChaosCastle())
			ThePetProcess().RenderPets();

		RenderBoids(true);
	}
	{
		ScopedRenderPhase phase(RenderPhaseObjects);
		RenderObjects_AfterCharacter();
	}

	{
		ScopedRenderPhase phase(RenderPhaseEffects);
		RenderJoints(byWaterMap);
		RenderEffects();
		RenderBlurs();
	}
	{
		ScopedRenderPhase phase(RenderPhaseSprites);
		CheckSprites();
		BeginSprite();
	}

	if ((gMapManager.WorldActive == WD_2DEVIAS && HeroTile != 3 && HeroTile < 10)
		|| IsIceCity()
		|| IsSantaTown()
		|| gMapManager.IsPKField()
		|| IsDoppelGanger2()
		|| gMapManager.IsEmpireGuardian1()
		|| gMapManager.IsEmpireGuardian2()
		|| gMapManager.IsEmpireGuardian3()
		|| gMapManager.IsEmpireGuardian4()
		|| IsUnitedMarketPlace()
		)
	{
		ScopedRenderPhase phase(RenderPhaseMisc);
		RenderLeaves();
	}

	{
		ScopedRenderPhase phase(RenderPhaseSprites);
		RenderSprites();
		RenderParticles();

		if ( IsWaterTerrain()==false )
		{
			RenderPoints ( byWaterMap );
		}
		EndSprite();
	}

	{
		ScopedRenderPhase phase(RenderPhaseEffects);
		RenderAfterEffects();
	}

    if(IsWaterTerrain() == true)
    {
		// Passe inteiro repetido: agua, joints, efeitos, blurs E todos os sprites
		// outra vez, entre um EndOpengl/BeginOpengl. So roda em mapa com agua, o que
		// explica por que world 0 e world 3 divergem tanto no tempo nao medido.
		ScopedRenderPhase phase(RenderPhaseWater);
        byWaterMap = 2;

		EndOpengl();
		if (gMapManager.InHellas(gMapManager.WorldActive))
		{
			BeginOpengl(0, 0, GetWindowsX, GetWindowsY);
		}
		else
		{
			BeginOpengl(0, 0, Width, Height);
		}
        RenderWaterTerrain();
        RenderJoints(byWaterMap );
        RenderEffects( true );
        RenderBlurs();
        CheckSprites();
        BeginSprite();

        if(gMapManager.WorldActive==WD_2DEVIAS && HeroTile!=3 && HeroTile<10)
            RenderLeaves();

		RenderSprites(byWaterMap);
		RenderParticles(byWaterMap);
        RenderPoints ( byWaterMap );

        EndSprite();
		EndOpengl();

		BeginOpengl( 0, 0, Width, Height );
    }

    if(gMapManager.InBattleCastle())
    {
        if(battleCastle::InBattleCastle2(Hero->Object.Position))
        {
            battleCastle::EndFog();
        }
    }

	{
		ScopedRenderPhase phase(RenderPhaseSelect);
		SelectObjects();
	}
	{
		// Todo o 2D: descricao de objeto, interface, party, NewUI, info e cursor.
		// Quatro pares BeginBitmap/EndBitmap, e cada BeginBitmap faz flush da fila
		// de opacos e troca projecao. Era o meu principal suspeito para os 76% da
		// v8; a v14 mostrou que o teto dele e ~640 us, mas ele nunca foi isolado.
		ScopedRenderPhase phase(RenderPhaseUi);
		BeginBitmap();
		RenderObjectDescription();

		if(CameraTopViewEnable == false)
		{
			RenderInterface(true);
		}
		RenderTournamentInterface();
		EndBitmap();

		g_pPartyManager->Render();
		g_pNewUISystem->Render();

		BeginBitmap();

		RenderInfomation();

#ifdef ENABLE_EDIT
		RenderDebugWindow();
#endif //ENABLE_EDIT

		EndBitmap();
		BeginBitmap();

		RenderCursor();

		EndBitmap();
		EndOpengl();
	}

	return true;
}

int TimeRemain = 40;
extern int ChatTime;
extern int WaterTextureNumber;

int TestTime = 0;
extern int  GrabScreen;

void MoveCharacter(CHARACTER *c,OBJECT *o);

int TimePrior = GetTickCount();

double target_fps = 60;
double ms_per_frame = 1000.0 / target_fps;

// O `targetFps = -1` que estava aqui sobrescrevia o proprio argumento, entao
// target_fps era -1 em qualquer chamada e ms_per_frame ficava -1000. Efeito:
// CheckRenderNextFrame() retornava sempre true e WaitForNextActivity() era
// codigo morto inalcancavel — o teto nunca existiu, nem o de 60. Rodar a 170 FPS
// num notebook derruba o FPS *sustentado* por throttling termico, entao o teto
// tem valor pratico. Convencao mantida: valor <= 0 significa ilimitado.
void SetTargetFps(double targetFps)
{
	target_fps = targetFps;
	ms_per_frame = (targetFps > 0.0) ? (1000.0 / targetFps) : -1.0;
}

double last_render_tick_count = 0;
double current_tick_count = 0;
double last_water_change = 0;

void UpdateSceneState()
{
	g_pNewKeyInput->ScanAsyncKeyState();

	// SendChat starts this legacy counter at 70.  The Winmain frame loop used
	// to advance it implicitly; the shared scene runner must do it explicitly
	// so a first message never blocks the rest of the session.
	if (ChatTime > 0)
		--ChatTime;

	g_dwMouseUseUIID = 0;

	switch (SceneFlag)
	{
	case LOG_IN_SCENE:
		NewMoveLogInScene();
		break;

	case CHARACTER_SCENE:
		NewMoveCharacterScene();
		break;

	case MAIN_SCENE:
		MoveMainScene();
		break;
	}

	MoveNotices();

	if (PressKey(VK_SNAPSHOT))
	{
		if (GrabEnable)
			GrabEnable = false;
		else
			GrabEnable = true;
	}


	
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(GrabFileName, gProtect->m_MainInfo.ScreenShotPath, st.wMonth, st.wDay, st.wHour, st.wMinute, GrabScreen);
	char Text[256];
	sprintf(Text, GlobalText[459], GrabFileName);
	char lpszTemp[64];
	wsprintf(lpszTemp, " [%s / %s]", g_ServerListManager->GetSelectServerName(), Hero->ID);
	strcat(Text, lpszTemp);
	int iCaptureMode = 1;
	
	if (GrabEnable)
	{
		SaveScreen();
	}

	if (GrabEnable && iCaptureMode == 1)
	{
		g_pChatListBox->AddText("", Text, SEASON3B::TYPE_SYSTEM_MESSAGE);
	}

	GrabEnable = false;
}

void MainScene(HDC hDC)
{
	if (LOG_IN_SCENE == SceneFlag || CHARACTER_SCENE == SceneFlag)
	{
		double dDeltaTick = g_pTimer->GetTimeElapsed();
		dDeltaTick = MIN(dDeltaTick, 200.0 * FPS_ANIMATION_FACTOR);
		//g_pTimer->ResetTimer();

		CInput::Instance().Update();
		CUIMng::Instance().Update(dDeltaTick);
	}

	constexpr int NumberOfWaterTextures = 32;
	constexpr double timePerFrame = 1000 / REFERENCE_FPS;
	auto time_since_last_render = current_tick_count - last_water_change;
	while (time_since_last_render > timePerFrame)
	{
		WaterTextureNumber++;
		WaterTextureNumber %= NumberOfWaterTextures;
		time_since_last_render -= timePerFrame;
		last_water_change = current_tick_count;
	}

	if (Destroy) 
{
		return;
	}
		
	// Fisica, gerencia de bitmaps e som 3D rodam dentro da janela de cpu_us e fora
	// de qualquer fase. Sao classificados como simulacao, nao como render: nenhum
	// deles emite geometria.
	{
		ScopedRenderPhase phase(RenderPhaseSimulation);
		g_PhysicsManager.Move(0.025f * FPS_ANIMATION_FACTOR);

		Bitmaps.Manage();

		Set3DSoundPosition();
	}

    if( gMapManager.WorldActive==WD_10HEAVEN )
    {
        glClearColor(3.f/256.f,25.f/256.f,44.f/256.f,1.f);
    }
#ifdef PJH_NEW_SERVER_SELECT_MAP
	else if (gMapManager.WorldActive == WD_73NEW_LOGIN_SCENE|| gMapManager.WorldActive == WD_74NEW_CHARACTER_SCENE)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
    }
#endif //PJH_NEW_SERVER_SELECT_MAP
    else if (gMapManager.InHellas(gMapManager.WorldActive))
    {
        glClearColor(30.f/256.f,40.f/256.f,40.f/256.f,1.f);
    }
    else if ( gMapManager.InChaosCastle()==true )
    {
        glClearColor ( 0.f, 0.f, 0.f, 1.f );
    }
    else if ( gMapManager.InBattleCastle() && battleCastle::InBattleCastle2( Hero->Object.Position ) )
    {
        glClearColor ( 0.f, 0.f, 0.f, 1.f );
    }
	else if ( gMapManager.WorldActive >= WD_45CURSEDTEMPLE_LV1 && gMapManager.WorldActive <= WD_45CURSEDTEMPLE_LV6) 
	{
		glClearColor(9.f/256.f,8.f/256.f,33.f/256.f,1.f);
	}
	else if ( gMapManager.WorldActive == WD_51HOME_6TH_CHAR 
#ifndef PJH_NEW_SERVER_SELECT_MAP
		|| World == WD_77NEW_LOGIN_SCENE
#endif //PJH_NEW_SERVER_SELECT_MAP
		)
	{
		glClearColor(178.f/256.f,178.f/256.f,178.f/256.f,1.f);
	}
#ifndef PJH_NEW_SERVER_SELECT_MAP
	else if(World == WD_78NEW_CHARACTER_SCENE)
	{
		glClearColor(0.f,0.f,0.f,1.f);
	}
#endif //PJH_NEW_SERVER_SELECT_MAP
	else if(gMapManager.WorldActive == WD_65DOPPLEGANGER1)
	{
		glClearColor(148.f/256.f,179.f/256.f,223.f/256.f,1.f);
	}
    else
    {
        glClearColor(0/256.f,0/256.f,0/256.f,1.f);
    }
		
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	int32_t DifTimer = 0;
	uint32_t LastTimeCurrent = TimePrior;
	TimePrior = GetTickCount();

	bool Success = false;

	try
	{
		if (SceneFlag == LOG_IN_SCENE)
		{
			Success = NewRenderLogInScene(hDC);
		}
		else if (SceneFlag == CHARACTER_SCENE)
		{
			Success = NewRenderCharacterScene(hDC);
		}
		else if (SceneFlag == MAIN_SCENE)
		{
			Success = RenderMainScene();
		}

		{
			ScopedRenderPhase phase(RenderPhaseEffects);
			g_PhysicsManager.Render();
		}

		//#if defined(_DEBUG) || defined(LDS_FOR_DEVELOPMENT_TESTMODE) || defined(LDS_UNFIXED_FIXEDFRAME_FORDEBUG)
		// A leitura ocorre antes de desenhar o overlay, portanto a amostra mostra
		// somente a cena. Ative com -renderstats junto de -glslrenderer.
		const bool showRenderStats = (::strstr(::GetCommandLineA(), "-renderstats") != NULL);
		const bool writeRenderStatsCsv = (::strstr(::GetCommandLineA(), "-renderstatscsv") != NULL);
		const Platform::LegacyRenderFrameStats renderStats = Platform::GetLegacyRenderFrameStats();
		if (writeRenderStatsCsv)
			CaptureRenderStatsCsv(renderStats, RenderStatsElapsedMicroseconds());
		// O overlay desenha DEPOIS da leitura de cpu_us, portanto cai no gap que a
		// v14 mediu entre 0,4 e 4,1 ms. Cada BeginBitmap faz flush da fila de
		// opacos e troca a projecao, entao nao e obviamente barato. Medido por
		// timestamp em vez de RAII para nao reindentar o bloco inteiro.
		const long long overlayStartUs = RenderStatsNowMicroseconds();
		BeginBitmap();
		unicode::t_char szDebugText[128];
		unicode::_sprintf(szDebugText, "FPS : %.1f Connected: %d", FPS, g_bGameServerConnected);
		unicode::t_char szMousePos[128];
		unicode::_sprintf(szMousePos, "MousePos : %d %d %d", MouseX, MouseY, MouseLButtonPush);
		g_pRenderText->SetFont(g_hFontBold);
		g_pRenderText->SetBgColor(0, 0, 0, 100);
		g_pRenderText->SetTextColor(255, 255, 255, 200);
		g_pRenderText->RenderText(120, 26, szDebugText);
		g_pRenderText->RenderText(120, 36, szMousePos);
		if (showRenderStats)
		{
			unicode::t_char szRenderStats[160];
			unicode::_sprintf(szRenderStats, "Render: draws %lu verts %lu vbo %lu KB (%lu sub %lu alloc) tex %lu/%lu KB",
				static_cast<unsigned long>(renderStats.drawCalls),
				static_cast<unsigned long>(renderStats.vertices),
				static_cast<unsigned long>(renderStats.vertexUploadBytes / 1024),
				static_cast<unsigned long>(renderStats.bufferSubDataCalls),
				static_cast<unsigned long>(renderStats.bufferDataCalls),
				static_cast<unsigned long>(renderStats.textureUploads),
				static_cast<unsigned long>(renderStats.textureUploadBytes / 1024));
			g_pRenderText->RenderText(120, 46, szRenderStats);
			unicode::_sprintf(szRenderStats, "State: tex %lu blend %lu depth %lu alpha %lu fog %lu prog %lu",
				static_cast<unsigned long>(renderStats.textureChanges),
				static_cast<unsigned long>(renderStats.blendStateChanges),
				static_cast<unsigned long>(renderStats.depthStateChanges),
				static_cast<unsigned long>(renderStats.alphaTestChanges),
				static_cast<unsigned long>(renderStats.fogChanges),
				static_cast<unsigned long>(renderStats.programChanges));
			g_pRenderText->RenderText(120, 56, szRenderStats);
			unicode::_sprintf(szRenderStats, "Flush: all %lu matrix %lu tex %lu blend %lu depth %lu alpha %lu fog %lu",
				static_cast<unsigned long>(renderStats.batchFlushes),
				static_cast<unsigned long>(renderStats.matrixFlushes),
				static_cast<unsigned long>(renderStats.textureFlushes),
				static_cast<unsigned long>(renderStats.blendFlushes),
				static_cast<unsigned long>(renderStats.depthFlushes),
				static_cast<unsigned long>(renderStats.alphaFlushes),
				static_cast<unsigned long>(renderStats.fogFlushes));
			g_pRenderText->RenderText(120, 66, szRenderStats);
		}
		g_pRenderText->SetFont(g_hFont);
		EndBitmap();
		g_pendingPhaseUs[RenderPhaseOverlay] += RenderStatsNowMicroseconds() - overlayStartUs;
		//#endif // defined(_DEBUG) || defined(LDS_FOR_DEVELOPMENT_TESTMODE) || defined(LDS_UNFIXED_FIXEDFRAME_FORDEBUG)

		if (Success)
		{
			// O ponto onde o driver bloqueia esperando a GPU. Se o frame e limitado
			// pela GPU, o custo aparece AQUI e em nenhuma das fases de cpu_us — era
			// exatamente o buraco que a v14 deixou aberto em world 3, onde o frame
			// cresceu 50% com cpu_us constante.
			const long long presentStartUs = RenderStatsNowMicroseconds();
			SwapBuffers(hDC);
			g_pendingPhaseUs[RenderPhasePresent] += RenderStatsNowMicroseconds() - presentStartUs;
		}

		if (EnableSocket && SceneFlag == MAIN_SCENE)
		{
#ifdef NEW_PROTOCOL_SYSTEM
			if (!gProtocolSend.CheckConnected())
#else
			if (SocketClient.GetSocket() == INVALID_SOCKET)
#endif
			{
				static BOOL s_bClosed = FALSE;
				if (!s_bClosed)
				{
					s_bClosed = TRUE;
					g_ErrorReport.Write("> Connection closed. ");
					g_ErrorReport.WriteCurrentTime();
					SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CServerLostMsgBoxLayout));
				}
			}
		}

		if (SceneFlag == MAIN_SCENE)
		{
			switch (gMapManager.WorldActive)
			{
			case WD_0LORENCIA:
				if (HeroTile == 4)
				{
					StopBuffer(SOUND_WIND01, true);
					StopBuffer(SOUND_RAIN01, true);
				}
				else
				{
					PlayBuffer(SOUND_WIND01, NULL, true);
					if (RainCurrent > 0)
						PlayBuffer(SOUND_RAIN01, NULL, true);
				}
				break;
			case WD_1DUNGEON:
				PlayBuffer(SOUND_DUNGEON01, NULL, true);
				break;
			case WD_2DEVIAS:
				if (HeroTile == 3 || HeroTile >= 10)
					StopBuffer(SOUND_WIND01, true);
				else
					PlayBuffer(SOUND_WIND01, NULL, true);
				break;
			case WD_3NORIA:
				PlayBuffer(SOUND_WIND01, NULL, true);
				if (rand() % 512 == 0)
					PlayBuffer(SOUND_FOREST01);
				break;
			case WD_4LOSTTOWER:
				PlayBuffer(SOUND_TOWER01, NULL, true);
				break;
			case WD_5UNKNOWN:
				//PlayBuffer(SOUND_BOSS01,NULL,true);
				break;
			case WD_7ATLANSE:
				PlayBuffer(SOUND_WATER01, NULL, true);
				break;
			case WD_8TARKAN:
				PlayBuffer(SOUND_DESERT01, NULL, true);
				break;
			case WD_10HEAVEN:
				PlayBuffer(SOUND_HEAVEN01, NULL, true);
				if ((rand() % 100) == 0)
				{
					//                PlayBuffer(SOUND_HEAVEN01);
				}
				else if ((rand() % 10) == 0)
				{
					//                PlayBuffer(SOUND_THUNDERS02);
				}
				break;
			case WD_58ICECITY_BOSS:
				PlayBuffer(SOUND_WIND01, NULL, true);
				break;
			case WD_79UNITEDMARKETPLACE:
			{
				PlayBuffer(SOUND_WIND01, NULL, true);
				PlayBuffer(SOUND_RAIN01, NULL, true);
			}
			break;
#ifdef ASG_ADD_MAP_KARUTAN
			case WD_80KARUTAN1:
				PlayBuffer(SOUND_KARUTAN_DESERT_ENV, NULL, true);
				break;
			case WD_81KARUTAN2:
				if (HeroTile == 12)
				{
					StopBuffer(SOUND_KARUTAN_DESERT_ENV, true);
					PlayBuffer(SOUND_KARUTAN_KARDAMAHAL_ENV, NULL, true);
				}
				else
				{
					StopBuffer(SOUND_KARUTAN_KARDAMAHAL_ENV, true);
					PlayBuffer(SOUND_KARUTAN_DESERT_ENV, NULL, true);
				}
				break;
#endif	// ASG_ADD_MAP_KARUTAN
			}
			if (gMapManager.WorldActive != WD_0LORENCIA && gMapManager.WorldActive != WD_2DEVIAS && gMapManager.WorldActive != WD_3NORIA && gMapManager.WorldActive != WD_58ICECITY_BOSS && gMapManager.WorldActive != WD_79UNITEDMARKETPLACE)
			{
				StopBuffer(SOUND_WIND01, true);
			}
			if (gMapManager.WorldActive != WD_0LORENCIA && gMapManager.InDevilSquare() == false && gMapManager.WorldActive != WD_79UNITEDMARKETPLACE)
			{
				StopBuffer(SOUND_RAIN01, true);
			}
			if (gMapManager.WorldActive != WD_1DUNGEON)
			{
				StopBuffer(SOUND_DUNGEON01, true);
			}
			if (gMapManager.WorldActive != WD_3NORIA)
			{
				StopBuffer(SOUND_FOREST01, true);
			}
			if (gMapManager.WorldActive != WD_4LOSTTOWER)
			{
				StopBuffer(SOUND_TOWER01, true);
			}
			if (gMapManager.WorldActive != WD_7ATLANSE)
			{
				StopBuffer(SOUND_WATER01, true);
			}
			if (gMapManager.WorldActive != WD_8TARKAN)
			{
				StopBuffer(SOUND_DESERT01, true);
			}
			if (gMapManager.WorldActive != WD_10HEAVEN)
			{
				StopBuffer(SOUND_HEAVEN01, true);
			}
			if (gMapManager.WorldActive != WD_51HOME_6TH_CHAR)
			{
				StopBuffer(SOUND_ELBELAND_VILLAGEPROTECTION01, true);
				StopBuffer(SOUND_ELBELAND_WATERFALLSMALL01, true);
				StopBuffer(SOUND_ELBELAND_WATERWAY01, true);
				StopBuffer(SOUND_ELBELAND_ENTERDEVIAS01, true);
				StopBuffer(SOUND_ELBELAND_WATERSMALL01, true);
				StopBuffer(SOUND_ELBELAND_RAVINE01, true);
				StopBuffer(SOUND_ELBELAND_ENTERATLANCE01, true);
			}
#ifdef ASG_ADD_MAP_KARUTAN
			if (!IsKarutanMap())
				StopBuffer(SOUND_KARUTAN_DESERT_ENV, true);
			if (World != WD_80KARUTAN1)
				StopBuffer(SOUND_KARUTAN_INSECT_ENV, true);
			if (World != WD_81KARUTAN2)
				StopBuffer(SOUND_KARUTAN_KARDAMAHAL_ENV, true);
#endif	// ASG_ADD_MAP_KARUTAN

			if (gMapManager.WorldActive == WD_0LORENCIA)
			{
				if (Hero->SafeZone)
				{
					if (HeroTile == 4)
						PlayMp3(g_lpszMp3[MUSIC_PUB]);
					else
						PlayMp3(g_lpszMp3[MUSIC_MAIN_THEME]);
				}
			}
			else
			{
				StopMp3(g_lpszMp3[MUSIC_PUB]);
				StopMp3(g_lpszMp3[MUSIC_MAIN_THEME]);
			}
			if (gMapManager.WorldActive == WD_2DEVIAS)
			{
				if (Hero->SafeZone)
				{
					if ((Hero->PositionX) >= 205 && (Hero->PositionX) <= 214 &&
						(Hero->PositionY) >= 13 && (Hero->PositionY) <= 31)
					{
						PlayMp3(g_lpszMp3[MUSIC_CHURCH]);
					}
					else
					{
						PlayMp3(g_lpszMp3[MUSIC_DEVIAS]);
					}
				}
			}
			else
			{
				StopMp3(g_lpszMp3[MUSIC_CHURCH]);
				StopMp3(g_lpszMp3[MUSIC_DEVIAS]);
			}
			if (gMapManager.WorldActive == WD_3NORIA)
			{
				if (Hero->SafeZone)
					PlayMp3(g_lpszMp3[MUSIC_NORIA]);
			}
			else
			{
				StopMp3(g_lpszMp3[MUSIC_NORIA]);
			}
			if (gMapManager.WorldActive == WD_1DUNGEON || gMapManager.WorldActive == WD_5UNKNOWN)
			{
				PlayMp3(g_lpszMp3[MUSIC_DUNGEON]);
			}
			else
			{
				StopMp3(g_lpszMp3[MUSIC_DUNGEON]);
			}

			if (gMapManager.WorldActive == WD_7ATLANSE) {
				PlayMp3(g_lpszMp3[MUSIC_ATLANS]);
			}
			else {
				StopMp3(g_lpszMp3[MUSIC_ATLANS]);
			}
			if (gMapManager.WorldActive == WD_10HEAVEN) {
				PlayMp3(g_lpszMp3[MUSIC_ICARUS]);
			}
			else {
				StopMp3(g_lpszMp3[MUSIC_ICARUS]);
			}
			if (gMapManager.WorldActive == WD_8TARKAN) {
				PlayMp3(g_lpszMp3[MUSIC_TARKAN]);
			}
			else {
				StopMp3(g_lpszMp3[MUSIC_TARKAN]);
			}
			if (gMapManager.WorldActive == WD_4LOSTTOWER) {
				PlayMp3(g_lpszMp3[MUSIC_LOSTTOWER_A]);
			}
			else {
				StopMp3(g_lpszMp3[MUSIC_LOSTTOWER_A]);
			}

			if (gMapManager.InHellas(gMapManager.WorldActive)) {
				PlayMp3(g_lpszMp3[MUSIC_KALIMA]);
			}
			else {
				StopMp3(g_lpszMp3[MUSIC_KALIMA]);
			}

			if (gMapManager.WorldActive == WD_31HUNTING_GROUND) {
				PlayMp3(g_lpszMp3[MUSIC_BC_HUNTINGGROUND]);
			}
			else {
				StopMp3(g_lpszMp3[MUSIC_BC_HUNTINGGROUND]);
			}

			if (gMapManager.WorldActive == WD_33AIDA) {
				PlayMp3(g_lpszMp3[MUSIC_BC_ADIA]);
			}
			else {
				StopMp3(g_lpszMp3[MUSIC_BC_ADIA]);
			}

			M34CryWolf1st::ChangeBackGroundMusic(gMapManager.WorldActive);
			M39Kanturu3rd::ChangeBackGroundMusic(gMapManager.WorldActive);

			if (gMapManager.WorldActive == WD_37KANTURU_1ST)
				PlayMp3(g_lpszMp3[MUSIC_KANTURU_1ST]);
			else
				StopMp3(g_lpszMp3[MUSIC_KANTURU_1ST]);
			M38Kanturu2nd::PlayBGM();
			SEASON3A::CGM3rdChangeUp::Instance().PlayBGM();
			if (gMapManager.IsCursedTemple())
			{
				g_CursedTemple->PlayBGM();
			}
			if (gMapManager.WorldActive == WD_51HOME_6TH_CHAR) {
				PlayMp3(g_lpszMp3[MUSIC_ELBELAND]);
			}
			else {
				StopMp3(g_lpszMp3[MUSIC_ELBELAND]);
			}

			if (gMapManager.WorldActive == WD_56MAP_SWAMP_OF_QUIET) {
				PlayMp3(g_lpszMp3[MUSIC_SWAMP_OF_QUIET]);
			}
			else {
				StopMp3(g_lpszMp3[MUSIC_SWAMP_OF_QUIET]);
			}


			g_Raklion.PlayBGM();
			g_SantaTown.PlayBGM();
			g_PKField.PlayBGM();
			g_DoppelGanger1.PlayBGM();
			g_EmpireGuardian1.PlayBGM();
			g_EmpireGuardian2.PlayBGM();
			g_EmpireGuardian3.PlayBGM();
			g_EmpireGuardian4.PlayBGM();
			g_UnitedMarketPlace.PlayBGM();
#ifdef ASG_ADD_MAP_KARUTAN
			g_Karutan1.PlayBGM();
#endif	// ASG_ADD_MAP_KARUTAN
		}
	}
	catch (const std::exception&)
	{
	}
}

float g_Luminosity;

extern int g_iNoMouseTime;
extern GLvoid KillGLWindow(GLvoid);


void WaitForNextActivity(bool usePreciseSleep)
{
	// We only sleep when we have enough time to sleep and have some additional rest time.
	const auto current_frame_time_ms = current_tick_count - last_render_tick_count;
	const auto current_ms_per_frame = ms_per_frame;
	if (current_ms_per_frame > 0 && current_frame_time_ms > 0 && current_frame_time_ms < current_ms_per_frame)
	{
		const auto sleep_threshold_ms = usePreciseSleep ? 4.0 : 16.0;
		const auto sleep_duration_offset_ms = usePreciseSleep ? 1.0 : 4.0;
		const auto max_sleep_ms = 10.0;
		const auto rest_ms = current_ms_per_frame - current_frame_time_ms;

		if (rest_ms - sleep_duration_offset_ms > sleep_threshold_ms)
		{
			const auto sleep_ms = min(rest_ms - sleep_duration_offset_ms, max_sleep_ms);
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(sleep_ms)));
		}
		else
		{
			std::this_thread::yield();
		}
	}
	else
	{
		std::this_thread::yield();
	}
}

bool CheckRenderNextFrame()
{
	current_tick_count = g_pTimer->GetTimeElapsed();
	const auto current_frame_time_ms = current_tick_count - last_render_tick_count;

	if (current_frame_time_ms >= ms_per_frame)
	{
		return true;
	}

	return false;
}

// v17 acrescenta as colunas de Crowd LOD. Arquivo novo em vez de colunas extras no
// v16 pela mesma razao de sempre: RotateRenderCsvIfHeaderDiffers compara o header e
// rotaciona, e manter o nome v16 arquivaria dados de v17 como "...v16.oldN.csv".
// O caminho e o header sobem JUNTOS -- foi a dessincronia entre os dois que
// corrompeu a geracao v15.
static const char* const kRenderCsvPath = "RenderPerformance_v17.csv";

// O header vive numa constante porque ele e comparado com o do arquivo existente,
// nao apenas escrito. Manter as duas copias sincronizadas a mao foi como o v15
// se corrompeu.
static const char* const kRenderCsvHeader =
	"scene,world,resolution,backend,gpu_skinning,instancing,transform_cache,batching,mesh_cache,cpu_matrices,frames,cpu_us_avg,cpu_us_p95,draws_avg,vertices_avg,vbo_kb_avg,buffer_data_avg,buffer_sub_data_avg,flushes_avg,texture_uploads_avg,texture_kb_avg,texture_changes_avg,flush_matrix_avg,flush_texture_avg,flush_blend_avg,flush_depth_avg,flush_alpha_avg,flush_fog_avg,gpu_mesh_draws_avg,gpu_mesh_indices_avg,gpu_mesh_upload_kb_avg,bone_palette_kb_avg,cpu_skinning_vertices_avg,cpu_skinning_normals_avg,gpu_skinning_fallbacks_avg,gpu_skinning_material_fallbacks_avg,gpu_skinning_geometry_fallbacks_avg,gpu_skinning_resource_fallbacks_avg,instanced_draws_avg,instances_avg,instance_batches_avg,instance_batch_max,instance_palette_dedup_avg,mesh_cache_hits_avg,mesh_cache_misses_avg,mesh_vertices_resident,mesh_indices_resident,transforms_exec_avg,transforms_skipped_avg,animations_exec_avg,animations_skipped_avg,uniform_calls_saved_avg,us_terrain,us_objects,us_characters,us_effects,us_sprites,us_simulation,us_select,us_setup_gl,us_frustum,us_misc,us_water,us_ui,us_framebegin,us_unmeasured,us_overlay,us_present,us_protocol,us_pump,us_limiter,us_frame_gap,us_char_pose,us_char_shadow,us_char_parts,us_char_transform,us_char_mesh,us_char_draw,us_char_link,us_char_link_draw,us_sim_ui,us_sim_objects,us_sim_chars,us_sim_effects,us_sim_rest,phase_nesting,us_matrix_readback,matrix_readback_calls,cpu_matrix_divergence_rel,gpu_us,gpu_timer_state,render_scale,vsync,fps_limit,frame_total_us,frame_total_us_max,us_present_max,us_effects_max,effects_live_avg,effects_live_max,wheel_trails_avg,wheel_trails_max,wheel_trail_cap,crowd_lod,crowd_spawn,crowd_spawn_monsters,crowd_spawn_npcs,crowd_max_full,chars_live_avg,chars_visible_avg,chars_visible_max,chars_culled_frustum_avg,chars_beyond_far_avg,chars_lod_forced_avg,chars_players_avg,chars_monsters_avg,chars_npcs_avg,chars_other_avg,chars_lod0_avg,chars_lod1_avg,chars_lod2_avg,chars_lod3_avg,char_poses_avg,char_part_meshes_avg,char_shadows_avg,char_batch_accum_avg,char_batch_breaks_avg,char_batch_run_max,fps,fps_period,fps_min\n";

// O header so era escrito com o arquivo vazio, entao um build com conjunto de
// colunas diferente ANEXAVA linhas de outra largura sob o header antigo. Foi
// exatamente isso que corrompeu o v15: o arquivo terminou com linhas de 79 e de
// 90 campos misturadas, e todo leitor de CSV desalinhou as colunas em silencio --
// `vsync` lia 232, `fps_limit` lia 714. Rotacionar em vez de anexar remove a
// classe de bug: nenhuma largura se mistura e nenhum dado se perde.
// Retorna false quando o arquivo existente tem outro header E a rotacao NAO deu
// certo. Nesse caso o chamador precisa desistir de escrever: anexar assim mesmo
// recria a corrupcao exata que esta funcao existe para impedir. O caso real nao e
// hipotetico -- o fluxo de trabalho e "capturar, depois abrir o CSV", e com o
// arquivo aberto no Excel o rename falha com sharing violation.
static bool RotateRenderCsvIfHeaderDiffers()
{
	FILE* existing = fopen(kRenderCsvPath, "r");
	if (existing == NULL)
		return true; // nao existe: escrever cria o arquivo com o header certo
	char firstLine[4096];
	firstLine[0] = 0;
	const bool didRead = (fgets(firstLine, sizeof(firstLine), existing) != NULL);
	fclose(existing);
	if (didRead && strcmp(firstLine, kRenderCsvHeader) == 0)
		return true; // mesmo conjunto de colunas: anexar e seguro

	// Nome do arquivo rotacionado derivado de kRenderCsvPath, nao escrito de novo a
	// mao: duas copias da mesma string sincronizadas manualmente foi o mecanismo da
	// falha do v15, e repeti-lo aqui arquivaria dados de v17 como "...v16.oldN".
	char base[256];
	size_t size = strlen(kRenderCsvPath);
	if (size >= sizeof(base)) return false;
	memcpy(base, kRenderCsvPath, size + 1);
	if (size > 4 && strcmp(base + size - 4, ".csv") == 0)
		base[size - 4] = 0;

	char destination[256];
	for (int i = 0; i < 100; ++i)
	{
		sprintf(destination, "%s.old%d.csv", base, i);
		FILE* test = fopen(destination, "r");
		if (test != NULL)
		{
			fclose(test);
			continue;
		}
		return rename(kRenderCsvPath, destination) == 0;
	}
	return false; // 100 slots ocupados: melhor nao escrever que corromper
}

// Grava uma amostra agregada, e nao um registro por frame. O aquecimento evita
// que loading/recriacao de recursos contamine a comparacao entre cenas.
static void CaptureRenderStatsCsv(const Platform::LegacyRenderFrameStats& stats, DWORD renderCpuUs)
{
	struct CaptureState
	{
		CaptureState() : scene(-1), world(-1), width(0), height(0), glslBackend(false), warmup(0), frames(0), cpuTotal(0),
			draws(0), vertices(0), vboBytes(0), bufferData(0), bufferSubData(0), flushes(0), textureUploads(0),
			textureBytes(0), textureChanges(0), matrixFlushes(0), textureFlushes(0), blendFlushes(0), depthFlushes(0),
			alphaFlushes(0), fogFlushes(0), gpuMeshDraws(0), gpuMeshIndices(0), gpuMeshUploadBytes(0), bonePaletteBytes(0), cpuSkinningVertices(0), cpuSkinningNormals(0), gpuSkinningFallbacks(0), gpuSkinningMaterialFallbacks(0), gpuSkinningGeometryFallbacks(0), gpuSkinningResourceFallbacks(0),
			instancedDraws(0), instancesSubmitted(0), instanceBatches(0), largestInstanceBatch(0), instancePaletteDedupHits(0),
			meshCacheHits(0), meshCacheMisses(0), meshVerticesResident(0), meshIndicesResident(0),
			transformsExecuted(0), transformsSkipped(0), animationsExecuted(0), animationsSkipped(0), uniformCallsSaved(0)
		{
			for (int i = 0; i < RenderPhaseCount; ++i) phaseUs[i] = 0;
			matrixReadbackUs = 0;
			matrixReadbackCalls = 0;
			gpuUs = 0;
			frameTotalUs = 0;
			fpsTotal = 0.0;
			nestingViolations = 0;
			frameTotalMax = 0; presentMax = 0; effectsMax = 0;
			liveEffectsTotal = 0; liveEffectsMax = 0;
			wheelTrailsTotal = 0; wheelTrailsMax = 0;
			charsLive = 0; charsVisible = 0; charsVisibleMax = 0; charsCulledFrustum = 0;
			charsBeyondFar = 0; charsForced = 0;
			for (int i = 0; i < CrowdLod::LevelCount; ++i) charLevels[i] = 0;
			for (int i = 0; i < CrowdLod::KindCount; ++i)  charKinds[i]  = 0;
			charPoses = 0; charPartMeshes = 0; charShadows = 0;
			charBatchAccum = 0; charBatchBreaks = 0; charBatchRunMax = 0;
		}
		unsigned long long phaseUs[RenderPhaseCount];
		unsigned long long nestingViolations;
		// Media de 120 frames esconde mergulho transitorio: uma Twisting Shash que
		// derruba o FPS por 1 segundo dilui a quase nada. Os picos existem para
		// tornar o sintoma relatado mensuravel -- sem eles a captura nao consegue
		// nem confirmar que a queda aconteceu.
		unsigned long long frameTotalMax, presentMax, effectsMax;
		unsigned long long liveEffectsTotal, liveEffectsMax;
		unsigned long long wheelTrailsTotal, wheelTrailsMax;
		// Crowd LOD. `charsVisibleMax` e pico e nao soma: a multidao de pior caso e
		// o que decide o orcamento, e a media de 120 frames a esconde.
		unsigned long long charsLive, charsVisible, charsVisibleMax, charsCulledFrustum;
		unsigned long long charsBeyondFar, charsForced;
		unsigned long long charKinds[CrowdLod::KindCount];
		unsigned long long charLevels[CrowdLod::LevelCount];
		unsigned long long charPoses, charPartMeshes, charShadows;
		unsigned long long charBatchAccum, charBatchBreaks, charBatchRunMax;
		unsigned long long matrixReadbackUs;
		unsigned long long matrixReadbackCalls;
		unsigned long long gpuUs;
		unsigned long long frameTotalUs;
		double fpsTotal;
		int scene, world, width, height;
		bool glslBackend;
		unsigned int warmup, frames;
		unsigned long long cpuTotal, draws, vertices, vboBytes, bufferData, bufferSubData, flushes, textureUploads,
			textureBytes, textureChanges, matrixFlushes, textureFlushes, blendFlushes, depthFlushes, alphaFlushes, fogFlushes,
			gpuMeshDraws, gpuMeshIndices, gpuMeshUploadBytes, bonePaletteBytes, cpuSkinningVertices, cpuSkinningNormals, gpuSkinningFallbacks, gpuSkinningMaterialFallbacks, gpuSkinningGeometryFallbacks, gpuSkinningResourceFallbacks,
			instancedDraws, instancesSubmitted, instanceBatches, largestInstanceBatch, instancePaletteDedupHits,
			meshCacheHits, meshCacheMisses, meshVerticesResident, meshIndicesResident,
			transformsExecuted, transformsSkipped, animationsExecuted, animationsSkipped, uniformCallsSaved;
		DWORD cpuSamples[120];
	};
	static CaptureState state;

	const int scene = SceneFlag;
	const int world = gMapManager.WorldActive;
	const int width = static_cast<int>(WindowWidth);
	const int height = static_cast<int>(WindowHeight);
	const bool glslBackend = Platform::IsGlslLegacyBackendEnabled();
	if (state.scene != scene || state.world != world || state.width != width || state.height != height || state.glslBackend != glslBackend)
	{
		state = CaptureState();
		state.scene = scene; state.world = world; state.width = width; state.height = height; state.glslBackend = glslBackend;
	}

	if (state.warmup++ < 180)
		return;

	const unsigned int sample = state.frames++;
	state.cpuSamples[sample] = renderCpuUs;
	state.cpuTotal += renderCpuUs;
	state.draws += stats.drawCalls; state.vertices += stats.vertices; state.vboBytes += stats.vertexUploadBytes;
	state.bufferData += stats.bufferDataCalls; state.bufferSubData += stats.bufferSubDataCalls; state.flushes += stats.batchFlushes;
	state.textureUploads += stats.textureUploads; state.textureBytes += stats.textureUploadBytes; state.textureChanges += stats.textureChanges;
	state.matrixFlushes += stats.matrixFlushes; state.textureFlushes += stats.textureFlushes; state.blendFlushes += stats.blendFlushes;
	state.depthFlushes += stats.depthFlushes; state.alphaFlushes += stats.alphaFlushes; state.fogFlushes += stats.fogFlushes;
	state.gpuMeshDraws += stats.staticMeshDrawCalls; state.gpuMeshIndices += stats.staticMeshIndices;
	state.gpuMeshUploadBytes += stats.staticMeshUploadBytes; state.bonePaletteBytes += stats.bonePaletteUploadBytes;
	state.cpuSkinningVertices += stats.cpuSkinningVertices; state.cpuSkinningNormals += stats.cpuSkinningNormals;
	state.gpuSkinningFallbacks += stats.gpuSkinningFallbacks;
	state.gpuSkinningMaterialFallbacks += stats.gpuSkinningMaterialFallbacks;
	state.gpuSkinningGeometryFallbacks += stats.gpuSkinningGeometryFallbacks;
	state.gpuSkinningResourceFallbacks += stats.gpuSkinningResourceFallbacks;
	state.instancedDraws += stats.instancedDrawCalls; state.instancesSubmitted += stats.instancesSubmitted;
	state.instanceBatches += stats.instanceBatchesFlushed; state.instancePaletteDedupHits += stats.instancePaletteDedupHits;
	// Pico, nao soma: a media de um maximo por frame nao diria nada sobre a
	// fragmentacao da chave de lote.
	if (stats.largestInstanceBatch > state.largestInstanceBatch)
		state.largestInstanceBatch = stats.largestInstanceBatch;
	state.meshCacheHits += stats.staticMeshCacheHits; state.meshCacheMisses += stats.staticMeshCacheMisses;
	state.meshVerticesResident += stats.staticMeshVerticesResident; state.meshIndicesResident += stats.staticMeshIndicesResident;
	state.transformsExecuted += stats.transformsExecuted; state.transformsSkipped += stats.transformsSkipped;
	state.animationsExecuted += stats.animationsExecuted; state.animationsSkipped += stats.animationsSkipped;
	state.uniformCallsSaved += stats.uniformCallsSaved;
	for (int i = 0; i < RenderPhaseCount; ++i)
		state.phaseUs[i] += static_cast<unsigned long long>(g_renderPhaseUs[i] > 0 ? g_renderPhaseUs[i] : 0);
	state.matrixReadbackUs += g_matrixReadbackUs;
	state.matrixReadbackCalls += g_matrixReadbackCalls;
	state.nestingViolations += static_cast<unsigned long long>(g_phaseNestingViolations > 0 ? g_phaseNestingViolations : 0);
	{
		const unsigned long long frameNow = static_cast<unsigned long long>(g_frameTotalUs > 0 ? g_frameTotalUs : 0);
		const unsigned long long presentNow = static_cast<unsigned long long>(g_renderPhaseUs[RenderPhasePresent] > 0 ? g_renderPhaseUs[RenderPhasePresent] : 0);
		const unsigned long long effectsNow = static_cast<unsigned long long>(g_renderPhaseUs[RenderPhaseEffects] > 0 ? g_renderPhaseUs[RenderPhaseEffects] : 0);
		const unsigned long long aliveNow = static_cast<unsigned long long>(g_liveEffects > 0 ? g_liveEffects : 0);
		if (frameNow > state.frameTotalMax) state.frameTotalMax = frameNow;
		if (presentNow > state.presentMax) state.presentMax = presentNow;
		if (effectsNow > state.effectsMax) state.effectsMax = effectsNow;
		if (aliveNow > state.liveEffectsMax) state.liveEffectsMax = aliveNow;
		state.liveEffectsTotal += aliveNow;
		const unsigned long long trailsNow = static_cast<unsigned long long>(g_liveWheelTrails > 0 ? g_liveWheelTrails : 0);
		if (trailsNow > state.wheelTrailsMax) state.wheelTrailsMax = trailsNow;
		state.wheelTrailsTotal += trailsNow;
	}
	{
		// Contadores de multidao do frame que acabou de ser desenhado. Ficam DENTRO
		// da janela da cena, entao nao tem o frame de atraso das colunas pos-cpu_us.
		const CrowdLod::FrameCounters& crowd = CrowdLod::GetFrameCounters();
		state.charsLive += crowd.live;
		state.charsVisible += crowd.visible;
		if (crowd.visible > state.charsVisibleMax) state.charsVisibleMax = crowd.visible;
		state.charsCulledFrustum += crowd.culledFrustum;
		state.charsBeyondFar += crowd.beyondFar;
		state.charsForced += crowd.forced;
		for (int i = 0; i < CrowdLod::LevelCount; ++i)
			state.charLevels[i] += crowd.levels[i];
		for (int i = 0; i < CrowdLod::KindCount; ++i)
			state.charKinds[i] += crowd.kinds[i];
		state.charPoses += crowd.posesComputed;
		state.charPartMeshes += crowd.partMeshes;
		state.charShadows += crowd.shadows;
		state.charBatchAccum += g_charBatchAccum;
		state.charBatchBreaks += g_charBatchBreaks;
		// Pico, nao soma: o maior lote do frame diz se existe caso bom, e a media dele
		// entre 120 frames nao diria nada.
		if (g_charBatchRunMax > state.charBatchRunMax) state.charBatchRunMax = g_charBatchRunMax;
	}
	state.gpuUs += Platform::GetLastGpuFrameTimeUs();
	state.frameTotalUs += static_cast<unsigned long long>(g_frameTotalUs > 0 ? g_frameTotalUs : 0);
	state.fpsTotal += FPS;
	if (state.frames < 120)
		return;

	DWORD sortedCpu[120];
	memcpy(sortedCpu, state.cpuSamples, sizeof(sortedCpu));
	for (int i = 0; i < 120; ++i)
		for (int j = i + 1; j < 120; ++j)
			if (sortedCpu[j] < sortedCpu[i]) { const DWORD value = sortedCpu[i]; sortedCpu[i] = sortedCpu[j]; sortedCpu[j] = value; }

	// A v6 acrescenta as colunas de instancing, cache de malha e cache de pose.
	// Arquivo novo em vez de colunas extras no v5: misturar linhas de larguras
	// diferentes quebraria qualquer leitor de CSV usado nas comparacoes.
	// Desistir da amostra e o comportamento certo: perder 120 frames de captura
	// custa uma re-execucao, escrever linha de outra largura custa o arquivo inteiro
	// e -- pior -- produz numero que parece valido.
	if (!RotateRenderCsvIfHeaderDiffers())
	{
		state = CaptureState();
		state.scene = scene; state.world = world; state.width = width; state.height = height; state.glslBackend = glslBackend;
		return;
	}
	FILE* file = fopen(kRenderCsvPath, "a+");
	if (file != NULL)
	{
		fseek(file, 0, SEEK_END);
		if (ftell(file) == 0)
			fputs(kRenderCsvHeader, file);
		const char* skinningMode = Platform::GetGpuSkinningMode() == Platform::GpuSkinningOff ? "off" :
			(Platform::GetGpuSkinningMode() == Platform::GpuSkinningCompare ? "compare" : "on");
		// Coluna explicita para o que ainda escapa das fases. Calcular aqui evita
		// que a leitura da planilha tenha que refazer a subtracao toda vez.
		// So as fases DE DENTRO de cpu_us entram na subtracao. Overlay, present,
		// protocolo e pump acontecem depois da leitura de cpu_us; incluir elas aqui
		// zeraria us_unmeasured por engano.
		double measuredRemainder = static_cast<double>(state.cpuTotal);
		for (int i = 0; i < RenderPhaseInsideCount; ++i)
			measuredRemainder -= static_cast<double>(state.phaseUs[i]);
		if (measuredRemainder < 0.0) measuredRemainder = 0.0;
		// O que sobra do periodo do frame depois de cpu_us e dos quatro blocos
		// pos-cpu_us. Se ficar grande, ainda ha caminho de frame sem instrumento.
		// Para em RenderPhaseOutsideCount: as fases de detalhe aninham dentro de
		// cpu_us e subtrai-las aqui contaria o mesmo tempo duas vezes.
		double gapRemainder = static_cast<double>(state.frameTotalUs) - static_cast<double>(state.cpuTotal);
		for (int i = RenderPhaseInsideCount; i < RenderPhaseOutsideCount; ++i)
			gapRemainder -= static_cast<double>(state.phaseUs[i]);
		if (gapRemainder < 0.0) gapRemainder = 0.0;
		// us_char_parts por subtracao: us_characters menos o corpo e a sombra. Evita
		// instrumentar as ~2.400 linhas de equipamento de RenderCharacter uma a uma.
		double characterParts = static_cast<double>(state.phaseUs[RenderPhaseCharacters])
			- static_cast<double>(state.phaseUs[RenderPhaseCharPose])
			- static_cast<double>(state.phaseUs[RenderPhaseCharShadow]);
		if (characterParts < 0.0) characterParts = 0.0;
		// Resto da simulacao, tambem por subtracao. Contem: fisica/Bitmaps.Manage/
		// som 3D, MoveItems, MoveLeaves, boids/peixes/insetos/chat, pets, direcao,
		// censura e console. Se ele dominar, o proximo corte e aqui dentro.
		double simulationRemainder = static_cast<double>(state.phaseUs[RenderPhaseSimulation])
			- static_cast<double>(state.phaseUs[RenderPhaseSimUi])
			- static_cast<double>(state.phaseUs[RenderPhaseSimObjects])
			- static_cast<double>(state.phaseUs[RenderPhaseSimChars])
			- static_cast<double>(state.phaseUs[RenderPhaseSimEffects]);
		if (simulationRemainder < 0.0) simulationRemainder = 0.0;
		// fps correto: reciproco da media do periodo, nao media dos reciprocos.
		// A coluna fps (media de 1/dt instantaneo) superestima em ate 11% nas
		// amostras rapidas — medido na propria v14. Ela fica para nao quebrar a
		// comparacao com as capturas antigas, mas fps_period e a metrica valida.
		const double averagePeriodUs = state.frameTotalUs / 120.0;
		const double fpsFromPeriod = (averagePeriodUs > 0.0) ? (1000000.0 / averagePeriodUs) : 0.0;
		// FPS do PIOR frame da janela. E o numero que corresponde ao que o jogador
		// sente quando reclama que "a magia derruba o FPS" -- a media nao mostra isso.
		const double fpsWorstFrame = (state.frameTotalMax > 0) ? (1000000.0 / static_cast<double>(state.frameTotalMax)) : 0.0;
		fprintf(file, "%d,%d,%dx%d,%s,%s,%s,%s,%s,%s,%s,120,%.2f,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%llu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.2f,%.0f,%.0f,%.6f,%.0f,%d,%d,%d,%.0f,%.0f,%.0f,%.0f,%.0f,%.1f,%.0f,%.1f,%.0f,%d,%s,%d,%d,%d,%d,%.1f,%.1f,%.0f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.0f,%.1f,%.1f,%.1f\n",
			scene, world, width, height, glslBackend ? "glsl" : "fixed", skinningMode,
			Platform::GetRenderFeatureModeName(Platform::RenderFeatureInstancing),
			Platform::GetRenderFeatureModeName(Platform::RenderFeatureStaticTransformCache),
			Platform::GetRenderFeatureModeName(Platform::RenderFeatureBatching),
			Platform::GetRenderFeatureModeName(Platform::RenderFeatureMeshCache),
			Platform::GetRenderFeatureModeName(Platform::RenderFeatureCpuMatrices),
			state.cpuTotal / 120.0, static_cast<unsigned long>(sortedCpu[113]),
			state.draws / 120.0, state.vertices / 120.0, state.vboBytes / (120.0 * 1024.0), state.bufferData / 120.0,
			state.bufferSubData / 120.0, state.flushes / 120.0, state.textureUploads / 120.0, state.textureBytes / (120.0 * 1024.0),
			state.textureChanges / 120.0, state.matrixFlushes / 120.0, state.textureFlushes / 120.0, state.blendFlushes / 120.0,
			state.depthFlushes / 120.0, state.alphaFlushes / 120.0, state.fogFlushes / 120.0,
			state.gpuMeshDraws / 120.0, state.gpuMeshIndices / 120.0, state.gpuMeshUploadBytes / (120.0 * 1024.0), state.bonePaletteBytes / (120.0 * 1024.0),
			state.cpuSkinningVertices / 120.0, state.cpuSkinningNormals / 120.0, state.gpuSkinningFallbacks / 120.0,
			state.gpuSkinningMaterialFallbacks / 120.0, state.gpuSkinningGeometryFallbacks / 120.0, state.gpuSkinningResourceFallbacks / 120.0,
			state.instancedDraws / 120.0, state.instancesSubmitted / 120.0, state.instanceBatches / 120.0,
			state.largestInstanceBatch, state.instancePaletteDedupHits / 120.0,
			state.meshCacheHits / 120.0, state.meshCacheMisses / 120.0,
			state.meshVerticesResident / 120.0, state.meshIndicesResident / 120.0,
			state.transformsExecuted / 120.0, state.transformsSkipped / 120.0,
			state.animationsExecuted / 120.0, state.animationsSkipped / 120.0, state.uniformCallsSaved / 120.0,
			state.phaseUs[RenderPhaseTerrain] / 120.0, state.phaseUs[RenderPhaseObjects] / 120.0,
			state.phaseUs[RenderPhaseCharacters] / 120.0, state.phaseUs[RenderPhaseEffects] / 120.0,
			state.phaseUs[RenderPhaseSprites] / 120.0, state.phaseUs[RenderPhaseSimulation] / 120.0,
			state.phaseUs[RenderPhaseSelect] / 120.0, state.phaseUs[RenderPhaseSetup] / 120.0,
			state.phaseUs[RenderPhaseFrustum] / 120.0,
			state.phaseUs[RenderPhaseMisc] / 120.0,
			state.phaseUs[RenderPhaseWater] / 120.0, state.phaseUs[RenderPhaseUi] / 120.0,
			state.phaseUs[RenderPhaseFrameBegin] / 120.0, measuredRemainder / 120.0,
			state.phaseUs[RenderPhaseOverlay] / 120.0, state.phaseUs[RenderPhasePresent] / 120.0,
			state.phaseUs[RenderPhaseProtocol] / 120.0, state.phaseUs[RenderPhasePump] / 120.0,
			state.phaseUs[RenderPhaseLimiter] / 120.0, gapRemainder / 120.0,
			state.phaseUs[RenderPhaseCharPose] / 120.0, state.phaseUs[RenderPhaseCharShadow] / 120.0,
			characterParts / 120.0,
			state.phaseUs[RenderPhaseCharTransform] / 120.0,
			state.phaseUs[RenderPhaseCharMesh] / 120.0,
			state.phaseUs[RenderPhaseCharDraw] / 120.0,
			state.phaseUs[RenderPhaseCharLink] / 120.0,
			state.phaseUs[RenderPhaseCharLinkDraw] / 120.0,
			state.phaseUs[RenderPhaseSimUi] / 120.0, state.phaseUs[RenderPhaseSimObjects] / 120.0,
			state.phaseUs[RenderPhaseSimChars] / 120.0, state.phaseUs[RenderPhaseSimEffects] / 120.0,
			simulationRemainder / 120.0,
			state.nestingViolations / 120.0,
			state.matrixReadbackUs / 120.0, state.matrixReadbackCalls / 120.0,
			g_cpuMatrixMaxDivergence, state.gpuUs / 120.0, Platform::GetGpuFrameTimerState(), g_renderScalePercent,
			g_vsyncInterval, target_fps,
			state.frameTotalUs / 120.0,
			static_cast<double>(state.frameTotalMax), static_cast<double>(state.presentMax),
			static_cast<double>(state.effectsMax),
			state.liveEffectsTotal / 120.0, static_cast<double>(state.liveEffectsMax),
			state.wheelTrailsTotal / 120.0, static_cast<double>(state.wheelTrailsMax),
			g_wheelTrailCap,
			CrowdLod::GetModeName(),
			CrowdLod::GetSpawnRequest(CrowdLod::KindPlayer),
			CrowdLod::GetSpawnRequest(CrowdLod::KindMonster),
			CrowdLod::GetSpawnRequest(CrowdLod::KindNpc),
			CrowdLod::GetMaxFull(),
			state.charsLive / 120.0, state.charsVisible / 120.0,
			static_cast<double>(state.charsVisibleMax),
			state.charsCulledFrustum / 120.0, state.charsBeyondFar / 120.0,
			state.charsForced / 120.0,
			state.charKinds[CrowdLod::KindPlayer] / 120.0,
			state.charKinds[CrowdLod::KindMonster] / 120.0,
			state.charKinds[CrowdLod::KindNpc] / 120.0,
			state.charKinds[CrowdLod::KindOther] / 120.0,
			state.charLevels[CrowdLod::LevelFull] / 120.0,
			state.charLevels[CrowdLod::LevelReduced] / 120.0,
			state.charLevels[CrowdLod::LevelMinimal] / 120.0,
			state.charLevels[CrowdLod::LevelHidden] / 120.0,
			state.charPoses / 120.0, state.charPartMeshes / 120.0, state.charShadows / 120.0,
			state.charBatchAccum / 120.0, state.charBatchBreaks / 120.0,
			static_cast<double>(state.charBatchRunMax),
			state.fpsTotal / 120.0, fpsFromPeriod, fpsWorstFrame);
		fclose(file);
	}
	state = CaptureState();
	state.scene = scene; state.world = world; state.width = width; state.height = height; state.glslBackend = glslBackend;
}

void RenderScene(HDC hDC)
{
    // A amostra e zerada antes de qualquer emissao deste quadro. Depois do
    // SwapBuffers, GetLegacyRenderFrameStats() contem exatamente o frame que
    // acabou de ser apresentado, pronto para o futuro overlay/CSV.
    Platform::ResetLegacyRenderFrameStats();
    const char* commandLine = ::GetCommandLineA();
    const char* whitelistArgument = ::strstr(commandLine, "-gpuskinning-models=");
    if (whitelistArgument != NULL)
    {
        whitelistArgument += strlen("-gpuskinning-models=");
        char whitelist[512]; size_t length = 0;
        while (whitelistArgument[length] != 0 && whitelistArgument[length] != ' ' && whitelistArgument[length] != '\t' && length + 1 < sizeof(whitelist)) ++length;
        memcpy(whitelist, whitelistArgument, length); whitelist[length] = 0;
        Platform::SetGpuSkinningModelWhitelist(whitelist);
    }
    else
        Platform::SetGpuSkinningModelWhitelist(NULL);
    if (::strstr(commandLine, "-gpuskinning=off") != NULL)
    {
        Platform::SetGpuSkinningMode(Platform::GpuSkinningOff);
        Platform::SetGpuSkinningDeployment(Platform::GpuSkinningQA);
    }
    else if (::strstr(commandLine, "-gpuskinning=dev") != NULL || ::strstr(commandLine, "-gpuskinning=compare") != NULL)
    {
        Platform::SetGpuSkinningMode(Platform::GpuSkinningCompare);
        Platform::SetGpuSkinningDeployment(Platform::GpuSkinningDevelopment);
    }
    else if (::strstr(commandLine, "-gpuskinning=production") != NULL)
    {
        Platform::SetGpuSkinningMode(Platform::GpuSkinningOn);
        Platform::SetGpuSkinningDeployment(Platform::GpuSkinningProductionWhitelist);
    }
    else if (::strstr(commandLine, "-gpuskinning=on") != NULL)
    {
        Platform::SetGpuSkinningMode(Platform::GpuSkinningOn);
        Platform::SetGpuSkinningDeployment(Platform::GpuSkinningProductionDefault);
    }
    else
    {
        Platform::SetGpuSkinningMode(Platform::GpuSkinningOn);
        Platform::SetGpuSkinningDeployment(Platform::GpuSkinningQA);
    }
    ParseRenderFeatureFlag(commandLine, "-instancing=", Platform::RenderFeatureInstancing);
    ParseRenderFeatureFlag(commandLine, "-statictransformcache=", Platform::RenderFeatureStaticTransformCache);
    ParseRenderFeatureFlag(commandLine, "-batching=", Platform::RenderFeatureBatching);
    ParseRenderFeatureFlag(commandLine, "-meshcache=", Platform::RenderFeatureMeshCache);
    ParseRenderFeatureFlag(commandLine, "-cpumatrices=", Platform::RenderFeatureCpuMatrices);
    {
        // -renderscale=N (porcento). Fora de 1..100 e ignorado.
        const char* scale = ::strstr(commandLine, "-renderscale=");
        if (scale != NULL)
        {
            const int value = atoi(scale + strlen("-renderscale="));
            if (value >= 1 && value <= 100) g_renderScalePercent = value;
        }
    }
    {
        // -wheeltrail=N limita os rastros simultaneos da Twisting Slash. Ausente
        // mantem ilimitado, o comportamento historico. O valor vai para o CSV: uma
        // captura limitada nao pode parecer ganho magico, mesma regra do renderscale.
        const char* trail = ::strstr(commandLine, "-wheeltrail=");
        if (trail != NULL)
        {
            const int value = atoi(trail + strlen("-wheeltrail="));
            if (value >= 0 && value <= 200) g_wheelTrailCap = value;
        }
        else
            g_wheelTrailCap = -1;
    }
    // Crowd LOD: -crowdlod=off|on|compare e -crowd=N. Depois do .ini de proposito
    // -- a precedencia e default compilado -> MainInfo.ini (Web/Android) -> argv.
    // No Web e no Android nao ha linha de comando e esta chamada nao faz nada.
    CrowdLod::ApplyCommandLine(commandLine);
    ParseModelListFlag(commandLine, "-instancing-models=", &Platform::SetInstancingModelWhitelist);
    Platform::BeginGpuSkinningFrame();
	Platform::BeginGpuFrameTimer();
	g_renderStatsStart = RenderStatsNowMicroseconds();
	g_frameTotalUs = (g_frameTopPrevUs != 0) ? (g_renderStatsStart - g_frameTopPrevUs) : 0;
	g_frameTopPrevUs = g_renderStatsStart;
	memset(g_renderPhaseUs, 0, sizeof(g_renderPhaseUs));
	// Contadores de multidao valem por frame, como as fases. Zerados aqui, lidos
	// por CaptureRenderStatsCsv depois da cena -- sem o frame de atraso das quatro
	// colunas pos-cpu_us, porque a contagem acontece dentro da janela da cena.
	CrowdLod::BeginFrame();
	g_charBatchAccum = 0; g_charBatchBreaks = 0;
	g_charBatchRunMax = 0; g_charBatchRunCurrent = 0;
	// Transfere as fases pos-cpu_us do frame anterior. Elas foram medidas depois
	// de o CSV ter lido o array, entao chegam com um frame de atraso.
	for (int i = RenderPhaseInsideCount; i < RenderPhaseCount; ++i)
	{
		g_renderPhaseUs[i] = g_pendingPhaseUs[i];
		g_pendingPhaseUs[i] = 0;
	}
	g_matrixReadbackUs = 0;
	g_matrixReadbackCalls = 0;
	g_phaseNestingViolations = 0;
    CalcFPS();
	{
		ScopedRenderPhase phase(RenderPhaseSimulation);
		UpdateSceneState();
	}

	last_render_tick_count = current_tick_count;

	try
	{
		g_Luminosity = sinf(WorldTime * 0.004f) * 0.15f + 0.6f;
		switch (SceneFlag)
		{
#ifdef MOVIE_DIRECTSHOW
		case MOVIE_SCENE:
			MovieScene(hDC);
			break;
#endif // MOVIE_DIRECTSHOW
		case WEBZEN_SCENE:
			WebzenScene(hDC);
			break;
		case LOADING_SCENE:
			LoadingScene(hDC);
			break;
		case LOG_IN_SCENE:
		case CHARACTER_SCENE:
		case MAIN_SCENE:
			MainScene(hDC);
			break;
		}

		if (g_iNoMouseTime > 31)
		{
			KillGLWindow();
		}
	}
	catch (const std::exception&)
	{
	}
	// Fecha a query aqui, e nao dentro de MainScene: MainScene so roda em
	// algumas cenas, e um Begin sem End deixa a query ativa para sempre — o
	// glBeginQuery seguinte falha e nenhuma amostra volta a ficar pronta.
	// O catch acima tambem tornava o par nao garantido.
	Platform::EndGpuFrameTimer();
}


bool GetTimeCheck(int DelayTime)
{
	int PresentTime = timeGetTime();
	
	if(g_bTimeCheck)
	{
		g_iBackupTime = PresentTime;
		g_bTimeCheck = false;
	}

	if(g_iBackupTime+DelayTime <= PresentTime)
	{
		g_bTimeCheck = true;
		return true;
	}
	return false;	
}
