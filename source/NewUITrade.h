//*****************************************************************************
// File: NewUITrade.h
//
// Desc: interface for the CNewUITrade class.
//		 °Å·¡Ã¢ Å¬·¡½º.
//
// producer: Ahn Sang-Kyu
//*****************************************************************************

#if !defined(AFX_NEWUITRADE_H__25FC9B24_8F86_4791_B246_689326623DFB__INCLUDED_)
#define AFX_NEWUITRADE_H__25FC9B24_8F86_4791_B246_689326623DFB__INCLUDED_

#pragma once

#include "NewUIBase.h"
#include "NewUIMessageBox.h"
#include "NewUIMyInventory.h"
#include "NewUIMyQuestInfoWindow.h"
#include "NewUIStorageInventory.h"

namespace SEASON3B
{
	class CNewUITrade : public CNewUIObj  
	{
	public:
		enum IMAGE_LIST
		{
			IMAGE_TRADE_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,	// Reference
			IMAGE_TRADE_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
			IMAGE_TRADE_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
			IMAGE_TRADE_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
			IMAGE_TRADE_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,

			IMAGE_TRADE_LINE = CNewUIMyQuestInfoWindow::IMAGE_MYQUEST_LINE,
			IMAGE_TRADE_NICK_BACK = BITMAP_INTERFACE_NEW_TRADE_BEGIN,
			IMAGE_TRADE_MONEY = CNewUIMyInventory::IMAGE_INVENTORY_MONEY,
			IMAGE_TRADE_CONFIRM = BITMAP_INTERFACE_NEW_TRADE_BEGIN + 1,
			IMAGE_TRADE_WARNING_ARROW = BITMAP_CURSOR+7,

			IMAGE_TRADE_BTN_CLOSE = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
			IMAGE_TRADE_BTN_ZEN_INPUT = CNewUIStorageInventory::IMAGE_STORAGE_BTN_INSERT_ZEN,
		};
		
	private:
		enum
		{
			TRADE_WIDTH = 190,
			TRADE_HEIGHT = 429,
			CONFIRM_WIDTH = 36,
			CONFIRM_HEIGHT = 29,
			COLUMN_TRADE_INVEN = 8,
			ROW_TRADE_INVEN = 4,
			MAX_TRADE_INVEN = COLUMN_TRADE_INVEN * ROW_TRADE_INVEN,
		};

		enum TRADE_BUTTON
		{
			BTN_CLOSE = 0,			// Ã¢ ´Ý±â.
			BTN_ZEN_INPUT,			// Á¨ ÀÔ·Â.
			MAX_BTN
		};

		CNewUIManager*			m_pNewUIMng;			// UI ¸Å´ÏÀú.
		POINT					m_Pos;					// Ã¢ÀÇ À§Ä¡.
		CNewUIButton			m_abtn[MAX_BTN];		// ¹öÆ°.
		POINT					m_posMyConfirm;			// ³» È®Á¤ ¹öÆ° À§Ä¡.
		CNewUIInventoryCtrl*	m_pYourInvenCtrl;		// »ó´ë¹æ ¹°Ç° ÄÁÆ®·Ñ.
		CNewUIInventoryCtrl*	m_pMyInvenCtrl;			// ³» ¹°Ç° ÄÁÆ®·Ñ.
		ITEM					m_aYourInvenBackUp[MAX_TRADE_INVEN];// »ó´ë¹æ ¹°Ç° ¹é¾÷.

