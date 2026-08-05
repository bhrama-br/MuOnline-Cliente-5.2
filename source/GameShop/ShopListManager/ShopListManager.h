
/*
ÀÛ¼ºÀÏ: 2009-07-24
ÀÛ¼ºÀÚ: ¹®»óÇö
¿ä¾à: ¼¥¸®½ºÆ® °ü¸®¸¦ À§ÇØ »ç¿ëµÇ´Â °´Ã¼
*/

#pragma once

#include "include.h"
#include "ListManager.h"
#include "ShopList.h"

class CShopListManager : public CListManager
{
public:
	CShopListManager();	
	virtual ~CShopListManager();

	CShopList*		GetListPtr() {return m_ShopList;};

private:
	CShopList*		m_ShopList;

	WZResult		LoadScript(bool bDonwLoad);
};