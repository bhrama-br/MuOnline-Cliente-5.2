
/**************************************************************************************************

ÀÛ¼ºÀÏ: 2009.10.06
ÀÛ¼ºÀÚ: ÁøÇýÁø

**************************************************************************************************/

#pragma once

#include "include.h"
#include "BannerInfoList.h"
#include "ListManager.h"

class CBannerListManager : public CListManager
{
public:
	CBannerListManager();	
	virtual ~CBannerListManager();

	CBannerInfoList*	GetListPtr() {return m_BannerInfoList;};

private:
	CBannerInfoList*	m_BannerInfoList;

	WZResult			LoadScript(bool bDonwLoad);
};
