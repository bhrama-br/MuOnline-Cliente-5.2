
/**************************************************************************************************

»óÇ°(¼Ó¼º) °´Ã¼

ÇöÀç »óÇ°ÀÇ ±âº» Á¤º¸¿Í ÇÑ °¡Áö ¼Ó¼º Á¤º¸¸¦ °¡Áö°í ÀÖ´Ù.
»óÇ° ¹øÈ£°¡ µ¿ÀÏÇÑ ¿©·¯ ¼Ó¼ºÀ» Á¶ÇÕÇÏ¿© ÇÑ °¡Áö »óÇ°À» Ç¥ÇöÇØ¾ß ÇÑ´Ù.

**************************************************************************************************/

#pragma once

#include "include.h"
#include <map>

class CShopProduct
{
public:
	CShopProduct();
	virtual ~CShopProduct();

	bool SetProduct(std::string strdata);

public:	
	int		ProductSeq;											//  1. »óÇ° ¹øÈ£
	char	ProductName[SHOPLIST_LENGTH_PRODUCTNAME];			//  2. »óÇ° ¸í
	char	PropertyName[SHOPLIST_LENGTH_PRODUCTPROPERTYNAME];	//  3. ¼Ó¼º ¸í
	char	Value[SHOPLIST_LENGTH_PRODUCTVALUE];				//  4. ¼Ó¼º °ª
	char	UnitName[SHOPLIST_LENGTH_PRODUCTUNITNAME];			//  5. ¼Ó¼º ´ÜÀ§ ¸í
	int		Price;												//  6. »óÇ° °¡°Ý
	int		PriceSeq;											//  7. »óÇ° °¡°Ý ¹øÈ£
	int		PropertyType;										//  8. ¼Ó¼º À¯Çü (141:¾ÆÀÌÅÛ ¼Ó¼º, 142:°¡°Ý ¼Ó¼º)
	int		MustFlag;											//  9. ÇÊ¼ö ¿©ºÎ (145:ÇÊ¼ö, 146:¼±ÅÃ)
	int		vOrder;												// 10. ¸ÞÀÎ ¼Ó¼º ±¸ºÐ (1:¸ÞÀÎ ¼Ó¼º, 9:¼­ºê ¼Ó¼º)
	int		DeleteFlag;											// 11. »èÁ¦ ¿©ºÎ (143: »èÁ¦, 144: È°¼º)
	int		StorageGroup;										// 12. º¸°üÇÔ ±×·ì À¯Çü
	int		ShareFlag;											// 13. ServerType(¼­¹ö À¯Çü) º° º¸°üÇÔ ³ëÃâ °øÀ¯ Ç×¸ñ ¿©ºÎ
	char	InGamePackageID[SHOPLIST_LENGTH_INGAMEPACKAGEID];	// 14. ¾ÆÀÌÅÛ ÄÚµå
	int		PropertySeq;										// 15. ¼Ó¼º ÄÚµå
	int		ProductType;										// 16. »óÇ° À¯Çü ÄÚµå
	int		UnitType;											// 17. ´ÜÀ§ ÄÚµå
};
