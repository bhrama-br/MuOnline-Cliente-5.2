///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#ifdef _WIN32
#include <dpapi.h>
#endif //_WIN32
#include <locale.h>
#include <zmouse.h>
#include <cmath>
#include "UIWindows.h"
#include "UIManager.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "ZzzOpenData.h"
#include "ZzzScene.h"
#include "Platform/LegacySceneRunner.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "zzzLodTerrain.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"
#include "Resource.h"
#ifdef _WIN32
#include <imm.h>
#endif //_WIN32
#include "zzzpath.h"
#include "Nprotect.h"
#include "Local.h"
#include "PersonalShopTitleImp.h"
#include "./Utilities/Log/ErrorReport.h"
#include "UIMapName.h"		// rozy
#include "./ExternalObject/leaf/ExceptionHandler.h"
#include "./Utilities/Dump/CrashReporter.h"
#include "./Utilities/Log/muConsoleDebug.h"
//#include "./Utilities/CpuUsage.h"
#include "ProtocolSend.h"
#include "ProtectSysKey.h"

#include "CBTMessageBox.h"
#include "./ExternalObject/leaf/regkey.h"

#include "CSChaosCastle.h"
#include "GMHellas.h"
#include <io.h>
#include "Input.h"
#include "Platform/PlatformWindow.h"
#include "Platform/PlatformRenderContext.h"
#include "./Time/Timer.h"
#include "UIMng.h"

#ifdef MOVIE_DIRECTSHOW
#include <dshow.h>
#include "MovieScene.h"
#endif // MOVIE_DIRECTSHOW
#include "GameCensorship.h"

#include "w_MapHeaders.h"

#include "w_PetProcess.h"

#include <ThemidaInclude.h>
#include "ItemPosition.h"
#include "ItemSize.h"
#include "Descriptions.h"
#include "MultiLanguage.h"
#include "Widescreen.h"
#include "Console.h"
#include "Visuals.h"
#include "DarkSpirit.h"
#include "DisableExcellent.h"
#include "CustomBow.h"
#include "CustomWing.h"
#include "CustomEffects.h"
#include "CustomSetEffect.h"
#include "CustomItemFloor.h"
#include "CustomItemForce.h"
#include "RenderModel.h"
#include "CustomJewel.h"
#include "ServerName.h"
#include "CreateFont.h"
#include "MessageColor.h"
#include "Controller.h"
#include "Monsters.h"
#include "MonsterName.h"
#include "MonsterGlow.h"
#include "MonsterEffect.h"
#include "CustomCape.h"
#include "CounterItem.h"
#include "CharacterList.h"
#include "Patente.h"
#include "ElementSlots.h"
#include "TradeX.h"
#include "CreateFont.h"
#ifdef NEW_MUHELPER_ON
#include "CAIController.h"
#endif
#include "WindowTray.h"

CUIMercenaryInputBox * g_pMercenaryInputBox = NULL;
CUITextInputBox * g_pSingleTextInputBox = NULL;
CUITextInputBox * g_pSinglePasswdInputBox = NULL;
int g_iChatInputType = 1;
extern BOOL g_bIMEBlock;

CChatRoomSocketList * g_pChatRoomSocketList = NULL;

CMultiLanguage *pMultiLanguage = NULL;

extern DWORD g_dwTopWindow;

#ifdef MOVIE_DIRECTSHOW
CMovieScene*	g_pMovieScene = NULL;
#endif // MOVIE_DIRECTSHOW

CUIManager* g_pUIManager = NULL;
CUIMapName* g_pUIMapName = NULL;		// rozy

float Time_Effect = 0;
bool ashies = false;
int weather = rand()%3;

HWND      g_hWnd  = NULL;
HINSTANCE g_hInst = NULL;
HDC       g_hDC   = NULL;
HGLRC     g_hRC   = NULL;
HFONT     g_hFont = NULL;
HFONT     g_hFontBold = NULL;
HFONT     g_hFontBig = NULL;
HFONT     g_hFixFont = NULL;
HFONT     g_hNewFonts[10];

CTimer* g_pTimer = new CTimer();    // performance counter.
bool      Destroy = false;
bool      ActiveIME = false;

BYTE*				RendomMemoryDump;
ITEM_ATTRIBUTE*		ItemAttRibuteMemoryDump;
CHARACTER*			CharacterMemoryDump;

int       RandomTable[100];

char TextMu[]       = "mu.exe";

CErrorReport g_ErrorReport;

BOOL g_bMinimizedEnabled = FALSE;
int g_iScreenSaverOldValue = 60*15;

extern float g_fScreenRate_x;	// ¡Ø
extern float g_fScreenRate_y;

#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
BOOL g_bUseWindowMode = TRUE;
#endif

char Mp3FileName[256];

#pragma comment(lib, "wzAudio.lib")
#include <wzAudio.h>

void StopMp3(char *Name, BOOL bEnforce)
{
    if(!m_MusicOnOff && !bEnforce) return;

	if(Mp3FileName[0] != NULL)
	{
		if(strcmp(Name, Mp3FileName) == 0) {
			wzAudioStop();
			Mp3FileName[0] = NULL;
		}
	}
}

void PlayMp3(char *Name, BOOL bEnforce )
{
	if(Destroy) return;
    if(!m_MusicOnOff && !bEnforce) return;

	if(strcmp(Name,Mp3FileName) == 0) 
	{
		return;
	}
	else 
	{
		wzAudioPlay(Name, 1);
		strcpy(Mp3FileName,Name);
	}
}

bool IsEndMp3()
{
	if (100 == wzAudioGetStreamOffsetRange())
		return true;
	return false;
}

int GetMp3PlayPosition()
{
	return wzAudioGetStreamOffsetRange();
}

extern int  LogIn;
extern char LogInID[];

void CheckHack( void)
{
	#ifdef NEW_PROTOCOL_SYSTEM
		gProtocolSend.SendCheckOnline();
	#else
		SendCheck();
	#endif
}

