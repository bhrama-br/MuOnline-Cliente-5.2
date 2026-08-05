//*****************************************************************************
// File: GMKarutan1.h
//
// Desc: Å³·çÅº1 ¸Ê, ¸ó½ºÅÍ.
//
// producer: Ahn Sang-Kyu (10.08.03)
//*****************************************************************************

#if !defined(AFX_GMKARUTAN1_H__A2F56C80_26D8_4474_AECE_63DA2DA511A9__INCLUDED_)
#define AFX_GMKARUTAN1_H__A2F56C80_26D8_4474_AECE_63DA2DA511A9__INCLUDED_

#pragma once

#ifdef ASG_ADD_MAP_KARUTAN

#include "w_BaseMap.h"

BoostSmartPointer( CGMKarutan1 );

class CGMKarutan1 : public BaseMap
{
protected:
	CGMKarutan1();

public:
	virtual ~CGMKarutan1();

	static CGMKarutan1Ptr Make();

// Object
	// ¿ÀºêÁ§Æ® »ý¼º
	virtual bool CreateObject(OBJECT* o);
	// ¿ÀºêÁ§Æ® ÇÁ·Î¼¼¼­
	virtual bool MoveObject(OBJECT* o);
	// ¿ÀºêÁ§Æ® ÀÌÆåÆ®
	virtual bool RenderObjectVisual(OBJECT* o, BMD* b);
	// ¿ÀºêÁ§Æ® ·£´õ
	virtual bool RenderObjectMesh(OBJECT* o, BMD* b, bool ExtraMon = 0);
	// ¸Ê °ü·Ã ¿ÀºêÁ§Æ® ÀÌÆåÆ®
	virtual void RenderAfterObjectMesh(OBJECT* o, BMD* b, bool ExtraMon = 0);
	
#ifdef ASG_ADD_KARUTAN_MONSTERS
// Character
	// ¸ó½ºÅÍ »ý¼º
	virtual CHARACTER* CreateMonster(int iType, int PosX, int PosY, int Key);
	// ¸ó½ºÅÍ(NPC) ÇÁ·Î¼¼¼­
	virtual bool MoveMonsterVisual(OBJECT* o, BMD* b);
	// ¸ó½ºÅÍ ½ºÅ³ ºí·¯ ÀÌÆåÆ®
	virtual void MoveBlurEffect(CHARACTER* c, OBJECT* o, BMD* b);
	// ¸ó½ºÅÍ ÀÌÆåÆ® ( ÀÏ¹Ý )	
	virtual bool RenderMonsterVisual(CHARACTER* c, OBJECT* o, BMD* b);
	// ¸ó½ºÅÍ ÀÌÆåÆ® ( ½ºÅ³ )
	virtual bool AttackEffectMonster(CHARACTER* c, OBJECT* o, BMD* b);
	// ½ºÅ³ ¾Ö´Ï¸ÞÀÌ¼Ç °ü·Ã ÇÔ¼ö
	virtual bool SetCurrentActionMonster(CHARACTER* c, OBJECT* o);
	
// Sound
	// ¸ó½ºÅÍ »ç¿îµå
	virtual bool PlayMonsterSound(OBJECT* o);
#endif	// ASG_ADD_KARUTAN_MONSTERS
	// ¿ÀºêÁ§Æ® »ç¿îµå
	virtual void PlayObjectSound(OBJECT* o);
	// ¹è°æÀ½¾Ç
	void PlayBGM();
};

extern bool IsKarutanMap();

#endif	// ASG_ADD_MAP_KARUTAN

#endif // !defined(AFX_GMKARUTAN1_H__A2F56C80_26D8_4474_AECE_63DA2DA511A9__INCLUDED_)
