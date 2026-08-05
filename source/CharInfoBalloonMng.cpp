//*****************************************************************************
// File: CharInfoBalloonMng.cpp
//
// Desc: implementation of the CCharInfoBalloonMng class.
//
// producer: Ahn Sang-Kyu
//*****************************************************************************

#include "stdafx.h"
#include "CharInfoBalloonMng.h"
#include "CharacterList.h"
#include "CharInfoBalloon.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCharInfoBalloonMng::CCharInfoBalloonMng() : m_pCharInfoBalloon(NULL)
{

}

CCharInfoBalloonMng::~CCharInfoBalloonMng()
{
	Release();
}

void CCharInfoBalloonMng::Release()
{
	SAFE_DELETE_ARRAY(m_pCharInfoBalloon);
}

//*****************************************************************************
// ÇÔ¼ö ÀÌ¸§ : Create()
// ÇÔ¼ö ¼³¸í : Ä³¸¯ÅÍ Á¤º¸ Ç³¼± ¸Å´ÏÀú »ý¼º.
//			   (Ä³¸¯ÅÍ ¼±ÅÃ¾À¿¡¼­ ¾²ÀÓ. Ç³¼± 5°³ »ý¼º.)
//*****************************************************************************
void CCharInfoBalloonMng::Create()
{
	if (NULL == m_pCharInfoBalloon)
		m_pCharInfoBalloon = new CCharInfoBalloon[gCharacterList.MaxCharacters];

	for (int i = 0; i < gCharacterList.MaxCharacters; ++i)
		m_pCharInfoBalloon[i].Create(&CharactersClient[i]);
}

//*****************************************************************************
// ÇÔ¼ö ÀÌ¸§ : Render()
// ÇÔ¼ö ¼³¸í : Ä³¸¯ÅÍ Á¤º¸ Ç³¼±µé ·»´õ.
//*****************************************************************************
void CCharInfoBalloonMng::Render()
{
	if (NULL == m_pCharInfoBalloon)
		return;

	for (int i = 0; i < gCharacterList.MaxCharacters; ++i)
		m_pCharInfoBalloon[i].Render();
}

//*****************************************************************************
// ÇÔ¼ö ÀÌ¸§ : UpdateDisplay()
// ÇÔ¼ö ¼³¸í : Ä³¸¯ÅÍ Á¤º¸¸¦ ¾÷µ¥ÀÌÆ®.
//*****************************************************************************
void CCharInfoBalloonMng::UpdateDisplay()
{
	if (NULL == m_pCharInfoBalloon)
		return;

	for (int i = 0; i < gCharacterList.MaxCharacters; ++i)
		m_pCharInfoBalloon[i].SetInfo();
}