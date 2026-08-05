/*******************************************************************************
*	ÀÛ ¼º ÀÚ : ÁøÇýÁø
*	ÀÛ ¼º ÀÏ : 2009.06.10
*	³»    ¿ë : FileDownloader
*				File ´ÜÀ§ ´Ù¿î·Îµå ±â´É Á¦°ø
*******************************************************************************/

#pragma once

#include "GameShop\ShopListManager\interface\IConnecter.h"
#include "GameShop\ShopListManager\interface\IDownloaderStateEvent.h"

class FileDownloader
{
public:
// Constructor, Destructor

	FileDownloader(IDownloaderStateEvent *	pStateEvent,
				   DownloadServerInfo *		pServerInfo,
				   DownloadFileInfo *		pFileInfo);
	~FileDownloader();


// public Function

	//					´Ù¿î·Îµå ÁßÁö
	void				Break();
	//					ÁöÁ¤ÇÑ ÆÄÀÏ ´Ù¿î·Îµå ½ÇÇà : ¼¼¼Ç, Ä¿³¼Æ®, ÆÄÀÏ ¿ÀÇÂ ¸ðµÎ Ã³¸®
	WZResult			DownloadFile();


private:
// private Function
	
	//					ÁøÇà ¿©ºÎ
	BOOL				CanBeContinue();
	//					¸±¸®Áî
	void				Release();

	//					Ä¿³ØÅÍ »ý¼º
	IConnecter *		CreateConnecter();
	//					Á¢¼Ó Ã³¸®
	WZResult 			CreateConnection();
	static unsigned int __stdcall RunConnectThread(LPVOID pParam);
	WZResult 			Connection();

	//					Àü¼Û Ã³¸®
	WZResult 			TransferRemoteFile();

	//					·ÎÄÃ ÆÄÀÏ »ý¼º
	WZResult 			CreateLocalFile();
	//					´Ù¿î·Îµå ÆÄÀÏ ÀÐ±â
	WZResult 			ReadRemoteFile(BYTE* byReadBuffer, DWORD* dwBytesRead);
	//					·ÎÄÃ ÆÄÀÏ ¾²±â
	WZResult 			WriteLocalFile(BYTE* byReadBuffer, DWORD dwBytesRead);

	//					´Ù¿î·Îµå ½ÃÀÛ ÀÌº¥Æ® º¸³»±â
	void				SendStartedDownloadFileEvent(ULONGLONG nFileLength);
	//					´Ù¿î·Îµå ¿Ï·á ÀÌº¥Æ® º¸³»±â
	void				SendCompletedDownloadFileEvent(WZResult wzResult);
	//					´Ù¿î·Îµå ÁøÇà »óÈ² ÀÌº¥Æ® º¸³»±â : ÆÐÅ¶ ´ÜÀ§
	void				SendProgressDownloadFileEvent(ULONGLONG nTotalBytesRead);


// Member Object

	//							´Ù¿î·Îµå ÁßÁö ÇÃ·¡±×
	volatile BOOL				m_bBreak;
	//							°á°ú..
	WZResult 					m_Result;

	//							´Ù¿î·Îµå »óÅÂ ÀÌº¥Æ® ¹ÞÀ» °´Ã¼
	IDownloaderStateEvent *		m_pStateEvent;
	//							´Ù¿î·Îµå ¼­¹ö Á¤º¸ °´Ã¼
	DownloadServerInfo *		m_pServerInfo;
	//							´Ù¿î·Îµå ÆÄÀÏ Á¤º¸ °´Ã¼
	DownloadFileInfo *			m_pFileInfo;
	//							Ä¿³ØÅÍ
	IConnecter *				m_pConnecter;

	//							WinINet ¼¼¼Ç ÇÚµé
	HINTERNET					m_hSession;
	//							WinINet Ä¿³¼¼Ç ÇÚµé
	HINTERNET					m_hConnection;
	//							¼­¹ö ÆÄÀÏ ÇÚµé
	HINTERNET					m_hRemoteFile;
	//							·ÎÄÃ ÆÄÀÏ ÇÚµé
	HANDLE						m_hLocalFile;
	//							ÆÄÀÏ »çÀÌÁî
	ULONGLONG					m_nFileLength;
};

