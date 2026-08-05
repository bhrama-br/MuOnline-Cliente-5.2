#pragma once

#include "Lua.h"
#include "LuaCharacter.h"
#include "LuaInterface.h"
#include "LuaLoadImage.h"
#include "LuaGlobal.h"
#include "Protocol.h"

class CCharacterList {
public:
	CCharacterList();
	~CCharacterList();

	void Init();
	static void OpenCharacterSceneData();
	static void MoveCharacterList();
	static void RenderCharacterList();
	void SetCharacterPosition(int slot);

private:
	CLuaCharacter m_LuaCharacter;
	LuaInterface m_LuaInterface;
	LuaLoadImage m_LuaLoadImage;
	LuaGlobal m_LuaGlobal;
	Lua m_Lua;

public:
	float MountAddSize;
	float FlyingAddSize;
	float FlyingAddHeight;
	int MaxCharacters;
	int MaxCharactersAccount;
};

extern CCharacterList gCharacterList;