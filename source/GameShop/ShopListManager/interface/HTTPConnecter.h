/*******************************************************************************
*	ÀÛ ¼º ÀÚ : ÁøÇýÁø
*	ÀÛ ¼º ÀÏ : 2009.07.07
*	³»    ¿ë : HTTP Connecter
*******************************************************************************/

#pragma once

#include "GameShop\ShopListManager\interface\IConnecter.h"

class HTTPConnecter : public IConnecter
{
public:
// Constructor, Destructor

	HTTPConnecter(DownloadServerInfo *	pServerInfo,
				   DownloadFileInfo *	pFileInfo);
	~HTTPConnecter();


// abstract Function

	//						¼¼¼Ç
	virtual WZResult		CreateSession(HINTERNET& hSession);
	//						Ä¿³¼Æ®
	virtual WZResult		CreateConnection(HINTERNET& hSession, 
											 HINTERNET& hConnection);
	//						´Ù¿î·Îµå ÆÄÀÏ ¿ÀÇÂ & »çÀÌÁî °¡Á®¿À±â
	virtual WZResult		OpenRemoteFile(HINTERNET& hConnection, 
										   HINTERNET& hRemoteFile, 
										   ULONGLONG& nFileLength);
	//						¸®¸ðÆ® ÆÄÀÏ ÀÐ±â
	virtual WZResult		ReadRemoteFile(HINTERNET& hRemoteFile, 
										   BYTE* byReadBuffer, 
										   DWORD* dwBytesRead);
};