GLvoid KillGLWindow(GLvoid)								
{
#if 0
	if (g_hRC)
	{
		if (!wglMakeCurrent(NULL,NULL))
		{
			g_ErrorReport.Write( "GL - Release Of DC And RC Failed\r\n");
			MessageBox(NULL,"Release Of DC And RC Failed.","Error",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(g_hRC))
		{
			g_ErrorReport.Write( "GL - Release Rendering Context Failed\r\n");
			MessageBox(NULL,"Release Rendering Context Failed.","Error",MB_OK | MB_ICONINFORMATION);
		}

		g_hRC=NULL;
	}

	if (g_hDC && !ReleaseDC(g_hWnd,g_hDC))
	{
		g_ErrorReport.Write( "GL - OpenGL Release Error\r\n");
		MessageBox(NULL,"OpenGL Release Error.","Error",MB_OK | MB_ICONINFORMATION);
		g_hDC=NULL;
	}

#endif
	if (!Platform::GetRenderContext().Destroy())
	{
		g_ErrorReport.Write("GL - Release rendering context failed - ErrorCode : %d\r\n", Platform::GetRenderContext().GetLastSystemError());
		MessageBox(NULL, "Release rendering context failed.", "Error", MB_OK | MB_ICONINFORMATION);
	}

	g_hRC = NULL;
	g_hDC = NULL;

#if (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
	{
		ChangeDisplaySettings(NULL,0);
		ShowCursor(TRUE);
	}
#else
#ifdef ENABLE_FULLSCREEN
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
#endif	// USER_WINDOW_MODE
	{
		ChangeDisplaySettings(NULL,0);
		ShowCursor(TRUE);
	}
#endif //ENABLE_FULLSCREEN
#endif	//WINDOWMODE(#else)
}


BOOL GetFileNameOfFilePath( char *lpszFile, char *lpszPath)
{
	int iFind = ( int)'\\';
	char *lpFound = lpszPath;
	char *lpOld = lpFound;
	while ( lpFound)
	{
		lpOld = lpFound;
		lpFound = strchr( lpFound + 1, iFind);
	}

	if ( strchr( lpszPath, iFind))
	{
		strcpy( lpszFile, lpOld + 1);
	}
	else
	{
		strcpy( lpszFile, lpOld);
	}

	BOOL bCheck = TRUE;
	for ( char *lpTemp = lpszFile; bCheck; ++lpTemp)
	{
		switch ( *lpTemp)
		{
		case '\"':
		case '\\':
		case '/':
		case ' ':
			*lpTemp = '\0';
		case '\0':
			bCheck = FALSE;
			break;
		}
	}

	return ( TRUE);
}

HANDLE g_hMainExe = INVALID_HANDLE_VALUE;

BOOL OpenMainExe( void)
{
#ifdef _DEBUG
	return ( TRUE);
#endif
	char lpszFile[MAX_PATH];
	char *lpszCommandLine = GetCommandLine();
	GetFileNameOfFilePath( lpszFile, lpszCommandLine);

	g_hMainExe = CreateFile( ( char*)lpszFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, 0);
	
	return ( INVALID_HANDLE_VALUE != g_hMainExe);
}

void CloseMainExe( void)
{
	CloseHandle( g_hMainExe);
}

WORD DecryptCheckSumKey( WORD wSource)
{
	WORD wAcc = wSource ^ 0xB479;
	return ( ( wAcc >> 10) << 4) | ( wAcc & 0xF);
}

DWORD GenerateCheckSum( BYTE *pbyBuffer, DWORD dwSize, WORD wKey)
{
	DWORD dwKey = ( DWORD)wKey;
	DWORD dwResult = dwKey << 9;
	for ( DWORD dwChecked = 0; dwChecked <= dwSize - 4; dwChecked += 4)
	{
		DWORD dwTemp;
		memcpy( &dwTemp, pbyBuffer + dwChecked, sizeof ( DWORD));

		switch ( ( dwChecked / 4 + wKey) % 3)
		{
		case 0:
			dwResult ^= dwTemp;
			break;
		case 1:
			dwResult += dwTemp;
			break;
		case 2:
			dwResult <<= ( dwTemp % 11);
			dwResult ^= dwTemp;
			break;
		}

		if ( 0 == ( dwChecked % 4))
		{
			dwResult ^= ( ( dwKey + dwResult) >> ( ( dwChecked / 4) % 16 + 3));
		}
	}

	return ( dwResult);
}

DWORD GetCheckSum( WORD wKey)
{
	wKey = DecryptCheckSumKey( wKey);

	char lpszFile[MAX_PATH];

	strcpy( lpszFile, "data\\local\\Gameguard.csr");

	HANDLE hFile = CreateFile( ( char*)lpszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if ( INVALID_HANDLE_VALUE == hFile)
	{
		return ( 0);
	}
	
	DWORD dwSize = GetFileSize( hFile, NULL);
	BYTE *pbyBuffer = new BYTE [dwSize];
	DWORD dwNumber;
	ReadFile( hFile, pbyBuffer, dwSize, &dwNumber, 0);
	CloseHandle( hFile);
	
	DWORD dwCheckSum = GenerateCheckSum(pbyBuffer, dwSize, wKey);
	delete [] pbyBuffer;
	
	return (dwCheckSum);
}


BOOL GetFileVersion( char *lpszFileName, WORD *pwVersion)
{
	DWORD dwHandle;
	DWORD dwLen = GetFileVersionInfoSize( lpszFileName, &dwHandle);
	if ( dwLen <= 0)
	{
		return ( FALSE);
	}

	BYTE *pbyData = new BYTE [dwLen];
	if ( !GetFileVersionInfo( lpszFileName, dwHandle, dwLen, pbyData))
	{
		delete [] pbyData;
		return ( FALSE);
	}

	VS_FIXEDFILEINFO *pffi;
	UINT uLen;
	if ( !VerQueryValue( pbyData, "\\", ( LPVOID*)&pffi, &uLen))
	{
		delete [] pbyData;
		return ( FALSE);
	}

	pwVersion[0] = HIWORD( pffi->dwFileVersionMS);
	pwVersion[1] = LOWORD( pffi->dwFileVersionMS);
	pwVersion[2] = HIWORD( pffi->dwFileVersionLS);
	pwVersion[3] = LOWORD( pffi->dwFileVersionLS);

	delete [] pbyData;
	return ( TRUE);
}

extern PATH     *path;

void DestroyWindow()
{
	//. save volume level
	leaf::CRegKey regkey;
	regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config");
	regkey.WriteDword("VolumeLevel", g_pOption->GetVolumeLevel());

	CUIMng::Instance().Release();

#ifdef MOVIE_DIRECTSHOW
	if(g_pMovieScene)
	{
		g_pMovieScene->Destroy();
	}
#endif // MOVIE_DIRECTSHOW

	//. release font handle
	if(g_hFont)
		DeleteObject((HGDIOBJ)g_hFont);

	if(g_hFontBold)
		DeleteObject((HGDIOBJ)g_hFontBold);

	if(g_hFontBig)
		DeleteObject((HGDIOBJ)g_hFontBig);

	if(g_hFixFont)
		::DeleteObject((HGDIOBJ)g_hFixFont);
	
	ReleaseCharacters();

    if ( path!=NULL )
    {
	    delete path;
    }
	SAFE_DELETE(GateAttribute);

	for ( int i = 0; i < MAX_SKILLS; ++i)
	{
	}
	SAFE_DELETE(SkillAttribute);

	SAFE_DELETE(CharacterMachine);

    DeleteWaterTerrain ();

#ifdef MOVIE_DIRECTSHOW
	if(SceneFlag != MOVIE_SCENE)
#endif // MOVIE_DIRECTSHOW
	{
		gMapManager.DeleteObjects();

		// Object.
		for(int i=MODEL_LOGO;i<MAX_MODELS;i++)
		{
			Models[i].Release();
		}

		// Bitmap
		Bitmaps.UnloadAllImages();
	}

	SAFE_DELETE_ARRAY( CharacterMemoryDump );
	SAFE_DELETE_ARRAY( ItemAttRibuteMemoryDump );
	SAFE_DELETE_ARRAY( RendomMemoryDump );
	SAFE_DELETE_ARRAY( ModelsDump );
	
#ifdef DYNAMIC_FRUSTRUM
	DeleteAllFrustrum();
#endif //DYNAMIC_FRUSTRUM

	SAFE_DELETE(g_pMercenaryInputBox);
	SAFE_DELETE(g_pSingleTextInputBox);
	SAFE_DELETE(g_pSinglePasswdInputBox);

	SAFE_DELETE(g_pChatRoomSocketList);
	SAFE_DELETE(g_pUIMapName);	// rozy
	SAFE_DELETE( g_pTimer );
	SAFE_DELETE(g_pUIManager);
	 
#ifdef MOVIE_DIRECTSHOW
	SAFE_DELETE(g_pMovieScene);
#endif // MOVIE_DIRECTSHOW

	SAFE_DELETE(pMultiLanguage);
	BoostRest( g_BuffSystem );
	BoostRest( g_MapProcess );
	BoostRest( g_petProcess );

	g_ErrorReport.Write( "Destroy" );
	 
	HWND shWnd = FindWindow(NULL, "MuPlayer");
	if(shWnd)
		SendMessage(shWnd, WM_DESTROY, 0, 0);
}
void DestroySound()
{
	for(int i=0;i<MAX_BUFFER;i++)
		ReleaseBuffer(i);

	FreeDirectSound();
	wzAudioDestroy();
}

int g_iInactiveTime = 0;
int g_iNoMouseTime = 0;
int g_iInactiveWarning = 0;
bool g_bWndActive = false;
bool HangulDelete = false;
int Hangul = 0;
bool g_bEnterPressed = false;

int g_iMousePopPosition_x = 0;
int g_iMousePopPosition_y = 0;

extern int TimeRemain;
extern bool EnableFastInput;
void MainScene(HDC hDC);

LONG FAR PASCAL WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    switch (msg)
    {
	case TRAYMODE_ICON_MESSAGE:
	{
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			gTrayMode.SwitchState();
			break;
		}
	}
	case WM_SYSKEYDOWN:
		{
			return 0;
		}
		break;
#if defined PROTECT_SYSTEMKEY && defined NDEBUG
#ifndef FOR_WORK
	case WM_SYSCOMMAND:
		{
			if(wParam == SC_KEYMENU || wParam == SC_SCREENSAVE)
			{
				return 0;
			}
		}
		break;
#endif // !FOR_WORK
#endif // PROTECT_SYSTEMKEY && NDEBUG
    case WM_ACTIVATE:
		if(LOWORD(wParam) == WA_INACTIVE)
		{
#ifdef ACTIVE_FOCUS_OUT
			if (g_bUseWindowMode == FALSE)
#endif	// ACTIVE_FOCUS_OUT
				g_bWndActive = false;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
			if (g_bUseWindowMode == TRUE)
			{
				MouseLButton = false;
				MouseLButtonPop = false;
				//MouseLButtonPush = false;
				MouseRButton = false;
				MouseRButtonPop = false;
				MouseRButtonPush = false;
				MouseLButtonDBClick = false;
				MouseMButton = false;
				MouseMButtonPop = false;
				MouseMButtonPush = false;
				MouseWheel = 0;
			}
#endif
		}
		else
		{
			g_bWndActive = true;
		}
		break;
	case WM_TIMER :
		//MessageBox(NULL,GlobalText[16],"Error",MB_OK);
		switch( wParam )
		{
		case HACK_TIMER:
			// PKD_ADD_BINARY_PROTECTION
			VM_START
			CheckHack();
			VM_END
			break;
		case WINDOWMINIMIZED_TIMER:
			PostMessage(g_hWnd, WM_CLOSE, 0, 0);
			break;
		case CHATCONNECT_TIMER:
			g_pFriendMenu->SendChatRoomConnectCheck();
			break;
		case SLIDEHELP_TIMER:
			if(g_bWndActive)
			{
				if(g_pSlideHelpMgr)
					g_pSlideHelpMgr->CreateSlideText();
			}
			break;
		}
		break;
	case WM_USER_MEMORYHACK:
		//SetTimer( g_hWnd, WINDOWMINIMIZED_TIMER, 1*1000, NULL);
		KillGLWindow();
		break;
	case WM_NPROTECT_EXIT_TWO:
		SendHackingChecked( 0x04, 0);
		SetTimer( g_hWnd, WINDOWMINIMIZED_TIMER, 1*1000, NULL);
		MessageBox(NULL,GlobalText[16],"Error",MB_OK);
		break;
	case WM_ASYNCSELECTMSG :
		switch( WSAGETSELECTEVENT( lParam ) )
		{
		case FD_CONNECT :
			break;
		case FD_READ :
			SocketClient.nRecv();
			break;
		case FD_WRITE :
			SocketClient.FDWriteSend();
			break;
		case FD_CLOSE :
			g_pChatListBox->AddText("", GlobalText[3], SEASON3B::TYPE_SYSTEM_MESSAGE);
#ifdef CONSOLE_DEBUG
			switch(WSAGETSELECTERROR(lParam))
			{
			case WSAECONNRESET:
				g_ConsoleDebug->Write(MCD_ERROR, "The connection was reset by the remote side.");
				g_ErrorReport.Write("The connection was reset by the remote side.\r\n");
				g_ErrorReport.WriteCurrentTime();
				break;
			case WSAECONNABORTED:
				g_ConsoleDebug->Write(MCD_ERROR, "The connection was terminated due to a time-out or other failure.");
				g_ErrorReport.Write("The connection was terminated due to a time-out or other failure.\r\n");
				g_ErrorReport.WriteCurrentTime();
				break;
			}
#endif // CONSOLE_DEBUG
			SocketClient.Close();

			#ifdef NEW_PROTOCOL_SYSTEM
				gProtocolSend.DisconnectServer();
			#endif	

			CUIMng::Instance().PopUpMsgWin(MESSAGE_SERVER_LOST);
			break;
		}
		break;
	case WM_CTLCOLOREDIT:
		SetBkColor((HDC)wParam, RGB(0, 0, 0));
		SetTextColor((HDC)wParam, RGB(255, 255, 255));
		return (LRESULT)GetStockObject(BLACK_BRUSH);
		break;
	case WM_ERASEBKGND:
		return TRUE;
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps ;
			HDC hDC = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps) ;
		}
		return 0;
		break;
	case WM_DESTROY:
		{
			Destroy = true;
			SocketClient.Close();

			#ifdef NEW_PROTOCOL_SYSTEM
				gProtocolSend.DisconnectServer();
			#endif	

			DestroySound();
			//DestroyWindow();
			KillGLWindow();	
			CloseMainExe();
			PostQuitMessage(0);
		}
		break;
    case WM_SETCURSOR:
        ShowCursor(false);
		break;
