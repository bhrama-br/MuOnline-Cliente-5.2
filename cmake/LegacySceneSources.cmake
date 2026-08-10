# Lista unica dos modulos de jogo que compilam sob clang (Emscripten e NDK).
#
# Vive fora de web/CMakeLists.txt porque os builds Web e Android precisam
# exatamente da mesma lista: mante-la em dois lugares faria as plataformas
# divergirem silenciosamente conforme novos modulos fossem liberados.
#
# Espera que MU_GAME_SRC ja aponte para a pasta source/ do cliente.

set(MU_LEGACY_SCENE_SOURCES
    ${MU_GAME_SRC}/BlackWin.cpp
    ${MU_GAME_SRC}/BoneManager.cpp
    ${MU_GAME_SRC}/Builder.cpp
    ${MU_GAME_SRC}/Button.cpp
    ${MU_GAME_SRC}/CComGem.cpp
    ${MU_GAME_SRC}/CDirection.cpp
    ${MU_GAME_SRC}/CGFxFontConfigParser.cpp
    ${MU_GAME_SRC}/CGFxInfoPopup.cpp
    ${MU_GAME_SRC}/CGFxMainUi.cpp
    ${MU_GAME_SRC}/CGFxProcess.cpp
    ${MU_GAME_SRC}/CGFxSample.cpp
    ${MU_GAME_SRC}/CKANTURUDirection.cpp
    ${MU_GAME_SRC}/CMVP1stDirection.cpp
    ${MU_GAME_SRC}/CSChaosCastle.cpp
    ${MU_GAME_SRC}/CSEventMatch.cpp
    ${MU_GAME_SRC}/CSItemOption.cpp
    ${MU_GAME_SRC}/CSParts.cpp
    ${MU_GAME_SRC}/CSPetSystem.cpp
    ${MU_GAME_SRC}/CSWaterTerrain.cpp
    ${MU_GAME_SRC}/CameraMove.cpp
    # Modulos de texto: so passaram a compilar depois que Platform/PlatformText.cpp
    # trocou os stubs GDI zerados por um rasterizador de verdade.
    ${MU_GAME_SRC}/CreateFont.cpp
    ${MU_GAME_SRC}/MultiLanguage.cpp
    ${MU_GAME_SRC}/UIControls.cpp
    ${MU_GAME_SRC}/ChangeRingManager.cpp
    ${MU_GAME_SRC}/CharInfoBalloon.cpp
    ${MU_GAME_SRC}/CharInfoBalloonMng.cpp
    ${MU_GAME_SRC}/CharMakeWin.cpp
    ${MU_GAME_SRC}/CharSelMainWin.cpp
    ${MU_GAME_SRC}/CharacterList.cpp
    ${MU_GAME_SRC}/CharacterManager.cpp
    ${MU_GAME_SRC}/CreditInfo.cpp
    ${MU_GAME_SRC}/CriticalSection.cpp
    # Crowd LOD. Entra na lista compartilhada no MESMO commit em que o modulo
    # nasce: fora dela o arquivo simplesmente nao existe no Web nem no Android, e
    # a divergencia so apareceria como link error muito depois.
    ${MU_GAME_SRC}/CrowdLod.cpp
    ${MU_GAME_SRC}/CustomBow.cpp
    ${MU_GAME_SRC}/CustomCape.cpp
    ${MU_GAME_SRC}/CustomEffects.cpp
    ${MU_GAME_SRC}/CustomItemFloor.cpp
    ${MU_GAME_SRC}/CustomItemForce.cpp
    ${MU_GAME_SRC}/CustomJewel.cpp
    ${MU_GAME_SRC}/CustomJewelStack.cpp
    ${MU_GAME_SRC}/CustomSetEffect.cpp
    ${MU_GAME_SRC}/DarkSpiritView.cpp
    ${MU_GAME_SRC}/Descriptions.cpp
    ${MU_GAME_SRC}/DialogInfo.cpp
    ${MU_GAME_SRC}/DisableExcellent.cpp
    ${MU_GAME_SRC}/DuelMgr.cpp
    ${MU_GAME_SRC}/ElementPet.cpp
    ${MU_GAME_SRC}/ElementSlots.cpp
    ${MU_GAME_SRC}/Event.cpp
    ${MU_GAME_SRC}/ExplanationInfo.cpp
    ${MU_GAME_SRC}/ExternalObject/Leaf/xstreambuf.cpp
    ${MU_GAME_SRC}/FilterInfo.cpp
    ${MU_GAME_SRC}/FilterNameInfo.cpp
    ${MU_GAME_SRC}/GIPetManager.cpp
    ${MU_GAME_SRC}/GM3rdChangeUp.cpp
    ${MU_GAME_SRC}/GMAida.cpp
    ${MU_GAME_SRC}/GMCryingWolf2nd.cpp
    ${MU_GAME_SRC}/GMCrywolf1st.cpp
    ${MU_GAME_SRC}/GMDoppelGanger1.cpp
    ${MU_GAME_SRC}/GMDoppelGanger2.cpp
    ${MU_GAME_SRC}/GMDoppelGanger3.cpp
    ${MU_GAME_SRC}/GMDoppelGanger4.cpp
    ${MU_GAME_SRC}/GMDuelArena.cpp
    ${MU_GAME_SRC}/GMEmpireGuardian1.cpp
    ${MU_GAME_SRC}/GMEmpireGuardian2.cpp
    ${MU_GAME_SRC}/GMEmpireGuardian3.cpp
    ${MU_GAME_SRC}/GMEmpireGuardian4.cpp
    ${MU_GAME_SRC}/GMHellas.cpp
    ${MU_GAME_SRC}/GMHuntingGround.cpp
    ${MU_GAME_SRC}/GMKarutan1.cpp
    ${MU_GAME_SRC}/GMNewTown.cpp
    ${MU_GAME_SRC}/GMSantaTown.cpp
    ${MU_GAME_SRC}/GMSwampOfQuiet.cpp
    ${MU_GAME_SRC}/GMUnitedMarketPlace.cpp
    ${MU_GAME_SRC}/GM_Kanturu_2nd.cpp
    ${MU_GAME_SRC}/GM_Kanturu_3rd.cpp
    ${MU_GAME_SRC}/GM_PK_Field.cpp
    ${MU_GAME_SRC}/GM_Raklion.cpp
    ${MU_GAME_SRC}/GM_kanturu_1st.cpp
    ${MU_GAME_SRC}/GOBoid.cpp
    ${MU_GAME_SRC}/GambleSystem.cpp
    ${MU_GAME_SRC}/GameCensorship.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSCommon.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSDeleteItemConfirm.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSGiftStorageItemInfo.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSStorageItemInfo.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSUseBuffConfirm.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSUseItemConfirm.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/BannerInfoList.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/BannerListManager.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/ShopCategoryList.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/ShopListManager.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/ShopPackageList.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/ShopProductList.cpp
    # Faltavam na lista: as classes-base do subsistema de loja. Os arquivos
    # *List.cpp estavam aqui, mas nao os que definem os destrutores VIRTUAIS de
    # CBannerInfo, CListManager e CShopPackage -- e sem a funcao-chave o linker
    # nao emite a vtable, o que aparece como "undefined symbol: vtable for X".
    ${MU_GAME_SRC}/GameShop/ShopListManager/BannerInfo.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/ListManager.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/ShopPackage.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/ShopCategory.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/ShopList.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/ShopProduct.cpp
    # WZResult e o tipo de retorno de todo o subsistema de loja; sem ele o link
    # fecha, mas cada chamada vira um stub que aborta em tempo de execucao.
    ${MU_GAME_SRC}/GameShop/ShopListManager/interface/WZResult/WZResult.cpp
    ${MU_GAME_SRC}/KeyGenerater.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/StringMethod.cpp
    ${MU_GAME_SRC}/GameShop/ShopListManager/StringToken.cpp
    ${MU_GAME_SRC}/GlobalPortalSystem.cpp
    ${MU_GAME_SRC}/GuildCache.cpp
    ${MU_GAME_SRC}/GuildManager.cpp
    ${MU_GAME_SRC}/HelperManager.cpp
    ${MU_GAME_SRC}/HelperView.cpp
    ${MU_GAME_SRC}/Interfaces.cpp
    ${MU_GAME_SRC}/ItemAddOptioninfo.cpp
    ${MU_GAME_SRC}/ItemInfo.cpp
    ${MU_GAME_SRC}/ItemMove.cpp
    ${MU_GAME_SRC}/ItemPosition.cpp
    ${MU_GAME_SRC}/ItemSize.cpp
    ${MU_GAME_SRC}/LoadingScene.cpp
    ${MU_GAME_SRC}/Local.cpp
    ${MU_GAME_SRC}/LoginMainWin.cpp
    ${MU_GAME_SRC}/LuaBMD.cpp
    ${MU_GAME_SRC}/LuaCharacter.cpp
    ${MU_GAME_SRC}/LuaCloth.cpp
    ${MU_GAME_SRC}/LuaEffects.cpp
    ${MU_GAME_SRC}/LuaEffectsNormal.cpp
    ${MU_GAME_SRC}/LuaItemObject.cpp
    ${MU_GAME_SRC}/LuaLoadImage.cpp
    # RegisterClassObject: a classe `Object` dos scripts (Alpha, Mesh, Light, TexCoord,
    # Hidden, acao/tempo de animacao) e as funcoes `worldTime`/`GetDoubleRender`.
    # RenderModel.lua, CharacterCreateCape.lua e PatentSystem.lua todos a usam.
    ${MU_GAME_SRC}/LuaObject.cpp
    ${MU_GAME_SRC}/LuaSocket.cpp
    ${MU_GAME_SRC}/LuaUser.cpp
    ${MU_GAME_SRC}/MapManager.cpp
    ${MU_GAME_SRC}/MatchEvent.cpp
    ${MU_GAME_SRC}/Math/ZzzMathLib.cpp
    ${MU_GAME_SRC}/Platform/LegacyFileAccess.cpp
    # FindFirstFile/FindNextFile: sem eles OpenFolder("Definitions") nao acha nada e
    # GET_ITEM fica nil em TODO script do cliente.
    ${MU_GAME_SRC}/Platform/LegacyDirectoryScan.cpp
    # Cifra dos pacotes de rede. No PC vem de SimpleModulus.lib (COFF x86, sem
    # fonte); este arquivo e a versao portavel, e esta todo sob !_WIN32.
    ${MU_GAME_SRC}/Platform/SimpleModulusPortable.cpp
    # Cliente de rede. WSAAsyncSelect (notificacao por mensagem de janela) nao
    # tem equivalente fora do Windows; LegacySocketPump entrega os mesmos
    # eventos por polling, um select() por quadro.
    # Vetores globais que o Winmain aloca e que OpenBasicData exige.
    # Subida da cena do cliente (WebzenScene + o que o Winmain faz antes).
    # Compartilhada para o Android nao ficar preso ao terreno de teste.
    ${MU_GAME_SRC}/Platform/LegacySceneBringup.cpp
    # Globais do Winmain. Compartilhados de proposito: quando viviam so no
    # lado Web, o Android os recebia zerados do arquivo de stubs e divergia
    # em silencio (g_strSelectedML vazio quebrava Data/Local).
    ${MU_GAME_SRC}/Platform/LegacyClientGlobals.cpp
    ${MU_GAME_SRC}/Platform/LegacyGlobalAllocations.cpp
    ${MU_GAME_SRC}/Platform/LegacySocketPump.cpp
    ${MU_GAME_SRC}/WSctlc.cpp
    ${MU_GAME_SRC}/Platform/PlatformShell.cpp
    ${MU_GAME_SRC}/Platform/PlatformText.cpp
    # Controle EDIT sintetico: sem ele CUITextInputBox nao tem onde guardar nem como
    # desenhar texto, e os campos de Conta/Senha do login ficam vazios e mudos.
    ${MU_GAME_SRC}/Platform/PlatformEditControl.cpp
    # Estado de teclado do jogo: sem ele GetAsyncKeyState devolve 0 e NENHUM atalho do
    # cliente funciona (Enter, ESC, F1-F12, teclas de habilidade).
    ${MU_GAME_SRC}/Platform/PlatformKeyboard.cpp
    # Os Init() de subsistemas de script do Winmain. Sem eles todo lua_State fica
    # vazio e o cliente chama funcoes de script que nao existem.
    ${MU_GAME_SRC}/Platform/LegacyLuaSubsystems.cpp
    ${MU_GAME_SRC}/MessageColor.cpp
    ${MU_GAME_SRC}/MixMgr.cpp
    ${MU_GAME_SRC}/MonkSystem.cpp
    ${MU_GAME_SRC}/MonsterEffect.cpp
    ${MU_GAME_SRC}/MonsterGlow.cpp
    ${MU_GAME_SRC}/MoveCommandData.cpp
    ${MU_GAME_SRC}/MovereqInfo.cpp
    ${MU_GAME_SRC}/MovieScene.cpp
    ${MU_GAME_SRC}/MsgWin.cpp
    ${MU_GAME_SRC}/NProtocol.cpp
    ${MU_GAME_SRC}/NewBloodCastleSystem.cpp
    ${MU_GAME_SRC}/NewChaosCastleSystem.cpp
    ${MU_GAME_SRC}/NewInventory.cpp
    ${MU_GAME_SRC}/NewUI3DRenderMng.cpp
    ${MU_GAME_SRC}/NewUIBattleSoccerScore.cpp
    ${MU_GAME_SRC}/NewUIBloodCastleEnter.cpp
    ${MU_GAME_SRC}/NewUIBloodCastleTime.cpp
    ${MU_GAME_SRC}/NewUIBuffWindow.cpp
    ${MU_GAME_SRC}/NewUIButton.cpp
    ${MU_GAME_SRC}/NewUICastleWindow.cpp
    ${MU_GAME_SRC}/NewUICatapultWindow.cpp
    ${MU_GAME_SRC}/NewUIChaosCastleTime.cpp
    ${MU_GAME_SRC}/NewUICharacterInfoWindow.cpp
    ${MU_GAME_SRC}/NewUIChatLogWindow.cpp
    ${MU_GAME_SRC}/NewUICommandWindow.cpp
    ${MU_GAME_SRC}/NewUICryWolf.cpp
    ${MU_GAME_SRC}/NewUICursedTempleEnter.cpp
    ${MU_GAME_SRC}/NewUICursedTempleResult.cpp
    ${MU_GAME_SRC}/NewUICursedTempleSystem.cpp
    ${MU_GAME_SRC}/NewUIDoppelGangerFrame.cpp
    ${MU_GAME_SRC}/NewUIDoppelGangerWindow.cpp
    ${MU_GAME_SRC}/NewUIDuelWatchMainFrameWindow.cpp
    ${MU_GAME_SRC}/NewUIDuelWatchUserListWindow.cpp
    ${MU_GAME_SRC}/NewUIDuelWatchWindow.cpp
    ${MU_GAME_SRC}/NewUIDuelWindow.cpp
    ${MU_GAME_SRC}/NewUIEmpireGuardianNPC.cpp
    ${MU_GAME_SRC}/NewUIEmpireGuardianTimer.cpp
    ${MU_GAME_SRC}/NewUIEnterDevilSquare.cpp
    ${MU_GAME_SRC}/NewUIExchangeLuckyCoin.cpp
    ${MU_GAME_SRC}/NewUIFriendWindow.cpp
    ${MU_GAME_SRC}/NewUIGateSwitchWindow.cpp
    ${MU_GAME_SRC}/NewUIGatemanWindow.cpp
    ${MU_GAME_SRC}/NewUIGensRanking.cpp
    ${MU_GAME_SRC}/NewUIGoldBowmanLena.cpp
    ${MU_GAME_SRC}/NewUIGoldBowmanWindow.cpp
    ${MU_GAME_SRC}/NewUIGroup.cpp
    ${MU_GAME_SRC}/NewUIHelpWindow.cpp
    ${MU_GAME_SRC}/NewUIHotKey.cpp
    ${MU_GAME_SRC}/NewUIItemEnduranceInfo.cpp
    ${MU_GAME_SRC}/NewUIItemExplanationWindow.cpp
    ${MU_GAME_SRC}/NewUIKanturuEvent.cpp
    ${MU_GAME_SRC}/NewUILuckyItemWnd.cpp
    ${MU_GAME_SRC}/NewUIManager.cpp
    ${MU_GAME_SRC}/NewUIMasterLevel.cpp
    ${MU_GAME_SRC}/NewUIMiniMap.cpp
    ${MU_GAME_SRC}/NewUIMixInventory.cpp
    ${MU_GAME_SRC}/NewUIMyInventory.cpp
    ${MU_GAME_SRC}/NewUIMyQuestInfoWindow.cpp
    ${MU_GAME_SRC}/NewUIMyShopInventory.cpp
    ${MU_GAME_SRC}/NewUINPCQuest.cpp
    ${MU_GAME_SRC}/NewUINPCShop.cpp
    ${MU_GAME_SRC}/NewUINameWindow.cpp
    ${MU_GAME_SRC}/NewUIOptionWindow.cpp
    ${MU_GAME_SRC}/NewUIPCPoint.cpp
    ${MU_GAME_SRC}/NewUIPartyInfoWindow.cpp
    ${MU_GAME_SRC}/NewUIPartyListWindow.cpp
    ${MU_GAME_SRC}/NewUIPetInfoWindow.cpp
    ${MU_GAME_SRC}/NewUIPurchaseShopInventory.cpp
    ${MU_GAME_SRC}/NewUIQuestProgress.cpp
    ${MU_GAME_SRC}/NewUIQuestProgressByEtc.cpp
    ${MU_GAME_SRC}/NewUIQuickCommandWindow.cpp
    ${MU_GAME_SRC}/NewUIRegistrationLuckyCoin.cpp
    ${MU_GAME_SRC}/NewUIRenderNumber.cpp
    ${MU_GAME_SRC}/NewUIScrollBar.cpp
    ${MU_GAME_SRC}/NewUISeigeWarfare.cpp
    ${MU_GAME_SRC}/NewUISetItemExplanation.cpp
    ${MU_GAME_SRC}/NewUISiegeWarBase.cpp
    ${MU_GAME_SRC}/NewUISiegeWarCommander.cpp
    ${MU_GAME_SRC}/NewUISiegeWarObserver.cpp
    ${MU_GAME_SRC}/NewUISiegeWarSoldier.cpp
    ${MU_GAME_SRC}/NewUISlideWindow.cpp
    ${MU_GAME_SRC}/NewUIStorageInventory.cpp
    ${MU_GAME_SRC}/NewUISystem.cpp
    ${MU_GAME_SRC}/NewUITextBox.cpp
    ${MU_GAME_SRC}/NewUIUnitedMarketPlaceWindow.cpp
    ${MU_GAME_SRC}/NewUIWindowMenu.cpp
    ${MU_GAME_SRC}/Observer.cpp
    ${MU_GAME_SRC}/OpenGL3/Shader/Shader.cpp
    ${MU_GAME_SRC}/PartyManager.cpp
    ${MU_GAME_SRC}/PhysicsManager.cpp
    ${MU_GAME_SRC}/PortalMgr.cpp
    ${MU_GAME_SRC}/Preview.cpp
    ${MU_GAME_SRC}/ProtocolSend.cpp
    ${MU_GAME_SRC}/QuestInfo.cpp
    ${MU_GAME_SRC}/QuestMng.cpp
    ${MU_GAME_SRC}/RenderModel.cpp
    ${MU_GAME_SRC}/SMD.cpp
    ${MU_GAME_SRC}/SMD2BMD.cpp
    ${MU_GAME_SRC}/ServerGroup.cpp
    ${MU_GAME_SRC}/ServerInfo.cpp
    ${MU_GAME_SRC}/ServerListManager.cpp
    ${MU_GAME_SRC}/ServerMsgWin.cpp
    ${MU_GAME_SRC}/ServerSelWin.cpp
    ${MU_GAME_SRC}/ShadowVolume.cpp
    ${MU_GAME_SRC}/SideHair.cpp
    ${MU_GAME_SRC}/SkillEffectMgr.cpp
    ${MU_GAME_SRC}/SkillInfo.cpp
    ${MU_GAME_SRC}/SkillManager.cpp
    ${MU_GAME_SRC}/SlideInfo.cpp
    ${MU_GAME_SRC}/Slider.cpp
    ${MU_GAME_SRC}/SocketSystem.cpp
    ${MU_GAME_SRC}/Sprite.cpp
    ${MU_GAME_SRC}/StackInterface.cpp
    ${MU_GAME_SRC}/SummonSystem.cpp
    ${MU_GAME_SRC}/SysMenuWin.cpp
    ${MU_GAME_SRC}/TestEventMap.cpp
    ${MU_GAME_SRC}/TextureScript.cpp
    ${MU_GAME_SRC}/Time/CTimCheck.cpp
    ${MU_GAME_SRC}/Time/Timer.cpp
    ${MU_GAME_SRC}/TradeXS.cpp
    ${MU_GAME_SRC}/UIDefaultBase.cpp
    ${MU_GAME_SRC}/UIGateKeeper.cpp
    ${MU_GAME_SRC}/UIGuardsMan.cpp
    ${MU_GAME_SRC}/UIGuildMaster.cpp
    ${MU_GAME_SRC}/UIJewelHarmony.cpp
    ${MU_GAME_SRC}/UIMapName.cpp
    ${MU_GAME_SRC}/UIPopup.cpp
    ${MU_GAME_SRC}/UISenatus.cpp
    ${MU_GAME_SRC}/Utilities/Memory/MemoryLock.cpp
    ${MU_GAME_SRC}/Visuals.cpp
    ${MU_GAME_SRC}/Widescreen.cpp
    ${MU_GAME_SRC}/Win.cpp
    ${MU_GAME_SRC}/WinEx.cpp
    ${MU_GAME_SRC}/ZzzAI.cpp
    ${MU_GAME_SRC}/ZzzBMD.cpp
    ${MU_GAME_SRC}/ZzzEffect.cpp
    ${MU_GAME_SRC}/ZzzEffectBlurSpark.cpp
    ${MU_GAME_SRC}/ZzzEffectFireLeave.cpp
    ${MU_GAME_SRC}/ZzzEffectJoint.cpp
    ${MU_GAME_SRC}/ZzzEffectMagicSkill.cpp
    ${MU_GAME_SRC}/ZzzEffectNoUse.cpp
    ${MU_GAME_SRC}/ZzzEffectPoint.cpp
    ${MU_GAME_SRC}/ZzzEffectPointer.cpp
    ${MU_GAME_SRC}/ZzzInfomation.cpp
    ${MU_GAME_SRC}/ZzzLodTerrain.cpp
    ${MU_GAME_SRC}/ZzzObject.cpp
    ${MU_GAME_SRC}/ZzzOpenglUtil.cpp
    ${MU_GAME_SRC}/ZzzCharacter.cpp
    ${MU_GAME_SRC}/ZzzInventory.cpp
    ${MU_GAME_SRC}/WSclient.cpp
    ${MU_GAME_SRC}/LoginWin.cpp
    ${MU_GAME_SRC}/Input.cpp
    ${MU_GAME_SRC}/NewUIInventoryCtrl.cpp
    ${MU_GAME_SRC}/CCRC32.cpp
    ${MU_GAME_SRC}/CSMapServer.cpp
    ${MU_GAME_SRC}/Camera3D.cpp
    ${MU_GAME_SRC}/CounterItem.cpp
    ${MU_GAME_SRC}/CustomWing.cpp
    ${MU_GAME_SRC}/DarkSpirit.cpp
    ${MU_GAME_SRC}/ExternalObject/Leaf/html_log.cpp
    ${MU_GAME_SRC}/ExternalObject/Leaf/xortrans.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSBuyConfirm.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSBuyPackageItem.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSBuySelectItem.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSSendGift.cpp
    ${MU_GAME_SRC}/GameShop/MsgBoxIGSSendGiftConfirm.cpp
    ${MU_GAME_SRC}/HelperSystem.cpp
    ${MU_GAME_SRC}/ItemManager.cpp
    # Console de depuracao. Fora do Windows so DOSConsole::Write vive (escreve em
    # stderr); o resto e AllocConsole e uma thread de stdin, sem equivalente.
    ${MU_GAME_SRC}/Console.cpp
    ${MU_GAME_SRC}/Lua.cpp
    ${MU_GAME_SRC}/MonsterName.cpp
    ${MU_GAME_SRC}/NewUIGuardWindow.cpp
    ${MU_GAME_SRC}/NewUIGuildInfoWindow.cpp
    ${MU_GAME_SRC}/NewUIGuildMakeWindow.cpp
    ${MU_GAME_SRC}/NewUIItemMng.cpp
    ${MU_GAME_SRC}/NewUIMainFrameWindow.cpp
    ${MU_GAME_SRC}/PList.cpp
    ${MU_GAME_SRC}/PacketManager.cpp
    ${MU_GAME_SRC}/ServerName.cpp
    ${MU_GAME_SRC}/TradeX.cpp
    ${MU_GAME_SRC}/UIGuildInfo.cpp
    ${MU_GAME_SRC}/UIMng.cpp
    ${MU_GAME_SRC}/UsefulDef.cpp
    ${MU_GAME_SRC}/ZzzScene.cpp
    ${MU_GAME_SRC}/ZzzTexture.cpp
    ${MU_GAME_SRC}/_GlobalFunctions.cpp
    ${MU_GAME_SRC}/npcBreeder.cpp
    ${MU_GAME_SRC}/npcCatapult.cpp
    ${MU_GAME_SRC}/npcGateSwitch.cpp
    ${MU_GAME_SRC}/w_Buff.cpp
    ${MU_GAME_SRC}/w_BuffStateSystem.cpp
    ${MU_GAME_SRC}/w_BuffStateValueControl.cpp
    ${MU_GAME_SRC}/w_CharacterInfo.cpp
    ${MU_GAME_SRC}/w_CursedTemple.cpp
    ${MU_GAME_SRC}/w_MapProcess.cpp
    ${MU_GAME_SRC}/w_ObjectInfo.cpp
    ${MU_GAME_SRC}/w_PetActionCollecter.cpp
    ${MU_GAME_SRC}/w_PetActionCollecter_Add.cpp
    ${MU_GAME_SRC}/w_PetActionDemon.cpp
    ${MU_GAME_SRC}/w_PetActionRound.cpp
    ${MU_GAME_SRC}/w_PetActionStand.cpp
    ${MU_GAME_SRC}/w_PetActionUnicorn.cpp
    ${MU_GAME_SRC}/w_PetProcess.cpp
    ${MU_GAME_SRC}/zzzMixInvetory.cpp
    ${MU_GAME_SRC}/zzzeffectsprite.cpp
    ${MU_GAME_SRC}/zzzpath.cpp

    # Modulos trazidos para fechar os simbolos que faltavam ao Android.
    # Cada um foi identificado mapeando os simbolos indefinidos da .so de
    # volta para o arquivo que os define.
    ${MU_GAME_SRC}/CSQuest.cpp
    ${MU_GAME_SRC}/ChaosGenesis.cpp
    ${MU_GAME_SRC}/CreditWin.cpp
    ${MU_GAME_SRC}/GMBattleCastle.cpp
    ${MU_GAME_SRC}/GameShop/InGameShopSystem.cpp
    ${MU_GAME_SRC}/GameShop/NewUIInGameShop.cpp
    ${MU_GAME_SRC}/GaugeBar.cpp
    ${MU_GAME_SRC}/GlobalBitmap.cpp
    ${MU_GAME_SRC}/LoadData.cpp
    ${MU_GAME_SRC}/LuaDecrypt.cpp
    ${MU_GAME_SRC}/LuaGlobal.cpp
    ${MU_GAME_SRC}/LuaInterface.cpp
    ${MU_GAME_SRC}/LuaOpenFolder.cpp
    ${MU_GAME_SRC}/LuaReg.cpp
    ${MU_GAME_SRC}/Monsters.cpp
    ${MU_GAME_SRC}/NewUIChatInputBox.cpp
    ${MU_GAME_SRC}/NewUICommon.cpp
    ${MU_GAME_SRC}/NewUICommonMessageBox.cpp
    ${MU_GAME_SRC}/NewUICustomMessageBox.cpp
    ${MU_GAME_SRC}/NewUIHeroPositionInfo.cpp
    ${MU_GAME_SRC}/NewUIMessageBox.cpp
    ${MU_GAME_SRC}/NewUIMoveCommandWindow.cpp
    ${MU_GAME_SRC}/NewUINPCDialogue.cpp
    ${MU_GAME_SRC}/NewUITrade.cpp
    ${MU_GAME_SRC}/OptionWin.cpp
    ${MU_GAME_SRC}/PersonalShopTitleImp.cpp
    ${MU_GAME_SRC}/Patente.cpp
    ${MU_GAME_SRC}/Protocol.cpp
    ${MU_GAME_SRC}/UIManager.cpp
    ${MU_GAME_SRC}/UIWindows.cpp
    ${MU_GAME_SRC}/Utilities/Log/muConsoleDebug.cpp
    ${MU_GAME_SRC}/ZzzEffectParticle.cpp
    ${MU_GAME_SRC}/ZzzInterface.cpp
    ${MU_GAME_SRC}/ZzzOpenData.cpp
    ${MU_GAME_SRC}/w_BasePet.cpp
    ${MU_GAME_SRC}/w_BuffScriptLoader.cpp
    ${MU_GAME_SRC}/w_BuffTimeControl.cpp
    ${MU_GAME_SRC}/Platform/PlatformAudio.cpp
    ${MU_GAME_SRC}/Platform/LegacyAudioBridge.cpp
)
