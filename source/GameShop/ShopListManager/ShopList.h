
/**************************************************************************************************

½ºÅ©¸³Æ® ¸ñ·Ï ÃÖ »óÀ§ °´Ã¼

Ä«Å×°í¸® ¸ñ·Ï, ÆÐÅ°Áö ¸ñ·Ï, »óÇ°(¼Ó¼º) ¸ñ·ÏÀ» °¡Áö°í ÀÖ´Ù.

**************************************************************************************************/

#pragma once

#include "ShopPackage.h"
#include "ShopProduct.h"

#include "ShopCategoryList.h"
#include "ShopPackageList.h"
#include "ShopProductList.h"

class CShopList  
{
public:
	CShopList();
	virtual ~CShopList();

	WZResult LoadCategroy(const char* szFilePath);
	WZResult LoadPackage (const char* szFilePath);
	WZResult LoadProduct (const char* szFilePath);	

	CShopCategoryList* GetCategoryListPtr() {return m_CategoryListPtr;};	// Ä«Å×°í¸® ¸ñ·Ï °¡Á®¿Â´Ù.
	CShopPackageList*  GetPackageListPtr()  {return m_PackageListPtr;};		// ÆÐÅ°Áö ¸ñ·Ï °¡Á®¿Â´Ù.
	CShopProductList*  GetProductListPtr()  {return m_ProductListPtr;};		// »óÇ°(¼Ó¼º) ¸ñ·Ï °¡Á®¿Â´Ù.

	void SetCategoryListPtr(CShopCategoryList* CategoryListPtr);
	void SetPackageListPtr (CShopPackageList* PackagePtr);
	void SetProductListPtr (CShopProductList* ProductListPtr);

private:	
	CShopCategoryList* m_CategoryListPtr;
	CShopPackageList*  m_PackageListPtr;
	CShopProductList*  m_ProductListPtr;

	FILE_ENCODE IsFileEncodingUtf8(const char* szFilePath);
	std::string GetDecodeingString(const char* str, FILE_ENCODE encode);
};