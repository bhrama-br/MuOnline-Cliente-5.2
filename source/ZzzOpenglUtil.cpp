#include <chrono>
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Platform/LegacyFileAccess.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "zzzObject.h"
#include "zzzcharacter.h"
#include "Zzzinfomation.h"
#include "NewUISystem.h"
#include "Platform/LegacyRenderAdapter.h"
#include "Platform/LegacyMatrixStack.h"
#include "Platform/RenderPipeline.h"
#include "Platform/PlatformShell.h"

int     OpenglWindowX;     
int     OpenglWindowY;     
int     OpenglWindowWidth; 
int     OpenglWindowHeight;
bool    CameraTopViewEnable = false;
float   CameraViewNear      = 20.f;
float   CameraViewFar       = 2000.f;
float   CameraFOV           = 55.f;
float	CameraZoom = 0.f;
float	AngleX3D = 0.f;
float	AngleY3D = 0.f;
float	AngleZ3D = 0.f;
float	AngleRL = 0.f;
vec3_t  CameraPosition;
vec3_t  CameraAngle;
float   CameraMatrix[3][4];
vec3_t  MousePosition;
vec3_t  MouseTarget;
float   g_fCameraCustomDistance = 0.f;
bool    FogEnable   = false;
GLfloat FogDensity  = 0.0004f;
GLfloat FogColor[4] = {30/256.f,20/256.f,10/256.f,};

unsigned int WindowWidth  = 1024;
unsigned int WindowHeight = 768;
int          MouseX       = WindowWidth/2;
int          MouseY       = WindowHeight/2;
int          BackMouseX   = MouseX;
int          BackMouseY   = MouseY;
bool         MouseLButton;
bool 		 MouseLButtonPop;
bool 		 MouseLButtonPush;
bool         MouseRButton;
bool 		 MouseRButtonPop;
bool 		 MouseRButtonPush;
bool 	   	 MouseLButtonDBClick;
bool         MouseMButton;
bool         MouseMButtonPop;
bool         MouseMButtonPush;
int          MouseWheel;
DWORD		 MouseRButtonPress = 0;
bool		 BlockClick = false;
bool		 LockPlayerWalk = false;

//bool    showShoppingMall = false;

void OpenExploper(char *Name,char *para)
{
	Platform::OpenExternalUrl(Name, para);
}

bool CheckID_HistoryDay ( char* Name, WORD day )
{
    typedef struct  __day_history__
    {
        char ID[MAX_ID_SIZE+1];
        WORD date;
    }dayHistory;

    FILE* fp;
    dayHistory days[100];
    int   count = 0;
    WORD  num = 0;
    bool  sameName = false;
    bool  update = true;

    if ( ( fp=Platform::LegacyFileOpen( "dconfig.ini", "rb" ) )!=NULL )
    {
        fread ( &num, sizeof( WORD ), 1, fp );
        
        if ( num>100 )
        {
            num = 0;
        }
        else
        {
            for ( int i=0; i<num; ++i )
            {
                fread ( days[i].ID, sizeof( char ), MAX_ID_SIZE+1, fp );
                fread ( &days[i].date, sizeof( WORD ), 1, fp );

                if ( !strcmp( days[i].ID, Name ) )
                {
                    sameName = true;
                    if ( days[i].date==day )
                    {
                        update = false;
                        break;
                    }
                    days[i].date = day;
                }
                count++;
            }
        }
        fclose ( fp );
    }

    if ( update )
    {
        if ( !sameName )
        {
            memcpy ( days[num].ID, Name, (MAX_ID_SIZE+1)*sizeof( char ) );
            days[num].date = day;

            num++;
        }

        fp = Platform::LegacyFileOpen( "dconfig.ini", "wb" );

        fwrite ( &num, sizeof( WORD ), 1, fp );
        for ( int i=0; i<num; ++i )
        {
            fwrite ( days[i].ID, sizeof( char ), MAX_ID_SIZE+1, fp );
            fwrite ( &days[i].date, sizeof( WORD ), 1, fp );
        }

        fclose ( fp );
    }

//    showShoppingMall = update;

    return  update;
}

bool GrabEnable = false;
char GrabFileName[MAX_PATH];
int  GrabScreen = 0;
bool GrabFirst = false;

void SaveScreen()
{
	GrabFirst = true;

	/*if(!GrabFirst)
	{
		GrabFirst = true;
		for(int i=0;i<10000;i++)
		{
			GrabScreen = i;
			if(GrabScreen<10)
				sprintf(GrabFileName,"Screen000%d",GrabScreen);
			else if(GrabScreen<100)
				sprintf(GrabFileName,"Screen00%d",GrabScreen);
			else if(GrabScreen<1000)
				sprintf(GrabFileName,"Screen0%d",GrabScreen);
			else
				sprintf(GrabFileName,"Screen%d",GrabScreen);

			strcat( GrabFileName, lpszFileName);
			FILE *fp = Platform::LegacyFileOpen(GrabFileName,"rb");
			if(fp==NULL)
				break;
			else
				fclose(fp);
		}
	}
	else
	{
		if(GrabScreen<10)
			sprintf(GrabFileName,"Screen000%d",GrabScreen);
		else if(GrabScreen<100)
			sprintf(GrabFileName,"Screen00%d",GrabScreen);
		else if(GrabScreen<1000)
			sprintf(GrabFileName,"Screen0%d",GrabScreen);
		else
			sprintf(GrabFileName,"Screen%d",GrabScreen);

		strcat( GrabFileName, lpszFileName);
	}*/

	// glReadPixels precisa observar tambem os sprites ainda na fila de UI.
	Platform::FlushOpaqueWorldRenderQueue();
	Platform::FlushLegacyRenderBatch();
	unsigned char *Buffer = new unsigned char [(int)WindowWidth*(int)WindowHeight*3];
	glReadPixels(0,0,(int)WindowWidth,(int)WindowHeight,GL_RGB,GL_UNSIGNED_BYTE,Buffer);
	WriteJpeg(GrabFileName,(int)WindowWidth,(int)WindowHeight,Buffer,100);

	SAFE_DELETE_ARRAY(Buffer);
	
	GrabScreen++;
	GrabScreen %= 10000;
}

float PerspectiveX;
float PerspectiveY;
int   ScreenCenterX;
int   ScreenCenterY;
int   ScreenCenterYFlip;

// Ultima matriz 3D publicada no adapter. A UI instala uma matriz ortografica
// temporaria; depois do pop ela pode restaurar esta copia sem consultar o GL.
static float g_legacyProjection3D[16];
static float g_legacyModelView3D[16];
static bool g_legacy3DMatricesKnown = false;
static float g_legacyMatrixSnapshotProjection[8][16];
static float g_legacyMatrixSnapshotModelView[8][16];
static bool g_legacyMatrixSnapshotValid[8] = { false };
static int g_legacyMatrixSnapshotDepth = 0;
static float g_spriteProjectionStack[8][16];
static float g_spriteModelViewStack[8][16];
static bool g_spriteMatrixSaved[8] = { false };
static int g_spriteMatrixDepth = 0;

static void RememberLegacy3DMatrices(const float* projection, const float* modelView)
{
	memcpy(g_legacyProjection3D, projection, sizeof(g_legacyProjection3D));
	memcpy(g_legacyModelView3D, modelView, sizeof(g_legacyModelView3D));
	g_legacy3DMatricesKnown = true;
}

// glGetFloatv e uma consulta SINCRONA: o driver precisa drenar o pipeline antes
// de devolver o estado. `setup` deu 41% do frame na v9 e nao ha nada nele que
// custe isso em CPU, entao estas leituras sao o principal suspeito. O contador
// existe para provar ou descartar isso, em vez de continuar supondo.
unsigned long long g_matrixReadbackUs = 0;
unsigned long long g_matrixReadbackCalls = 0;

namespace
{
	struct ScopedMatrixReadbackTimer
	{
		ScopedMatrixReadbackTimer()
			: start(std::chrono::steady_clock::now()) {}
		~ScopedMatrixReadbackTimer()
		{
			g_matrixReadbackUs += static_cast<unsigned long long>(
				std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::steady_clock::now() - start).count());
			++g_matrixReadbackCalls;
		}
		std::chrono::steady_clock::time_point start;
	};
}

void GetOpenGLMatrix(float Matrix[3][4])
{
	float OpenGLMatrix[16];
#ifdef _WIN32
	{
		ScopedMatrixReadbackTimer timer;
		glGetFloatv(GL_MODELVIEW_MATRIX,OpenGLMatrix);
	}
#else
	// GLES3 nao tem pilha de matrizes nem GL_MODELVIEW_MATRIX; a leitura vinha
	// como INVALID_ENUM e o destino ficava com lixo. A fonte e a pilha em CPU.
	memcpy(OpenGLMatrix, Platform::LegacyGetMatrix(Platform::LegacyMatrixModelView),
	       sizeof(OpenGLMatrix));
#endif
	for(int i=0;i<3;i++)
	{
		for(int j=0;j<4;j++)
		{
			Matrix[i][j] = OpenGLMatrix[j*4+i];
		}
	}
}

// BeginOpengl precisa tanto da matriz para CameraMatrix quanto para o adapter
// GLSL. No Windows ambas vinham de glGetFloatv separadamente; fazer a copia a
// partir da mesma leitura evita uma consulta sincrona extra ao driver por passe.
// Recalcula na CPU exatamente a sequencia que BeginOpengl acabou de aplicar ao
// GL. Nao depende de estado anterior: comeca em identidade, como o proprio
// BeginOpengl faz depois do glPushMatrix. A matematica e a mesma que o alvo Web
// ja usa em producao, entao nao e codigo novo e sim codigo que o PC nunca
// compilou.
static void ComputeCameraMatricesOnCpu(float aspectWidth, float aspectHeight,
	float* projection, float* modelView)
{
	Platform::LegacySetMatrixMode(Platform::LegacyMatrixProjection);
	Platform::LegacyLoadIdentity();
	Platform::LegacyPerspective(CameraFOV, aspectWidth / aspectHeight, CameraViewNear, CameraViewFar * 1.4f);
	memcpy(projection, Platform::LegacyGetMatrix(Platform::LegacyMatrixProjection), sizeof(float) * 16);

	Platform::LegacySetMatrixMode(Platform::LegacyMatrixModelView);
	Platform::LegacyLoadIdentity();
	Platform::LegacyRotate(CameraAngle[1], 0.f, 1.f, 0.f);
	if (CameraTopViewEnable == false)
		Platform::LegacyRotate(CameraAngle[0], 1.f, 0.f, 0.f);
	Platform::LegacyRotate(CameraAngle[2], 0.f, 0.f, 1.f);
	Platform::LegacyTranslate(-CameraPosition[0], -CameraPosition[1], -CameraPosition[2]);
	memcpy(modelView, Platform::LegacyGetMatrix(Platform::LegacyMatrixModelView), sizeof(float) * 16);
}

// Divergencia maxima entre o calculo em CPU e a leitura do driver, no modo
// compare. Vale zero quando nunca divergiram.
float g_cpuMatrixMaxDivergence = 0.f;

void SyncLegacyRenderMatricesAndCamera(float cameraMatrix[3][4])
{
#ifdef _WIN32
	float projection[16];
	float modelView[16];
	{
		ScopedMatrixReadbackTimer timer;
		glGetFloatv(GL_PROJECTION_MATRIX, projection);
		glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
	}
	Platform::GetLegacyRenderAdapter().SetMatrices(projection, modelView);
	RememberLegacy3DMatrices(projection, modelView);

	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 4; ++j)
			cameraMatrix[i][j] = modelView[j * 4 + i];
	}
#else
	GetOpenGLMatrix(cameraMatrix);
	Platform::LegacyApplyMatricesToAdapter();
#endif
}

