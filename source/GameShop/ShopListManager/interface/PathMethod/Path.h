/*******************************************************************************
*	ÀÛ ¼º ÀÚ : ÁøÇýÁø
*	ÀÛ ¼º ÀÏ : 2009.06.10
*	³»    ¿ë : ±âÅ¸ ¸Þ¼Òµå
*******************************************************************************/

#pragma once

class Path
{
public:

	//					¸ðµâ ÀüÃ¼ °æ·Î °¡Á®¿À±â
	static TCHAR*		GetCurrentFullPath(TCHAR* szPath);
	//					¸ðµâ µð·ºÅä¸® °¡Á®¿À±â
	static TCHAR*		GetCurrentDirectory(TCHAR* szPath);
	//					¸ðµâ ÆÄÀÏ ÀÌ¸§ °¡Á®¿À±â
	static TCHAR*		GetCurrentFileName(TCHAR* szPath);

	//					Æú´õ ¹®ÀÚ¿­ ¸¸µé±â : ¸Ç µÚ¿¡ "\\" ºÙ¿©ÁØ´Ù.
	static TCHAR*		SetDirString(TCHAR * szPath);
	//					Æú´õ ¹®ÀÚ¿­ ¸¸µé±â : ¸Ç µÚ¿¡ "\\" Á¦°Å
	static TCHAR*		ClearDirString(TCHAR * szPath);
	
	//					Æú´õ ¹®ÀÚ¿­ ¸¸µé±â : ÆÄÀÏ¸í Á¦°ÅÇÑ °æ·Î
	static TCHAR*		GetDirectory(TCHAR * szPath);
	//					ÆÄÀÏ ¹®ÀÚ¿­ ¸¸µé±â : ÆÐ½º Á¦°ÅÇÑ ÆÄÀÏ ¸í
	static TCHAR*		GetFileName(TCHAR * szPath);

	//					/ => \\ ·Î º¯°æ
	static TCHAR*		ChangeSlashToBackSlash(TCHAR * szPath);
	//					\\ => / ·Î º¯°æ
	static TCHAR*		ChangeBackSlashToSlash(TCHAR * szPath);

	//					ÆÄÀÏ¿¡¼­ ¸¶Áö¸· ÁÙ ÀÐ¾î¿À±â
	static BOOL			ReadFileLastLine(TCHAR * szFile, TCHAR * szLastLine);
	//					»õ ÆÄÀÏ¿¡ ÇÑÁÙ ¾²±â
	static BOOL			WriteNewFile(TCHAR * szFile, TCHAR * szText, INT nTextSize);
	//					ÆÄÀÏ °æ·Î µð·ºÅä¸® »ý¼º
	static BOOL			CreateDirectorys(TCHAR * szFilePath, BOOL bIsFile);
};

