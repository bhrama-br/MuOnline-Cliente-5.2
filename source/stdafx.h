// stdafx.h : include file for standard system include files,
#pragma once

//warining
#pragma warning( disable : 4067 ) 
#pragma warning( disable : 4786 ) 
#pragma warning( disable : 4800 ) 
#pragma warning( disable : 4996 ) 
#pragma warning( disable : 4244 )
#pragma warning( disable : 4237 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4503 ) 
#pragma warning( disable : 4267 ) 
#pragma warning( disable : 4091 ) 
#pragma warning( disable : 4819 )
#pragma warning( disable : 4505 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4702 )
#pragma warning( disable : 4838 )
#pragma warning( disable : 5208 )
//#pragma warning( disable : 4482 )
//#pragma warning( disable : 4700 )
//#pragma warning( disable : 4748 )
//#pragma warning( disable : 4786 )
#pragma warning( disable : 28159 )
#pragma warning( disable : 26812 )

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN	
	
#ifndef _USE_32BIT_TIME_T
	#define _USE_32BIT_TIME_T
#endif //_USE_32BIT_TIME_T

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#pragma warning( push, 3 )

#ifdef _WIN32

#include <windows.h>

//windows
#include <WinSock2.h>
#include <mmsystem.h>
#include <shellapi.h>

#endif //_WIN32

//c runtime
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>

#ifdef _WIN32
#include <malloc.h>
#include <tchar.h>
#include <mbstring.h>
#include <conio.h>
#endif //_WIN32

#include <string>
#include <list>
#include <map>
#include <deque>
#include <algorithm>
#include <vector>
#include <queue>
#include <thread>


#pragma warning( pop )

// Tipos Win32 minimos para os alvos Emscripten/Android; no-op no Windows.
#include "Platform/WindowsCompat.h"

//opengl
#ifdef _WIN32
#include <gl/glew.h>
#include <gl/gl.h>
#else
#include <GLES3/gl3.h>
// Pilha de matrizes em CPU no lugar do GL_MODELVIEW/GL_PROJECTION do pipeline fixo.
#include "Platform/LegacyMatrixStack.h"
#endif

//patch
//winmain
#include "Winmain.h"
#include "Defined_Global.h"

//client
#include "_define.h"
#include "_enum.h"
#include "_types.h"
#include "_struct.h"	
#include "w_WindowMessageHandler.h"
#include "_GlobalFunctions.h"
#include "_TextureIndex.h"	
#include "InfoHelperFunctions.h"
#include "UIDefaultBase.h"
#include "NewUICommon.h"
#include "./Math/ZzzMathLib.h"
#include "ZzzOpenglUtil.h"
#include "Widescreen.h"
#include "Protect.h"
#include "ItemManager.h"

// Por ultimo, e de proposito: este cabecalho define macros com os nomes das
// funcoes de matriz do GL. Vindo depois de todos os includes, ele reescreve as
// CHAMADAS do cliente sem tocar nas DECLARACOES do <GL/gl.h>.
#include "Platform/LegacyMatrixMirror.h"