void gluPerspective2(float Fov,float Aspect,float ZNear,float ZFar)
{
    gluPerspective(Fov,Aspect,ZNear,ZFar);

	ScreenCenterX      = OpenglWindowX + OpenglWindowWidth/2;
	ScreenCenterY      = OpenglWindowY + OpenglWindowHeight/2;
	ScreenCenterYFlip  = WindowWidth - ScreenCenterY;

	float AspectY = (float)(WindowHeight)/(float)(OpenglWindowHeight);
	PerspectiveX = tanf(Fov*0.5f*3.141592f/180.f)/(float)(OpenglWindowWidth /2)*Aspect;
	PerspectiveY = tanf(Fov*0.5f*3.141592f/180.f)/(float)(OpenglWindowHeight/2)*AspectY;
	//PerspectiveX = (float)ScreenCenterX/tanf(Fov*0.5f*3.141592f/180.f)*Aspect;
	//PerspectiveY = (float)ScreenCenterY/tanf(Fov*0.5f*3.141592f/180.f);
}

extern float g_fScreenRate_x;	// ¡Ø
extern float g_fScreenRate_y;

DWORD ConvertWX(float a1)	// ConvertModelX (1.0
{
	//return a1 * 1.25;
	return (DWORD)(g_fScreenRate_y <= 1.25) ? a1 * g_fScreenRate_x : a1 * g_fScreenRate_y;
}

void CreateScreenVector ( int sx, int sy, vec3_t Target, bool bFixView )
{
	sx = ConvertWX(sx);
	sy = sy * WindowHeight / GetWindowsY;
	vec3_t p1,p2;
    if ( bFixView )
    {
	    p1[0] =  (float)(sx-ScreenCenterX) * CameraViewFar * PerspectiveX;
	    p1[1] = -(float)(sy-ScreenCenterY) * CameraViewFar * PerspectiveY;
	    p1[2] = -CameraViewFar;
    }
    else
    {
	    p1[0] =  (float)(sx-ScreenCenterX) * RENDER_ITEMVIEW_FAR * PerspectiveX;
	    p1[1] = -(float)(sy-ScreenCenterY) * RENDER_ITEMVIEW_FAR * PerspectiveY;
	    p1[2] = -RENDER_ITEMVIEW_FAR;
    }

	p2[0] = -CameraMatrix[0][3];
	p2[1] = -CameraMatrix[1][3];
	p2[2] = -CameraMatrix[2][3];
	VectorIRotate(p2,CameraMatrix,MousePosition);
	VectorIRotate(p1,CameraMatrix,p2);
	VectorAdd(MousePosition,p2,Target);
}

void Projection(vec3_t Position,int *sx,int *sy)
{
	vec3_t TrasformPosition;
	VectorTransform(Position,CameraMatrix,TrasformPosition);
	*sx = -(int)(TrasformPosition[0] / PerspectiveX / TrasformPosition[2]) + ScreenCenterX;
	*sy =  (int)(TrasformPosition[1] / PerspectiveY / TrasformPosition[2]) + ScreenCenterY;
	*sx = GetWindowsX * *sx / WindowWidth;
	*sy = *sy * GetWindowsY / (int)WindowHeight;
}

void TransformPosition(vec3_t Position,vec3_t WorldPosition,int *x,int *y)
{
	vec3_t Temp;
	VectorSubtract(Position,CameraPosition,Temp);
	VectorRotate(Temp,CameraMatrix,WorldPosition);

	*x = (int)(WorldPosition[0]/PerspectiveX/-WorldPosition[2]) + (ScreenCenterX);
	*y = (int)(WorldPosition[1]/PerspectiveY/-WorldPosition[2]) + (ScreenCenterYFlip);
	//*y = (int)(WorldPosition[1]/PerspectiveY/-WorldPosition[2]) + (WindowHeight/2);
}

bool TestDepthBuffer(vec3_t Position)
{
	vec3_t WorldPosition;
	int x,y;
    TransformPosition(Position,WorldPosition,&x,&y);
	if(x<OpenglWindowX ||
		y<OpenglWindowY ||
		x>=(int)OpenglWindowX+OpenglWindowWidth ||
		y>=(int)OpenglWindowY+OpenglWindowHeight) return false;

	// A leitura de profundidade e uma barreira de ordenacao para a UI pendente.
	Platform::FlushLegacyRenderBatch();
	Platform::FlushOpaqueWorldRenderQueue();
	GLfloat key[3];
    glReadPixels(x,y,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,key);

	float z = 1.f - CameraViewNear/-WorldPosition[2] + CameraViewNear/CameraViewFar;
	if(key[0] >= z) return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// opengl render util
///////////////////////////////////////////////////////////////////////////////

int  CachTexture = -1;
bool TextureEnable;
bool DepthTestEnable;
bool CullFaceEnable;
bool DepthMaskEnable;
bool AlphaTestEnable;
int  AlphaBlendType;
static bool StencilTestEnable = false;
static bool StencilFuncKnown = false;
static unsigned int StencilFunction;
static int StencilReference;
static unsigned int StencilMask;
static bool StencilOpKnown = false;
static unsigned int StencilFail;
static unsigned int StencilDepthFail;
static unsigned int StencilDepthPass;
static bool DepthFuncKnown = false;
static unsigned int DepthFunction;

void BindTexture(int tex)
{
	// O adapter e avisado SEMPRE, fora do cache de CachTexture.
	//
	// O cache existe para evitar glBindTexture redundante, e no PC isso e seguro
	// porque o estado vive no GL. No backend GLSL nao: o adapter guarda a textura
	// e a religa no desenho (glBindTexture(m_texture) no End), derivando dela o
	// uniforme uUseTexture.
	//
	// Ha caminhos que mexem na textura POR FORA desta funcao -- Sprite::Render
	// chama renderer.BindTexture direto, e ZzzBMD usa glBindTexture cru no passe
	// de lightmap. Depois de um desses, CachTexture fica obsoleto: a proxima
	// chamada com o MESMO indice era descartada pelo cache e o adapter continuava
	// com a textura anterior. O sintoma era geometria do mundo desenhada sem
	// textura (ceu cinza plano, cena lavada), sem erro de GL nenhum.
	const unsigned int nome = (tex >= 0)
		? Bitmaps[tex].TextureNumber
		: static_cast<unsigned int>(-1 * tex);

	if (Platform::IsGlslLegacyBackendEnabled())
	{
		Platform::GetLegacyRenderAdapter().BindTexture(nome);
		CachTexture = tex;
		return;
	}


#if !defined(_WIN32)
	// Só fora do Windows. No PC o adapter emite um glBindTexture de verdade,
	// entao chamá-lo sempre anularia o cache e acrescentaria uma chamada GL por
	// bind no caminho quente de render. No backend GLSL a chamada e apenas
	// guardar um inteiro, entao e barata.
	Platform::GetLegacyRenderAdapter().BindTexture(nome);
#endif

	if(CachTexture != tex)
	{
		// A textura do lote anterior precisa chegar ao driver antes de trocar o
		// binding direto. Sem esta barreira, o adaptador descarregava os quads
		// pendentes ja com a textura do proximo botao/campo da UI.
		Platform::FlushLegacyRenderBatch();
      	CachTexture = tex;
		glBindTexture(GL_TEXTURE_2D, nome);
		Platform::InvalidateLegacyRenderStateCache();
#if defined(_WIN32)
		Platform::GetLegacyRenderAdapter().BindTexture(nome);
#endif
	}
}

bool TextureStream = false;

extern  int test;
// Stream de textura legacy: mantinha um glBegin(GL_TRIANGLES) aberto entre chamadas
// de funcao, fechando apenas na troca de textura ou em EndTextureStream. Nao possui
// nenhum chamador no projeto e o modelo de lote aberto nao tem equivalente no
// ILegacyRenderAdapter, que exige Begin/End no mesmo escopo. Preservado como
// referencia; reativar exige suporte a lote persistente no adapter.
#if 0
void BindTextureStream(int tex)
{
	if(CachTexture != tex)
	{
		CachTexture = tex;
		if(TextureStream)
			glEnd();
		BITMAP_t *b = &Bitmaps[tex];
		glBindTexture(GL_TEXTURE_2D,b->TextureNumber);

        glBegin(GL_TRIANGLES);
		TextureStream = true;
	}
}

void EndTextureStream()
{
	if(TextureStream)
     	glEnd();
	TextureStream = false;
}
#endif

void EnableDepthTest()
{
    if(!DepthTestEnable) 
	{
		DepthTestEnable = true;
		SetLegacyDepthTest(true);
	}
}

void DisableDepthTest()
{
    if(DepthTestEnable) 
	{
		DepthTestEnable = false;
		SetLegacyDepthTest(false);
	}
}

void EnableDepthMask()
{
    if(!DepthMaskEnable) 
	{
		Platform::FlushLegacyRenderBatch();
		DepthMaskEnable = true;
     	glDepthMask(true);
	}
}

void DisableDepthMask()
{
    if(DepthMaskEnable) 
	{
		Platform::FlushLegacyRenderBatch();
		DepthMaskEnable = false;
     	glDepthMask(false);
	}
}

void EnableCullFace()
{
    if(!CullFaceEnable) 
	{
		Platform::FlushLegacyRenderBatch();
		CullFaceEnable = true;
        glEnable(GL_CULL_FACE);
	}
}

void DisableCullFace()
{
    if(CullFaceEnable) 
	{
		Platform::FlushLegacyRenderBatch();
		CullFaceEnable = false;
        glDisable(GL_CULL_FACE);
	}
}

void EnableStencilTest()
{
    if (!StencilTestEnable)
    {
        Platform::FlushLegacyRenderBatch();
        StencilTestEnable = true;
        glEnable(GL_STENCIL_TEST);
    }
}

void DisableStencilTest()
{
    if (StencilTestEnable)
    {
        Platform::FlushLegacyRenderBatch();
        StencilTestEnable = false;
        glDisable(GL_STENCIL_TEST);
    }
}

void SetLegacyDepthFunc(unsigned int function)
{
    if (!DepthFuncKnown || DepthFunction != function)
    {
        Platform::FlushLegacyRenderBatch();
        DepthFuncKnown = true;
        DepthFunction = function;
        glDepthFunc(static_cast<GLenum>(function));
    }
}

void SetLegacyStencilFunc(unsigned int function, int reference, unsigned int mask)
{
    if (!StencilFuncKnown || StencilFunction != function || StencilReference != reference || StencilMask != mask)
    {
        Platform::FlushLegacyRenderBatch();
        StencilFuncKnown = true;
        StencilFunction = function;
        StencilReference = reference;
        StencilMask = mask;
        glStencilFunc(static_cast<GLenum>(function), reference, mask);
    }
}

void SetLegacyStencilOp(unsigned int fail, unsigned int depthFail, unsigned int depthPass)
{
    if (!StencilOpKnown || StencilFail != fail || StencilDepthFail != depthFail || StencilDepthPass != depthPass)
    {
        Platform::FlushLegacyRenderBatch();
        StencilOpKnown = true;
        StencilFail = fail;
        StencilDepthFail = depthFail;
        StencilDepthPass = depthPass;
        glStencilOp(static_cast<GLenum>(fail), static_cast<GLenum>(depthFail), static_cast<GLenum>(depthPass));
    }
}

void DisableTexture( bool AlphaTest )
{
    EnableDepthMask();
    if ( AlphaTest==true )
    {
        if(!AlphaTestEnable) 
	    {
		    AlphaTestEnable = true;
	        SetLegacyAlphaTest(true);
	    }
    }
    else
    {
        if(AlphaTestEnable) 
	    {
		    AlphaTestEnable = false;
	        SetLegacyAlphaTest(false);
	    }
    }
    if(TextureEnable) 
	{
		TextureEnable = false;
		SetLegacyTexture2D(false);
	}
}

void DisableAlphaBlend()
{
    if(AlphaBlendType != 0) 
	{
		// SetBlendMode descarrega o lote sob o blend anterior. A chamada GL
		// precisa ocorrer depois; do contrario o lote anterior recebe o blend
		// do proximo elemento e a transparencia da UI fica incorreta.
		Platform::GetLegacyRenderAdapter().SetBlendMode(0);
		AlphaBlendType = 0;
		glDisable(GL_BLEND);
	}
    EnableCullFace();
    EnableDepthMask();
    if(AlphaTestEnable) 
	{
		AlphaTestEnable = false;
	    SetLegacyAlphaTest(false);
	}
    if(!TextureEnable) 
	{
		TextureEnable = true;
		SetLegacyTexture2D(true);
	}
	if(FogEnable)
		SetLegacyFog(true);
}

void EnableAlphaTest(bool DepthMask)
{
    if(AlphaBlendType != 2)
	{
		Platform::GetLegacyRenderAdapter().SetBlendMode(2);
		AlphaBlendType = 2;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
    DisableCullFace();
	if(DepthMask)
        EnableDepthMask();
    if(!AlphaTestEnable) 
	{
		AlphaTestEnable = true;
	    SetLegacyAlphaTest(true);
	}
    if(!TextureEnable) 
	{
		TextureEnable = true;
		SetLegacyTexture2D(true);
	}
	if(FogEnable)
		SetLegacyFog(true);
}

void EnableAlphaBlend()
{
    if(AlphaBlendType != 3)
	{
		Platform::GetLegacyRenderAdapter().SetBlendMode(3);
		AlphaBlendType = 3;
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE,GL_ONE);
	}
    DisableCullFace();
    DisableDepthMask();
    if(AlphaTestEnable) 
	{
		AlphaTestEnable = false;
	    SetLegacyAlphaTest(false);
	}
    if(!TextureEnable) 
	{
		TextureEnable = true;
		SetLegacyTexture2D(true);
	}
	if(FogEnable)
		SetLegacyFog(false);
}

void EnableAlphaBlendMinus()
{
    if(AlphaBlendType != 4)
	{
		Platform::GetLegacyRenderAdapter().SetBlendMode(4);
		AlphaBlendType = 4;
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_COLOR);
	}
    DisableCullFace();
    DisableDepthMask();
    if(AlphaTestEnable) 
	{
		AlphaTestEnable = false;
	    SetLegacyAlphaTest(false);
	}
    if(!TextureEnable) 
	{
		TextureEnable = true;
		SetLegacyTexture2D(true);
	}
	if(FogEnable)
		SetLegacyFog(true);
}

