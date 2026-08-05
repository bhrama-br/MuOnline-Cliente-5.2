// GM_Kanturu_In.h: interface for the GM_Kanturu_In class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GM_KANTURU_IN_H__37F1E344_37B1_497A_9B04_84409C768A74__INCLUDED_)
#define AFX_GM_KANTURU_IN_H__37F1E344_37B1_497A_9B04_84409C768A74__INCLUDED_

#pragma once

#include "ZzzBMD.h"

// TODO
namespace M38Kanturu2nd 
{
	// Ä­Åõ·ç ¿ÀºêÁ§Æ® °ü·Ã
	bool Create_Kanturu2nd_Object(OBJECT* o);										// ¿ÀºêÁ§Æ® »ý¼º
	bool Move_Kanturu2nd_Object(OBJECT* o);											// ¿ÀºêÁ§Æ® ¾÷µ¥ÀÌÆ®
	bool Render_Kanturu2nd_ObjectVisual(OBJECT* o, BMD* b);							// ¿ÀºêÁ§Æ® ÀÌÆåÆ® È¿°ú
	bool Render_Kanturu2nd_ObjectMesh(OBJECT* o, BMD* b,bool ExtraMon = 0);			// ¿ÀºêÁ§Æ® ·»´õ¸µ(¸ó½ºÅÍ Æ÷ÇÔ)
	void Render_Kanturu2nd_AfterObjectMesh(OBJECT* o, BMD* b);

	// Ä­Åõ·ç³»ºÎ ¸ó½ºÅÍ °ü·Ã
	CHARACTER*	Create_Kanturu2nd_Monster(int iType, int PosX, int PosY, int Key);	// ¸ó½ºÅÍ »ý¼º ÇÔ¼ö
	bool	Set_CurrentAction_Kanturu2nd_Monster(CHARACTER* c, OBJECT* o);		// ¸ó½ºÅÍ ÇöÀç ¾×¼Ç ¼¼ÆÃ
	bool	AttackEffect_Kanturu2nd_Monster(CHARACTER* c, OBJECT* o, BMD* b);	// ¸ó½ºÅÍ °ø°Ý ÀÌÆåÆ®
	bool	Move_Kanturu2nd_MonsterVisual(CHARACTER* c,OBJECT* o, BMD* b);		// ¸ó½ºÅÍ È¿°ú ¾÷µ¥ÀÌÆ®
	void	Move_Kanturu2nd_BlurEffect(CHARACTER* c, OBJECT* o, BMD* b);		// ¸ó½ºÅÍ ¹«±âÀÇ ÀÜ»ó Ã³¸®
	bool	Render_Kanturu2nd_MonsterObjectMesh(OBJECT* o, BMD* b,int ExtraMon);	// ¸ó½ºÅÍ ¿ÀºêÁ§Æ® ·»´õ¸µ
	bool	Render_Kanturu2nd_MonsterVisual(CHARACTER* c, OBJECT* o, BMD* b);	// ¸ó½ºÅÍ È¿°ú ·»´õ¸µ

	// Ä­Åõ·ç³»ºÎ ¸Ê °ü·Ã
	bool		Is_Kanturu2nd();						// Ä­Åõ·ç³»ºÎ ¸ÊÀÎ°¡?
	bool		Is_Kanturu2nd_3rd();					// Ä­Åõ·ç³»ºÎ¿Í 3Â÷ ¸ÊÀÎ°¡?

	// »ç¿îµå
	void	Sound_Kanturu2nd_Object(OBJECT* o);		// ¿ÀºêÁ§Æ® »ç¿îµå
	void	PlayBGM();
};

class CTrapCanon
{
public:
	CTrapCanon();
	~CTrapCanon();

private:
	void Initialize();
	void Destroy();

public:
	void Open_TrapCanon();
	CHARACTER* Create_TrapCanon(int iPosX, int iPosY, int iKey);
	void Render_Object(OBJECT* o, BMD* b);
	void Render_Object_Visual(CHARACTER* c, OBJECT* o, BMD* b);
	void Render_AttackEffect(CHARACTER* c, OBJECT* o, BMD* b);
};

extern CTrapCanon g_TrapCanon;

#endif // !defined(AFX_GM_KANTURU_IN_H__37F1E344_37B1_497A_9B04_84409C768A74__INCLUDED_)

