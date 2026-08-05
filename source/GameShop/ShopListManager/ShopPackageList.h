
/**************************************************************************************************

ÀüÃ¼ ÆäÅ°Áö ¸ñ·Ï °´Ã¼

iterator¸¦ ÀÌ¿ëÇÏ¿© ¼øÂ÷ÀûÀ¸·Î ÆäÅ°Áö °´Ã¼¸¦ °¡Á®¿Ã ¼ö ÀÖ´Ù.
ÆäÅ°Áö ¹øÈ£¸¦ ÀÌ¿ëÇÏ¿© ÆÐÅ°Áö °´Ã¼¸¦ °¡Á®¿Ã ¼ö ÀÖ´Ù.

**************************************************************************************************/

#pragma once

#include "ShopPackage.h"
#include <map>

class CShopPackageList
{
public:
	CShopPackageList(void);
	~CShopPackageList(void);

	int GetSize();
	void Clear();

	virtual void Append(CShopPackage package);

	void SetFirst();											// ÆÐÅ°Áö ¸ñ·Ï¿¡¼­ Ã¹ ¹øÂ° ÆÐÅ°Áö¸¦ °¡¸®Å°°Ô ÇÑ´Ù.
	bool GetNext(CShopPackage& package);						// ÇöÀç ÆÐÅ°Áö °´Ã¼¸¦ ³Ñ±â°í ´ÙÀ½ ÆÐÅ°Áö °´Ã¼¸¦ °¡¸®Å°°Ô ÇÑ´Ù.
	
	bool GetValueByKey(int nKey, CShopPackage& package);		// ÆÐÅ°Áö ¹øÈ£·Î ÇØ´ç ÆÐÅ°Áö °´Ã¼ °¡Á®¿À±â
	bool GetValueByIndex(int nIndex, CShopPackage& package);	// ÀÎµ¦½º ¹øÈ£·Î ÇØ´ç ÆÐÅ°Áö °´Ã¼ °¡Á®¿À±â
	
	bool SetPacketLeftCount(int PackageSeq, int nCount);

protected:
	std::map<int, CShopPackage> m_Packages;
	std::map<int, CShopPackage>::iterator m_iter;
	std::vector<int> m_PackageIndex;
};