void EnableAlphaBlend2()
{
    if(AlphaBlendType != 5)
	{
		Platform::GetLegacyRenderAdapter().SetBlendMode(5);
		AlphaBlendType = 5;
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE_MINUS_SRC_COLOR,GL_ONE);
	}
    DisableCullFace();
    DisableDepthMask();
    if(AlphaTestEnable) 
	{
		AlphaTestEnable = false;
	    SetLegacyAlphaTest(false);
	}
    if(!TextureEnable) 
	{
		TextureEnable = true;
		SetLegacyTexture2D(true);
	}
	if(FogEnable)
		SetLegacyFog(true);
}

void EnableAlphaBlend3()
{
    if(AlphaBlendType != 6)
	{
		Platform::GetLegacyRenderAdapter().SetBlendMode(6);
		AlphaBlendType = 6;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
    DisableCullFace();
    DisableDepthMask();
    if(AlphaTestEnable) 
	{
		AlphaTestEnable = false;
	    SetLegacyAlphaTest(false);
	}
    if(!TextureEnable) 
	{
		TextureEnable = true;
		SetLegacyTexture2D(true);
	}
	if(FogEnable)
		SetLegacyFog(true);
}

void EnableAlphaBlend4()
{
    if(AlphaBlendType != 7)
	{
		Platform::GetLegacyRenderAdapter().SetBlendMode(7);
		AlphaBlendType = 7;
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_COLOR);
	}
    DisableCullFace();
    DisableDepthMask();
    if(AlphaTestEnable) 
	{
		AlphaTestEnable = false;
	    SetLegacyAlphaTest(false);
	}
    if(!TextureEnable) 
	{
		TextureEnable = true;
		SetLegacyTexture2D(true);
	}
	if(FogEnable)
		SetLegacyFog(true);
}

void EnableLightMap()
{
    if(AlphaBlendType != 1)
	{
		Platform::GetLegacyRenderAdapter().SetBlendMode(1);
		AlphaBlendType = 1;
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO,GL_SRC_COLOR);
	}
    EnableCullFace();
    EnableDepthMask();
    if(AlphaTestEnable) 
	{
		AlphaTestEnable = false;
	    SetLegacyAlphaTest(false);
	}
    if(!TextureEnable) 
	{
		TextureEnable = true;
		SetLegacyTexture2D(true);
	}
	if(FogEnable)
		SetLegacyFog(true);
}

void glViewport2(int x,int y,int Width,int Height)
{
   	OpenglWindowX      = x;
   	OpenglWindowY      = y;
   	OpenglWindowWidth  = Width;
   	OpenglWindowHeight = Height;
    glViewport(x,WindowHeight-(y+Height),Width,Height);
}

extern float g_fScreenRate_x;
extern float g_fScreenRate_y;

float ConvertX(float x) {

	return (float)(x * g_fScreenRate_x);
}

float ConvertY(float y)
{
	return (float)(y * g_fScreenRate_y);
}

void BeginOpengl(int x,int y,int Width,int Height )
{
	// Troca de projecao/viewport e uma barreira de ordem para comandos do mundo.
	Platform::FlushOpaqueWorldRenderQueue();
	x = x * WindowWidth / GetWindowsX;
	y = y * WindowHeight / GetWindowsY;
	Width = Width * WindowWidth / GetWindowsX;
	Height = Height * WindowHeight / GetWindowsY;

    glMatrixMode(GL_PROJECTION);
	glPushMatrix();
    glLoadIdentity();
    glViewport2(x,y,Width,Height);

	gluPerspective2(CameraFOV,(float)Width/(float)Height,CameraViewNear,CameraViewFar*1.4f);
    
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
    glLoadIdentity();
    glRotatef(CameraAngle[1],0.f,1.f,0.f);
    if(CameraTopViewEnable == false)
		glRotatef(CameraAngle[0],1.f,0.f,0.f);
    glRotatef(CameraAngle[2],0.f,0.f,1.f);
    glTranslatef(-CameraPosition[0],-CameraPosition[1],-CameraPosition[2]);

    SetLegacyAlphaTest(false);
    SetLegacyTexture2D(true);
    SetLegacyDepthTest(true);
    // Culling e escrita de profundidade passam pelo cache; em frames onde o
    // estado ja esta correto evitamos duas chamadas ao driver.
    EnableCullFace();
    EnableDepthMask();
    AlphaTestEnable = false;
	TextureEnable   = true;
	DepthTestEnable = true;
    SetLegacyDepthFunc(GL_LEQUAL);
	SetLegacyAlphaRef(0.25f);
	if(FogEnable) 
	{
		SetLegacyFog(true);
#ifdef _WIN32
		glFogi(GL_FOG_MODE, GL_LINEAR);
		glFogf(GL_FOG_DENSITY, FogDensity);
		glFogfv(GL_FOG_COLOR, FogColor);
#endif
	}
	else
	{
		SetLegacyFog(false);
	}

    // BeginOpengl e o unico sitio onde a sequencia de matrizes e integralmente
    // conhecida (LoadIdentity + perspectiva + tres rotacoes + translacao), entao
    // e o unico onde o valor pode ser recalculado em vez de lido do driver. Os
    // demais SyncLegacyRenderMatrices vem depois de manipulacoes arbitrarias em
    // codigo de efeito e continuam com o readback.
#ifdef _WIN32
    const Platform::RenderFeatureMode cpuMatrixMode =
        Platform::GetRenderFeatureMode(Platform::RenderFeatureCpuMatrices);
    if (cpuMatrixMode != Platform::RenderFeatureDisabled)
    {
        float cpuProjection[16];
        float cpuModelView[16];
        ComputeCameraMatricesOnCpu((float)Width, (float)Height, cpuProjection, cpuModelView);

        if (cpuMatrixMode == Platform::RenderFeatureCompare)
        {
            // Compare aqui nao alterna por frame: faz as duas coisas e mede a
            // diferenca. Uma divergencia de matriz nao produz cintilacao obvia,
            // produz geometria sutilmente errada — um numero e mais confiavel
            // que o olho.
            float glProjection[16];
            float glModelView[16];
            {
                ScopedMatrixReadbackTimer timer;
                glGetFloatv(GL_PROJECTION_MATRIX, glProjection);
                glGetFloatv(GL_MODELVIEW_MATRIX, glModelView);
            }
            for (int i = 0; i < 16; ++i)
            {
                const float dp = fabsf(glProjection[i] - cpuProjection[i]);
                const float dm = fabsf(glModelView[i] - cpuModelView[i]);
                if (dp > g_cpuMatrixMaxDivergence) g_cpuMatrixMaxDivergence = dp;
                if (dm > g_cpuMatrixMaxDivergence) g_cpuMatrixMaxDivergence = dm;
            }
        }

        Platform::GetLegacyRenderAdapter().SetMatrices(cpuProjection, cpuModelView);
        RememberLegacy3DMatrices(cpuProjection, cpuModelView);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 4; ++j)
                CameraMatrix[i][j] = cpuModelView[j * 4 + i];
        return;
    }
#endif
    SyncLegacyRenderMatricesAndCamera(CameraMatrix);
}

void SetLegacyTexture2D(bool enabled)
{
#ifdef _WIN32
    if (!Platform::IsGlslLegacyBackendEnabled())
        enabled ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
#endif
    Platform::GetLegacyRenderAdapter().SetTexture2D(enabled);
}

void SetLegacyDepthTest(bool enabled)
{
    Platform::GetLegacyRenderAdapter().SetDepthTest(enabled);
}

void SetLegacyAlphaTest(bool enabled)
{
#ifdef _WIN32
    if (!Platform::IsGlslLegacyBackendEnabled())
        enabled ? glEnable(GL_ALPHA_TEST) : glDisable(GL_ALPHA_TEST);
#endif
    Platform::GetLegacyRenderAdapter().SetAlphaTest(enabled);
}

void SetLegacyAlphaRef(float reference)
{
#ifdef _WIN32
    if (!Platform::IsGlslLegacyBackendEnabled())
        glAlphaFunc(GL_GREATER, reference);
#endif
    Platform::GetLegacyRenderAdapter().SetAlphaTestRef(reference);
}

void SetLegacyFog(bool enabled)
{
#ifdef _WIN32
    if (!Platform::IsGlslLegacyBackendEnabled())
        enabled ? glEnable(GL_FOG) : glDisable(GL_FOG);
#endif
    // Em GLES3 o fog vira uniforme do shader. ATENCAO: o legado chama
    // glFogi(GL_FOG_MODE, GL_LINEAR) mas nunca define GL_FOG_START/GL_FOG_END,
    // entao herda os defaults do GL (0 e 1) — o que satura o fog logo apos o
    // near plane. Os limites abaixo reproduzem esse default; se o resultado
    // divergir do PC, calibrar aqui comparando com o build de referencia.
    Platform::GetLegacyRenderAdapter().SetFog(enabled, FogColor, 0.f, 1.f);
}

// Ultima cor definida. O legado a relia com glGetFloatv(GL_CURRENT_COLOR), que
// no GLES3 e INVALID_ENUM e devolvia lixo ao chamador.
static float g_corLegadaAtual[4] = { 1.f, 1.f, 1.f, 1.f };

void SetLegacyColor4f(float red,float green,float blue,float alpha)
{
#ifdef _WIN32
    if (!Platform::IsGlslLegacyBackendEnabled())
        glColor4f(red,green,blue,alpha);
#endif
    g_corLegadaAtual[0] = red;   g_corLegadaAtual[1] = green;
    g_corLegadaAtual[2] = blue;  g_corLegadaAtual[3] = alpha;
    Platform::GetLegacyRenderAdapter().Color4f(red,green,blue,alpha);
}

