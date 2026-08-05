/*******************************************************************************
*	ÀÛ ¼º ÀÚ : ÁøÇýÁø
*	ÀÛ ¼º ÀÏ : 2009.07.07
*	³»    ¿ë : Connecter Interface
*******************************************************************************/

#pragma once

#include "GameShop\ShopListManager\interface\WZResult\WZResult.h"
#include "GameShop\ShopListManager\interface\DownloadInfo.h"

class IConnecter
{
public:
// Constructor, Destructor

	IConnecter(DownloadServerInfo *	pServerInfo,
			   DownloadFileInfo *	pFileInfo)
			   :	m_pServerInfo(pServerInfo),
					m_pFileInfo(pFileInfo)
	{};
	~IConnecter(){};


// abstract Function
	
	//						¼¼¼Ç
	virtual WZResult		CreateSession(HINTERNET& hSession) = 0;
	//						Ä¿³¼Æ®
	virtual WZResult		CreateConnection(HINTERNET& hSession, 
											 HINTERNET& hConnection) = 0;
	//						´Ù¿î·Îµå ÆÄÀÏ ¿ÀÇÂ & »çÀÌÁî °¡Á®¿À±â
	virtual WZResult		OpenRemoteFile(HINTERNET& hConnection, 
										   HINTERNET& hRemoteFile, 
										   ULONGLONG& nFileLength) = 0;
	//						¸®¸ðÆ® ÆÄÀÏ ÀÐ±â
	virtual WZResult		ReadRemoteFile(HINTERNET& hRemoteFile, 
										   BYTE* byReadBuffer, 
										   DWORD* dwBytesRead) = 0;


protected:
// Member Object

	//						°á°ú..
	WZResult 				m_Result;
	//						´Ù¿î·Îµå ¼­¹ö Á¤º¸ °´Ã¼
	DownloadServerInfo *	m_pServerInfo;
	//						´Ù¿î·Îµå ÆÄÀÏ Á¤º¸ °´Ã¼
	DownloadFileInfo *		m_pFileInfo;
};

