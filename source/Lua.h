#pragma once

extern "C" {
#include <Lua/lua.h>
#include <Lua/lua.hpp>
#include <Lua/lauxlib.h>
}

#include "CriticalSection.h"

class Lua {
public:
	Lua();
	virtual ~Lua();

	void RegisterLua();
	void CloseLua();
	lua_State* GetState();
	void DoFile(const char* szFileName);
	bool Generic_Call(const char* func, const char* sig, ...);

private:
	lua_State* m_luaState;

private:
	CCriticalSection m_critical;
};