void GetLegacyColor4f(float* destino)
{
    for (int i = 0; i < 4; ++i) destino[i] = g_corLegadaAtual[i];
}

void SetLegacyColor4ub(unsigned char red,unsigned char green,unsigned char blue,unsigned char alpha)
{
    SetLegacyColor4f(red/255.f, green/255.f, blue/255.f, alpha/255.f);
}

void SetLegacyColor3ub(unsigned char red,unsigned char green,unsigned char blue)
{
    // glColor3* implica alpha 1.0.
    SetLegacyColor4f(red/255.f, green/255.f, blue/255.f, 1.f);
}

void SetLegacyColor3f(float red,float green,float blue)
{
    // glColor3* define alpha implicito 1.0.
    SetLegacyColor4f(red,green,blue,1.f);
}

void SetLegacyColor3fv(const float* color)
{
    SetLegacyColor4f(color[0],color[1],color[2],1.f);
}

void SyncLegacyRenderMatrices()
{
#ifdef _WIN32
    // No PC as matrizes seguem vivendo na pilha do OpenGL.
    float projection[16];
    float modelView[16];
    {
        ScopedMatrixReadbackTimer timer;
        glGetFloatv(GL_PROJECTION_MATRIX, projection);
        glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    }
    Platform::GetLegacyRenderAdapter().SetMatrices(projection, modelView);
    RememberLegacy3DMatrices(projection, modelView);
#else
    // Em GLES3/WebGL2 nao ha pilha de matrizes: a fonte e a pilha em CPU.
    Platform::LegacyApplyMatricesToAdapter();
#endif
}

void PushLegacyRenderMatrixSnapshot()
{
	const int slot = g_legacyMatrixSnapshotDepth++;
	if (Platform::IsGlslLegacyBackendEnabled() && g_legacy3DMatricesKnown && slot < 8)
	{
		memcpy(g_legacyMatrixSnapshotProjection[slot], g_legacyProjection3D, sizeof(g_legacyProjection3D));
		memcpy(g_legacyMatrixSnapshotModelView[slot], g_legacyModelView3D, sizeof(g_legacyModelView3D));
		g_legacyMatrixSnapshotValid[slot] = true;
	}
}

void PopLegacyRenderMatrixSnapshot()
{
	const int slot = --g_legacyMatrixSnapshotDepth;
	if (Platform::IsGlslLegacyBackendEnabled() && slot >= 0 && slot < 8 && g_legacyMatrixSnapshotValid[slot])
	{
		Platform::GetLegacyRenderAdapter().SetMatrices(g_legacyMatrixSnapshotProjection[slot], g_legacyMatrixSnapshotModelView[slot]);
		g_legacyMatrixSnapshotValid[slot] = false;
	}
	else
		SyncLegacyRenderMatrices();
}

// BeginBitmap sempre instala gluOrtho2D(0, WindowWidth, 0, WindowHeight)
// com modelview identidade. No backend GLSL publicar estes valores diretamente
// evita duas consultas glGetFloatv ao driver para um estado que ja conhecemos.
static void SyncLegacyBitmapMatrices()
{
    if (!Platform::IsGlslLegacyBackendEnabled())
    {
        SyncLegacyRenderMatrices();
        return;
    }

    float projection[16] = { 0.f };
    float modelView[16] = { 0.f };
    projection[0] = 2.f / static_cast<float>(WindowWidth);
    projection[5] = 2.f / static_cast<float>(WindowHeight);
    projection[10] = -1.f;
    projection[12] = -1.f;
    projection[13] = -1.f;
    projection[15] = 1.f;
    modelView[0] = modelView[5] = modelView[10] = modelView[15] = 1.f;
    Platform::GetLegacyRenderAdapter().SetMatrices(projection, modelView);
}

void EndOpengl()
{
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void UpdateMousePositionn()
{
	vec3_t vPos;

	glLoadIdentity();
	glTranslatef(-CameraPosition[0],-CameraPosition[1],-CameraPosition[2]);
	GetOpenGLMatrix(CameraMatrix);

	Vector(-CameraMatrix[0][3], -CameraMatrix[1][3], -CameraMatrix[2][3], vPos);
	VectorIRotate(vPos, CameraMatrix, MousePosition);
}

#ifdef LDS_ADD_MULTISAMPLEANTIALIASING
BOOL IsGLExtensionSupported(const char *extension)
{
	const size_t extlen = strlen(extension);
	const char *supported = NULL;
	
	// Try To Use wglGetExtensionStringARB On Current DC, If Possible
	PROC wglGetExtString = wglGetProcAddress("wglGetExtensionsStringARB");
	
	if (wglGetExtString)
		supported = ((char*(__stdcall*)(HDC))wglGetExtString)(wglGetCurrentDC());
	
	// If That Failed, Try Standard Opengl Extensions String
	if (supported == NULL)
		supported = (char*)glGetString(GL_EXTENSIONS);
	
	// If That Failed Too, Must Be No Extensions Supported
	if (supported == NULL)
		return FALSE;
	
	// Begin Examination At Start Of String, Increment By 1 On False Match
	for (const char* p = supported; ; p++)
	{
		// Advance p Up To The Next Possible Match
		p = strstr(p, extension);
		
		if (p == NULL)
			return FALSE;															// No Match
		
		if ((p==supported || p[-1]==' ') && (p[extlen]=='\0' || p[extlen]==' '))
			return TRUE;															// Match
	}
}

int GetFPSLimit()
{
    return GetDeviceCaps(g_hDC, VREFRESH);
}
BOOL InitGLMultisample(HINSTANCE hInstance,HWND hWnd,PIXELFORMATDESCRIPTOR pfd, int iRequestMSAAValue,int& OutiPixelFormat )
{
	BOOL bIsGLMultisampleSupported = FALSE;

#if defined(_DEBUG)
	CheckGLError( __FILE__, __LINE__ );
#endif // defined(_DEBUG)

	// See If The String Exists In WGL!
	if (!IsGLExtensionSupported("WGL_ARB_multisample"))
	{
		bIsGLMultisampleSupported=FALSE;
		return FALSE;
	}
	
#if defined(_DEBUG)
	CheckGLError( __FILE__, __LINE__ );
#endif // defined(_DEBUG)
	// Get Our Pixel Format
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");	
	if (!wglChoosePixelFormatARB) 
	{
		bIsGLMultisampleSupported=FALSE;
		return FALSE;
	}

#if defined(_DEBUG)
	CheckGLError( __FILE__, __LINE__ );
#endif // defined(_DEBUG)
	
	// Get Our Current Device Context
	HDC hDC = GetDC(hWnd);
	
	int		valid;
	UINT	numFormats;
	float	fAttributes[] = {0,0};
	
	// These Attributes Are The Bits We Want To Test For In Our Sample
	// Everything Is Pretty Standard, The Only One We Want To 
	// Really Focus On Is The SAMPLE BUFFERS ARB And WGL SAMPLES
	// These Two Are Going To Do The Main Testing For Whether Or Not
	// We Support Multisampling On This Hardware.
	int iAttributes[] =
	{
		WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
			WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
			WGL_COLOR_BITS_ARB,24,
			WGL_ALPHA_BITS_ARB,8,
			WGL_DEPTH_BITS_ARB,16,
			WGL_STENCIL_BITS_ARB,0,
			WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
			WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
			WGL_SAMPLES_ARB, iRequestMSAAValue,					// xN MultiSampling (N=4,2,1)
			0,0
	};
	
#if defined(_DEBUG)
	CheckGLError( __FILE__, __LINE__ );
#endif // defined(_DEBUG)

	// First We Check To See If We Can Get A Pixel Format For 4 Samples
	valid = wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&OutiPixelFormat,&numFormats);

#if defined(_DEBUG)
	CheckGLError( __FILE__, __LINE__ );
#endif // defined(_DEBUG)
	
	// If We Returned True, And Our Format Count Is Greater Than 1
	if (valid && numFormats >= 1)
	{
		bIsGLMultisampleSupported = TRUE;
		return bIsGLMultisampleSupported;
	}
	
#if defined(_DEBUG)
	CheckGLError( __FILE__, __LINE__ );
#endif // defined(_DEBUG)

	// Our Pixel Format With 4 Samples Failed, Test For 2 Samples
	iAttributes[19] = 2;
	valid = wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&OutiPixelFormat,&numFormats);
	if (valid && numFormats >= 1)
	{
		bIsGLMultisampleSupported = TRUE;
		return bIsGLMultisampleSupported;
	}

#if defined(_DEBUG)
	CheckGLError( __FILE__, __LINE__ );
#endif // defined(_DEBUG)
	
	// Return The Valid Format
	return  bIsGLMultisampleSupported;
}

void SetEnableMultisample()
{
	if( TRUE == g_bSupportedMSAA )
	{
		#ifdef _WIN32
		glEnable(GL_MULTISAMPLE_ARB);
#endif							// Enable Multisampling
	}

#if defined(_DEBUG)
	CheckGLError( __FILE__, __LINE__ );
#endif // defined(_DEBUG)
}

void SetDisableMultisample()
{
	if( TRUE == g_bSupportedMSAA )
	{
		#ifdef _WIN32
		glDisable(GL_MULTISAMPLE_ARB);
#endif							// Enable Multisampling
	}

#if defined(_DEBUG)
	CheckGLError( __FILE__, __LINE__ );
#endif // defined(_DEBUG)
}

#endif // LDS_ADD_MULTISAMPLEANTIALIASING

///////////////////////////////////////////////////////////////////////////////
// render util
///////////////////////////////////////////////////////////////////////////////

// TEXCOORD mudou para ZzzOpenglUtil.h -- ver o comentario lá.

