
#pragma once

#include "include.h"
#include <map>

class CBannerInfo
{
public:
	CBannerInfo();
	virtual ~CBannerInfo();

	bool	SetBanner(std::string strdata, std::string strDirPath, bool bDonwLoad);

public:	
	int		BannerSeq;									//  1. ¹è³Ê ±×·ì ¼ø¹ø
	char	BannerName[BANNER_LENGTH_NAME];				//  2. ¹è³Ê ±×·ì ¸í
	char	BannerImageURL[INTERNET_MAX_URL_LENGTH];	//  3. ¹è³Ê ÀÌ¹ÌÁö URL
	int		BannerOrder;								//  4. ¹è³Ê ³ëÃâ ¼ø¼­
	int		BannerDirection;							//  5. ¹è³Ê ³ëÃâ ¹æÇâ * °øÅë ÄÚµå Á¤ÀÇ Âü°í
	tm 		BannerStartDate;							//  6. ¹è³Ê ³ëÃâ ½ÃÀÛÀÏ
	tm 		BannerEndDate;								//  7. ¹è³Ê ³ëÃâ Á¾·áÀÏ
	char	BannerLinkURL[INTERNET_MAX_URL_LENGTH];		//  8. ¹è³Ê ¸µÅ© URL

	char	BannerImagePath[MAX_PATH];					// ¹è³Ê ÀÌ¹ÌÁö ÆÄÀÏ ·ÎÄÃ °æ·Î
};
