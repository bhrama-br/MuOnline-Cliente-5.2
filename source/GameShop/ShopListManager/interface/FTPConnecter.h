/*******************************************************************************
*	ÀÛ ¼º ÀÚ : ÁøÇýÁø
*	ÀÛ ¼º ÀÏ : 2009.07.07
*	³»    ¿ë : FTP Connecter
*******************************************************************************/

#pragma once

#include "GameShop\ShopListManager\interface\IConnecter.h"

class FTPConnecter : public IConnecter
{
public:
// Constructor, Destructor

	FTPConnecter(DownloadServerInfo *	pServerInfo,
				 DownloadFileInfo *		pFileInfo);
	~FTPConnecter();


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

