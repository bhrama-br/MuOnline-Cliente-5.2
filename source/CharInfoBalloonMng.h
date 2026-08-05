//*****************************************************************************
// File: CharInfoBalloonMng.h
//
// Desc: interface for the CCharInfoBalloonMng class.
//		 Ä³¸¯ÅÍ Á¤º¸ Ç³¼± °ü¸® Å¬·¡½º.(Ä³¸¯ÅÍ ¼±ÅÃ¾À¿¡¼­ ¾²ÀÓ)
//
// producer: Ahn Sang-Kyu
//*****************************************************************************

#if !defined(AFX_CHARINFOBALLOONMNG_H__37129186_F7FE_4FBC_87BD_189E01191E8F__INCLUDED_)
#define AFX_CHARINFOBALLOONMNG_H__37129186_F7FE_4FBC_87BD_189E01191E8F__INCLUDED_

#pragma once

class CCharInfoBalloon;

class CCharInfoBalloonMng  
{
protected:
	CCharInfoBalloon*	m_pCharInfoBalloon;	// Ä³¸¯ÅÍ Á¤º¸ Ç³¼± ¹è¿­ÀÇ ÁÖ¼Ò.

public:
	CCharInfoBalloonMng();
	virtual ~CCharInfoBalloonMng();

	void Release();
	void Create();
	void Render();
	void UpdateDisplay();
};

#endif // !defined(AFX_CHARINFOBALLOONMNG_H__37129186_F7FE_4FBC_87BD_189E01191E8F__INCLUDED_)