#if 0
void RenderBox(float Matrix[3][4])
{
	vec3_t BoundingBoxMin;
	vec3_t BoundingBoxMax;
	Vector(-10.f,-30.f,-10.f,BoundingBoxMin);
	Vector(10.f,0.f,10.f,BoundingBoxMax);

	vec3_t BoundingVertices[8];
	Vector(BoundingBoxMax[0],BoundingBoxMax[1],BoundingBoxMax[2],BoundingVertices[0]);
	Vector(BoundingBoxMax[0],BoundingBoxMax[1],BoundingBoxMin[2],BoundingVertices[1]);
	Vector(BoundingBoxMax[0],BoundingBoxMin[1],BoundingBoxMax[2],BoundingVertices[2]);
	Vector(BoundingBoxMax[0],BoundingBoxMin[1],BoundingBoxMin[2],BoundingVertices[3]);
	Vector(BoundingBoxMin[0],BoundingBoxMax[1],BoundingBoxMax[2],BoundingVertices[4]);
	Vector(BoundingBoxMin[0],BoundingBoxMax[1],BoundingBoxMin[2],BoundingVertices[5]);
	Vector(BoundingBoxMin[0],BoundingBoxMin[1],BoundingBoxMax[2],BoundingVertices[6]);
	Vector(BoundingBoxMin[0],BoundingBoxMin[1],BoundingBoxMin[2],BoundingVertices[7]);
	
	vec3_t TransformVertices[8];
	for(int j=0;j<8;j++)
	{
   		VectorTransform(BoundingVertices[j],Matrix,TransformVertices[j]);
	}
	
	glBegin(GL_QUADS);
	//glBegin(GL_LINES);
	glColor3f(0.2f,0.2f,0.2f);
	glTexCoord2f( 1.0F, 1.0F); glVertex3fv(TransformVertices[7]);
	glTexCoord2f( 1.0F, 0.0F); glVertex3fv(TransformVertices[6]);
	glTexCoord2f( 0.0F, 0.0F); glVertex3fv(TransformVertices[4]);
	glTexCoord2f( 0.0F, 1.0F); glVertex3fv(TransformVertices[5]);
	
	glColor3f(0.2f,0.2f,0.2f);
	glTexCoord2f( 0.0F, 1.0F); glVertex3fv(TransformVertices[0]);
	glTexCoord2f( 1.0F, 1.0F); glVertex3fv(TransformVertices[2]);
	glTexCoord2f( 1.0F, 0.0F); glVertex3fv(TransformVertices[3]);
	glTexCoord2f( 0.0F, 0.0F); glVertex3fv(TransformVertices[1]);
	
	glColor3f(0.6f,0.6f,0.6f);
	glTexCoord2f( 1.0F, 1.0F); glVertex3fv(TransformVertices[7]);
	glTexCoord2f( 1.0F, 0.0F); glVertex3fv(TransformVertices[3]);
	glTexCoord2f( 0.0F, 0.0F); glVertex3fv(TransformVertices[2]);
	glTexCoord2f( 0.0F, 1.0F); glVertex3fv(TransformVertices[6]);
	
	glColor3f(0.6f,0.6f,0.6f);
	glTexCoord2f( 0.0F, 1.0F); glVertex3fv(TransformVertices[0]);
	glTexCoord2f( 1.0F, 1.0F); glVertex3fv(TransformVertices[1]);
	glTexCoord2f( 1.0F, 0.0F); glVertex3fv(TransformVertices[5]);
	glTexCoord2f( 0.0F, 0.0F); glVertex3fv(TransformVertices[4]);
	
	glColor3f(0.4f,0.4f,0.4f);
	glTexCoord2f( 1.0F, 1.0F); glVertex3fv(TransformVertices[7]);
	glTexCoord2f( 1.0F, 0.0F); glVertex3fv(TransformVertices[5]);
	glTexCoord2f( 0.0F, 0.0F); glVertex3fv(TransformVertices[1]);
	glTexCoord2f( 0.0F, 1.0F); glVertex3fv(TransformVertices[3]);
	
	glColor3f(0.4f,0.4f,0.4f);
	glTexCoord2f( 0.0F, 1.0F); glVertex3fv(TransformVertices[0]);
	glTexCoord2f( 1.0F, 1.0F); glVertex3fv(TransformVertices[4]);
	glTexCoord2f( 1.0F, 0.0F); glVertex3fv(TransformVertices[6]);
	glTexCoord2f( 0.0F, 0.0F); glVertex3fv(TransformVertices[2]);
	glEnd();
}
#endif

void RenderBox(float Matrix[3][4])
{
    vec3_t BoundingBoxMin;
    vec3_t BoundingBoxMax;
    Vector(-10.f,-30.f,-10.f,BoundingBoxMin);
    Vector(10.f,0.f,10.f,BoundingBoxMax);

    vec3_t BoundingVertices[8];
    Vector(BoundingBoxMax[0],BoundingBoxMax[1],BoundingBoxMax[2],BoundingVertices[0]);
    Vector(BoundingBoxMax[0],BoundingBoxMax[1],BoundingBoxMin[2],BoundingVertices[1]);
    Vector(BoundingBoxMax[0],BoundingBoxMin[1],BoundingBoxMax[2],BoundingVertices[2]);
    Vector(BoundingBoxMax[0],BoundingBoxMin[1],BoundingBoxMin[2],BoundingVertices[3]);
    Vector(BoundingBoxMin[0],BoundingBoxMax[1],BoundingBoxMax[2],BoundingVertices[4]);
    Vector(BoundingBoxMin[0],BoundingBoxMax[1],BoundingBoxMin[2],BoundingVertices[5]);
    Vector(BoundingBoxMin[0],BoundingBoxMin[1],BoundingBoxMax[2],BoundingVertices[6]);
    Vector(BoundingBoxMin[0],BoundingBoxMin[1],BoundingBoxMin[2],BoundingVertices[7]);

    vec3_t TransformVertices[8];
    for (int j = 0; j < 8; ++j)
        VectorTransform(BoundingVertices[j], Matrix, TransformVertices[j]);

    Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
    renderer.Begin(Platform::LegacyPrimitiveQuads);
    renderer.Color4f(0.2f, 0.2f, 0.2f, 1.f);
    renderer.TexCoord2f(1.f, 1.f); renderer.Vertex3fv(TransformVertices[7]);
    renderer.TexCoord2f(1.f, 0.f); renderer.Vertex3fv(TransformVertices[6]);
    renderer.TexCoord2f(0.f, 0.f); renderer.Vertex3fv(TransformVertices[4]);
    renderer.TexCoord2f(0.f, 1.f); renderer.Vertex3fv(TransformVertices[5]);
    renderer.Color4f(0.2f, 0.2f, 0.2f, 1.f);
    renderer.TexCoord2f(0.f, 1.f); renderer.Vertex3fv(TransformVertices[0]);
    renderer.TexCoord2f(1.f, 1.f); renderer.Vertex3fv(TransformVertices[2]);
    renderer.TexCoord2f(1.f, 0.f); renderer.Vertex3fv(TransformVertices[3]);
    renderer.TexCoord2f(0.f, 0.f); renderer.Vertex3fv(TransformVertices[1]);
    renderer.Color4f(0.6f, 0.6f, 0.6f, 1.f);
    renderer.TexCoord2f(1.f, 1.f); renderer.Vertex3fv(TransformVertices[7]);
    renderer.TexCoord2f(1.f, 0.f); renderer.Vertex3fv(TransformVertices[3]);
    renderer.TexCoord2f(0.f, 0.f); renderer.Vertex3fv(TransformVertices[2]);
    renderer.TexCoord2f(0.f, 1.f); renderer.Vertex3fv(TransformVertices[6]);
    renderer.Color4f(0.6f, 0.6f, 0.6f, 1.f);
    renderer.TexCoord2f(0.f, 1.f); renderer.Vertex3fv(TransformVertices[0]);
    renderer.TexCoord2f(1.f, 1.f); renderer.Vertex3fv(TransformVertices[1]);
    renderer.TexCoord2f(1.f, 0.f); renderer.Vertex3fv(TransformVertices[5]);
    renderer.TexCoord2f(0.f, 0.f); renderer.Vertex3fv(TransformVertices[4]);
    renderer.Color4f(0.4f, 0.4f, 0.4f, 1.f);
    renderer.TexCoord2f(1.f, 1.f); renderer.Vertex3fv(TransformVertices[7]);
    renderer.TexCoord2f(1.f, 0.f); renderer.Vertex3fv(TransformVertices[5]);
    renderer.TexCoord2f(0.f, 0.f); renderer.Vertex3fv(TransformVertices[1]);
    renderer.TexCoord2f(0.f, 1.f); renderer.Vertex3fv(TransformVertices[3]);
    renderer.Color4f(0.4f, 0.4f, 0.4f, 1.f);
    renderer.TexCoord2f(0.f, 1.f); renderer.Vertex3fv(TransformVertices[0]);
    renderer.TexCoord2f(1.f, 1.f); renderer.Vertex3fv(TransformVertices[4]);
    renderer.TexCoord2f(1.f, 0.f); renderer.Vertex3fv(TransformVertices[6]);
    renderer.TexCoord2f(0.f, 0.f); renderer.Vertex3fv(TransformVertices[2]);
    renderer.End();
}

#if 0
void RenderPlane3D(float Width,float Height,float Matrix[3][4])
{
	vec3_t BoundingVertices[4];
	Vector(-Width,-Width, Height,BoundingVertices[3]);
	Vector( Width, Width, Height,BoundingVertices[2]);
	Vector( Width, Width,-Height,BoundingVertices[1]);
	Vector(-Width,-Width,-Height,BoundingVertices[0]);
	
	vec3_t TransformVertices[4];
	for(int j=0;j<4;j++)
	{
   		VectorTransform(BoundingVertices[j],Matrix,TransformVertices[j]);
	}
	
	glBegin(GL_QUADS);
	glTexCoord2f( 0.f, 1.f); glVertex3fv(TransformVertices[0]);
	glTexCoord2f( 1.f, 1.f); glVertex3fv(TransformVertices[1]);
	glTexCoord2f( 1.f, 0.f); glVertex3fv(TransformVertices[2]);
	glTexCoord2f( 0.f, 0.f); glVertex3fv(TransformVertices[3]);
	glEnd();
}
#endif

void RenderPlane3D(float Width,float Height,float Matrix[3][4])
{
    vec3_t BoundingVertices[4];
    Vector(-Width,-Width, Height,BoundingVertices[3]);
    Vector( Width, Width, Height,BoundingVertices[2]);
    Vector( Width, Width,-Height,BoundingVertices[1]);
    Vector(-Width,-Width,-Height,BoundingVertices[0]);

    vec3_t TransformVertices[4];
    for (int j = 0; j < 4; ++j)
        VectorTransform(BoundingVertices[j], Matrix, TransformVertices[j]);

    Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
    renderer.Begin(Platform::LegacyPrimitiveQuads);
    renderer.TexCoord2f(0.f, 1.f); renderer.Vertex3fv(TransformVertices[0]);
    renderer.TexCoord2f(1.f, 1.f); renderer.Vertex3fv(TransformVertices[1]);
    renderer.TexCoord2f(1.f, 0.f); renderer.Vertex3fv(TransformVertices[2]);
    renderer.TexCoord2f(0.f, 0.f); renderer.Vertex3fv(TransformVertices[3]);
    renderer.End();
}

void BeginSprite()
{
	Platform::FlushOpaqueWorldRenderQueue();
	const int spriteMatrixSlot = g_spriteMatrixDepth++;
	const bool canRestoreMatrices = Platform::IsGlslLegacyBackendEnabled() &&
		g_legacy3DMatricesKnown && spriteMatrixSlot < 8;
	if (canRestoreMatrices)
	{
		memcpy(g_spriteProjectionStack[spriteMatrixSlot], g_legacyProjection3D, sizeof(g_legacyProjection3D));
		memcpy(g_spriteModelViewStack[spriteMatrixSlot], g_legacyModelView3D, sizeof(g_legacyModelView3D));
		g_spriteMatrixSaved[spriteMatrixSlot] = true;
	}
	glPushMatrix();
	glLoadIdentity();
	// Sprites e particulas ja chegam no espaco da camera. O adapter GLSL precisa
	// receber a matriz identidade antes de acumular os quads no VBO dinamico.
	if (canRestoreMatrices)
	{
		float identity[16] = { 0.f };
		identity[0] = identity[5] = identity[10] = identity[15] = 1.f;
		Platform::GetLegacyRenderAdapter().SetMatrices(g_spriteProjectionStack[spriteMatrixSlot], identity);
	}
	else
		SyncLegacyRenderMatrices();
	// RenderSprites/RenderParticles preservam a ordem de emissao. O adapter so
	// junta quads consecutivos com o mesmo estado (textura e blend), portanto
	// transparencias nunca sao reordenadas.
	Platform::GetLegacyRenderAdapter().BeginBatch();
}

void EndSprite()
{
	// Envia o buffer dinamico enquanto a matriz de sprites ainda esta ativa.
	Platform::GetLegacyRenderAdapter().EndBatch();
	glPopMatrix();
	const int spriteMatrixSlot = --g_spriteMatrixDepth;
	if (Platform::IsGlslLegacyBackendEnabled() && spriteMatrixSlot >= 0 && spriteMatrixSlot < 8 && g_spriteMatrixSaved[spriteMatrixSlot])
	{
		Platform::GetLegacyRenderAdapter().SetMatrices(g_spriteProjectionStack[spriteMatrixSlot], g_spriteModelViewStack[spriteMatrixSlot]);
		g_spriteMatrixSaved[spriteMatrixSlot] = false;
	}
	else
		SyncLegacyRenderMatrices();
}

