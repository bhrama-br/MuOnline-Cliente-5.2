// stdafx.cpp : source file that includes just the standard includes
//	Online.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// LuaJIT nao possui backend WebAssembly; o projeto passa a usar Lua padrao
// 5.3, que compila para wasm e Android. A API utilizada e a mesma.
#pragma comment (lib, "lua5.3.5-static.lib")

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file