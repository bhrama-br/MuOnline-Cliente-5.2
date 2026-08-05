// LoadData.cpp: implementation of the CLoadData class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LoadData.h"
#include "GlobalBitmap.h"
#include "ZzzBMD.h"
#include "ZzzTexture.h"

CLoadData gLoadData;

CLoadData::CLoadData() // OK
{

}

CLoadData::~CLoadData() // OK
{
}

void CLoadData::AccessModel(int Type,const char *Dir,const char *FileName,int i)
{
	char Name[64];
	if(i == -1)
     	sprintf(Name,"%s.bmd",FileName);
	else if(i < 10)
     	sprintf(Name,"%s0%d.bmd",FileName,i);
	else
     	sprintf(Name,"%s%d.bmd",FileName,i);

    bool Success = false;

	Models[Type].m_iBMDSeqID = Type;

	Success = Models[Type].Open2(Dir,Name);

    if ( Success==false && ( strcmp(FileName,"Monster")==NULL || strcmp(FileName,"Player")==NULL || strcmp(FileName,"PlayerTest")==NULL || strcmp(FileName,"Angel")==NULL ) )
    {
		char Text[256];
    	sprintf(Text,"%s file does not exist.",Name );
		MessageBox(g_hWnd,Text,NULL,MB_OK);
		SendMessage(g_hWnd,WM_DESTROY,0,0);
    }
}

void CLoadData::OpenTexture(int Model,const char *SubFolder, int Wrap, int Type,bool Check)
{
	BMD* pModel = &Models[Model];

	for(int i=0;i<pModel->NumMeshs;i++)
	{
		Texture_t* pTexture = &pModel->Textures[i];
		
		char szFullPath[256] = {0, };
		strcpy(szFullPath,"Data\\");
		strcat(szFullPath, SubFolder);
		strcat(szFullPath, pTexture->FileName);
		
		char __ext[_MAX_EXT] = {0, };
		_splitpath(pTexture->FileName, NULL, NULL, NULL, __ext);

		// `IndexTexture` vem de `new GLuint[...]`, ou seja, memoria NAO INICIALIZADA.
		// A cadeia de `else if` abaixo cobre apenas extensao .t* e .j*: qualquer outra
		// (ou nome sem extensao, ou nome de 32 bytes sem terminador no .bmd) deixava a
		// entrada com lixo, e o teste `== BITMAP_UNKNOWN` logo adiante nao pegava esse
		// caso -- a malha era desenhada com um indice de textura invalido, o que sai na
		// tela como geometria BRANCA e sem nenhuma mensagem de erro.
		// Semear com BITMAP_UNKNOWN faz o proprio tratamento de erro que ja existe
		// (FindTextureByName / "OpenTexture Failed") cobrir tambem esse caminho.
		pModel->IndexTexture[i] = BITMAP_UNKNOWN;

		if(pTexture->FileName[0]=='s' && pTexture->FileName[1]=='k' && pTexture->FileName[2]=='i')
			pModel->IndexTexture[i] = BITMAP_SKIN;
#ifdef PBG_ADD_NEWCHAR_MONK
		else if(!strnicmp(pTexture->FileName, "level", 5))
		{
			pModel->IndexTexture[i] = BITMAP_SKIN;
		}
#else //PBG_ADD_NEWCHAR_MONK
		else if(pTexture->FileName[0]=='l' && pTexture->FileName[1]=='e' && pTexture->FileName[2]=='v' &&
			pTexture->FileName[3]=='e' && pTexture->FileName[4]=='l')
			pModel->IndexTexture[i] = BITMAP_SKIN;
#endif //PBG_ADD_NEWCHAR_MONK
		else if(pTexture->FileName[0]=='h' && pTexture->FileName[1]=='i' && pTexture->FileName[2]=='d')
			pModel->IndexTexture[i] = BITMAP_HIDE;
		else if (pTexture->FileName[0]=='h' && pTexture->FileName[1]=='a' && pTexture->FileName[2]=='i' && 
			pTexture->FileName[3]=='r' )
			pModel->IndexTexture[i] = BITMAP_HAIR;
		else if(tolower(__ext[1])=='t')
			pModel->IndexTexture[i] = Bitmaps.LoadImage(szFullPath,GL_NEAREST,Wrap);
		else if(tolower(__ext[1])=='j')
			pModel->IndexTexture[i] = Bitmaps.LoadImage(szFullPath,Type,Wrap);

		if(pModel->IndexTexture[i] == BITMAP_UNKNOWN)
		{
			BITMAP_t* pBitmap = Bitmaps.FindTextureByName(pTexture->FileName);
			if(pBitmap)
			{
				Bitmaps.LoadImage(pBitmap->BitmapIndex, pBitmap->FileName);
				pModel->IndexTexture[i] = pBitmap->BitmapIndex;
			}
			else
			{
				char szErrorMsg[256] = {0, };
				sprintf(szErrorMsg, "OpenTexture Failed: %s of %s", szFullPath, pModel->Name);
#ifdef FOR_WORK
				PopUpErrorCheckMsgBox(szErrorMsg);
#else // FOR_WORK
				PopUpErrorCheckMsgBox(szErrorMsg, true);
#endif // FOR_WORK
				break;
				
			}
		}
	}
}