void RenderSprite(int Texture,vec3_t Position,float Width,float Height,vec3_t Light,float Rotation,float u,float v,float uWidth,float vHeight)
{
    BindTexture(Texture);

	vec3_t p2;
	VectorTransform(Position,CameraMatrix,p2);
	//VectorCopy(Position,p2);
	float x = p2[0];
	float y = p2[1];
	float z = p2[2];

	Width  *= 0.5f;
	Height *= 0.5f;

	vec3_t p[4];
	if(Rotation==0)
	{
		Vector(x-Width, y-Height, z, p[0]);
		Vector(x+Width, y-Height, z, p[1]);
		Vector(x+Width, y+Height, z, p[2]);
		Vector(x-Width, y+Height, z, p[3]);
	}
	else
	{
		vec3_t p2[4];
		Vector(-Width,-Height, z, p2[0]);
		Vector( Width,-Height, z, p2[1]);
		Vector( Width, Height, z, p2[2]);
		Vector(-Width, Height, z, p2[3]);
		vec3_t Angle;
		Vector(0.f,0.f,Rotation,Angle);
		float Matrix[3][4];
		AngleMatrix(Angle,Matrix);
		for(int i=0;i<4;i++) 
		{
			VectorRotate(p2[i],Matrix,p[i]);
			p[i][0] += x;
			p[i][1] += y;
		}
	}
	
	float c[4][2];
	TEXCOORD(c[3],u       ,v        );
	TEXCOORD(c[2],u+uWidth,v        );
	TEXCOORD(c[1],u+uWidth,v+vHeight);
	TEXCOORD(c[0],u       ,v+vHeight);

	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.Begin(Platform::LegacyPrimitiveQuads);
    if(Bitmaps[Texture].Components==3)
		renderer.Color4f(Light[0], Light[1], Light[2], 1.f);
	else
	{
		if(Texture == BITMAP_BLOOD+1 || Texture == BITMAP_FONT_HIT)
			renderer.Color4f(Light[0],Light[1],Light[2],1.f);
		else
			renderer.Color4f(Light[0],Light[1],Light[2],Light[0]);
	}
	for(int i=0;i<4;i++)
	{
		renderer.TexCoord2f(c[i][0],c[i][1]);
		renderer.Vertex3fv(p[i]);
	}
	renderer.End();
}

void RenderSpriteUV(int Texture,vec3_t Position,float Width,float Height,float (*UV)[2],vec3_t Light[4],float Alpha)
{
    BindTexture(Texture);

	vec3_t p2;
	VectorTransform(Position,CameraMatrix,p2);
	float x = p2[0];
	float y = p2[1];
	float z = p2[2];
	
	Width  *= 0.5f;
	Height *= 0.5f;
	vec3_t p[4];
	Vector(x-Width, y-Height, z, p[0]);
	Vector(x+Width, y-Height, z, p[1]);
	Vector(x+Width, y+Height, z, p[2]);
	Vector(x-Width, y+Height, z, p[3]);

	// Preserva o blend e a cor que o emissor de particula/texto configurou.
	// O lote de sprites ainda agrega quads consecutivos no adaptador.
	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	for(int i=0;i<4;i++)
	{
		renderer.Color4f(Light[i][0],Light[i][1],Light[i][2],Alpha);
		renderer.TexCoord2f(UV[i][0],UV[i][1]);
		renderer.Vertex3fv(p[i]);
	}
	renderer.End();
}

void RenderNumber(vec3_t Position,int Num,vec3_t Color,float Alpha,float Scale)
{
	vec3_t p;
	VectorCopy(Position,p);
	vec3_t Light[4];
	VectorCopy(Color,Light[0]);
	VectorCopy(Color,Light[1]);
	VectorCopy(Color,Light[2]);
	VectorCopy(Color,Light[3]);
    if(Num == -1)
	{
		float UV[4][2];
		TEXCOORD(UV[0],0.f       ,32.f/32.f);
		TEXCOORD(UV[1],32.f/256.f,32.f/32.f);
		TEXCOORD(UV[2],32.f/256.f,17.f/32.f);
		TEXCOORD(UV[3],0.f       ,17.f/32.f);
		RenderSpriteUV(BITMAP_FONT+1,p,45,20,UV,Light,Alpha);
	}
	else if(Num == -2)
	{
		RenderSprite(BITMAP_FONT_HIT,p,32*Scale,20*Scale, Light[0], 0.f, 0.f, 0.f, 27.f/32.f, 15.f/16.f);	
	}
	else
	{
		char Text[32];
		itoa(Num,Text,10);
		p[0] -= strlen(Text)*5.f;
		unsigned int Length = strlen(Text);
		p[0] -= Length*Scale*0.125f;
		p[1] -= Length*Scale*0.125f;
		for(unsigned int i=0;i<Length;i++)
		{
			float UV[4][2];
			float u = (float)(Text[i]-48)*16.f/256.f;
			TEXCOORD(UV[0],u           ,16.f/32.f);
			TEXCOORD(UV[1],u+16.f/256.f,16.f/32.f);
			TEXCOORD(UV[2],u+16.f/256.f,0.f);
			TEXCOORD(UV[3],u           ,0.f);
			RenderSpriteUV(BITMAP_FONT+1,p,Scale,Scale,UV,Light,Alpha);
			p[0] += Scale*0.5f;
			p[1] += Scale*0.5f;
		}
	}
}

float RenderNumber2D(float x,float y,int Num,float Width,float Height)
{
	char Text[32];
	itoa(Num,Text,10);
	int Length = (int)strlen(Text);
	x -= Width*Length/2;
	for(int i=0;i<Length;i++)
	{
		float u = (float)(Text[i]-48)*16.f/256.f;
      	//glColor3fv(Color);
		RenderBitmap(BITMAP_FONT+1,x,y,Width,Height,u,0.f,16.f/256.f,16.f/32.f);
		x += Width*0.7f;
	}
	return x;
}

void BeginBitmap()
{
	Platform::FlushOpaqueWorldRenderQueue();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
    glLoadIdentity();

    glViewport(0,0,WindowWidth,WindowHeight);
    gluPerspective(CameraFOV,(WindowWidth)/((float)WindowHeight),CameraViewNear,CameraViewFar);
    
	glLoadIdentity();
    gluOrtho2D(0,WindowWidth,0,WindowHeight);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

    glLoadIdentity();
	DisableDepthTest();

    // BeginOpengl ja sincronizou as matrizes com o adapter, mas aqui elas mudam
    // para a projecao ortografica de tela. Sem sincronizar de novo, o adapter
    // continua desenhando com a matriz de PERSPECTIVA e todo sprite de UI cai
    // fora do enquadramento: a tela fica limpa mesmo com os quads sendo
    // emitidos. No Windows a funcao le a pilha do GL, entao nada muda la.
    SyncLegacyBitmapMatrices();
    Platform::GetLegacyRenderAdapter().BeginBatch();
}

void EndBitmap()
{
	// Termina o lote antes de restaurar as matrizes 3D: todos os sprites usam a
	// projecao ortografica atual e a ordem de emissao permanece inalterada.
	Platform::GetLegacyRenderAdapter().EndBatch();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

    // Restaura no adapter as matrizes que o pop devolveu. No GLSL elas ja
    // foram publicadas antes de entrar na UI; a copia CPU evita dois glGet.
    if (Platform::IsGlslLegacyBackendEnabled() && g_legacy3DMatricesKnown)
        Platform::GetLegacyRenderAdapter().SetMatrices(g_legacyProjection3D, g_legacyModelView3D);
    else
        SyncLegacyRenderMatrices();
}

void RenderColorRadius(float x, float y, float Width, float Height, float Alpha, int Flag, float radius)
{
	DisableTexture();

	x = ConvertX(x);
	y = ConvertY(y);
	Width = ConvertX(Width);
	Height = ConvertY(Height);
	y = WindowHeight - y;

	float innerWidth = Width - 2 * radius;
	float innerHeight = Height - 2 * radius;

	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.SetTexture2D(false);

	// Definir a cor de acordo com o Alpha e Flag
	bool ApplyColor = false;
	float ColorRed = 1.f, ColorGreen = 1.f, ColorBlue = 1.f;
	if (Alpha > 0.f)
	{
		if (Flag == 0)
		{
			ColorRed = 1.f; ColorGreen = 1.f; ColorBlue = 1.f;  // Branco com Alpha
			ApplyColor = true;
		}
		else if (Flag == 1)
		{
			ColorRed = 0.f; ColorGreen = 0.f; ColorBlue = 0.f;  // Preto com Alpha
			ApplyColor = true;
		}
	}

	// Desenhar o retângulo central
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	if (ApplyColor) renderer.Color4f(ColorRed, ColorGreen, ColorBlue, Alpha);
	renderer.Vertex3f(x + radius, y - radius, 0.f);
	renderer.Vertex3f(x + innerWidth + radius, y - radius, 0.f);
	renderer.Vertex3f(x + innerWidth + radius, y - innerHeight - radius, 0.f);
	renderer.Vertex3f(x + radius, y - innerHeight - radius, 0.f);
	renderer.End();

	// Desenhar as bordas retangulares (entre os cantos)
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	if (ApplyColor) renderer.Color4f(ColorRed, ColorGreen, ColorBlue, Alpha);

	// Borda superior
	renderer.Vertex3f(x + radius, y, 0.f);
	renderer.Vertex3f(x + innerWidth + radius, y, 0.f);
	renderer.Vertex3f(x + innerWidth + radius, y - radius, 0.f);
	renderer.Vertex3f(x + radius, y - radius, 0.f);

	// Borda inferior
	renderer.Vertex3f(x + radius, y - Height, 0.f);
	renderer.Vertex3f(x + innerWidth + radius, y - Height, 0.f);
	renderer.Vertex3f(x + innerWidth + radius, y - Height + radius, 0.f);
	renderer.Vertex3f(x + radius, y - Height + radius, 0.f);

	// Borda esquerda
	renderer.Vertex3f(x, y - radius, 0.f);
	renderer.Vertex3f(x + radius, y - radius, 0.f);
	renderer.Vertex3f(x + radius, y - Height + radius, 0.f);
	renderer.Vertex3f(x, y - Height + radius, 0.f);

	// Borda direita
	renderer.Vertex3f(x + Width, y - radius, 0.f);
	renderer.Vertex3f(x + Width - radius, y - radius, 0.f);
	renderer.Vertex3f(x + Width - radius, y - Height + radius, 0.f);
	renderer.Vertex3f(x + Width, y - Height + radius, 0.f);

	renderer.End();

	// Desenhar os cantos arredondados (arcos nos quatro cantos)
	int num_segments = 20; // Define o número de segmentos para suavizar os cantos

	// Inferior esquerdo
	renderer.Begin(Platform::LegacyPrimitiveTriangleFan);
	if (ApplyColor) renderer.Color4f(ColorRed, ColorGreen, ColorBlue, Alpha);
	renderer.Vertex3f(x + radius, y - Height + radius, 0.f);
	for (int i = 0; i <= num_segments; i++) {
		float theta = (M_PI / 2.0f) * (i / (float)num_segments);
		float cx = radius * cosf(theta);
		float cy = radius * sinf(theta);
		renderer.Vertex3f(x + radius - cx, y - Height + radius - cy, 0.f);
	}
	renderer.End();

	// Inferior direito
	renderer.Begin(Platform::LegacyPrimitiveTriangleFan);
	if (ApplyColor) renderer.Color4f(ColorRed, ColorGreen, ColorBlue, Alpha);
	renderer.Vertex3f(x + Width - radius, y - Height + radius, 0.f);
	for (int i = 0; i <= num_segments; i++)
	{
		float theta = (M_PI / 2.0f) * (i / (float)num_segments);
		float cx = radius * cosf(theta);
		float cy = radius * sinf(theta);
		renderer.Vertex3f(x + Width - radius + cx, y - Height + radius - cy, 0.f);
	}
	renderer.End();

	// Superior direito
	renderer.Begin(Platform::LegacyPrimitiveTriangleFan);
	if (ApplyColor) renderer.Color4f(ColorRed, ColorGreen, ColorBlue, Alpha);
	renderer.Vertex3f(x + Width - radius, y - radius, 0.f);
	for (int i = 0; i <= num_segments; i++)
	{
		float theta = (M_PI / 2.0f) * (i / (float)num_segments);
		float cx = radius * cosf(theta);
		float cy = radius * sinf(theta);
		renderer.Vertex3f(x + Width - radius + cx, y - radius + cy, 0.f);
	}
	renderer.End();

	// Superior esquerdo
	renderer.Begin(Platform::LegacyPrimitiveTriangleFan);
	if (ApplyColor) renderer.Color4f(ColorRed, ColorGreen, ColorBlue, Alpha);
	renderer.Vertex3f(x + radius, y - radius, 0.f);
	for (int i = 0; i <= num_segments; i++)
	{
		float theta = (M_PI / 2.0f) * (i / (float)num_segments);
		float cx = radius * cosf(theta);
		float cy = radius * sinf(theta);
		renderer.Vertex3f(x + radius - cx, y - radius + cy, 0.f);
	}
	renderer.End();
}

