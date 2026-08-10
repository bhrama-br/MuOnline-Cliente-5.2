#include "stdafx.h"

#include "CrowdLod.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace
{
	// Altura tipica de um player em unidades de mundo. Numero aproximado de
	// proposito: ele serve para colocar o personagem numa FAIXA de tamanho, nao
	// para desenhar nada. Medir a caixa real de cada modelo por frame custaria
	// mais que o LOD economiza, e mudaria a faixa conforme a animacao.
	const float kCharacterWorldHeight = 130.f;

	// CameraFOV do projeto (Widescreen.cpp:127). Usado so quando o valor recebido
	// e absurdo -- preferivel a produzir NaN e classificar tudo como LevelHidden.
	const float kFallbackFieldOfView = 35.f;

	const float kPi = 3.14159265358979f;

	// Personagem colado na camera: retorna um numero grande em vez de dividir por
	// algo perto de zero.
	const float kHugePixels = 1.0e6f;

	// Espelham KIND_PLAYER/KIND_MONSTER/KIND_NPC de _define.h:144. Copiados em vez de
	// incluir o header: este modulo compila igual nos tres alvos e nao arrasta header
	// de jogo. Se os valores mudarem lá, a traducao aqui erra o bucket -- por isso os
	// numeros estao escritos com o nome ao lado, e nao soltos.
	const int kLegacyKindPlayer  = 1;   // KIND_PLAYER
	const int kLegacyKindMonster = 2;   // KIND_MONSTER
	const int kLegacyKindNpc     = 4;   // KIND_NPC

	struct State
	{
		State()
			: mode(CrowdLod::ModeOff)
			, maxFull(0)
		{
			for (int i = 0; i < CrowdLod::KindCount; ++i)
				spawnRequest[i] = 0;
			// Ponto de partida para calibrar em `compare`, NAO valores medidos: a
			// Fase 0 e que produz a captura com multidao. Um personagem com 32 px
			// de altura na tela nao mostra luva nem bota; com 12 px e um borrao de
			// dois pixels de largura.
			pixels[CrowdLod::LevelFull]    = 0.f;   // nao usado: LevelFull e o resto
			pixels[CrowdLod::LevelReduced] = 64.f;
			pixels[CrowdLod::LevelMinimal] = 32.f;
			pixels[CrowdLod::LevelHidden]  = 12.f;
		}

		CrowdLod::Mode mode;
		float          pixels[CrowdLod::LevelCount];
		int            maxFull;
		int            spawnRequest[CrowdLod::KindCount];
	};

	State& GetState()
	{
		static State state;
		return state;
	}

	CrowdLod::FrameCounters& GetCounters()
	{
		static CrowdLod::FrameCounters counters;
		return counters;
	}

	bool IgualSemCaso(const char* a, const char* b)
	{
		if (a == NULL || b == NULL) return false;
		while (*a != 0 && *b != 0)
		{
			const char ca = (*a >= 'A' && *a <= 'Z') ? (char)(*a - 'A' + 'a') : *a;
			const char cb = (*b >= 'A' && *b <= 'Z') ? (char)(*b - 'A' + 'a') : *b;
			if (ca != cb) return false;
			++a; ++b;
		}
		return *a == *b;
	}

	// Um valor de pixel invertido (L1 <= L2) faria a classificacao pular faixas em
	// silencio, e o CSV registraria uma distribuicao que nao corresponde a nenhum
	// corte real. Melhor recusar o valor e ficar com o default.
	bool LimiaresConsistentes(float reduced, float minimal, float hidden)
	{
		if (reduced <= 0.f || minimal <= 0.f || hidden <= 0.f) return false;
		return reduced > minimal && minimal > hidden;
	}
}

namespace CrowdLod
{
	void SetMode(Mode mode)
	{
		if (mode == ModeOff || mode == ModeOn || mode == ModeCompare)
			GetState().mode = mode;
	}

	Mode GetMode()
	{
		return GetState().mode;
	}

