// GMDoppelGanger3.h: interface for the GMDoppelGanger3 class.
//////////////////////////////////////////////////////////////////////
#pragma once
#include "w_BaseMap.h"

BoostSmartPointer( CGMDoppelGanger3 );
class CGMDoppelGanger3 : public BaseMap  
{
public:
	static CGMDoppelGanger3Ptr Make();
	virtual ~CGMDoppelGanger3();

public:	// Object
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
	
public: // Sound
	// ¸ó½ºÅÍ »ç¿îµå
	virtual bool PlayMonsterSound(OBJECT* o);
	// ¿ÀºêÁ§Æ® »ç¿îµå
	virtual void PlayObjectSound(OBJECT* o);

public:
	void Init();
	void Destroy();

protected:
	CGMDoppelGanger3();
};

extern bool IsDoppelGanger3();
