#pragma once

#include "Lua.h"
#include "LuaEffectsNormal.h"

class CCustomEffects {
public:
	CCustomEffects();
	~CCustomEffects();

	void Init();
	void SetInfo(DWORD_PTR ObjectStruct, BMD* BMDStruct, int Type);

private:
	Lua m_Lua;
	CLuaEffectNormal m_LuaEffects;
};

extern CCustomEffects gCustomEffects;