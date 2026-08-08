#pragma once
#include "zzzinfomation.h"

extern int MenuStateCurrent;
extern int MenuStateNext;
extern int  SceneFlag;
extern int  MoveSceneFrame;
//extern bool EnableEdit;
extern int  ErrorMessage;
extern bool InitServerList;
extern bool InitLogIn;
extern bool InitLoading;
extern bool InitCharacterScene;
extern bool InitMainScene;
extern bool EnableMainRender;
extern char *szServerIpAddress;
extern unsigned short g_ServerPort;
extern int g_iLengthAuthorityCode;
extern int CreateAccount;

// Valor <= 0 significa ilimitado. Ate a v14 esta funcao sobrescrevia o proprio
// argumento e o teto nunca era aplicado; ver o comentario na definicao.
void SetTargetFps(double targetFps);
extern void WaitForNextActivity(bool usePreciseSleep);

// Swap interval aplicado: -1 = default do driver, 0 = off, 1 = on. Vai para o CSV
// porque um vsync forcado no painel do driver esconde qualquer otimizacao.
extern int g_vsyncInterval;

// Reparticao do periodo do frame fora de RenderScene. O laco principal em
// Winmain.cpp contribui com o pump de mensagens e o protocolo, que ficam dentro
// de frame_total_us mas fora de cpu_us. Unidades em microssegundos.
long long FrameLoopNowMicroseconds();
void RecordFrameProtocolUs(long long microseconds);
void RecordFramePumpUs(long long microseconds);
// Sleep do limitador (-fpslimit). Coluna propria para nao poluir us_frame_gap,
// que significa "caminho de frame ainda sem instrumento".
void RecordFrameLimiterUs(long long microseconds);

// Detalhamento de us_characters (33,7% do frame na v14, a maior fatia medida).
// Estas colunas ANINHAM dentro de us_characters — sobreposicao, nao particao — e
// nao entram em nenhuma soma do CSV. `us_char_parts` sai por subtracao.
void RecordCharPoseUs(long long microseconds);
void RecordCharShadowUs(long long microseconds);

extern void LogInScene(HDC hDC);
extern void LoadingScene(HDC hDC);
extern void Scene(HDC Hdc);
extern bool CheckName();
void    StartGame();
extern bool CheckRenderNextFrame();
extern void RenderScene(HDC Hdc);

BOOL	ShowCheckBox( int num, int index, int message=MESSAGE_TRADE_CHECK );

int	SeparateTextIntoLines( const char* lpszText, char *lpszSeparated, int iMaxLine, int iLineSize);

bool	GetTimeCheck(int DelayTime);
void	SetEffectVolumeLevel(int level);
void    SetViewPortLevel ( int level );

bool IsEnterPressed();
void SetEnterPressed( bool enterpressed );