		char					m_szYourID[MAX_ID_SIZE+1];// °Å·¡ »ç¿ëÀÚÀÇ ¾ÆÀÌµð.
		int						m_nYourLevel;			// °Å·¡ »ç¿ëÀÚÀÇ ·¹º§.
		int						m_nYourGuildType;		// »ó´ë¹æ ±æµå Å¸ÀÔ.
		int						m_nYourTradeGold;		// »ó´ë¹æ °Å·¡ÇÒ µ·.
		int						m_nMyTradeGold;			// ÀÚ½ÅÀÇ °Å·¡ÇÒ µ·.
		int						m_nTempMyTradeGold;		// ÀÚ½ÅÀÇ °Å·¡ÇÒ µ· ÀÓ½Ã °ø°£.
		bool					m_bYourConfirm;			// »ó´ë¹æ °Å·¡ °áÁ¤ »óÅÂ.
		bool					m_bMyConfirm;			// ÀÚ½ÅÀÇ °Å·¡ °áÁ¤ »óÅÂ.
		int						m_nMyTradeWait;			// ÀÚ½ÅÀÇ °Å·¡ °áÁ¤ ¹öÆ° ¸ø ´©¸£°Ô ÇÏ´Â ´ë±â ½Ã°£.
		bool					m_bTradeAlert;			// °Å·¡½Ã °æ°í.

	public:
		CNewUITrade();
		virtual ~CNewUITrade();

		bool Create(CNewUIManager* pNewUIMng, int x, int y);
		void Release();
		
		void SetPos(int x, int y);
		const POINT& GetPos() const;
		
		bool UpdateMouseEvent();
		bool UpdateKeyEvent();
		bool Update();
		bool Render();
		
		float GetLayerDepth();	//. 2.1f

		static void UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB);
		
		// (°ÝÀÚ ¸ð¾çÀÇ) »ó´ëÆí °Å·¡ ¼ÒÁöÇ° ÄÁÆ®·ÑÀ» ¾òÀ½.
		CNewUIInventoryCtrl* GetYourInvenCtrl() const
		{ return m_pYourInvenCtrl; }
		// (°ÝÀÚ ¸ð¾çÀÇ) ÀÚ½ÅÀÇ °Å·¡ ¼ÒÁöÇ° ÄÁÆ®·ÑÀ» ¾òÀ½.
		CNewUIInventoryCtrl* GetMyInvenCtrl() const
		{ return m_pMyInvenCtrl; }

		void ProcessCloseBtn();
		void ProcessClosing();

		void GetYourID(char* pszYourID);
		void SetYourTradeGold(int nGold){ m_nYourTradeGold = nGold; }

		void SendRequestMyGoldInput(int nInputGold);
		void SendRequestItemToMyInven(ITEM* pItemObj,
			int nTradeIndex, int nInvenIndex);

		void ProcessToReceiveTradeRequest(BYTE* pbyYourID);
		void ProcessToReceiveTradeResult(LPPTRADE pTradeData);
		void ProcessToReceiveYourItemDelete(BYTE byYourInvenIndex);
		void DeleteMyTrade(int Slot);
		void ProcessToReceiveYourItemAdd(BYTE byYourInvenIndex, BYTE* pbyItemPacket);
		void ProcessToReceiveMyTradeGold(BYTE bySuccess);
		void ProcessToReceiveYourConfirm(BYTE byState);
		void ProcessToReceiveTradeExit(BYTE byState);
		void ProcessToReceiveTradeItems(int nIndex, BYTE* pbyItemPacket);

		void AlertTrade();

		int GetPointedItemIndexMyInven();
		int GetPointedItemIndexYourInven();

		void SendRequestItemToTrade(ITEM* pItemObj, int nInvenIndex, int nTradeIndex);
	private:
		void LoadImages();
		void UnloadImages();

		void RenderBackImage();
		void RenderText();
		void RenderWarningArrow();

		void ProcessMyInvenCtrl();
		bool ProcessBtns();

		void ConvertYourLevel(int& rnLevel, DWORD& rdwColor);

		void InitTradeInfo();
		void InitYourInvenBackUp();
		void BackUpYourInven(int nYourInvenIndex);
		void BackUpYourInven(ITEM* pYourItemObj);
		void AlertYourTradeInven();

	};
}

#endif // !defined(AFX_NEWUITRADE_H__25FC9B24_8F86_4791_B246_689326623DFB__INCLUDED_)
