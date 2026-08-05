
/**************************************************************************************************

ÆäÅ°Áö °´Ã¼

ÇöÀç ÆäÅ°Áö Á¤º¸¸¦ °¡Áö°í ÀÖ´Ù.
ÇöÀç ÆäÅ°Áö¿¡ µî·Ï µÇ¾îÀÖ´Â "\xBB\xF3\xC7\xB0 \xB9\xF8\xC8\xA3" ¸ñ·Ï°ú "\xB0\xA1\xB0\xDD \xB9\xF8\xC8\xA3" ¸ñ·ÏÀ» °¡Áö°í ÀÖ´Ù.

ÆÐÅ°Áö¿¡ ¿©·¯ »óÇ°ÀÌ µé¾îÀÖ´Â °æ¿ì¿¡´Â °¡°Ý ¹øÈ£°¡ ÇÑ °³ ÀÌ´Ù.
ÆÐÅ°Áö¿¡ ¿©·¯ °¡°ÝÀÌ ¼³Á¤µÇ¾î ÀÖ´Ù¸é »óÇ°Àº ÇÑ °³ ÀÌ´Ù.

(exe 1)
A ÆäÅ°Áö¿¡ a¿Í a' ¶ó´Â »óÇ°ÀÌ µÎ °³ ÀÖ´Ù¸é ¿©·¯ °¡°ÝÀÌ ¼³Á¤ µÉ ¼ö ¾ø´Ù. 
-> A»óÇ° a, a' 5000¿ø

(exe 2)
B ÆäÅ°Áö¿¡ b ¶ó´Â »óÇ°ÀÌ ÇÏ³ª¸¸ ÀÖ´Ù¸é 1000¿ø, 2000¿ø, 3000¿ø À¸·Î ¿©·¯ °¡°ÝÀÌ ¼³Á¤ µÉ ¼ö ÀÖ´Ù. 
-> B»óÇ° b 1ÁÖÀÏ 1000¿ø
-> B»óÇ° b 2ÁÖÀÏ 2000¿ø
-> B»óÇ° b 4ÁÖÀÏ 3000¿ø
¼¼ °¡Áö¸¦ ´Ù¸¥ ÆäÅ°Áö·Î º¸¿©ÁÖ¾î¾ß ÇÑ´Ù.

**************************************************************************************************/

#pragma once

#include "include.h"
#include <time.h>

class CShopPackage
{
public:
	CShopPackage();
	virtual ~CShopPackage();

	bool	SetPackage(std::string strdata);
	void	SetLeftCount(int nCount);

	int		GetProductCount();									// ÆÐÅ°Áö ³»ºÎÀÇ »óÇ° ¼ö °¡Á®¿À±â
	void	SetProductSeqFirst();								// ÆÐÅ°Áö ³»ºÎÀÇ Ã¹¹øÂ° »óÇ° ¹øÈ£¿¡ À§Ä¡
	bool	GetProductSeqFirst(int& ProductSeq);				// ÆÐÅ°Áö ³»ºÎÀÇ Ã¹¹øÂ° »óÇ° ¹øÈ£¸¦ °¡Á®¿À°í ´ÙÀ½ »óÇ° ¹øÈ£·Î ÀÌµ¿
	bool	GetProductSeqNext(int& ProductSeq);					// »óÇ° ¹øÈ£ °¡Á®¿À°í ´ÙÀ½ À§Ä¡·Î ÀÌµ¿

