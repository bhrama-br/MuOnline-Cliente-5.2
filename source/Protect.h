#pragma once

#include "HelperSystem.h"
#include "CustomJewel.h"

struct MAIN_FILE_INFO
{
	char CustomerName[32];
	char IpAddress[32];
	WORD IpAddressPort;
	char ClientVersion[8];
	char ClientSerial[17];
	char WindowName[32];
	char ScreenShotPath[50];
	int CharListS13;
	char m_PrivateCode[125];
	BYTE LuaCrypt;
};

struct LOAD_CUSTOM_JEWEL_INFO
{
	CUSTOM_JEWEL_INFO CustomJewelInfo[MAX_CUSTOM_JEWEL];
};

class CProtect
{
public:
	CProtect();
	virtual ~CProtect();
	bool ReadMainFile(char* Path);
	bool ReadCustomJewelConfig(char* name);

public:
	LOAD_CUSTOM_JEWEL_INFO m_CustomJewel;
	MAIN_FILE_INFO m_MainInfo;
	DWORD m_ClientFileCRC;
};

extern CProtect* gProtect;