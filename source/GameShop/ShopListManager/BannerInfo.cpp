//************************************************************************
//
// Decompiled by @myheart, @synth3r
// <https://forum.ragezone.com/members/2000236254.html>
//
//
// FILE: BannerInfo.cpp
//
//

#include "stdafx.h"
#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
#include "BannerInfo.h"
#include "StringToken.h"
#include "StringMethod.h"

#ifdef _WIN32
#include <UrlMon.h>
#pragma comment(lib,"Urlmon.lib")
#else
// URLDownloadToFile e API COM do Windows (Urlmon). Fora dele o download do
// banner da loja nao existe: o banner simplesmente nao aparece, que e o
// mesmo efeito de um download que falha. Recusar em vez de fingir sucesso
// evita que o codigo adiante tente abrir um arquivo que nunca foi criado.
static inline HRESULT URLDownloadToFile(void*, const char*, const char*, DWORD, void*)
{
    PLATFORM_STUB_ONCE("URLDownloadToFile (banner da loja nao baixado)");
    return (HRESULT)0x80004005L;   // E_FAIL
}
#endif

CBannerInfo::CBannerInfo() // OK
{
	memset(this->BannerName,0,sizeof(this->BannerName));
	memset(this->BannerImageURL,0,sizeof(this->BannerImageURL));
	memset(this->BannerImagePath,0,sizeof(this->BannerImagePath));
	memset(this->BannerLinkURL,0,sizeof(this->BannerLinkURL));
}
CBannerInfo::~CBannerInfo() // OK
{

}

bool	CBannerInfo::SetBanner(std::string strdata,std::string strDirPath,bool bDonwLoad) // OK
{
	if(strdata.empty())
		return 0;

	CStringToken Token(strdata,"@");

	if(Token.hasMoreTokens()==0)
		return 0;

	this->BannerSeq = atoi(Token.nextToken().c_str());

	StringCchCopy(this->BannerName,sizeof(this->BannerName),Token.nextToken().c_str());

	StringCchCopy(this->BannerImageURL,sizeof(this->BannerImageURL),Token.nextToken().c_str());

	this->BannerOrder = atoi(Token.nextToken().c_str());
	this->BannerDirection = atoi(Token.nextToken().c_str());

	CStringMethod::ConvertStringToDateTime(this->BannerStartDate,Token.nextToken());
	CStringMethod::ConvertStringToDateTime(this->BannerEndDate,Token.nextToken());

	StringCchCopy(this->BannerLinkURL,sizeof(this->BannerLinkURL),Token.nextToken().c_str());

	std::string url = this->BannerImageURL;
	std::size_t pos = url.rfind("/",std::string::npos);

	if(pos!= std::string::npos)
	{
		std::string sub = url.substr(pos+1,url.length()-pos-1);

		StringCchPrintf(this->BannerImagePath,sizeof(this->BannerImagePath),"%s%s",strDirPath.c_str(),sub.c_str());

		if(bDonwLoad||GetFileAttributes(this->BannerImagePath)==INVALID_FILE_ATTRIBUTES)
		{
			URLDownloadToFile(0,this->BannerImageURL,this->BannerImagePath,0,0);
		}
	}

	return 1;
}
#endif