
#ifndef _GMHUNTINGGROUND_H_
#define _GMHUNTINGGROUND_H_

//. soyaviper

namespace M31HuntingGround {		//. °ø¼º »ç³ÉÅÍ

	bool IsInHuntingGround();
	bool IsInHuntingGroundSection2(const vec3_t Position);

	//. ¿ÀºêÁ§Æ®
	bool CreateHuntingGroundObject(OBJECT* pObject);
	bool MoveHuntingGroundObject(OBJECT* pObject);
	bool RenderHuntingGroundObjectVisual(OBJECT* pObject, BMD* pModel);
	bool RenderHuntingGroundObjectMesh(OBJECT* pObject, BMD* pModel,bool ExtraMon = 0);
	
	//. ¸ó½ºÅÍ
	CHARACTER* CreateHuntingGroundMonster(int iType, int PosX, int PosY, int Key);

	bool MoveHuntingGroundMonsterVisual(OBJECT* pObject, BMD* pModel);
	void MoveHuntingGroundBlurEffect(CHARACTER* pCharacter, OBJECT* pObject, BMD* pModel);
	bool RenderHuntingGroundMonsterObjectMesh(OBJECT* pObject, BMD* pModel,bool ExtraMon);
	bool RenderHuntingGroundMonsterVisual(CHARACTER* pCharacter, OBJECT* pObject, BMD* pModel);
	
	bool AttackEffectHuntingGroundMonster(CHARACTER* pCharacter, OBJECT* pObject, BMD* pModel);
	bool SetCurrentActionHuntingGroundMonster(CHARACTER* pCharacter, OBJECT* pObject);
	

	//. È­¸é Ã³¸®
	bool CreateMist(PARTICLE* pParticleObj);
}

#endif // _GMHUNTINGGROUND_H_