	int		GetPriceCount();									// ÆÐÅ°Áö ³»ºÎÀÇ °¡°Ý ¼ö °¡Á®¿À±â
	void	SetPriceSeqFirst();									// ÆÐÅ°Áö ³»ºÎÀÇ Ã¹¹øÂ° °¡°Ý ¹øÈ£¿¡ À§Ä¡
	bool	GetPriceSeqFirst(int& PriceSeq);					// ÆÐÅ°Áö ³»ºÎÀÇ Ã¹¹øÂ° °¡°Ý ¹øÈ£¸¦ °¡Á®¿À°í ´ÙÀ½ °¡°Ý ¹øÈ£·Î ÀÌµ¿
	bool	GetPriceSeqNext(int& PriceSeq);						// °¡°Ý ¹øÈ£ °¡Á®¿À°í ´ÙÀ½ À§Ä¡·Î ÀÌµ¿

public:
	int		ProductDisplaySeq;									//  1. ÆÐÅ°Áö°¡ ¼ÓÇØÀÖ´Â Ä«Å×°í¸® ¹øÈ£
	int		ViewOrder;											//  2. ³ëÃâ ¼ø¼­
	int		PackageProductSeq;									//  3. ÆÐÅ°Áö ¹øÈ£
	char	PackageProductName[SHOPLIST_LENGTH_PACKAGENAME];	//  4. ÆÐÅ°Áö ¸í
	int		PackageProductType;									//  5. ÆÐÅ°Áö À¯Çü (170:ÀÏ¹Ý »óÇ°, 171:ÀÌº¥Æ® »óÇ°)
	int		Price;												//  6. °¡°Ý
	char	Description[SHOPLIST_LENGTH_PACKAGEDESC];			//  7. »ó¼¼ ¼³¸í
	char	Caution[SHOPLIST_LENGTH_PACKAGECAUTION];			//  8. ÁÖÀÇ »çÇ×
	int		SalesFlag;											//  9. ±¸¸Å °¡´É ¿©ºÎ(±¸¸Å¹öÆ° ³ëÃâ¿©ºÎ) (182:°¡´É, 183:ºÒ°¡)
	int		GiftFlag;											// 10. ¼±¹° °¡´É ¿©ºÎ(¼±¹°¹öÆ° ³ëÃâ¿©ºÎ) (184:°¡´É, 185:ºÒ°¡)
	tm		StartDate;											// 11. ÆÇ¸Å ½ÃÀÛÀÏ
	tm		EndDate;											// 12. ÆÇ¸Å Á¾·áÀÏ
	int		CapsuleFlag;										// 13. Ä¸½¶ »óÇ° ±¸ºÐ (176:Ä¸½¶, 177:ÀÏ¹Ý)
	int		CapsuleCount;										// 14. ÆÐÅ°Áö¿¡ Æ÷ÇÔµÈ »óÇ° °³¼ö
	char	ProductCashName[SHOPLIST_LENGTH_PACKAGECASHNAME];	// 15. ¼ÒÁø Ä³½Ã ¸í
	char	PricUnitName[SHOPLIST_LENGTH_PACKAGEPRICEUNIT];		// 16. °¡°Ý ´ÜÀ§ Ç¥½Ã ¸í
	int		DeleteFlag;											// 17. »èÁ¦ ¿©ºÎ (180:»èÁ¦, 181:È°¼º)
	int		EventFlag;											// 18. ÀÌº¥Æ® »óÇ° ¿©ºÎ (199:ÀÌº¥Æ® »óÇ°, 200:ÀÏ¹Ý »óÇ°)
	int		ProductAmount;										// 19. ÇÑÁ¤ »óÇ° ¿©ºÎ	
	char	InGamePackageID[SHOPLIST_LENGTH_INGAMEPACKAGEID];	// 21. ÆÐÅ°Áö ¾ÆÀÌÅÛ ÄÚµå
	int		ProductCashSeq;										// 22. ¼ÒÁø Ä³½Ã À¯Çü ÄÚµå
	int		PriceCount;											// 23. ´ÜÀ§ »óÇ° °¡°Ý Á¤Ã¥ º¸À¯ °³¼ö (´ÜÀ§ »óÇ°ÀÌ 1°³ÀÎ °æ¿ì¿¡¸¸ PriceSeq°¡ ¿©·¯ °³ÀÏ ¼ö ÀÖ´Ù.)
	bool	DeductMileageFlag;									// 25. ¸¶ÀÏ¸®Áö·Î Â÷°¨ »óÇ° ¿©ºÎ (false : ÀÏ¹Ý, true : ¸¶ÀÏ¸®Áö Â÷°¨ »óÇ°)
	int		CashType;											// 26. ±Û·Î¹ú Àü¿ë : Wcoin(C), WCoin(P) ±¸ºÐ
	int		CashTypeFlag;										// 27. ±Û·Î¹ú Àü¿ë : Wcoin(C), WCoin(P) ¼±ÅÃ or ÀÚµ¿ ¿©ºÎ(668: °³ÀÎ¼±ÅÃ, 669: ÀÚµ¿Â÷°¨)

	int		LeftCount;											// ÀÜ¿© °³¼ö

private:
	void	SetProductSeqList(std::string strdata);
	void	SetPriceSeqList(std::string strdata);

	std::vector<int> ProductSeqList;							// 20. ÆÐÅ°Áö¿¡ Æ÷ÇÔµÈ »óÇ° ¹øÈ£ ¸ñ·Ï
	std::vector<int>::iterator ProductSeqIter;

	std::vector<int> PriceSeqList;								// 24. ÆÐÅ°Áö¿¡ Æ÷ÇÔµÇ´Â °¡°Ý ¹øÈ£ ¸ñ·Ï (»óÇ°ÀÌ 1°³ÀÎ °æ¿ì¿¡¸¸ °ªÀ» Á¦°ø.)
	std::vector<int>::iterator PriceSeqIter;
};
