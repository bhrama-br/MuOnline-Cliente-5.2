// TestEventMap.cpp: implementation of the TestEventMap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestEventMap.h"

#ifdef PSW_ADD_TESTMAP

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TestEventMapPtr	TestEventMap::Make()
{
	TestEventMapPtr map( new TestEventMap );
	return map;
}

TestEventMap::TestEventMap() : m_IsTest( true )
{

}

TestEventMap::~TestEventMap()
{

}


// ¿ÀºêÁ§Æ® »ý¼º
bool TestEventMap::CreateObject(OBJECT* o)
{
	return false;
}

// ¿ÀºêÁ§Æ® ÇÁ·Î¼¼¼­
bool TestEventMap::MoveObject(OBJECT* o)
{
	return false;
}

// ¿ÀºêÁ§Æ® ÀÌÆåÆ®
bool TestEventMap::RenderObjectVisual(OBJECT* o, BMD* b)
{
	return false;
}

// ¿ÀºêÁ§Æ® ¸Å½¬ ÀÌÆåÆ®
bool TestEventMap::RenderObjectMesh(OBJECT* o, BMD* b, bool ExtraMon)
{
	return false;
}

// ¸Ê °ü·Ã ¿ÀºêÁ§Æ® ÀÌÆåÆ®
void TestEventMap::RenderAfterObjectMesh(OBJECT* o, BMD* b, bool ExtraMon)
{

}

// ¸ó½ºÅÍ »ý¼º
CHARACTER* TestEventMap::CreateMonster(int iType, int PosX, int PosY, int Key)
{
	CHARACTER* pCharacter = NULL;
	return pCharacter;
}

// ¸ó½ºÅÍ(NPC) ÇÁ·Î¼¼¼­
bool TestEventMap::MoveMonsterVisual(OBJECT* o, BMD* b)
{
	return false;
}
// ¸ó½ºÅÍ ½ºÅ³ ºí·¯ ÀÌÆåÆ®
void TestEventMap::MoveBlurEffect(CHARACTER* c, OBJECT* o, BMD* b)
{

}

// ¸ó½ºÅÍ ÀÌÆåÆ® ( ÀÏ¹Ý )	
bool TestEventMap::RenderMonsterVisual(CHARACTER* c, OBJECT* o, BMD* b)
{
	return false;
}

// ¸ó½ºÅÍ ÀÌÆåÆ® ( ½ºÅ³ )
bool TestEventMap::AttackEffectMonster(CHARACTER* c, OBJECT* o, BMD* b)
{
	return false;
}

// ½ºÅ³ ¾Ö´Ï¸ÞÀÌ¼Ç °ü·Ã ÇÔ¼ö
bool TestEventMap::SetCurrentActionMonster(CHARACTER* c, OBJECT* o)
{
	return false;
}

bool TestEventMap::PlayMonsterSound(OBJECT* o)
{
	return false;
}

void TestEventMap::PlayObjectSound(OBJECT* o)
{

}

#endif //PSW_ADD_TESTMAP