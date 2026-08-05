// ShadowVolume.h: interface for the CShadowVolume class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHADOWVOLUME_H__8C7DEDBA_0557_4B98_AD53_900F41EAC8AD__INCLUDED_)
#define AFX_SHADOWVOLUME_H__8C7DEDBA_0557_4B98_AD53_900F41EAC8AD__INCLUDED_

#pragma once

#include "ZzzBmd.h"

typedef struct
{
	short	m_nVertexIndex[2];
	short	m_nMesh;
	short	m_nNormalIndex[2];
} St_Edges;


class CShadowVolume
{
public:
	CShadowVolume();
	virtual ~CShadowVolume();

	void Clear( void);

	// a) ÃÖÁ¾ »ý¼ºµÈ ¼¨µµ¿ì º¼·ý Á¤º¸
protected:
	short	m_nNumVertices;	// Á¡ °³¼ö
	vec3_t	*m_pVertices;	// Á¡µé
protected:
	BOOL GetReadyToCreate( vec3_t ppVertexTransformed[MAX_MESH][MAX_VERTICES], BMD *b, OBJECT *o, bool SkipTga=true);	// »ý¼º
public:
	virtual void Create( vec3_t ppVertexTransformed[MAX_MESH][MAX_VERTICES], BMD *b, OBJECT *o, bool SkipTga=true);	// »ý¼º
	virtual void Destroy( void);	// Á¦°Å
	void RenderAsFrame( void);	// ¼¨µµ¿ì º¼·ýÀ» frame À¸·Î ±×¸®±â
	void Shade( void);	// ¹öÆÛ¿¡ ±×¸²ÀÚ ±×¸®±â

	// b) Áß°£ °úÁ¤
protected:
	vec3_t m_vLight;	// ºû
	int m_iNumEdge;		// °¡ÀåÀÚ¸® °³¼ö
	St_Edges *m_pEdges;	// °¡ÀåÀÚ¸®
	void DeterminateSilhouette( short nMesh, vec3_t ppVertexTransformed[MAX_MESH][MAX_VERTICES], short nNumTriangles, Triangle_t *pTriangles, bool Tga);	// Mesh º° °¡ÀåÀÚ¸® µû±â
	void AddEdge( short nV1, short nV2, short nMesh);	// °¡ÀåÀÚ¸® Ãß°¡
	void AddEdgeFast( short nV1, short nV2, short nMesh, int iTriangle, int Edge, Triangle_t *pTriangles);	// °¡ÀåÀÚ¸® Ãß°¡
	void GenerateSidePolygon( vec3_t ppVertexTransformed[MAX_MESH][MAX_VERTICES]);	// °¡ÀåÀÚ¸®¸¦ ÀÌ¿ëÇÑ Æú¸®°ï »ý¼º

	// c) Ç¥Çö
protected:
	void RenderShadowVolume( void);	// ¼¨µµ¿ì º¼·ýÀ» ÁöÁ¤µÈ ¹æ½ÄÀ¸·Î ±×¸®±â
};


void InsertShadowVolume( CShadowVolume *psv);
void RenderShadowVolumesAsFrame( void);
void ShadeWithShadowVolumes( void);
void RenderShadowToScreen( void);


//#endif //USE_SHADOWVOLUME


#endif // !defined(AFX_SHADOWVOLUME_H__8C7DEDBA_0557_4B98_AD53_900F41EAC8AD__INCLUDED_)
