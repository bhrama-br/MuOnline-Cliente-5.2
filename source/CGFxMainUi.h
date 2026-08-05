#pragma once

#ifdef LDK_ADD_SCALEFORM

#include "GTimer.h"
#include "GFile.h"
#include "GFxLoader.h"
#include "GFxEvent.h"
#include "GFxPlayer.h"
#include "GFxFontLib.h"

#include "FxPlayerLog.h"
#include "GrendererOGL.h"

#include "CGFxProcess.h"

#define MAX_ITEM_SLOT 4
#define MAX_SKILL_HOT_KEY 11

typedef struct vecter2d
{
	float x;
	float y;

	vecter2d()
	{
		x = 0;
		y = 0;
	}
}VECTOR2D, *LPVECTOR2D;

enum SKILLTEXTURETYPE
{
	SKILL_TYPE_COMMAND = 0,
	SKILL_TYPE_SKILL1,
	SKILL_TYPE_SKILL2,
	SKILL_TYPE_SKILL3,
};

inline bool CanRegisterItemHotKey(int iType)
{
	switch(iType)
	{
	case ITEM_POTION+0:		// »ç°ú
	case ITEM_POTION+1:		// ÀÛÀºÄ¡·á¹°¾à	
	case ITEM_POTION+2:		// Áß°£Ä¡·á¹°¾à	
	case ITEM_POTION+3:		// Å«Ä¡·á¹°¾à	
	case ITEM_POTION+4:		// ÀÛÀº¸¶³ª¹°¾à	
	case ITEM_POTION+5:		// Áß°£¸¶³ª¹°¾à	
	case ITEM_POTION+6:		// Å«¸¶³ª¹°¾à		
	case ITEM_POTION+7:		// °ø¼º¹°¾à(Ãàº¹ÀÇ¹°¾à, ¿µÈ¥ÀÇ¹°¾à)
	case ITEM_POTION+8:		// ÇØµ¶¹°¾à
	case ITEM_POTION+9:		// ¼ú
	case ITEM_POTION+10:	// ¸¶À»±ÍÈ¯¹®¼­
	case ITEM_POTION+20:	// »ç¶ûÀÇ ¹¦¾à
	case ITEM_POTION+35:	// ÀÛÀºSDÈ¸º¹¹°¾à
	case ITEM_POTION+36:	// Áß°£SDÈ¸º¹¹°¾à
	case ITEM_POTION+37:	// Å«SDÈ¸º¹¹°¾à
	case ITEM_POTION+38:	// ÀÛÀºº¹ÇÕ¹°¾à
	case ITEM_POTION+39:	// Áß°£º¹ÇÕ¹°¾à
	case ITEM_POTION+40:	// Å«º¹ÇÕ¹°¾à
	case ITEM_POTION+46:	// Àè¿À·£ÅÏÀÇÃàº¹
	case ITEM_POTION+47:	// Àè¿À·£ÅÏÀÇºÐ³ë
	case ITEM_POTION+48:	// Àè¿À·£ÅÏÀÇ¿ÜÄ§
	case ITEM_POTION+49:	// Àè¿À·£ÅÏÀÇÀ½½Ä
	case ITEM_POTION+50:	// Àè¿À·£ÅÏÀÇÀ½·á
#ifdef PSW_ELITE_ITEM
	case ITEM_POTION+70:    // ºÎºÐÀ¯·áÈ­ ¿¤¸®Æ® Ã¼·Â ¹°¾à
	case ITEM_POTION+71:    // ºÎºÐÀ¯·áÈ­ ¿¤¸®Æ® ¸¶³ª ¹°¾à
#endif //PSW_ELITE_ITEM
#ifdef PSW_ELITE_ITEM
	case ITEM_POTION+78:    // ºÎºÐÀ¯·áÈ­ ÈûÀÇ ºñ¾à
	case ITEM_POTION+79:    // ºÎºÐÀ¯·áÈ­ ¹ÎÃ¸ÀÇ ºñ¾à
	case ITEM_POTION+80:    // ºÎºÐÀ¯·áÈ­ Ã¼·ÂÀÇ ºñ¾à
	case ITEM_POTION+81:    // ºÎºÐÀ¯·áÈ­ ¿¡³ÊÁöÀÇ ºñ¾à
	case ITEM_POTION+82:    // ºÎºÐÀ¯·áÈ­ Åë¼ÖÀÇ
#endif //PSW_ELITE_ITEM
#ifdef PSW_NEW_ELITE_ITEM
	case ITEM_POTION+94:    // ºÎºÐÀ¯·áÈ­ ¿¤¸®Æ® Áß°£ Ã¼·Â ¹°¾à
#endif //PSW_NEW_ELITE_ITEM
#ifdef CSK_EVENT_CHERRYBLOSSOM
	case ITEM_POTION+85:	// º¢²É¼ú
	case ITEM_POTION+86:	// º¢²É°æ´Ü
	case ITEM_POTION+87:	// º¢²ÉÀÙ
#endif //CSK_EVENT_CHERRYBLOSSOM
#ifdef YDG_ADD_CS7_ELITE_SD_POTION
	case ITEM_POTION+133:	// ¿¤¸®Æ®SDÈ¸º¹¹°¾à
#endif	// YDG_ADD_CS7_ELITE_SD_POTION
		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------
// GFxTutorial Definition
//--------------------------------------------------------------------------------------
class CMainUIFSCHandler;
class CMainUIEIHandler;

class CGFxMainUi : public CGFXBase
{
public:
	static CGFxMainUi* Make(GFxRegistType _UIType, mapGFXContainer *_GfxRepo, const char* _pfilename, UInt _loadConstants = GFxLoader::LoadKeepBindData, UPInt _memoryArena = 0, UInt _renderFlag = GFxRenderConfig::RF_EdgeAA | GFxRenderConfig::RF_StrokeMask, GFxMovieView::ScaleModeType _scaleType = GFxMovieView::SM_NoScale, FontConfig* _pconfig = NULL,const char* _languag = NULL);

	virtual ~CGFxMainUi();

	virtual bool OnCreateDevice(SInt bufw, SInt bufh, SInt left, SInt top, SInt w, SInt h, UInt flags = 0);
	virtual bool OnResetDevice();
	virtual bool OnLostDevice();
	virtual bool OnDestroyDevice();

	virtual bool Init();
	virtual bool Update();
	virtual bool Render();
	virtual bool RenderModel();

//	virtual GPtr<GFxMovieView> GetMovie();
	virtual GFxMovieView* GetMovie();

	virtual bool IsVisible()				{ return m_bVisible; }
	virtual void SetVisible(bool _visible)	{ if(!m_bLock && m_bVisible!= _visible) m_bVisible = _visible; }

	virtual bool IsLock()					{ return m_bLock; }
	virtual void SetUnLockVisible()			{ m_bLock = FALSE; }
	virtual void SetLockVisible(bool _visible) { m_bLock = TRUE; m_bVisible = _visible; }

	GPtr<GFxLog> GetLog() { return m_gfxLoader.GetLog(); }
	void LogPrintf(const char *format, va_list al)
	{
		m_gfxLoader.GetLog()->LogMessageVarg(GFxLog::Log_Channel_General, format, al);
	}

public:
	void SetSkillClearHotKey();

	void SetSkillHotKey(int _iHotKey, int _skillType, bool _invoke = true);
	int GetSkillHotKey(int _iHotKey);

	void SetSkillSlot();
	void SetSkillInfo(int Type);
	void SetSkillSlotVisible(bool _visible) { m_isSkillSlotVisible = _visible; }

	void SetOverItemSlotNum(int _iHotKey) { m_iOverItemSlot = _iHotKey; }
	void SetItemClearHotKey();
	void SetItemHotKey(int _iHotKey, int _iItemType, int _iItemLevel);
	int GetItemHotKey(int _iHotKey);
	int GetItemHotKeyLevel(int _iHotKey);

	void SetUseItemSlotNum(int _slotNum) { m_iUseItemSlotNum = _slotNum; }

private:
	CGFxMainUi();

	bool InitGFx(	const char* _pfilename, 
					UInt _loadConstants = GFxLoader::LoadKeepBindData,
					UPInt _memoryArena = 0,
					UInt _renderFlag = GFxRenderConfig::RF_EdgeAA | GFxRenderConfig::RF_StrokeMask, 
					GFxMovieView::ScaleModeType _scaleType = GFxMovieView::SM_NoScale,
					FontConfig* _pconfig = NULL, const char* _languag = NULL);

	void UpdateMenuBtns();
	void UpdateGaugeHpMp();
	void UpdateGaugeSd();
	void UpdateGaugeAg();
	void UpdateGaugeExp();
	void UpdateGaugeStamina();
	void UpdateItemSlot();
	void UpdateSkillSlot();

	void RenderObjectScreen(int Type,int ItemLevel,int Option1,int ExtOption,vec3_t Target,int Select,bool PickUp);

	int GetHotKeyItemIndex(int iType, bool bItemCount = false);
	bool GetHotKeyCommonItem(IN int iHotKey, OUT int& iStart, OUT int& iEnd);

	bool GetSkillNumber(int skillType, int *outSkillNum, int *outTextureNum);
	bool GetSkillDisable(int slotNum, int* _array);
	void GetSkillDelay(int skillType, int* _array); //Â÷ÈÄ ÀÛ¾÷¿ë

	bool GetAttackDamage ( int* iMinDamage, int* iMaxDamage );

private:
	GPtr<GRendererOGL>		m_pRenderer;
	GPtr<GFxRenderConfig>	m_pRenderConfig;
	GPtr<GFxRenderStats>	m_pRenderStats;

	GFxLoader           m_gfxLoader;
	GPtr<GFxMovieDef>   m_pUIMovieDef;
	GPtr<GFxMovieView>  m_pUIMovie;
	GFxMovieInfo		m_gfxMovieInfo;

	CMainUIFSCHandler	*m_pFSHandler;
	CMainUIEIHandler		*m_pEIHandler;

	// Movie frame time
	bool m_bVisible;
	bool m_bLock;
	bool m_bWireFrame;
	DWORD m_dwMovieLastTime;

	int m_iViewType;
	//OBJECT ObjectSelect;

	//gauge
	int m_iHpPercent;
	bool m_isIntoxication;
	int m_iMpPercent;
	int m_iSDPercent;
	int m_iAGPercent;
	int m_iExpPercent;
	int m_iExpMin;
	int m_iExpMax;
	bool m_bMasterLv;

	bool m_bStaVisible;
	int m_iStaPercent;

	//item
	int m_iItemType[MAX_ITEM_SLOT];
	int m_iItemLevel[MAX_ITEM_SLOT];
	int m_iItemCount[MAX_ITEM_SLOT];

	int m_iUseItemSlotNum;
	int m_iOverItemSlot;

	//skill
	int m_iHotKeySkillIndex[MAX_SKILL_HOT_KEY];
	bool m_isHotKeySkillCantUse[MAX_SKILL_HOT_KEY];
	int m_iHotKeySkillType[MAX_SKILL_HOT_KEY];		//¸¶½ºÅÍ½ºÅ³ º¯°æ½Ã È®ÀÎ¿ë º¯¼ö

	bool m_isSkillSlotVisible;
	int m_iSkillSlotCount;
	int m_iPetSlotCount;
	int m_iSkillSlotIndex[MAX_MAGIC];
	bool m_isSkillSlotCantUse[MAX_MAGIC];
	int m_iSkillSlotType[MAX_MAGIC];		//¸¶½ºÅÍ½ºÅ³ º¯°æ½Ã È®ÀÎ¿ë º¯¼ö

};

//--------------------------------------------------------------------------------------
// FSCommand Handler
//--------------------------------------------------------------------------------------
class CMainUIFSCHandler : public GFxFSCommandHandler
{
public:
	virtual void Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg);
};

//--------------------------------------------------------------------------------------
// ExternalInterface Handler
//--------------------------------------------------------------------------------------
class CMainUIEIHandler : public GFxExternalInterface
{
private:
	CGFxMainUi *pMainUi;

public:
	void SetMainUi(CGFxMainUi* _mainUi) { pMainUi = _mainUi; }

public:
	virtual void Callback(GFxMovieView* pmovieView, const char* methodName, const GFxValue* args, UInt argCount);
};

#endif //LDK_ADD_SCALEFORM
