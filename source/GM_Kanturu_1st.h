//*****************************************************************************
// File: GM_kanturu_1st.h
//
// Desc: Ä­Åõ¸£ 1Â÷(¿ÜºÎ) ¸Ê, ¸ó½ºÅÍ.
//
// producer: Ahn Sang-Kyu
//*****************************************************************************

#ifndef _GM_KANTURU_1ST_H_
#define _GM_KANTURU_1ST_H_

namespace M37Kanturu1st 
{
	bool IsKanturu1st();						// Ä­Åõ¸£ ¿ÜºÎ ¸ÊÀÎ°¡?

	// ¿ÀºêÁ§Æ® °ü·Ã
	bool CreateKanturu1stObject(OBJECT* pObject);
	bool MoveKanturu1stObject(OBJECT* pObject);
	bool RenderKanturu1stObjectVisual(OBJECT* pObject, BMD* pModel);
	bool RenderKanturu1stObjectMesh(OBJECT* o, BMD* b, bool ExtraMon = 0);	// ¿ÀºêÁ§Æ® ·»´õ(¸ó½ºÅÍ Æ÷ÇÔ)
	void RenderKanturu1stAfterObjectMesh(OBJECT* o, BMD* b);				// ¹ÝÅõ¸í ¿ÀºêÁ§Æ® ³ªÁß ·£´õ.

	// ¸ó½ºÅÍ °ü·Ã
	CHARACTER* CreateKanturu1stMonster(int iType, int PosX, int PosY, int Key);	// ¸ó½ºÅÍ »ý¼º
	bool SetCurrentActionKanturu1stMonster(CHARACTER* c, OBJECT* o);		// ¸ó½ºÅÍ ÇöÀç ¾×¼Ç ¼¼ÆÃ
	bool AttackEffectKanturu1stMonster(CHARACTER* c, OBJECT* o, BMD* b);	// ¸ó½ºÅÍ °ø°Ý ÀÌÆåÆ®
	bool MoveKanturu1stMonsterVisual(CHARACTER* c,OBJECT* o, BMD* b);		// ¸ó½ºÅÍ È¿°ú ¾÷µ¥ÀÌÆ®
	void MoveKanturu1stBlurEffect(CHARACTER* c, OBJECT* o, BMD* b);		// ¸ó½ºÅÍ ¹«±âÀÇ ÀÜ»ó Ã³¸®
	bool RenderKanturu1stMonsterObjectMesh(OBJECT* o, BMD* b,int ExtraMon);	// ¸ó½ºÅÍ ¿ÀºêÁ§Æ® ·»´õ¸µ
	bool RenderKanturu1stMonsterVisual(CHARACTER* c, OBJECT* o, BMD* b);	// ¸ó½ºÅÍ È¿°ú ·»´õ¸µ
};

#endif	// _GM_KANTURU_1ST_H_