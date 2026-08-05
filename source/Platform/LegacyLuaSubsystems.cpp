// Inicializacao dos subsistemas guiados por Lua, fora do Windows.
//
// POR QUE EXISTE: `Winmain.cpp` chama, em sequencia, o `Init()` de 28 subsistemas
// (linhas 1494-1548). Quase todos fazem a mesma coisa -- registram as funcoes C++ no
// seu proprio `lua_State` e carregam um script de `Data/Configs/Lua/`. Como o Winmain
// e o ponto de entrada Win32, nada disso rodava no Web/Android: cada `lua_State`
// ficava vazio e TODA chamada do cliente para um script devolvia
//
//   luacall_Generic_Call error running function 'X': 'attempt to call a nil value'
//
// O log da cena de selecao de personagem tinha ~20 mil dessas por sessao
// (`RenderModelBody`, `CreateEffectSetPlayer`, `CharacterItensEffect`, `RenderProc`,
// `UpdateProc`, `StartLoadImages`, `LoadImageCape`, `StartPatentLoadImage`), e o
// sintoma visivel era a lista de personagens da Season 13 nao desenhar nada -- o
// caminho `gProtect->m_MainInfo.CharListS13 == 1` chama `RenderProc` a cada quadro, e
// a funcao nunca existiu.
//
// A ORDEM E A MESMA do Winmain, de proposito. Alguns scripts dependem de estado
// preparado por outro (`Font.lua` precisa vir antes de `gCreateFont.SetFont`), e
// reordenar aqui criaria uma diferenca entre plataformas dificil de rastrear.
//
// DUAS OMISSOES DELIBERADAS em relacao ao Winmain:
//
//   1. `gProtect->ReadMainFile("Data\\Configs\\Configs.xtm")` e
//      `ReadCustomJewelConfig`, e o `gCustomJewel.Load(...)` que consome o segundo.
//      O `gProtect` deste port e armazenamento zerado (ver LegacyClientGlobals.cpp),
//      nao um `CProtect` de verdade, entao nao ha o que carregar.
//   2. `gTrayMode.Load()` -- icone da bandeja do Windows.

#if !defined(_WIN32)

#include "../stdafx.h"

#include "../ItemManager.h"
#include "../HelperSystem.h"
#include "../DarkSpirit.h"
#include "../ItemPosition.h"
#include "../ItemSize.h"
#include "../Descriptions.h"
#include "../Visuals.h"
#include "../DisableExcellent.h"
#include "../CustomBow.h"
#include "../CustomWing.h"
#include "../CustomEffects.h"
#include "../CustomSetEffect.h"
#include "../CustomItemFloor.h"
#include "../CustomItemForce.h"
#include "../RenderModel.h"
#include "../ServerName.h"
#include "../CreateFont.h"
#include "../MessageColor.h"
#include "../Monsters.h"
#include "../MonsterName.h"
#include "../MonsterGlow.h"
#include "../MonsterEffect.h"
#include "../CustomCape.h"
#include "../CustomJewelStack.h"
#include "../CounterItem.h"
#include "../CharacterList.h"
#include "../Patente.h"
#include "../ElementSlots.h"

#include "LegacySceneBringup.h"

namespace Platform
{
    void InicializarSubsistemasDeScript()
    {
        // Idempotente: `CriarCenaDeTitulo` pode ser chamada mais de uma vez (troca de
        // resolucao), e recarregar os scripts duplicaria registro de classe em cada
        // lua_State.
        static bool jaFeito = false;
        if (jaFeito) return;
        jaFeito = true;

        gItemManager.Init();
        gHelperSystem.Init();
        gDarkSpirit.Init();
        gCustomItemPosition.Init();
        gCustomItemSize.Init();
        gDescriptions.Init();
        Visuals.Init();
        gDisableExcellent.Init();
        gCustomBow.Init();
        gCustomWing.Init();
        gCustomEffects.Init();
        gCCustomSetEffect.Init();
        gCustomItemFloor.Init();
        gCustomItemForce.Init();
        gRenderModel.Init();
        gCustomServerName.Init();
        gCreateFont.Init();
        gMessageColor.Init();
        gMonsters.Init();
        gMonsterName.Init();
        gMonsterGlow.Init();
        gMonsterEffect.Init();
        gCustomCape.Init();
        gJewelStack.Init();
        gCounterItem.Init();
        gCharacterList.Init();
        gPatente.Init();
        gControlSlot.Init();
    }
}

#endif  // !_WIN32
