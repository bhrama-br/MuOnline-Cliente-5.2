/*******************************************************************************
*	ÀÛ ¼º ÀÚ : ÁøÇýÁø
*	ÀÛ ¼º ÀÏ : 2009.06.10
*	³»    ¿ë : Interface - Download State Event °¡»ó Å¬·¡½º
*				´Ù¿î·Îµå »óÅÂ¸¦ Àü´Þ ¹ÞÀ» °´Ã¼
*				¶óÀÌºê·¯¸®¸¦ »ç¿ëÇÒ ÇÁ·Î±×·¥¿¡¼­ ÀçÁ¤ÀÇÇÏ¿© »ç¿ë
*******************************************************************************/

#pragma once

class IDownloaderStateEvent
{
public:
// Constructor, Destructor

	IDownloaderStateEvent(){};
	virtual ~IDownloaderStateEvent(){};


// abstract Function

	//				´Ù¿î·Îµå ½ÃÀÛ ÀÌº¥Æ® ÇÚµé·¯
	virtual void	OnStartedDownloadFile(TCHAR* szFileName, ULONGLONG uFileLength) = 0;
	//				´Ù¿î·Îµå ÁøÇà »óÈ² ÀÌº¥Æ® ÇÚµé·¯ : ÆÐÅ¶ ´ÜÀ§
	virtual void	OnProgressDownloadFile(TCHAR* szFileName, ULONGLONG uDownloadFileLength) = 0;
	//				´Ù¿î·Îµå Á¾·á ÀÌº¥Æ® ÇÚµé·¯
	virtual void	OnCompletedDownloadFile(TCHAR* szFileName, WZResult wzResult) = 0;
};

