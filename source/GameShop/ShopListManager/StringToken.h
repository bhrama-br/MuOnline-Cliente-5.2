
#pragma once

#include "include.h"

class CStringToken  
{
public:
	CStringToken();
	virtual ~CStringToken();
	CStringToken(const std::string& dataLine, const std::string& delim);
	
	size_t countTokens();						//ÅäÅ«ÀÇ °¹¼ö 
	bool hasMoreTokens();						//ÅäÅ«ÀÌ Á¸ÀçÇÏ´ÂÁö È®ÀÎ 
	std::string nextToken();					//´ÙÀ½ ÅäÅ« 

private: 
	std::string data;
	std::string delimiter;						//µ¥ÀÌÅÍ, ±¸ºÐÀÚ 
	std::vector<std::string> tokens;			//ÅäÅ«À» º¤ÅÍ¿¡ ÀúÀå 
	std::vector<std::string>::iterator index;	//º¤ÅÍ¿¡ ´ëÇÑ ¹Ýº¹ÀÚ 

	void split();								//½ºÆ®¸µÀ» ±¸ºÐÀÚ·Î ³ª´²¼­ º¤ÅÍ¿¡ ÀúÀå 
	void IsNullString(std::string::size_type pos);						//ÅäÅ«¿¡ ³ÎÀÌ ÀÖÀ¸¸é º¤ÅÍ¿¡ ³Î°ª ³Ö¾îÁÖ±â
};
