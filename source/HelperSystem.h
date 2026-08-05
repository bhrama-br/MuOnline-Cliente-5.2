#pragma once

#include "Lua.h"
#include "LuaBMD.h"

class HELPER_INFO {
public:
	// ModelID e LIDO em quatro lugares (CheckIsHelper, CheckHelperType,
	// GetHelperModel, CheckIsFenrirOrDino) e NUNCA escrito em lugar nenhum: SetHelper,
	// o unico ponto que preenche a struct, nao o atribui. Sem semear aqui, o valor era
	// indeterminado.
	//
	// O efeito era grave e distante: `CheckIsHelper(x)` devolve verdadeiro quando
	// `ModelID == x`, e `Draw_RenderObject` usa isso para desviar o objeto para o
	// renderizador de helper e RETORNAR. Com a memoria nova zerada (que e o caso no
	// wasm), ModelID valia 0 -- e todo objeto de mundo com `Type == 0` desaparecia sem
	// erro nenhum. Na cena de login isso eram OS TRES VELEIROS, que sao justamente o
	// Type 0. No Windows o lixo de pilha costuma ser diferente de zero, entao o defeito
	// nunca se manifestou: funcionava por sorte.
	//
	// -1 e o mesmo sentinela ja usado por ItemIndex, e nenhum tipo de modelo valido e
	// negativo.
	HELPER_INFO() {
		this->ItemIndex = -1;
		this->ModelID = -1;
		this->Type = 0;
		this->Movement = 0;
		this->Miniature = 0;
		this->HeightFloor = 0.f;
		this->Size = 0.f;
		this->SizeCharList = 0.f;
		this->SizeMiniature = 0.f;
		this->VelocityMiniature = 0.f;
		this->Model[0] = '\0';
		this->ObjectModel[0] = '\0';
	};

	~HELPER_INFO() {

	};

	int ItemIndex;
	int Type;
	int Movement;
	float HeightFloor;
	float Size;
	float SizeCharList;
	char Model[32];
	char ObjectModel[32];
	int ModelID;
	int Miniature;
	float SizeMiniature;
	float VelocityMiniature;
};

class CHelperSystem
{
public:
	CHelperSystem();
	~CHelperSystem();

	void Init();
	int GetHelperModel(int ItemIndex);
	bool CheckIsHelper(int ItemIndex);
	bool CheckHelperType(int ItemIndex, int value);
	HELPER_INFO* GetHelper(int ItemIndex);

	void InvCreateEquippingEffect(DWORD ItemIndex);
	int CheckIsFenrirOrDino(int ItemIndex);
public:
	std::map<int, HELPER_INFO> m_HelperInfo;

public:
	Lua m_Lua;
	LuaBMD m_LuaBMD;
};

extern CHelperSystem gHelperSystem;