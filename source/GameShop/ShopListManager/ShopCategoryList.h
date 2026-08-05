
/**************************************************************************************************

ÀüÃ¼ Ä«Å×°í¸® ¸ñ·Ï °´Ã¼

iterator¸¦ ÀÌ¿ëÇÏ¿© ¼øÂ÷ÀûÀ¸·Î Ä«Å×°í¸® °´Ã¼¸¦ °¡Á®¿Ã ¼ö ÀÖ´Ù.
Ä«Å×°í¸® ¹øÈ£¸¦ ÀÌ¿ëÇÏ¿© Ä«Å×°í¸® °´Ã¼¸¦ °¡Á®¿Ã ¼ö ÀÖ´Ù.

**************************************************************************************************/

#pragma once

#include "ShopCategory.h"
#include <map>

class CShopCategoryList
{
public:
	CShopCategoryList(void);
	~CShopCategoryList(void);

	void Clear();
	
	int GetSize();
	virtual void Append(CShopCategory category);

	void SetFirst();											// Ä«Å×°í¸® ¸ñ·Ï¿¡¼­ Ã¹ ¹øÂ° Ä«Å×°í¸®¸¦ °¡¸®Å°°Ô ÇÑ´Ù.
	bool GetNext(CShopCategory& category);						// ÇöÀç Ä«Å×°í¸® °´Ã¼¸¦ ³Ñ±â°í ´ÙÀ½ Ä«Å×°í¸® °´Ã¼¸¦ °¡¸®Å°°Ô ÇÑ´Ù.

	bool GetValueByKey(int nKey, CShopCategory& category);		// Ä«Å×°í¸® ¼ø¹øÀ¸·Î Ä«Å×°í¸® °´Ã¼¸¦ °¡Á®¿Â´Ù.
	bool GetValueByIndex(int nIndex, CShopCategory& category);	// ÀÎµ¦½º ¹øÈ£·Î Ä«Å×°í¸® °´Ã¼¸¦ °¡Á®¿Â´Ù. 

	bool InsertPackage(int Category, int Package);
	bool RefreshPackageSeq(int Category, int PackageSeqs[], int PackageCount);

protected:
	std::map<int, CShopCategory> m_Categroys;				// Ä«Å×°í¸® °´Ã¼ ¸Ê
	std::map<int, CShopCategory>::iterator m_Categoryiter;	// Ä«Å×°í¸® iterator
	std::vector<int> m_CategoryIndex;						// Ä«Å×°í¸® ¹øÈ£ ÀÎµ¦½º ¸ñ·Ï

};
