//*****************************************************************************
// File: UsefulDef.h
//
// Desc: À¯¿ëÇÑ Á¤ÀÇ ¸ðÀ½.
//
// producer: Ahn Sang-Kyu
//*****************************************************************************

#ifndef _USEFUL_DEF_H_
#define _USEFUL_DEF_H_

#ifdef KJH_ADD_VS2008PORTING_ARRANGE_INCLUDE
	#include <WTypes.h>
	#include <stdlib.h>
#endif // KJH_ADD_VS2008PORTING_ARRANGE_INCLUDE

#define MIN(a, b)			(((a) < (b)) ? (a) : (b))
#define MAX(a, b)			(((a) > (b)) ? (a) : (b))
// valÀ» ÃÖÀú l, ÃÖ°í h·Î Á¦ÇÑ.
#define LIMIT(val, l, h)	((val) < (l) ? (l) : (val) > (h) ? (h) : (val))
#define SWAP(a, b)			{ (a) ^= (b) ^= (a) ^= (b); }

#define	ARGB(a,r,g,b)	(((DWORD)(a))<<24 | (DWORD)(r) | ((DWORD)(g))<<8 | ((DWORD)(b))<<16)

/*	Áßº¹ÀÌ¹Ç·Î »èÁ¦.
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
*/

//*****************************************************************************
// ÇÔ¼ö ÀÌ¸§ : Random()
// ÇÔ¼ö ¼³¸í : nMin°ú nMax »çÀÌÀÇ °ª Áß ·£´ý°ª ¸®ÅÏ.
// ¸Å°³ º¯¼ö : nMin	: ÃÖ¼Ò°ª.
//			   nMax	: ÃÖ´ë°ª.
//*****************************************************************************
inline int Random(int nMin, int nMax)
{
    return rand() % (nMax - nMin + 1) + nMin;
}

bool ReduceStringByPixel(LPTSTR lpszDst, int nDstSize, LPCTSTR lpszSrc, int nPixel);
#if defined KJH_ADD_INGAMESHOP_UI_SYSTEM || defined LJH_MOD_TO_USE_DIVIDESTRINGBYPIXEL_FUNC
int DivideStringByPixel(LPTSTR alpszDst, int nDstRow, int nDstColumn, LPCTSTR lpszSrc,
						int nPixelPerLine, bool bSpaceInsert = true, const char szNewLineChar = ';');
#else // define KJH_ADD_INGAMESHOP_UI_SYSTEM || define LJH_MOD_TO_USE_DIVIDESTRINGBYPIXEL_FUNC
int DivideStringByPixel(LPTSTR alpszDst, int nDstRow, int nDstColumn, LPCTSTR lpszSrc,
						int nPixelPerLine, bool bSpaceInsert = true);
#endif // define KJH_ADD_INGAMESHOP_UI_SYSTEM || define LJH_MOD_TO_USE_DIVIDESTRINGBYPIXEL_FUNC
int DivideString(LPTSTR alpszDst, int nDstRow, int nDstColumn, LPCTSTR lpszSrc);

BOOL CheckErrString(LPTSTR lpszTarget);

#endif // _USEFUL_DEF_H_