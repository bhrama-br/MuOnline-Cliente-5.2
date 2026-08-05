/*******************************************************************************
*	ÀÛ ¼º ÀÚ : ÁøÇýÁø
*	ÀÛ ¼º ÀÏ : 2009.06.10
*	³»    ¿ë : Download¿¡ ÇÊ¿äÇÑ Á¤º¸ ¼³Á¤
*******************************************************************************/

#pragma once

#ifdef _WIN32
#include <Wininet.h>
#else
// WinInet e exclusivo do Windows; o download da loja precisa de uma
// implementacao HTTP na camada Platform para Web/Android.
#include "Platform/WindowsCompat.h"
#endif //_WIN32

#define DL_DEFAULT_BUFFER_SIZE			4096

typedef enum _DownloaderType
{
	FTP,
	HTTP,
} DownloaderType;

class DownloadFileInfo
{
public:
// Constructor, Destructor

	DownloadFileInfo();
	~DownloadFileInfo();

// Get Function
	//			ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	TCHAR *		GetFileName();
	TCHAR *		GetLocalFilePath();
	TCHAR *		GetRemoteFilePath();
	TCHAR *		GetTargetDirPath();
	ULONGLONG	GetFileLength();
// Set Function
	//		ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	void	SetFilePath(TCHAR * szFileName, 
						TCHAR * szLocalFilePath, 
						TCHAR * szRemoteFilePath,
						TCHAR * szTargerDirPath);
	void	SetFileLength(ULONGLONG uFileLength);

private:
// Member Object

	//			ï¿½ï¿½ï¿½ï¿½ ï¿½Ì¸ï¿½
	TCHAR		m_szFileName[MAX_PATH];
	//			ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½Ã¼ ï¿½ï¿½ï¿½
	TCHAR		m_szLocalFilePath[MAX_PATH];
	//			ï¿½ï¿½ï¿½ï¿½Æ® ï¿½ï¿½Ã¼ ï¿½ï¿½ï¿½
	TCHAR		m_szRemoteFilePath[INTERNET_MAX_URL_LENGTH];
	//			ï¿½ï¿½Ä¡ Ç®ï¿½î³¾ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½
	TCHAR		m_szTargerDirPath[MAX_PATH];
	//			ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	ULONGLONG	m_uFileLength;
};

class DownloadServerInfo
{
public:
// Constructor, Destructor

	DownloadServerInfo();
	~DownloadServerInfo();

	
// Get Function

	//				ï¿½ï¿½ï¿½ï¿½ ï¿½Ö¼ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	TCHAR *			GetServerURL();
	//				ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	TCHAR *			GetUserID();
	//				¼­¹ö Á¢¼Ó °èÁ¤ ºñ¹ø °¡Á®¿À±â
	TCHAR *			GetPassword();
	//				ï¿½ï¿½Æ® ï¿½ï¿½È£ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	INTERNET_PORT	GetPort();
	//				Å¸ï¿½ï¿½Îµï¿½ Å¸ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	DownloaderType	GetDownloaderType();
	//				ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	DWORD			GetReadBufferSize();
	//				Ä¿ï¿½ï¿½Æ® Å¸ï¿½Ó¾Æ¿ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
	DWORD			GetConnectTimeout();
	//				ï¿½ï¿½ï¿½î¾²ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	BOOL			IsOverWrite();
	//				ï¿½Ð½Ãºï¿½ ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	BOOL			IsPassive();


// Set Function

	//				ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	void			SetServerInfo(TCHAR *			szServerURL, 
								  INTERNET_PORT		nPort,
								  TCHAR *			szUserID, 
								  TCHAR *			szPassword);
	//				ï¿½Ù¿ï¿½Îµï¿½ Å¸ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	void			SetDownloaderType(DownloaderType dwDownloaderType);
	//				ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	void			SetReadBufferSize(DWORD dwReadBufferSize);
	//				·ÎÄÃ ÆÄÀÏ Á¸Àç ½Ã µ¤¾î¾²±â ¼³Á¤
	void			SetOverWrite(BOOL bOverWrite);
	//				ï¿½Ð½Ãºï¿½ ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
	void			SetPassiveMode(BOOL bPassive);
	//				Ä¿ï¿½ï¿½Æ® Å¸ï¿½Ó¾Æ¿ï¿½ ï¿½ï¿½ï¿½ï¿½
	void			SetConnectTimeout(DWORD dwConnectTimeout);


private:
// Member Object

	// 							Server ÁÖ¼Ò
	TCHAR						m_szServerURL[INTERNET_MAX_URL_LENGTH];
	// 							ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½
	TCHAR						m_szUserID[INTERNET_MAX_USER_NAME_LENGTH];	
	// 							Á¢¼Ó °èÁ¤ Password
	TCHAR						m_szPassword[INTERNET_MAX_PASSWORD_LENGTH];
	// 							Á¢¼Ó Æ÷Æ® default = INTERNET_DEFAULT_FTP_PORT (21)
	INTERNET_PORT				m_nPort;
	//							´Ù¿î·Î´õ Å¸ÀÔ - ÇÁ·ÎÅäÄÝ
	DownloaderType				m_DownloaderType;
	// 							´Ù¿î·Îµå ÆÐÅ¶ »çÀÌÁî Á¦ÇÑ default = 4096
	DWORD						m_dwReadBufferSize;
	//							Local File Á¸ÀçÇÒ °æ¿ì µ¤¾î¾²±â ¼³Á¤ default = TRUE
	BOOL						m_bOverWrite;
	//							Passive ¼³Á¤ default = FALSE
	BOOL						m_bPassive;
	//							Ä¿ï¿½ï¿½Æ® Å¸ï¿½Ó¾Æ¿ï¿½
	DWORD						m_dwConnectTimeout;
};

