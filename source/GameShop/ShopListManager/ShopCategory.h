
/**************************************************************************************************

Ä«Å×°í¸® °´Ã¼

ÇöÀç Ä«Å×°í¸® Á¤º¸¸¦ °¡Áö°í ÀÖ´Ù.
ÇöÀç Ä«Å×°í¸® ÇÏÀ§ÀÇ "\xC4\xAB\xC5\xD7\xB0\xED\xB8\xAE \xB9\xF8\xC8\xA3"¸¦ ¸ñ·ÏÀ¸·Î °¡Áö°í ÀÖ´Ù.
ÇöÀç Ä«Å×°í¸®°¡ ÃÖ ÇÏÀ§ Ä«Å×°í¸®¶ó¸é "\xC6\xD0\xC5\xB0\xC1\xF6 \xB9\xF8\xC8\xA3" ¸ñ·ÏÀ» °¡Áö°í ÀÖ´Ù.

**************************************************************************************************/

#pragma once

#include "include.h"

class CShopCategory
{
public:
	CShopCategory();
	virtual ~CShopCategory();
	
	bool SetCategory(std::string strdata);

	void SetCategoryFirst();							// ÇÏÀ§ Ä«Å×°í¸® ¸ñ·ÏÀÇ Ã¹ ¹øÂ° Ç×·ÏÀ» °¡¸®Å°µµ·Ï ¼³Á¤ÇÑ´Ù.
	bool GetCategoryNext(int& CategorySeq);				// ÇÏÀ§ Ä«Å×°í¸® ¹øÈ£¸¦ ¸®ÅÏÇÏ°í ´ÙÀ½ ÇÏÀ§ Ä«Å×°í¸® ¹øÈ£¸¦ °¡¸®Å²´Ù.

	void SetPackagSeqFirst();							// Ä«Å×°í¸®¿¡ µî·ÏµÇ¾î ÀÖ´Â ÆäÅ°Áö ¸ñ·ÏÀÇ Ã¹ ¹øÂ° Ç×¸ñÀ» °¡¸®Å°µµ·Ï ¼³Á¤ÇÑ´Ù.
	bool GetPackagSeqNext(int& PackagSeq);				// ÆäÅ°Áö ¹øÈ£¸¦ ¸®ÅÏÇÏ°í ´ÙÀ½ ÆäÅ°Áö ¹øÈ£¸¦ °¡¸®Å²´Ù.

	void AddPackageSeq(int PackageSeq);
	void ClearPackageSeq();

public:
	int ProductDisplaySeq;								// 1. Ä«Å×°í¸® ¹øÈ£
	char CategroyName[SHOPLIST_LENGTH_CATEGORYNAME];	// 2. Ä«Å×°í¸® ÀÌ¸§
	int EventFlag;										// 3. ÀÌº¥Æ® Ä«Å×°í¸® ¿©ºÎ (199:ÀÌº¥Æ®, 200:ÀÏ¹Ý)	
	int OpenFlag;										// 4. °ø°³ ¿©ºÎ (201:°ø°³, 202: ºñ°ø°³)
	int ParentProductDisplaySeq;						// 5. ºÎ¸ð Ä«Å×°í¸® ¹øÈ£
	int DisplayOrder;									// 6. ³ëÃâ ¼ø¼­
	int Root;											// 7. ÃÖ»óÀ§ Ä«Å×°í¸® ±¸ºÐ (1: ÃÖ»óÀ§, 0: ¼­ºê)

	std::vector<int> CategoryList;						// ÇöÀç Ä«Å×°í¸®ÀÇ ÇÏÀ§ "\xC4\xAB\xC5\xD7\xB0\xED\xB8\xAE \xB9\xF8\xC8\xA3" ¸ñ·Ï
	std::vector<int>::iterator Categoryiter;

	std::vector<int> PackageList;						// ÇöÀç Ä«Å×°í¸®¿¡ Æ÷ÇÔµÈ "\xC6\xE4\xC5\xB0\xC1\xF6 \xB9\xF8\xC8\xA3" ¸ñ·Ï
	std::vector<int>::iterator Packageiter;
};
