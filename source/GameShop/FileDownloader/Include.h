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
#endif //_WIN32
#include <Wininet.h>

#ifdef _WIN32
#include <crtdbg.h>
#endif //_WIN32
#ifdef _WIN32
#include <tchar.h>
#endif //_WIN32
#include <strsafe.h>

#include "GameShop\ShopListManager\interface\WZResult\WZResult.h"
#include "GameShop\ShopListManager\interface\DownloadInfo.h"
#include "GameShop\ShopListManager\interface\IDownloaderStateEvent.h"

#pragma comment(lib, "Wininet.lib")