#if (defined WINDOWMODE)
	case WM_SIZE:
		if ( SIZE_MINIMIZED == wParam && g_bUseWindowMode == FALSE )
		{
			if ( !( g_bMinimizedEnabled))
			{
				DWORD dwMess[SIZE_ENCRYPTION_KEY];
				for ( int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
				{
					dwMess[i] = GetTickCount();
				}
				g_SimpleModulusCS.LoadKeyFromBuffer( ( BYTE*)dwMess, FALSE, FALSE, FALSE, TRUE);
			}
		}
		break;
#else
#ifdef NDEBUG
#ifndef FOR_WORK
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	case WM_SIZE:
		if ( SIZE_MINIMIZED == wParam
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
			&& g_bUseWindowMode == FALSE
#endif
			)
		{
			if ( !( g_bMinimizedEnabled))
			{
				SendHackingChecked( 0x05, 0);
				DWORD dwMess[SIZE_ENCRYPTION_KEY];
				for ( int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
				{
					dwMess[i] = GetTickCount();
				}
				g_SimpleModulusCS.LoadKeyFromBuffer( ( BYTE*)dwMess, FALSE, FALSE, FALSE, TRUE);
			}
		}
		break;
#endif
#endif
#endif
#endif	//WINDOWMODE(#else)
//-----------------------------
	default:
		if (msg >= WM_CHATROOMMSG_BEGIN && msg < WM_CHATROOMMSG_END)
			g_pChatRoomSocketList->ProcessSocketMessage(msg - WM_CHATROOMMSG_BEGIN, WSAGETSELECTEVENT(lParam));
		break;
	}

	MouseLButtonDBClick = false;
	if (MouseLButtonPop == true && (g_iMousePopPosition_x != MouseX || g_iMousePopPosition_y != MouseY)) 
		MouseLButtonPop = false;
    switch (msg)
	{
	case WM_MOUSEMOVE:
		{
			// - ConverCursor & CompareToCursor (1.04E)
			static int WindowLeft = 640;
			WindowLeft = WindowWidth / g_fScreenRate_y;
			MouseX = (float)LOWORD(lParam) / g_fScreenRate_y;
			MouseY = (float)HIWORD(lParam) / g_fScreenRate_y;
			if(MouseX < 0) 
				MouseX = 0;
			if (MouseX > WindowLeft)
				MouseX = WindowLeft;
			if(MouseY < 0) 
				MouseY = 0;
			if(MouseY > GetWindowsY)
				MouseY = GetWindowsY;
		}
		break;
	case WM_LBUTTONDOWN:
		g_iNoMouseTime = 0;
		MouseLButtonPop = false;
		if(!MouseLButton) 
			MouseLButtonPush = true;
		MouseLButton = true;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		SetCapture(g_hWnd);
#endif
		break;
	case WM_LBUTTONUP:
		g_iNoMouseTime = 0;
		MouseLButtonPush = false;
		//if(MouseLButton) MouseLButtonPop = true;
		MouseLButtonPop = true;
		MouseLButton = false;
		g_iMousePopPosition_x = MouseX;
		g_iMousePopPosition_y = MouseY;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		ReleaseCapture();
#endif
		break;
	case WM_RBUTTONDOWN:
		g_iNoMouseTime = 0;
		MouseRButtonPop = false;
		if(!MouseRButton) MouseRButtonPush = true;
		MouseRButton = true;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		SetCapture(g_hWnd);
#endif
		break;
	case WM_RBUTTONUP:
		g_iNoMouseTime = 0;
		MouseRButtonPush = false;
		if(MouseRButton) MouseRButtonPop = true;
		MouseRButton = false;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		ReleaseCapture();
#endif
		break;
    case WM_LBUTTONDBLCLK:
		g_iNoMouseTime = 0;
		MouseLButtonDBClick = true;
		break;
	case WM_MBUTTONDOWN:
		g_iNoMouseTime = 0;
		MouseMButtonPop = false;
		if(!MouseMButton) MouseMButtonPush = true;
		MouseMButton = true;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		SetCapture(g_hWnd);
#endif
		break;
	case WM_MBUTTONUP:
		g_iNoMouseTime = 0;
		MouseMButtonPush = false;
		if(MouseMButton) MouseMButtonPop = true;
		MouseRButton = false;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		ReleaseCapture();
#endif
		break;
	case WM_MOUSEWHEEL:
        {
            MouseWheel = (short)HIWORD( wParam)/WHEEL_DELTA;
        }
        break;
	case WM_IME_NOTIFY:
		{
			if (g_iChatInputType == 1)
			{
				switch (wParam)
				{
				case IMN_SETCONVERSIONMODE:
					if (GetFocus() == g_hWnd)
					{
						CheckTextInputBoxIME(IME_CONVERSIONMODE);
					}
					break;
				case IMN_SETSENTENCEMODE:
					if (GetFocus() == g_hWnd)
					{
						CheckTextInputBoxIME(IME_SENTENCEMODE);
					}
					break;
				default:
					break;
				}
			}
		}
		break;
	case WM_CHAR:
		{
			switch(wParam)
			{
			case VK_RETURN:
				{
					SetEnterPressed( true );
				}
				break;
			}
		}
		break;
    }

	if( g_BuffSystem ) {
		LRESULT result;
		TheBuffStateSystem().HandleWindowMessage( msg, wParam, lParam, result );
	}

    return DefWindowProc(hwnd,msg,wParam,lParam);
}