void RenderColor(float x,float y,float Width,float Height,float Alpha,int Flag)
{
    DisableTexture();

	x = ConvertX(x);
	y = ConvertY(y);
	Width  = ConvertX(Width);
	Height = ConvertY(Height);

	float p[4][2];
	y = WindowHeight - y;

	p[0][0] = x      ;p[0][1] = y;
	p[1][0] = x      ;p[1][1] = y-Height;
	p[2][0] = x+Width;p[2][1] = y-Height;
	p[3][0] = x+Width;p[3][1] = y;

	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.SetTexture2D(false);
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	for(int i=0;i<4;i++)
	{
		if(Alpha > 0.f)
		{
			if(Flag == 0)
				renderer.Color4f(1.f,1.f,1.f,Alpha);
			else
			if(Flag == 1)
				renderer.Color4f(0.f,0.f,0.f,Alpha);
		}
		renderer.Vertex3f(p[i][0], p[i][1], 0.f);
		if(Alpha > 0.f)
		{
			renderer.Color4f(1.f,1.f,1.f,1.f);
		}
	}
	renderer.End();
}

void EndRenderColor()
{
	SetLegacyColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	SetLegacyTexture2D(true);
	Platform::GetLegacyRenderAdapter().Color4f(1.f, 1.f, 1.f, 1.f);
	Platform::GetLegacyRenderAdapter().SetTexture2D(true);
}

void RenderColorBitmap(int Texture,float x,float y,float Width,float Height,float u,float v,float uWidth, float vHeight, unsigned int color)
{
	x = ConvertX(x);
	y = ConvertY(y);

	Width = ConvertX(Width);
	Height = ConvertY(Height);

    BindTexture(Texture);

	float p[4][2];

	y = WindowHeight - y;

	p[0][0] = x      ;p[0][1] = y;
	p[1][0] = x      ;p[1][1] = y-Height;
	p[2][0] = x+Width;p[2][1] = y-Height;
	p[3][0] = x+Width;p[3][1] = y;

	float c[4][2];
	TEXCOORD(c[0],u       ,v        );
	TEXCOORD(c[3],u+uWidth,v        );
	TEXCOORD(c[2],u+uWidth,v+vHeight);
	TEXCOORD(c[1],u       ,v+vHeight);

	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.SetTexture2D(true);
	renderer.Begin(Platform::LegacyPrimitiveQuads);

	for(int i=0;i<4;i++)
	{
		renderer.Color4f((color & 0xff) / 255.f,
			(color >> 8 & 0xff) / 255.f,
			(color >> 16 & 0xff) / 255.f,
			(color >> 24 & 0xff) / 255.f);

		renderer.TexCoord2f(c[i][0],c[i][1]);
		renderer.Vertex3f(p[i][0], p[i][1], 0.f);

		renderer.Color4f(1.f,1.f,1.f,1.f);
	}
	renderer.End();
}

void RenderBitmap(int Texture,float x,float y,float Width,float Height,float u,float v,float uWidth,float vHeight,bool Scale,bool StartScale,float Alpha)
{
	if(StartScale)
	{
		x = ConvertX(x);
		y = ConvertY(y);
	}
	if(Scale)
	{
		Width = ConvertX(Width);
		Height = ConvertY(Height);
	}

    BindTexture(Texture);

	float p[4][2];

	y = WindowHeight - y;
	
	p[0][0] = x      ;p[0][1] = y;
	p[1][0] = x      ;p[1][1] = y-Height;
	p[2][0] = x+Width;p[2][1] = y-Height;
	p[3][0] = x+Width;p[3][1] = y;

	float c[4][2];
	TEXCOORD(c[0],u       ,v        );
	TEXCOORD(c[3],u+uWidth,v        );
	TEXCOORD(c[2],u+uWidth,v+vHeight);
	TEXCOORD(c[1],u       ,v+vHeight);

	// A UI legado deixa blend, alpha test e cor corrente preparados pelo
	// chamador. Um RenderCommand com material padrao os sobrescreve e deixa
	// botoes, textos e janelas incorretos no backend GLSL. Quads ainda entram
	// no lote aberto por BeginBitmap.
	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.SetTexture2D(true);
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	for(int i=0;i<4;i++)
	{
		if(Alpha > 0.f)
			renderer.Color4f(1.f,1.f,1.f,Alpha);
		renderer.TexCoord2f(c[i][0],c[i][1]);
		renderer.Vertex3f(p[i][0], p[i][1], 0.f);
		if(Alpha > 0.f)
			renderer.Color4f(1.f,1.f,1.f,1.f);
	}
	renderer.End();
}

void RenderBitmapRotate(int Texture,float x,float y,float Width,float Height,float Rotate,float u,float v,float uWidth,float vHeight)
{
	x = ConvertX(x);
	y = ConvertY(y);
	Width = ConvertX(Width);
	Height = ConvertY(Height);
	//x -= Width *0.5f;
	//y -= Height*0.5f;
    BindTexture(Texture);

	vec3_t p[4],p2[4];

	y = WindowHeight - y;

	Vector(-Width*0.5f, Height*0.5f,0.f,p[0]);
	Vector(-Width*0.5f,-Height*0.5f,0.f,p[1]);
	Vector( Width*0.5f,-Height*0.5f,0.f,p[2]);
	Vector( Width*0.5f, Height*0.5f,0.f,p[3]);

	vec3_t Angle;
	Vector(0.f,0.f,Rotate,Angle);
	float Matrix[3][4];
	AngleMatrix(Angle,Matrix);

	float c[4][2];
	TEXCOORD(c[0],u       ,v        );
	TEXCOORD(c[3],u+uWidth,v        );
	TEXCOORD(c[2],u+uWidth,v+vHeight);
	TEXCOORD(c[1],u       ,v+vHeight);

	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.SetTexture2D(true);
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	for(int i=0;i<4;i++)
	{
		renderer.TexCoord2f(c[i][0],c[i][1]);
      	VectorRotate(p[i],Matrix,p2[i]);
		renderer.Vertex3f(p2[i][0] + x, p2[i][1] + y, p2[i][2]);
	}
	renderer.End();
}

void RenderBitRotate(int Texture,float x,float y,float Width,float Height,float Rotate)
{
	x = ConvertX(x);
	y = ConvertY(y);
	Width = ConvertX(Width);
	Height = ConvertY(Height);

    BindTexture(Texture);

	vec3_t p[4],p2[4];

	y = Height - y;

	float cx = (Width/2.f) - (Width - x);
	float cy = (Height/2.f) - (Height - y);

	float ax = (-Width*0.5f) + cx;
	float bx = (Width*0.5f) + cx;
	float ay = (-Height*0.5f) + cy;
	float by = (Height*0.5f) + cy;

	Vector(ax, by, 0.f, p[0]);
	Vector(ax, ay, 0.f, p[1]);
	Vector(bx, ay, 0.f, p[2]);
	Vector(bx, by, 0.f, p[3]);

	vec3_t Angle;
	Vector(0.f,0.f,Rotate,Angle);
	float Matrix[3][4];
	AngleMatrix(Angle,Matrix);

	float c[4][2];
	TEXCOORD(c[0],0.f,0.f);
	TEXCOORD(c[3],1.f,0.f);
	TEXCOORD(c[2],1.f,1.f);
	TEXCOORD(c[1],0.f,1.f);

	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.SetTexture2D(true);
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	for(int i=0;i<4;i++)
	{
		renderer.TexCoord2f(c[i][0],c[i][1]);
		VectorRotate(p[i],Matrix,p2[i]);
		renderer.Vertex3f(p2[i][0] + (WindowWidth / 2.f), p2[i][1] + (WindowHeight / 2.f), p2[i][2]);
	}
	renderer.End();
}

void RenderPointRotate(int Texture,float ix,float iy,float iWidth,float iHeight,float x,float y,float Width,float Height,float Rotate,float Rotate_Loc,float uWidth,float vHeight,int Num)
{
	int i = 0;
	vec3_t p,p2[4],p3,p4[4],Angle;
	float c[4][2],Matrix[3][4];

	ix = ConvertX(ix);
	iy = ConvertY(iy);
	x = ConvertX(x);
	y = ConvertY(y);
	Width = ConvertX(Width);
	Height = ConvertY(Height);

    BindTexture(Texture);

	y = Height - y;
	iy = Height - iy;

	Vector((ix - (Width*0.5f)) + ((Width/2.f) - (Width - x)), (iy - (Height*0.5f)) + ((Height/2.f) - (Height - y)), 0.f, p);

	Vector(0.f,0.f,Rotate,Angle);
	AngleMatrix(Angle,Matrix);

	VectorRotate(p,Matrix,p3);

	Vector(-(iWidth*0.5f), (iHeight*0.5f), 0.f, p2[0]);
	Vector(-(iWidth*0.5f), -(iHeight*0.5f), 0.f, p2[1]);
	Vector((iWidth*0.5f), -(iHeight*0.5f), 0.f, p2[2]);
	Vector((iWidth*0.5f), (iHeight*0.5f), 0.f, p2[3]);

	Vector(0.f,0.f,Rotate_Loc,Angle);
	AngleMatrix(Angle,Matrix);

	TEXCOORD(c[0],0.f       ,0.f        );
	TEXCOORD(c[3],uWidth,0.f       );
	TEXCOORD(c[2],uWidth,vHeight);
	TEXCOORD(c[1],0.f       ,vHeight);

	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.SetTexture2D(true);
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	for(i=0;i<4;i++)
	{
		renderer.TexCoord2f(c[i][0],c[i][1]);

		Matrix[0][3] = p3[0]+25;
		Matrix[1][3] = p3[1];
		VectorTransform(p2[i], Matrix, p4[i]);

		renderer.Vertex3f(p4[i][0] + (WindowWidth / 2.f), p4[i][1] + (WindowHeight / 2.f), p4[i][2]);
	}
	renderer.End();

	if(Num > -1)
	{
		float dx,dy;
		dx = p4[0][0]+(WindowWidth/2.f);
		dy = p4[0][1]+(WindowHeight/2.f);
		dx = dx * GWidescreen.JCWinWidth / (float)(WindowWidth);
		dy = dy * (float)(480.f/WindowHeight);
		if(Num >= 100)
		{
			g_pNewUIMiniMap->SetBtnPos(Num - 100,dx - (iWidth/2) , (480 - dy) - (iHeight/2), iWidth,iHeight);
		}
		else
		{
			g_pNewUIMiniMap->SetBtnPos(Num ,dx, 480 - dy, iWidth/2,iHeight/2);
		}
	}
}

