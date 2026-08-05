// MsgBoxIGSStorageItemInfo.h: interface for the MsgBoxIGSUseItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MSGBOXIGSUSEITEM_H__5E717B05_9D6D_4E85_B168_47D5EEA59CF7__INCLUDED_)
#define AFX_MSGBOXIGSUSEITEM_H__5E717B05_9D6D_4E85_B168_47D5EEA59CF7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM

#include "UIControls.h"
#include "NewUIMessageBox.h"
#include "NewUICommonMessageBox.h"

using namespace SEASON3B;

class CMsgBoxIGSStorageItemInfo : public CNewUIMessageBoxBase, public INewUI3DRenderObj
{
public:
	enum IMAGE_IGS_STORAGE_ITEM_INFO
	{
		IMAGE_IGS_BUTTON	= BITMAP_IGS_MSGBOX_BUTTON,				// ÀÎ°ÔÀÓ¼¥ ¹öÆ°
		IMAGE_IGS_FRAME		= BITMAP_IGS_MSGBOX_STORAGE_ITEM,		// Main Frame
	};
	
	enum IMAGESIZE_IGS_STORAGE_ITEM_INFO
	{
		IMAGE_IGS_WINDOW_WIDTH = 640,	// ÀÎ°ÔÀÓ¼¥ ¹è°æ »çÀÌÁî
		IMAGE_IGS_WINDOW_HEIGHT = 429,
		IMAGE_IGS_FRAME_WIDTH = 210,	// ¸Þ¼¼Áö¹Ú½º Size
		IMAGE_IGS_FRAME_HEIGHT = 202,
		IMAGE_IGS_BTN_WIDTH = 52,		// ¹öÆ° Size
		IMAGE_IGS_BTN_HEIGHT = 26,	
	};
	
	// ¸Þ¼¼Áö¹Ú½º»óÀÇ »ó´ëÁÂÇ¥
	enum IGS_STORAGE_ITEM_INFO_POS
	{
		IGS_BTN_OK_POS_X		= 43,	// ¹öÆ°
		IGS_BTN_CANCEL_POS_X	= 115,
		IGS_BTN_POS_Y			= 168,
		IGS_TEXT_TITLE_POS_Y	= 10,	// Title
		IGS_TEXT_ITEM_NAME_POS_Y		= 100,	// ItemName
		IGS_TEXT_ITEM_INFO_POS_X		= 30,	// ItemInfo
		IGS_TEXT_ITEM_INFO_NUM_POS_Y	= 124,
		IGS_TEXT_ITEM_INFO_PERIOD_POS_Y	= 140,
		IGS_TEXT_ITEM_INFO_WIDTH		= 150,
		IGS_3DITEM_POS_X = 56,		// 3D ¾ÆÀÌÅÛ ·£´õ ½ÃÀÛ À§Ä¡(BOX ±âÁØ)
		IGS_3DITEM_POS_Y = 34,
		IGS_3DITEM_WIDTH = 97,		// 3D ¾ÆÀÌÅÛ ·£´õ °ø°£¼³Á¤
		IGS_3DITEM_HEIGHT = 60,
	};
	
public:
	CMsgBoxIGSStorageItemInfo();
	~CMsgBoxIGSStorageItemInfo();
	
	bool Create(float fPriority = 3.f);
	void Release();
	bool Update();
	bool Render();

	bool IsVisible() const;

	void Render3D();
	
	void Initialize(int iStorageSeq, int iStorageItemSeq, WORD wItemCode, unicode::t_char szItemType,
					unicode::t_char* pszName, unicode::t_char* pszNum, unicode::t_char* pszPeriod);
	
	static CALLBACK_RESULT LButtonUp(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam);
	static CALLBACK_RESULT OKButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam);
	static CALLBACK_RESULT CancelButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam);
	
private:
	void SetAddCallbackFunc();
	void SetButtonInfo();
	
	void RenderFrame();
	void RenderTexts();
	void RenderButtons();
	
	void LoadImages();
	void UnloadImages();

private:
	// buttons
	CNewUIMessageBoxButton m_BtnUse;
	CNewUIMessageBoxButton m_BtnCancel;

	int		m_iStorageSeq;			// º¸°üÇÔ ¼ø¹ø
	int		m_iStorageItemSeq;		// º¸°üÇÔ »óÇ° ¼ø¹ø
	WORD	m_wItemCode;			// ¾ÆÀÌÅÛ ÄÚµå

	unicode::t_char	m_szName[MAX_TEXT_LENGTH];		// ¾ÆÀÌÅÛ ÀÌ¸§
	unicode::t_char	m_szNum[MAX_TEXT_LENGTH];
	unicode::t_char	m_szPeriod[MAX_TEXT_LENGTH];
	unicode::t_char m_szItemType;	// »óÇ°±¸ºÐ (C : Ä³½Ã, P : »óÇ°)
};

////////////////////////////////////////////////////////////////////
// LayOut
////////////////////////////////////////////////////////////////////
class CMsgBoxIGSStorageItemInfoLayout : public TMsgBoxLayout<CMsgBoxIGSStorageItemInfo>
{
public:
	bool SetLayout();
};


#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM

#endif // !defined(AFX_MSGBOXIGSUSEITEM_H__5E717B05_9D6D_4E85_B168_47D5EEA59CF7__INCLUDED_)
