/*******************************************************************************
*	ÀÛ ¼º ÀÚ : ÁøÇýÁø
*	ÀÛ ¼º ÀÏ : 2009.06.10
*	³»    ¿ë : Include Header
*******************************************************************************/

#pragma once

#pragma warning(disable : 4995)

#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <Wininet.h>
#include <crtdbg.h>
#include <tchar.h>
#include <strsafe.h>
#else
// WinInet/tchar/strsafe sao exclusivos do Windows; o download da loja precisa de
// uma implementacao HTTP na camada Platform para Web/Android.
#include "Platform/WindowsCompat.h"
#endif //_WIN32

#include "GameShop\ShopListManager\interface\WZResult/WZResult.h"
#include "GameShop\ShopListManager\interface\DownloadInfo.h"
#include "GameShop\ShopListManager\interface\IDownloaderStateEvent.h"

#pragma comment(lib, "Wininet.lib")