	const char* GetModeName()
	{
		switch (GetState().mode)
		{
		case ModeOn:      return "on";
		case ModeCompare: return "compare";
		default:          return "off";
		}
	}

	void SetPixelThresholds(float reduced, float minimal, float hidden)
	{
		if (!LimiaresConsistentes(reduced, minimal, hidden))
			return;
		State& state = GetState();
		state.pixels[LevelReduced] = reduced;
		state.pixels[LevelMinimal] = minimal;
		state.pixels[LevelHidden]  = hidden;
	}

	float GetPixelThreshold(Level level)
	{
		if (level < 0 || level >= LevelCount) return 0.f;
		return GetState().pixels[level];
	}

	void SetMaxFull(int maxFull)
	{
		GetState().maxFull = (maxFull > 0) ? maxFull : 0;
	}

	int GetMaxFull()
	{
		return GetState().maxFull;
	}

	Kind KindFromLegacyKind(int legacyKind)
	{
		if (legacyKind == kLegacyKindPlayer)  return KindPlayer;
		if (legacyKind == kLegacyKindMonster) return KindMonster;
		if (legacyKind == kLegacyKindNpc)     return KindNpc;
		return KindOther;
	}

	void SetSpawnRequest(Kind kind, int count)
	{
		if (kind < 0 || kind >= KindCount) return;
		// O teto real e o tamanho do pool CharactersClient, e quem sabe disso e o
		// rig. Aqui so barra valor negativo e absurdo, para um -crowd=999999999
		// nao virar aritmetica estranha antes de chegar la.
		if (count < 0)    count = 0;
		if (count > 4096) count = 4096;
		GetState().spawnRequest[kind] = count;
	}

	int GetSpawnRequest(Kind kind)
	{
		if (kind < 0 || kind >= KindCount) return 0;
		return GetState().spawnRequest[kind];
	}

	bool ApplyIniKey(const char* key, const char* value)
	{
		if (key == NULL || value == NULL) return false;

		if (IgualSemCaso(key, "CrowdLod"))
		{
			if      (IgualSemCaso(value, "on"))      SetMode(ModeOn);
			else if (IgualSemCaso(value, "compare")) SetMode(ModeCompare);
			else if (IgualSemCaso(value, "off"))     SetMode(ModeOff);
			// Valor desconhecido mantem o modo atual: um typo no .ini nao deve
			// ligar nem desligar otimizacao em silencio.
			return true;
		}
		if (IgualSemCaso(key, "CrowdLodPixelsL1") || IgualSemCaso(key, "CrowdLodPixelsL2")
			|| IgualSemCaso(key, "CrowdLodPixelsL3"))
		{
			State& state = GetState();
			float reduced = state.pixels[LevelReduced];
			float minimal = state.pixels[LevelMinimal];
			float hidden  = state.pixels[LevelHidden];
			const float parsed = (float)atof(value);
			if      (IgualSemCaso(key, "CrowdLodPixelsL1")) reduced = parsed;
			else if (IgualSemCaso(key, "CrowdLodPixelsL2")) minimal = parsed;
			else                                            hidden  = parsed;
			SetPixelThresholds(reduced, minimal, hidden);
			return true;
		}
		if (IgualSemCaso(key, "CrowdMaxFull"))
		{
			SetMaxFull(atoi(value));
			return true;
		}
		// "CrowdSpawn" sem sufixo continua sendo o de player: e a chave que ja estava
		// documentada, e mudar o significado dela silenciosamente faria um .ini
		// existente passar a spawnar outra coisa.
		if (IgualSemCaso(key, "CrowdSpawn") || IgualSemCaso(key, "CrowdSpawnPlayers"))
		{
			SetSpawnRequest(KindPlayer, atoi(value));
			return true;
		}
		if (IgualSemCaso(key, "CrowdSpawnMonsters"))
		{
			SetSpawnRequest(KindMonster, atoi(value));
			return true;
		}
		if (IgualSemCaso(key, "CrowdSpawnNpcs"))
		{
			SetSpawnRequest(KindNpc, atoi(value));
			return true;
		}
		return false;
	}