void RenderBitmapLocalRotate(int Texture,float x,float y,float Width,float Height,float Rotate,float u,float v,float uWidth,float vHeight)
{
    BindTexture(Texture);

	vec3_t p[4];
	x = ConvertX(x);
	y = ConvertY(y);
	y = WindowHeight - y;
	Width = ConvertX(Width);
	Height = ConvertY(Height);

	vec3_t vCenter, vDir;
	Vector(x, y, 0, vCenter);
	Vector(Width*0.5f, -Height*0.5f, 0, vDir);
	p[0][0] = vCenter[0] + (vDir[0])*cosf(Rotate);
	p[0][1] = vCenter[1] + (vDir[1])*sinf(Rotate);
	p[1][0] = vCenter[0] + (vDir[0])*sinf(Rotate);
	p[1][1] = vCenter[1] - (vDir[1])*cosf(Rotate);
	p[2][0] = vCenter[0] - (vDir[0])*cosf(Rotate);
	p[2][1] = vCenter[1] - (vDir[1])*sinf(Rotate);
	p[3][0] = vCenter[0] - (vDir[0])*sinf(Rotate);
	p[3][1] = vCenter[1] + (vDir[1])*cosf(Rotate);

	float c[4][2];
	TEXCOORD(c[0],u       ,v        );
	TEXCOORD(c[3],u+uWidth,v        );
	TEXCOORD(c[2],u+uWidth,v+vHeight);
	TEXCOORD(c[1],u       ,v+vHeight);

	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.SetTexture2D(true);
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	for(int i=0;i<4;i++)
	{
		renderer.TexCoord2f(c[i][0],c[i][1]);
		renderer.Vertex3f(p[i][0], p[i][1], p[i][2]);
	}
	renderer.End();
}

void RenderBitmapAlpha(int Texture,float sx,float sy,float Width,float Height)
{
    EnableAlphaTest();
    BindTexture(Texture);

	sy = WindowHeight - sy;
	for(int y=0;y<4;y++)
	{
		for(int x=0;x<4;x++)
		{
         	float p[4][2];
			p[0][0] = sx+((x  )*Width)*0.25f; p[0][1] = sy-((y  )*Height)*0.25f;
			p[1][0] = sx+((x  )*Width)*0.25f; p[1][1] = sy-((y+1)*Height)*0.25f;
			p[2][0] = sx+((x+1)*Width)*0.25f; p[2][1] = sy-((y+1)*Height)*0.25f;
			p[3][0] = sx+((x+1)*Width)*0.25f; p[3][1] = sy-((y  )*Height)*0.25f;
			
         	float c[4][2];
			TEXCOORD(c[0],(x  )*0.25f,(y  )*0.25f);
			TEXCOORD(c[1],(x  )*0.25f,(y+1)*0.25f);
			TEXCOORD(c[2],(x+1)*0.25f,(y+1)*0.25f);
			TEXCOORD(c[3],(x+1)*0.25f,(y  )*0.25f);

			float Alpha[4] = {1.f,1.f,1.f,1.f};
			if(x==0) {Alpha[0] = 0.f;Alpha[1] = 0.f;}
			if(x==3) {Alpha[2] = 0.f;Alpha[3] = 0.f;}
			if(y==0) {Alpha[0] = 0.f;Alpha[3] = 0.f;}
			if(y==3) {Alpha[1] = 0.f;Alpha[2] = 0.f;}
			/*if(x==0&&y==0) Alpha[0] = 0.f;
			if(x==0&&y==3) Alpha[1] = 0.f;
			if(x==3&&y==3) Alpha[2] = 0.f;
			if(x==3&&y==0) Alpha[3] = 0.f;*/
			
			Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
			renderer.Begin(Platform::LegacyPrimitiveQuads);
			for(int i=0;i<4;i++)
			{
				renderer.Color4f(1.f,1.f,1.f,Alpha[i]);
				renderer.TexCoord2f(c[i][0],c[i][1]);
				renderer.Vertex3f(p[i][0], p[i][1], 0.f);
			}
			renderer.End();
		}
	}
}

void RenderBitmapUV(int Texture,float x,float y,float Width,float Height,float u,float v,float uWidth,float vHeight)
{
	x = ConvertX(x);
	y = ConvertY(y);
	Width = ConvertX(Width);
	Height = ConvertY(Height);
    BindTexture(Texture);

	float p[4][2];
	y = WindowHeight - y;
	p[0][0] = x      ;p[0][1] = y;
	p[1][0] = x      ;p[1][1] = y-Height;
	p[2][0] = x+Width;p[2][1] = y-Height;
	p[3][0] = x+Width;p[3][1] = y;

	float c[4][2];
	TEXCOORD(c[0],u       ,v        +vHeight*0.25f);
	TEXCOORD(c[3],u+uWidth,v        );
	TEXCOORD(c[2],u+uWidth,v+vHeight);
	TEXCOORD(c[1],u       ,v+vHeight-vHeight*0.25f);

	Platform::ILegacyRenderAdapter& renderer = Platform::GetLegacyRenderAdapter();
	renderer.SetTexture2D(true);
	renderer.Begin(Platform::LegacyPrimitiveQuads);
	for(int i=0;i<4;i++)
	{
		renderer.TexCoord2f(c[i][0],c[i][1]);
		renderer.Vertex3f(p[i][0], p[i][1], 0.f);
	}
	renderer.End();
}

///////////////////////////////////////////////////////////////////////////////
// collision detect util
///////////////////////////////////////////////////////////////////////////////

float absf(float a)
{
	if(a < 0.f) return -a;
	return a;
}

float minf(float a,float b)
{
	if(a > b) return b;
	return a;
}

float maxf(float a,float b)
{
	if(a > b) return a;
	return b;
}

int InsideTest(float x,float y,float z,int n,float *v1,float *v2,float *v3,float *v4,int flag,float type)
{
    if(type > 0.f) 
		flag <<= 3;

    int i;
    vec3_t *vtx[4];
	vtx[0] = (vec3_t *)v1;
	vtx[1] = (vec3_t *)v2;
	vtx[2] = (vec3_t *)v3;
	vtx[3] = (vec3_t *)v4;

    int j = n-1;
    switch(flag)
	{
	case 1:
        for(i=0;i<n;j=i,i++)
		{
			float d = ((*vtx[i])[1]-y) * ((*vtx[j])[2]-z) - ((*vtx[j])[1]-y) * ((*vtx[i])[2]-z);
			if(d <= 0.f) 
				return false;
		}
		break;
	case 2:
        for(i=0;i<n;j=i,i++)
		{
			float d = ((*vtx[i])[2]-z) * ((*vtx[j])[0]-x) - ((*vtx[j])[2]-z) * ((*vtx[i])[0]-x);
			if(d <= 0.f) 
				return false;
		}
		break;
	case 4:
        for(i=0;i<n;j=i,i++)
		{
			float d = ((*vtx[i])[0]-x) * ((*vtx[j])[1]-y) - ((*vtx[j])[0]-x) * ((*vtx[i])[1]-y);
			if(d <= 0.f) 
				return false;
		}
		break;
	case 8:
        for(i=0;i<n;j=i,i++)
		{
			float d = ((*vtx[i])[1]-y) * ((*vtx[j])[2]-z) - ((*vtx[j])[1]-y) * ((*vtx[i])[2]-z);
			if(d >= 0.f) 
				return false;
		}
		break;
	case 16:
        for(i=0;i<n;j=i,i++)
		{
			float d = ((*vtx[i])[2]-z) * ((*vtx[j])[0]-x) - ((*vtx[j])[2]-z) * ((*vtx[i])[0]-x);
			if(d >= 0.f) 
				return false;
		}
		break;
	case 32:
        for(i=0;i<n;j=i,i++)
		{
			float d = ((*vtx[i])[0]-x) * ((*vtx[j])[1]-y) - ((*vtx[j])[0]-x) * ((*vtx[i])[1]-y);
			if(d >= 0.f) 
				return false;
		}
		break;
	}
	
    return true;
}

float Distance;

void InitCollisionDetectLineToFace()
{
	Distance = 9999999.f;
}

vec3_t CollisionPosition;

bool CollisionDetectLineToFace(vec3_t Position,vec3_t Target,int Polygon,float *v1,float *v2,float *v3,float *v4,vec3_t Normal,bool Collision)
{
	vec3_t Direction;
	VectorSubtract(Target,Position,Direction);
	float a = DotProduct(Direction,Normal);
	if(a >= 0.f) return false;
	float b = DotProduct(Position,Normal) - DotProduct(v1,Normal);
	float t = -b/a;
	if(t >= 0.f && t <= Distance)
	{
		float X = Direction[0] * t + Position[0];
		float Y = Direction[1] * t + Position[1];
		float Z = Direction[2] * t + Position[2];
		int Count = 0;
		float MIN = minf(minf(absf(Direction[0]),absf(Direction[1])),absf(Direction[2]));
		if(MIN == absf(Direction[0]))
		{
			if( (Y >= minf(Position[1],Target[1]) && Y <= maxf(Position[1],Target[1])) &&
				(Z >= minf(Position[2],Target[2]) && Z <= maxf(Position[2],Target[2])) ) Count++;
		}
		else if(MIN == absf(Direction[1]))
		{
			if( (Z >= minf(Position[2],Target[2]) && Z <= maxf(Position[2],Target[2])) &&
				(X >= minf(Position[0],Target[0]) && X <= maxf(Position[0],Target[0])) ) Count++;
		}
		else
		{
			if( (X >= minf(Position[0],Target[0]) && X <= maxf(Position[0],Target[0])) &&
				(Y >= minf(Position[1],Target[1]) && Y <= maxf(Position[1],Target[1])) ) Count++;
		}
		if(Count == 0) return false;
		Count = 0;
		if(Normal[0]<=-0.5f || Normal[0]>=0.5f)
		{
			Count += InsideTest(X,Y,Z,Polygon,v1,v2,v3,v4,1,Normal[0]);
		}
		else if(Normal[1]<=-0.5f || Normal[1]>=0.5f)
		{
			Count += InsideTest(X,Y,Z,Polygon,v1,v2,v3,v4,2,Normal[1]);
		}
		else
		{
			Count += InsideTest(X,Y,Z,Polygon,v1,v2,v3,v4,4,Normal[2]);
		}
		if(Count == 0) return false;
		if(Collision)
		{
      		Distance = t;
			Vector(X,Y,Z,CollisionPosition);
		}
		return true;
	}
	return false;
}

bool ProjectLineBox(vec3_t ax, vec3_t p1, vec3_t p2, OBB_t obb)
{
	float P1 = DotProduct(ax, p1);
	float P2 = DotProduct(ax, p2);
	
	float mx1 = maxf(P1, P2);
	float mn1 = minf(P1, P2);
	
	float ST = DotProduct(ax, obb.StartPos);
	float Q1 = DotProduct(ax, obb.XAxis);
	float Q2 = DotProduct(ax, obb.YAxis);
	float Q3 = DotProduct(ax, obb.ZAxis);
	
	float mx2 = ST;
	float mn2 = ST;
	
	if (Q1>0)	mx2+=Q1; else mn2+=Q1;
	if (Q2>0)	mx2+=Q2; else mn2+=Q2;
	if (Q3>0) mx2+=Q3; else mn2+=Q3;
	
	if (mn1 > mx2) return false;
	if (mn2 > mx1) return false;
	
	return true;
}

bool CollisionDetectLineToOBB(vec3_t p1, vec3_t p2, OBB_t obb)
{
	vec3_t e1;
	vec3_t eq11,eq12,eq13;

	VectorSubtract(p2,p1,e1);

	CrossProduct( e1, obb.XAxis, eq11);
	CrossProduct( e1, obb.YAxis, eq12);
	CrossProduct( e1, obb.ZAxis, eq13);

	if (!ProjectLineBox(eq11,p1,p2,obb) ) return false;
	if (!ProjectLineBox(eq12,p1,p2,obb) ) return false;
	if (!ProjectLineBox(eq13,p1,p2,obb) ) return false;

	if (!ProjectLineBox(obb.XAxis,p1,p2,obb) ) return false;
	if (!ProjectLineBox(obb.YAxis,p1,p2,obb) ) return false;
	if (!ProjectLineBox(obb.ZAxis,p1,p2,obb) ) return false;

	return true;
}