bool CreateOpenglWindow()
{
#if 0
    PIXELFORMATDESCRIPTOR pfd;

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 16;
	pfd.cDepthBits   = 16;

	if (!(g_hDC=GetDC(g_hWnd)))
	{
		g_ErrorReport.Write( "OpenGL Get DC Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL,GlobalText[4],"OpenGL Get DC Error.",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	GLuint PixelFormat;

	if (!(PixelFormat=ChoosePixelFormat(g_hDC,&pfd)))
	{
		g_ErrorReport.Write( "OpenGL Choose Pixel Format Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL,GlobalText[4],"OpenGL Choose Pixel Format Error.",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!SetPixelFormat(g_hDC,PixelFormat,&pfd))
	{
		g_ErrorReport.Write( "OpenGL Set Pixel Format Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL,GlobalText[4],"OpenGL Set Pixel Format Error.",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(g_hRC=wglCreateContext(g_hDC)))
	{
		g_ErrorReport.Write( "OpenGL Create Context Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL,GlobalText[4],"OpenGL Create Context Error.",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!wglMakeCurrent(g_hDC,g_hRC))
	{
		g_ErrorReport.Write( "OpenGL Make Current Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL,GlobalText[4],"OpenGL Make Current Error.",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

#endif
	Platform::RenderContextConfig config;
	config.window = g_hWnd;
	config.colorBits = 16;
	config.depthBits = 16;
	config.doubleBuffered = true;
	// O adaptador GLSL e o caminho padrao em todas as plataformas. O renderizador
	// fixo fica disponivel somente como rota de comparacao/recuperacao no PC.
	// -glslrenderer continua aceito por compatibilidade com atalhos antigos.
	config.enableShaderBackend = (::strstr(::GetCommandLineA(), "-fixedrenderer") == NULL);

	if (!Platform::GetRenderContext().Create(config))
	{
		g_ErrorReport.Write("OpenGL context creation failed - ErrorCode : %d\r\n", Platform::GetRenderContext().GetLastSystemError());
		KillGLWindow();
		MessageBox(NULL, GlobalText[4], "OpenGL context creation failed.", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	g_hDC = static_cast<HDC>(Platform::GetRenderContext().GetNativeDeviceContext());
	g_hRC = static_cast<HGLRC>(Platform::GetRenderContext().GetNativeRenderContext());
	g_ErrorReport.Write("OpenGL renderer: %s\r\n", config.enableShaderBackend ? "GLSL legacy adapter" : "fixed-function comparison fallback");

	// -vsync=on|off. Ausente NAO chama nada: o swap interval fica no default do
	// driver, que e exatamente o comportamento de todas as capturas anteriores.
	// Trocar o default silenciosamente invalidaria a comparacao com elas. A flag
	// existe porque um vsync forcado no painel trava o FPS em 60 e esconde
	// qualquer otimizacao — sem ela nao ha como saber em que regime a medicao foi
	// feita. O valor efetivo vai para a coluna `vsync` do CSV.
	{
		const char* vsyncArgument = ::strstr(::GetCommandLineA(), "-vsync=");
		if (vsyncArgument != NULL)
		{
			typedef BOOL (WINAPI *SwapIntervalProc)(int);
			const SwapIntervalProc swapInterval =
				reinterpret_cast<SwapIntervalProc>(::wglGetProcAddress("wglSwapIntervalEXT"));
			if (swapInterval != NULL)
			{
				const int requested = (::strstr(vsyncArgument, "-vsync=on") != NULL) ? 1 : 0;
				if (swapInterval(requested))
					g_vsyncInterval = requested;
				else
					g_ErrorReport.Write("wglSwapIntervalEXT(%d) failed - ErrorCode : %d\r\n", requested, GetLastError());
			}
			else
				g_ErrorReport.Write("wglSwapIntervalEXT unavailable; swap interval left at driver default\r\n");
		}
	}

	ShowWindow(g_hWnd,SW_SHOW);
	SetForegroundWindow(g_hWnd);
	SetFocus(g_hWnd);
	return true;
}

#if 0
HWND StartWindow(HINSTANCE hCurrentInst, int nCmdShow)
{
	char* windowName = gProtect->m_MainInfo.WindowName;

	WNDCLASS wndClass;
	HWND hWnd;

	wndClass.style = CS_OWNDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hCurrentInst;
	wndClass.hIcon = LoadIcon(hCurrentInst, (LPCTSTR)IDI_ICON1);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = windowName;
	RegisterClass(&wndClass);

	if (g_bUseWindowMode == TRUE)
	{
		RECT rc = { 0, 0, WindowWidth, WindowHeight };
		if (g_bUseWindowMode == TRUE)
			AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN, NULL);
		else
			AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, NULL);
		rc.right -= rc.left;
		rc.bottom -= rc.top;

		if (g_bUseWindowMode == TRUE)
		{
			hWnd = CreateWindow(
				windowName, windowName,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN,
				(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
				rc.right,
				rc.bottom,
				NULL, NULL, hCurrentInst, NULL);
		}
		else
		{
			hWnd = CreateWindow(
				windowName, windowName,
				WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
				(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
				rc.right,
				rc.bottom,
				NULL, NULL, hCurrentInst, NULL);
		}
	}
	else
	{
		hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_APPWINDOW,
			windowName, windowName,
			WS_POPUP,
			0, 0,
			WindowWidth,
			WindowHeight,
			NULL, NULL, hCurrentInst, NULL);
	}

	return hWnd;
}
#endif

HWND StartWindow(HINSTANCE hCurrentInst, int nCmdShow)
{
	Platform::WindowConfig config;
	config.className = gProtect->m_MainInfo.WindowName;
	config.instance = hCurrentInst;
	config.width = WindowWidth;
	config.height = WindowHeight;
	config.windowed = (g_bUseWindowMode == TRUE);

	return static_cast<HWND>(Platform::GetWindow().Create(config, reinterpret_cast<Platform::WindowProcedure>(&WndProc)));
}

char m_ID[11];
char m_Version[11];
char m_ExeVersion[11];
int  m_SoundOnOff;
int  m_MusicOnOff;
int  m_Resolution;
int	m_nColorDepth;
int	g_iRenderTextType = 0;
int	m_CameraOnOff;

char g_aszMLSelection[MAX_LANGUAGE_NAME_LENGTH] = {'\0'};
std::string g_strSelectedML = "";

BOOL OpenInitFile()
{
	gController.Load();

	//char szTemp[50];
	char szIniFilePath[256+20]="";
	char szCurrentDir[256];

	GetCurrentDirectory(256, szCurrentDir);

	strcpy(szIniFilePath, szCurrentDir);
	if( szCurrentDir[strlen(szCurrentDir)-1] == '\\' ) 
		strcat(szIniFilePath, "config.ini");
	else strcat(szIniFilePath, "\\config.ini");

	GetPrivateProfileString ("LOGIN", "Version", "", m_Version, 11, szIniFilePath);

	szServerIpAddress = new char[32];

	memset(szServerIpAddress, 0, sizeof(char) * 32);
	memcpy(szServerIpAddress, gProtect->m_MainInfo.IpAddress, sizeof(char) * 32);

	g_ServerPort = gProtect->m_MainInfo.IpAddressPort;

	Version[0] = gProtect->m_MainInfo.ClientVersion[0] + 1;
	Version[1] = gProtect->m_MainInfo.ClientVersion[2] + 2;
	Version[2] = gProtect->m_MainInfo.ClientVersion[3] + 3;
	Version[3] = gProtect->m_MainInfo.ClientVersion[5] + 4;
	Version[4] = gProtect->m_MainInfo.ClientVersion[6] + 5;

	memcpy(Serial, gProtect->m_MainInfo.ClientSerial, sizeof(Serial));

	char *lpszCommandLine = GetCommandLine();
	char lpszFile[MAX_PATH];
	if ( GetFileNameOfFilePath( lpszFile, lpszCommandLine))
	{
		WORD wVersion[4];
		if ( GetFileVersion( lpszFile, wVersion))
		{
			sprintf( m_ExeVersion, "%d.%02d", wVersion[0], wVersion[1]);
			if ( wVersion[2] > 0)
			{
                char lpszMinorVersion[3] = "a";
                if ( wVersion[2]>26 )
                {
                    lpszMinorVersion[0] = 'A';
				    lpszMinorVersion[0] += ( wVersion[2] - 27 );
                    lpszMinorVersion[1] = '+';
                }
                else
                {
				    lpszMinorVersion[0] += ( wVersion[2] - 1);
                }
				strcat( m_ExeVersion, lpszMinorVersion);
			}
		}
		else
		{
			strcpy( m_ExeVersion, m_Version);
		}
	}
	else
	{
		strcpy( m_ExeVersion, m_Version);
	}

//#ifdef _DEBUG

	m_ID[0] = '\0';
	m_SoundOnOff = 1;
	m_MusicOnOff = 1;
	m_Resolution = 0;
	m_nColorDepth = 0;

	HKEY hKey;
	DWORD dwDisp;
	DWORD dwSize;
	if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, & hKey, &dwDisp))
	{
		dwSize = 11;
		if ( RegQueryValueEx (hKey, "ID", 0, NULL, (LPBYTE)m_ID, & dwSize) != ERROR_SUCCESS)
		{
		}
		dwSize = sizeof ( int);
		if ( RegQueryValueEx (hKey, "SoundOnOff", 0, NULL, (LPBYTE) & m_SoundOnOff, &dwSize) != ERROR_SUCCESS)
		{
			m_SoundOnOff = true;
		}
		dwSize = sizeof ( int);
		if ( RegQueryValueEx (hKey, "MusicOnOff", 0, NULL, (LPBYTE) & m_MusicOnOff, &dwSize) != ERROR_SUCCESS)
		{
			m_MusicOnOff = false;
		}
		dwSize = sizeof ( int);
		if ( RegQueryValueEx (hKey, "Resolution", 0, NULL, (LPBYTE) & m_Resolution, &dwSize) != ERROR_SUCCESS)
			m_Resolution = 1;

		if (0 == m_Resolution)
			m_Resolution = 1;

	    if ( RegQueryValueEx (hKey, "ColorDepth", 0, NULL, (LPBYTE) & m_nColorDepth, &dwSize) != ERROR_SUCCESS)
	    {
		    m_nColorDepth = 0;
	    }
		dwSize = sizeof ( int);
		if ( RegQueryValueEx (hKey, "TextOut", 0, NULL, (LPBYTE) & g_iRenderTextType, &dwSize) != ERROR_SUCCESS)
		{
			g_iRenderTextType = 0;
		}

		g_iChatInputType = 1;

#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		dwSize = sizeof ( int);
		if ( RegQueryValueEx (hKey, "WindowMode", 0, NULL, (LPBYTE) & g_bUseWindowMode, &dwSize) != ERROR_SUCCESS)
		{
			g_bUseWindowMode = FALSE;
		}
#endif // USER_WINDOW_MODE

		dwSize = MAX_LANGUAGE_NAME_LENGTH;
		if ( RegQueryValueEx (hKey, "LangSelection", 0, NULL, (LPBYTE)g_aszMLSelection, &dwSize) != ERROR_SUCCESS)
		{
			strcpy(g_aszMLSelection, "Eng");
		}
		g_strSelectedML = g_aszMLSelection;
	}
	RegCloseKey( hKey);

	switch(m_Resolution)
	{
	case 0:WindowWidth = 640; WindowHeight = 480; break;
	case 1:WindowWidth = 800; WindowHeight = 600; break;
	case 2:WindowWidth = 1024; WindowHeight = 768; break;
	case 3:WindowWidth = 1280; WindowHeight = 960; break;
	case 4:WindowWidth = 1360; WindowHeight = 768; break;
	case 5:WindowWidth = 1440; WindowHeight = 900; break;
	case 6:WindowWidth = 1600; WindowHeight = 900; break;
	case 7:WindowWidth = 1680; WindowHeight = 1050; break;
	case 8:WindowWidth = 1920; WindowHeight = 1080; break;
	}
	
	if (m_Resolution > 3)
	{
		g_fScreenRate_x = 1.6f;		// ¡Ø
		g_fScreenRate_y = 1.6f;
	}
	else
	{
		g_fScreenRate_x = (float)WindowHeight / 480;		// ¡Ø
		g_fScreenRate_y = (float)WindowHeight / 480;
	}

	GWidescreen.Init();

	return TRUE;
}

BOOL Util_CheckOption( char *lpszCommandLine, unsigned char cOption, char *lpszString)
{
	unsigned char cComp[2];
	cComp[0] = cOption; cComp[1] = cOption;
	if ( islower( ( int)cOption))
	{
		cComp[1] = toupper( ( int)cOption);
	}
	else if ( isupper( ( int)cOption))
	{
		cComp[1] = tolower( ( int)cOption);
	}

	int nFind = ( int)'/';
	unsigned char *lpFound = ( unsigned char*)lpszCommandLine;
	while ( lpFound)
	{
		lpFound = ( unsigned char*)strchr( ( char*)( lpFound + 1), nFind);
		if ( lpFound && ( *( lpFound + 1) == cComp[0] || *( lpFound + 1) == cComp[1]))
		{	// ¹ß°ß
			if ( lpszString)
			{
				int nCount = 0;
				for ( unsigned char *lpSeek = lpFound + 2; *lpSeek != ' ' && *lpSeek != '\0'; lpSeek++)
				{
					nCount++;
				}

				memcpy( lpszString, lpFound + 2, nCount);
				lpszString[nCount] = '\0';
			}
			return ( TRUE);
		}
	}

	return ( FALSE);
}

BOOL UpdateFile( char *lpszOld, char *lpszNew)
{
	SetFileAttributes(lpszOld, FILE_ATTRIBUTE_NORMAL);
	SetFileAttributes(lpszNew, FILE_ATTRIBUTE_NORMAL);

	DWORD dwStartTickCount = ::GetTickCount();
	while(::GetTickCount() - dwStartTickCount < 5000) {
		if ( CopyFile( lpszOld, lpszNew, FALSE))
		{	// ¼º°ø
			DeleteFile( lpszOld);
			return ( TRUE);
		}
	}
	g_ErrorReport.Write("%s to %s CopyFile Error : %d\r\n", lpszNew, lpszOld, GetLastError());
	return ( FALSE);
}

#include <tlhelp32.h>

BOOL KillExeProcess( char *lpszExe)
{
	HANDLE hProcessSnap = NULL; 
    BOOL bRet = FALSE; 
    PROCESSENTRY32 pe32 = { 0 }; 
 
    //  Take a snapshot of all processes in the system. 

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 

    if (hProcessSnap == INVALID_HANDLE_VALUE) 
        return (FALSE); 
 
    //  Fill in the size of the structure before using it. 

    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    //  Walk the snapshot of the processes, and for each process, 
    //  display information. 

    if (Process32First(hProcessSnap, &pe32)) 
    {
        do 
        { 
			if(stricmp(pe32.szExeFile, lpszExe) == 0)
			{
				HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

				if(process)
				{
					TerminateProcess(process, 0);
				}
			}
        } while (Process32Next(hProcessSnap, &pe32)); 
        bRet = TRUE; 
    } 
    else 
        bRet = FALSE;    // could not walk the list of processes 
 
    // Do not forget to clean up the snapshot object. 

    CloseHandle (hProcessSnap);

	return bRet;
}

char g_lpszCmdURL[50];
BOOL GetConnectServerInfo( PSTR szCmdLine, char *lpszURL, WORD *pwPort)
{
	char lpszTemp[256] = {0, };
	if( Util_CheckOption( szCmdLine, 'y', lpszTemp))
	{
		BYTE bySuffle[] = { 0x0C, 0x07, 0x03, 0x13 };

		for(int i=0; i<(int)strlen(lpszTemp); i++)
			lpszTemp[i] -= bySuffle[i%4];
		strcpy(lpszURL, lpszTemp);

		if( Util_CheckOption( szCmdLine, 'z', lpszTemp)) 
		{
			for(int j=0; j<(int)strlen(lpszTemp); j++)
				lpszTemp[j] -= bySuffle[j%4];
			*pwPort = atoi( lpszTemp);
		}
		g_ErrorReport.Write("[Virtual Connection] Connect IP : %s, Port : %d\r\n", lpszURL, *pwPort);
		return (TRUE);
	}
	if ( !Util_CheckOption( szCmdLine, 'u', lpszTemp))
	{
		return ( FALSE);
	}
	strcpy( lpszURL, lpszTemp);
	if ( !Util_CheckOption( szCmdLine, 'p', lpszTemp))
	{
		return ( FALSE);
	}
	*pwPort = atoi( lpszTemp);

	return ( TRUE);
}


extern int TimeRemain;
BOOL g_bInactiveTimeChecked = FALSE;
void MoveObject(OBJECT *o);

#ifdef _WIN32
#include <dbghelp.h>
#endif //_WIN32
#include <Utilities/CpuUsage.h>

#pragma comment(lib,"dbghelp.lib")

bool ExceptionCallback(_EXCEPTION_POINTERS* pExceptionInfo )
{
	// MiniDumpWriteDump captures this handler's context on some legacy x86
	// systems. Preserve the original fault context and its stack separately so
	// it can be symbolized after a client crash.
	if (pExceptionInfo != NULL && pExceptionInfo->ContextRecord != NULL)
	{
		HANDLE contextFile = CreateFile("CrashContext.bin", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (contextFile != INVALID_HANDLE_VALUE)
		{
			DWORD written = 0;
			const DWORD stackSize = 8192;
			WriteFile(contextFile, pExceptionInfo->ExceptionRecord, sizeof(EXCEPTION_RECORD), &written, NULL);
			WriteFile(contextFile, pExceptionInfo->ContextRecord, sizeof(CONTEXT), &written, NULL);
			WriteFile(contextFile, &pExceptionInfo->ContextRecord->Esp, sizeof(DWORD), &written, NULL);
			WriteFile(contextFile, reinterpret_cast<const void*>(pExceptionInfo->ContextRecord->Esp), stackSize, &written, NULL);
			CloseHandle(contextFile);
		}
	}

	char path[MAX_PATH];

	SYSTEMTIME SystemTime;

	GetLocalTime(&SystemTime);

	wsprintf(path, "%d-%d-%d_%dh%dm%ds.dmp", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

	HANDLE file = CreateFile(path, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (file != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;

		mdei.ThreadId = GetCurrentThreadId();

		mdei.ExceptionPointers = pExceptionInfo;

		mdei.ClientPointers = 0;

		if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, (MINIDUMP_TYPE)(MiniDumpScanMemory + MiniDumpWithIndirectlyReferencedMemory), &mdei, 0, 0) != 0)
		{
			CloseHandle(file);
			return EXCEPTION_EXECUTE_HANDLER;
		}
	}
	CloseHandle(file);

	return EXCEPTION_CONTINUE_SEARCH;
}

double CPU_AVG = 0.0;
void RecordCpuUsage()
{
	constexpr int max_recordings = 60;
	double CPU_Recordings[max_recordings] = { 0.0 };
	double currentAvg = 0.0;
	double sum = 0.0;
	int count = 0;
	int numFilled = 0;
	auto lastUpdateTime = std::chrono::steady_clock::now();

	while (!Destroy)
	{
		double currentUsage = CpuUsage::Instance()->GetUsage();

		currentUsage = max(0.0, min(100.0, currentUsage));

		// Subtract the old value to maintain the sum
		sum -= CPU_Recordings[count];

		sum += currentUsage;

		CPU_Recordings[count] = currentUsage;

		// Update the count (wrap around when full - FIFO behavior)
		count = (count + 1) % max_recordings;

		if (numFilled < max_recordings)
		{
			numFilled++;
		}

		// Calculate the current average
		currentAvg = sum / numFilled;

		// Update the CPU_AVG every 250 ms
		auto currentTime = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastUpdateTime).count() >= 250)
		{
			CPU_AVG = currentAvg;
			lastUpdateTime = currentTime;
		}

		// Sleep to match a 60Hz frame rate as the basis
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow)
{
	MSG msg;

	gController.Instance = hInstance;

	// Keep a local minidump when the legacy client faults.  The dump is written
	// beside Main.exe and lets us identify the exact crashing function.
	leaf::AttachExceptionHandler(ExceptionCallback);

	gProtect = new CProtect;

	if (gProtect->ReadMainFile("Data\\Configs\\Configs.xtm") == 0)
	{
		MessageBox(0, "Erro de configura\xE7\xE3o. Consulte o suporte! #1", "Erro de inicializa\xE7\xE3o!", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}
	if (gProtect->ReadCustomJewelConfig(("Data\\Configs\\CustomJewel.xtm")) == 0)
	{
		MessageBox(0, "Erro de configura\xE7\xE3o. Consulte o suporte! #2", "Erro de inicializa\xE7\xE3o!", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}

	gTrayMode.Load();

	gCustomJewel.Load(gProtect->m_CustomJewel.CustomJewelInfo);

	gItemManager.Init();

	gHelperSystem.Init();

	gDarkSpirit.Init();

	gCustomItemPosition.Init();

	gCustomItemSize.Init();

	gDescriptions.Init();

	Visuals.Init();

	gDisableExcellent.Init();

	gCustomBow.Init();

	gCustomWing.Init();

	gCustomEffects.Init();

	gCCustomSetEffect.Init();

	gCustomItemFloor.Init();

	gCustomItemForce.Init();

	gRenderModel.Init();

	gCustomServerName.Init();

	gCreateFont.Init();

	gMessageColor.Init();

	gMonsters.Init();

	gMonsterName.Init();

	gMonsterGlow.Init();

	gMonsterEffect.Init();

	gCustomCape.Init();

	gJewelStack.Init();

	gCounterItem.Init();

	gCharacterList.Init();

	gPatente.Init();

	gControlSlot.Init();

	char lpszExeVersion[256] = "unknown";

	char *lpszCommandLine = GetCommandLine();
	char lpszFile[MAX_PATH];
	WORD wVersion[4] = { 0,};
	if ( GetFileNameOfFilePath( lpszFile, lpszCommandLine))
	{
		if ( GetFileVersion( lpszFile, wVersion))
		{
			sprintf( lpszExeVersion, "%d.%02d", wVersion[0], wVersion[1]);
			if ( wVersion[2] > 0)
			{
				char lpszMinorVersion[2] = "a";
				lpszMinorVersion[0] += ( wVersion[2] - 1);
				strcat( lpszExeVersion, lpszMinorVersion);
			}
		}
	}

#ifdef CONSOLE
	Console.Init();
#endif
	g_ErrorReport.Write( "\r\n");
	g_ErrorReport.WriteLogBegin();
	g_ErrorReport.AddSeparator();
	g_ErrorReport.Write( "Mu online %s (%s) executed. (%d.%d.%d.%d)\r\n", lpszExeVersion, "Eng", wVersion[0], wVersion[1], wVersion[2], wVersion[3]);

	g_ConsoleDebug->Write(MCD_NORMAL, "Mu Online (Version: %d.%d.%d.%d)", wVersion[0], wVersion[1], wVersion[2], wVersion[3]);

	g_ErrorReport.WriteCurrentTime();
	ER_SystemInfo si;
	ZeroMemory( &si, sizeof ( ER_SystemInfo));
	GetSystemInfo( &si);
	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteSystemInfo( &si);
	g_ErrorReport.AddSeparator();

	// PKD_ADD_BINARY_PROTECTION
	VM_START
	WORD wPortNumber;	
	if ( GetConnectServerInfo( szCmdLine, g_lpszCmdURL, &wPortNumber))
	{
		szServerIpAddress = g_lpszCmdURL;
		g_ServerPort = wPortNumber;
	}
	VM_END

	if ( !OpenMainExe())
	{
		return false;
	}

	// PKD_ADD_BINARY_PROTECTION
	VM_START
	g_SimpleModulusCS.LoadEncryptionKey( "Data\\Enc1.dat");
	g_SimpleModulusSC.LoadDecryptionKey( "Data\\Dec2.dat");
	VM_END

	g_ErrorReport.Write( "> To read config.ini.\r\n");

	if( OpenInitFile() == FALSE )
	{
		g_ErrorReport.Write( "config.ini read error\r\n");
		return false;
	}

	pMultiLanguage = new CMultiLanguage(g_strSelectedML);

	if (g_iChatInputType == 1)
		ShowCursor(FALSE);

	g_ErrorReport.Write( "> Enum display settings.\r\n");
	DEVMODE DevMode;
	DEVMODE* pDevmodes;
	int nModes = 0;
	while (EnumDisplaySettings(NULL, nModes, &DevMode)) nModes++;
	pDevmodes = new DEVMODE[nModes+1];
	nModes = 0;
	while (EnumDisplaySettings(NULL, nModes, &pDevmodes[nModes])) nModes++;

	DWORD dwBitsPerPel = 16;
	for(int n1=0; n1<nModes; n1++)
	{
		if(pDevmodes[n1].dmBitsPerPel == 16 && m_nColorDepth == 0) {
			dwBitsPerPel = 16; break;
		}
		if(pDevmodes[n1].dmBitsPerPel == 24 && m_nColorDepth == 1) {
			dwBitsPerPel = 24; break;
		}
		if(pDevmodes[n1].dmBitsPerPel == 32 && m_nColorDepth == 1) {
			dwBitsPerPel = 32; break;
		}
	}

#ifdef ENABLE_FULLSCREEN
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
#endif	// USER_WINDOW_MODE
	{
		for(int n2=0; n2<nModes; n2++)
		{
			if(pDevmodes[n2].dmPelsWidth==WindowWidth && pDevmodes[n2].dmPelsHeight==WindowHeight && pDevmodes[n2].dmBitsPerPel == dwBitsPerPel)
			{
				g_ErrorReport.Write( "> Change display setting %dx%d.\r\n", pDevmodes[n2].dmPelsWidth, pDevmodes[n2].dmPelsHeight);
				ChangeDisplaySettings(&pDevmodes[n2],0);
				break;
			}
		}
	}
#endif //ENABLE_FULLSCREEN

	delete [] pDevmodes;

	g_ErrorReport.Write( "> Screen size = %d x %d.\r\n", WindowWidth, WindowHeight);

    g_hInst = hInstance;
    g_hWnd = StartWindow(hInstance,nCmdShow);
	g_ErrorReport.Write( "> Start window success.\r\n");

    if ( !CreateOpenglWindow())
	{
		return FALSE;
	}

	g_ErrorReport.Write( "> OpenGL init success.\r\n");
	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteOpenGLInfo();
	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteSoundCardInfo(); 

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

	//g_ErrorReport.WriteImeInfo( g_hWnd);
	g_ErrorReport.AddSeparator();
	
	switch(WindowWidth)
	{
		case 640:  FontHeight = 12; break;
		case 800:  FontHeight = 13; break;
		case 1024: FontHeight = 13; break;
		case 1200: FontHeight = 16; break;
		case 1280: FontHeight = 16; break;
		case 1365: FontHeight = 16; break;
		case 1400: FontHeight = 16; break;
		case 1600: FontHeight = 16; break;
		case 1920: FontHeight = 17; break;
		default: FontHeight = 16; break;
	}
	
	// -fpslimit=N aplica um teto de N FPS; ausente mantem ilimitado, que e o
	// comportamento medido em todas as capturas anteriores. O teto so passou a
	// funcionar depois da correcao em SetTargetFps — ele era descartado antes.
	{
		double targetFps = -1.0; // unlimited
		const char* limitArgument = ::strstr(::GetCommandLineA(), "-fpslimit=");
		if (limitArgument != NULL)
		{
			const int requested = atoi(limitArgument + strlen("-fpslimit="));
			if (requested >= 10 && requested <= 1000)
				targetFps = static_cast<double>(requested);
			else
				// Um `-fpslimit=6O` (letra O) vira 6 no atoi e cai fora da faixa. Sem
				// este aviso a captura sai SEM teto e o unico sinal e a coluna
				// fps_limit valendo -1 -- facil de nao ver justamente na rodada cujo
				// proposito era comparar com e sem teto.
				g_ErrorReport.Write("-fpslimit=%d fora da faixa 10..1000; sem teto de FPS\r\n", requested);
		}
		SetTargetFps(targetFps);
	}

	FontHeight = static_cast<int>(std::ceil(12 + ((WindowHeight - 480) / 200.f)));
	
	int nFixFontHeight = WindowHeight <= 600 ? 14 : 15;
	int nFixFontSize;
	int iFontSize;

	iFontSize = FontHeight - 1;
	nFixFontSize = nFixFontHeight - 1;

	gCreateFont.SetFont(iFontSize, FontHeight, nFixFontSize, nFixFontHeight);

	//g_hFont = CreateFont(iFontSize + gCreateFont.m_fontInfo[0].AddSize, 0, 0, 0, (gCreateFont.m_fontInfo[0].Bold == 1 ? FW_BOLD : FW_NORMAL), 0, 0, 0, gCreateFont.m_fontInfo[0].CharSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, gCreateFont.m_fontInfo[0].Name.c_str());
	//g_hFontBold = CreateFont(iFontSize + gCreateFont.m_fontInfo[1].AddSize, 0, 0, 0, (gCreateFont.m_fontInfo[1].Bold == 1 ? FW_BOLD : FW_NORMAL), 0, 0, 0, gCreateFont.m_fontInfo[1].CharSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, gCreateFont.m_fontInfo[1].Name.c_str());
	//g_hFontBig = CreateFont((iFontSize * 2) + gCreateFont.m_fontInfo[2].AddSize, 0, 0, 0, (gCreateFont.m_fontInfo[2].Bold == 1 ? FW_BOLD : FW_NORMAL), 0, 0, 0, gCreateFont.m_fontInfo[2].CharSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, gCreateFont.m_fontInfo[2].Name.c_str());
	//g_hFixFont = CreateFont(nFixFontSize + gCreateFont.m_fontInfo[3].AddSize, 0, 0, 0, (gCreateFont.m_fontInfo[3].Bold == 1 ? FW_BOLD : FW_NORMAL), 0, 0, 0, gCreateFont.m_fontInfo[3].CharSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, gCreateFont.m_fontInfo[3].Name.c_str());
	
	setlocale( LC_ALL, "english");

	CInput::Instance().Create(g_hWnd, WindowWidth, WindowHeight);

	g_pNewUISystem->Create();

	if(m_MusicOnOff)
	{
		wzAudioCreate(g_hWnd);
		wzAudioOption(WZAOPT_STOPBEFOREPLAY, 1);
	}

	if(m_SoundOnOff)
	{
		InitDirectSound(g_hWnd);
		leaf::CRegKey regkey;
		regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config");
		DWORD value;
		if(!regkey.ReadDword("VolumeLevel", value))
		{
			value = 5;	//. default setting
			regkey.WriteDword("VolumeLevel", value);
		}
		if(value<0 || value>=10)
			value = 5;
		
		g_pOption->SetVolumeLevel(int(value));
		SetEffectVolumeLevel(g_pOption->GetVolumeLevel());
	}

	SetTimer(g_hWnd, HACK_TIMER, 20*1000, NULL);

	srand((unsigned)time(NULL));
	for(int i=0;i<100;i++)
		RandomTable[i] = rand()%360;

	//memorydump[0]
	RendomMemoryDump = new BYTE [rand()%100+1];


	GateAttribute				= new GATE_ATTRIBUTE [MAX_GATES];
	SkillAttribute				= new SKILL_ATTRIBUTE[MAX_SKILLS];

	//memorydump[1]
	ItemAttRibuteMemoryDump		= new ITEM_ATTRIBUTE [MAX_ITEM+1024];
	ItemAttribute				= ((ITEM_ATTRIBUTE*)ItemAttRibuteMemoryDump)+rand()%1024;

	//memorydump[2]
	CharacterMemoryDump			= new CHARACTER      [MAX_CHARACTERS_CLIENT+1+128];
	CharactersClient			= ((CHARACTER*)CharacterMemoryDump)+rand()%128;
	CharacterMachine			= new CHARACTER_MACHINE;

	memset(GateAttribute       ,0,sizeof(GATE_ATTRIBUTE   )*(MAX_GATES              ));
	memset(ItemAttribute       ,0,sizeof(ITEM_ATTRIBUTE   )*(MAX_ITEM               ));
	memset(SkillAttribute      ,0,sizeof(SKILL_ATTRIBUTE  )*(MAX_SKILLS             ));
	memset(CharacterMachine    ,0,sizeof(CHARACTER_MACHINE));

    CharacterAttribute   = &CharacterMachine->Character;
    CharacterMachine->Init();
	Hero = &CharactersClient[0];	

	if (g_iChatInputType == 1)
	{
		 g_pMercenaryInputBox = new CUIMercenaryInputBox;
		 g_pSingleTextInputBox = new CUITextInputBox;
		 g_pSinglePasswdInputBox = new CUITextInputBox;
	}
	
	g_pChatRoomSocketList = new CChatRoomSocketList;
	g_pUIManager = new CUIManager;
	g_pUIMapName = new CUIMapName;	// rozy
	g_pTimer = new CTimer();

#ifdef MOVIE_DIRECTSHOW
	g_pMovieScene = new CMovieScene;
#endif // MOVIE_DIRECTSHOW

#ifdef NEW_MUHELPER_ON
	pAIController = new CAIController(Hero);
#endif

	g_BuffSystem = BuffStateSystem::Make();

	g_MapProcess = MapProcess::Make();

	g_petProcess = PetProcess::Make();

	CUIMng::Instance().Create();

	if (g_iChatInputType == 1)
	{
		g_pMercenaryInputBox->Init(g_hWnd);
		g_pSingleTextInputBox->Init(g_hWnd, 200, 20);
		g_pSinglePasswdInputBox->Init(g_hWnd, 200, 20, 9, TRUE);
		g_pSingleTextInputBox->SetState(UISTATE_HIDE);
		g_pSinglePasswdInputBox->SetState(UISTATE_HIDE);

		g_pMercenaryInputBox->SetFont(g_hFont);
		g_pSingleTextInputBox->SetFont(g_hFont);
		g_pSinglePasswdInputBox->SetFont(g_hFont);

		g_bIMEBlock = FALSE;
		HIMC  hIMC = ImmGetContext(g_hWnd);
		ImmSetConversionStatus(hIMC, IME_CMODE_ALPHANUMERIC, IME_SMODE_NONE );
		ImmReleaseContext(g_hWnd, hIMC);
		SaveIMEStatus();
		g_bIMEBlock = TRUE;
	}
#if (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
	{
		int nOldVal;
		SystemParametersInfo(SPI_SCREENSAVERRUNNING, 1, &nOldVal, 0);
		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &g_iScreenSaverOldValue, 0);
		SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, 300*60, NULL, 0);
	}
#else
#ifdef NDEBUG
#ifndef FOR_WORK
#ifdef ACTIVE_FOCUS_OUT
	if (g_bUseWindowMode == FALSE)
	{
#endif	// ACTIVE_FOCUS_OUT
		int nOldVal; // °ªÀÌ µé¾î°¥ ÇÊ¿ä°¡ ¾øÀ½
		SystemParametersInfo(SPI_SCREENSAVERRUNNING, 1, &nOldVal, 0);  // ´ÜÃàÅ°¸¦ ¸ø¾²°Ô ÇÔ
		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &g_iScreenSaverOldValue, 0);  // ½ºÅ©¸°¼¼ÀÌ¹ö Â÷´Ü
		SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, 300*60, NULL, 0);  // ½ºÅ©¸°¼¼ÀÌ¹ö Â÷´Ü
#ifdef ACTIVE_FOCUS_OUT
	}
#endif	// ACTIVE_FOCUS_OUT
#endif
#endif
#endif	//WINDOWMODE(#else)

#ifdef SAVE_PACKET
	DeleteFile( PACKET_SAVE_FILE);
#endif

#if defined PROTECT_SYSTEMKEY && defined NDEBUG
#ifndef FOR_WORK
	ProtectSysKey::AttachProtectSysKey(g_hInst, g_hWnd);
#endif // !FOR_WORK
#endif // PROTECT_SYSTEMKEY && NDEBUG

	constexpr auto target_resolution = 1;
	auto precise = timeBeginPeriod(target_resolution);

	while (1)
	{
		// O pump e o protocolo estao dentro de frame_total_us (medido topo a topo de
		// RenderScene) mas fora de cpu_us. A v14 deixou 0,4 a 4,1 ms nesse gap sem
		// instrumento, e em world 3 era ele que crescia enquanto cpu_us ficava
		// parado. Medido aqui para fechar a conta.
		const long long pumpStartUs = FrameLoopNowMicroseconds();
		const bool pumpedMessage = Platform::GetWindow().PumpEvents();
		RecordFramePumpUs(FrameLoopNowMicroseconds() - pumpStartUs);
		if (pumpedMessage)
		{
			if (Platform::GetWindow().QuitRequested())
				break;
		}
		else
		{
			if (CheckRenderNextFrame())
			{
				//Scene
#if (defined WINDOWMODE)
				if (g_bUseWindowMode || g_bWndActive)
				{
					Platform::RenderLegacySceneFrame(g_hDC);
				}
#ifndef FOR_WORK
				else if (g_bUseWindowMode == FALSE)
				{
					SetForegroundWindow(g_hWnd);
					SetFocus(g_hWnd);

					if (g_iInactiveWarning > 1)
					{
						SetTimer(g_hWnd, WINDOWMINIMIZED_TIMER, 1 * 1000, NULL);
						PostMessage(g_hWnd, WM_CLOSE, 0, 0);
					}
					else
					{
						g_iInactiveWarning++;
						g_bMinimizedEnabled = TRUE;
						ShowWindow(g_hWnd, SW_MINIMIZE);
						g_bMinimizedEnabled = FALSE;
						ShowWindow(g_hWnd, SW_MAXIMIZE);
					}
				}
#endif//FOR_WORK
#else//WINDOWMODE
				if (g_bWndActive)
					Scene(g_hDC);

#endif	//WINDOWMODE(#else)
			}
			else
			{
				const long long idlePumpStartUs = FrameLoopNowMicroseconds();
				const bool idlePumped = Platform::GetWindow().PumpEvents();
				RecordFramePumpUs(FrameLoopNowMicroseconds() - idlePumpStartUs);
				if (!idlePumped)
				{
					// Com -fpslimit=N este caminho volta a ser alcancavel: e o sleep
					// do limitador. Sem limite ele so faz yield. Medido em coluna
					// propria porque espera deliberada nao pode parecer trabalho nao
					// instrumentado em us_frame_gap.
					const long long limiterStartUs = FrameLoopNowMicroseconds();
					WaitForNextActivity(precise == TIMERR_NOERROR);
					RecordFrameLimiterUs(FrameLoopNowMicroseconds() - limiterStartUs);
				}
			}
		}
		const long long protocolStartUs = FrameLoopNowMicroseconds();
	#ifdef NEW_PROTOCOL_SYSTEM
		if(SceneFlag < CHARACTER_SCENE)
			ProtocolCompiler();

		g_pChatRoomSocketList->ProtocolCompile();
		gProtocolSend.RecvMessage();
	#else
		ProtocolCompiler();
		g_pChatRoomSocketList->ProtocolCompile();
	#endif
		RecordFrameProtocolUs(FrameLoopNowMicroseconds() - protocolStartUs);

		
    } // while( 1 )

	if (precise == TIMERR_NOERROR)
	{
		timeEndPeriod(target_resolution);
	}

	std::thread cpuUsageRecorder(RecordCpuUsage);
	
	DestroyWindow();

    return msg.wParam;
}

