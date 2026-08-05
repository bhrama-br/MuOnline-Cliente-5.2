// TestEventMap.h: interface for the TestEventMap class.
//
//////////////////////////////////////////////////////////////////////

#ifdef PSW_ADD_TESTMAP

#if !defined(AFX_TESTEVENTMAP_H__9ABFF262_D93D_4B3C_9620_681662E7A384__INCLUDED_)
#define AFX_TESTEVENTMAP_H__9ABFF262_D93D_4B3C_9620_681662E7A384__INCLUDED_

#pragma once

#include "w_BaseMap.h"

BoostSmartPointer( TestEventMap );
class TestEventMap : public BaseMap
{
public:
	static TestEventMapPtr	Make();
	virtual ~TestEventMap();	

public:	// Object
	// ¿ÀºêÁ§Æ® »ý¼º
	virtual bool CreateObject(OBJECT* o);
	// ¿ÀºêÁ§Æ® ÇÁ·Î¼¼¼­
	virtual bool MoveObject(OBJECT* o);
	// ¿ÀºêÁ§Æ® ÀÌÆåÆ®
	virtual bool RenderObjectVisual(OBJECT* o, BMD* b);
	// ¿ÀºêÁ§Æ® ¸Å½¬ ÀÌÆåÆ®
	virtual bool RenderObjectMesh(OBJECT* o, BMD* b, bool ExtraMon = 0);
	// ¸Ê °ü·Ã ¿ÀºêÁ§Æ® ÀÌÆåÆ®
	virtual void RenderAfterObjectMesh(OBJECT* o, BMD* b, bool ExtraMon = 0);
	
public:	// Character
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

public:
	virtual bool PlayMonsterSound(OBJECT* o);
	virtual void PlayObjectSound(OBJECT* o);

public:
	const bool GetTest() const;

public:
	TestEventMap();

private:
	bool m_IsTest;

};

inline
const bool TestEventMap::GetTest() const
{
	return m_IsTest;
}

#endif // !defined(AFX_TESTEVENTMAP_H__9ABFF262_D93D_4B3C_9620_681662E7A384__INCLUDED_)

#endif //PSW_ADD_TESTMAP