	void ApplyCommandLine(const char* commandLine)
	{
		if (commandLine == NULL) return;

		// "-crowdlod=" e procurado como string inteira, e "-crowd=" tem o '=' no
		// nome: nenhuma das duas casa com a outra por acidente.
		const char* modeArgument = ::strstr(commandLine, "-crowdlod=");
		if (modeArgument != NULL)
		{
			modeArgument += strlen("-crowdlod=");
			if      (::strncmp(modeArgument, "off", 3) == 0)     SetMode(ModeOff);
			else if (::strncmp(modeArgument, "compare", 7) == 0) SetMode(ModeCompare);
			else if (::strncmp(modeArgument, "on", 2) == 0)      SetMode(ModeOn);
		}

		// Ordem importa na busca: "-crowdmonsters=" e "-crowdnpcs=" NAO contem
		// "-crowd=" (o '=' faz parte do padrao), entao nao ha casamento cruzado.
		const char* players = ::strstr(commandLine, "-crowd=");
		if (players != NULL)
			SetSpawnRequest(KindPlayer, atoi(players + strlen("-crowd=")));

		const char* monsters = ::strstr(commandLine, "-crowdmonsters=");
		if (monsters != NULL)
			SetSpawnRequest(KindMonster, atoi(monsters + strlen("-crowdmonsters=")));

		const char* npcs = ::strstr(commandLine, "-crowdnpcs=");
		if (npcs != NULL)
			SetSpawnRequest(KindNpc, atoi(npcs + strlen("-crowdnpcs=")));
	}

	float ScreenPixels(float distance, float viewportHeight, float fieldOfViewDegrees)
	{
		if (viewportHeight <= 0.f) return kHugePixels;
		if (distance <= 1.f)       return kHugePixels;

		float fieldOfView = fieldOfViewDegrees;
		if (!(fieldOfView > 1.f && fieldOfView < 179.f))
			fieldOfView = kFallbackFieldOfView;

		const float tangent = tanf(fieldOfView * 0.5f * kPi / 180.f);
		if (tangent <= 0.f) return kHugePixels;

		return viewportHeight * kCharacterWorldHeight / (2.f * distance * tangent);
	}

	Level ClassifyByScreenPixels(float pixels)
	{
		const State& state = GetState();
		if (pixels < state.pixels[LevelHidden])  return LevelHidden;
		if (pixels < state.pixels[LevelMinimal]) return LevelMinimal;
		if (pixels < state.pixels[LevelReduced]) return LevelReduced;
		return LevelFull;
	}

	void BeginFrame()
	{
		FrameCounters& counters = GetCounters();
		memset(&counters, 0, sizeof(counters));
	}

	const FrameCounters& GetFrameCounters()
	{
		return GetCounters();
	}

	void CountCharacter(Kind kind, bool visible, bool beyondFar, bool forced, Level level)
	{
		FrameCounters& counters = GetCounters();
		++counters.live;
		if (visible) ++counters.visible; else ++counters.culledFrustum;
		if (beyondFar) ++counters.beyondFar;
		if (forced)    ++counters.forced;
		// Os dois indices sao validados aqui e nao no chamador: e o que garante as
		// identidades do CSV (soma dos tipos == vivos, soma dos niveis == vivos).
		// Um valor fora da faixa cairia em `live` sem cair em nenhum bucket, e o
		// leitor acusaria a divergencia sem dizer onde.
		if (kind >= 0 && kind < KindCount)
			++counters.kinds[kind];
		else
			++counters.kinds[KindOther];
		if (level >= 0 && level < LevelCount)
			++counters.levels[level];
	}

	void CountPose()     { ++GetCounters().posesComputed; }
	void CountPartMesh() { ++GetCounters().partMeshes; }
	void CountShadow()   { ++GetCounters().shadows; }
}
