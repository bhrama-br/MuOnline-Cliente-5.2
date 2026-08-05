// ItemManager.h: interface for the CItemManager class.
//////////////////////////////////////////////////////////////////////
#pragma once

#include "Lua.h"

struct CUSTOM_ITEM_INFO
{
	int Index;
	int ItemIndex;
	int ColorR;
	int ColorG;
	int ColorB;
	char ModelName[32];
};

class CItemManager
{
public:
	CItemManager();
	virtual ~CItemManager();
	void Init();
	BOOL GetCustomItemColor(int Type, float* ItemColor);
	int GET_ITEM(int section, int index);
	int GET_ITEM_MODEL(int section, int index);
	void GetItemColor(int Type, float Alpha, float Bright, vec3_t Light, bool ExtraMon = false);
public:
	std::map<int, CUSTOM_ITEM_INFO> m_CustomItemInfo;
private:
	Lua m_Lua;
};

extern CItemManager gItemManager;