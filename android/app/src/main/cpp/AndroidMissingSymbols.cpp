// GERADO AUTOMATICAMENTE - nao editar a mao. Ver genstubs2.py.
//
// Os subsistemas que ainda nao foram portados (rede, audio, protecao, globais de
// Winmain) deixam simbolos indefinidos. No Web isso e tolerado por
// -sERROR_ON_UNDEFINED_SYMBOLS=0, que cria um stub por funcao e so falha quando a
// chamada acontece. O linker dinamico do Android nao tem equivalente: com
// BIND_NOW ele resolve tudo no dlopen e a biblioteca sequer carrega, como
// aconteceu com "cannot locate symbol g_hFixFont".
//
// Este arquivo reproduz a semantica do Web: cada funcao ausente vira um stub que
// registra o proprio nome no logcat e aborta; cada dado ausente ganha
// armazenamento zerado. A lista encolhe conforme os subsistemas forem portados.
//
// Para regerar depois de mudar o conjunto de modulos:
//   1. tire este arquivo do CMakeLists e linke com -Wl,--unresolved-symbols=ignore-all
//   2. llvm-readelf --dyn-syms libmu_legacy_platform.so \\
//        | awk '$7=="UND" && $8 !~ /@/ {print $4" "$8}' | sort -u > und.txt
//   3. python genstubs2.py und.txt llvm-cxxfilt AndroidMissingSymbols.cpp

#include <android/log.h>
#include <stdlib.h>

namespace
{
    // Registra uma vez por simbolo, em nivel ERROR, e devolve nulo.
    //
    // A primeira versao abortava, por preferir falha ruidosa a resultado errado
    // em silencio. Nao da: construtores estaticos dos modulos ligados rodam no
    // dlopen, e um deles chama RegisterLuaReg — o app morria antes de desenhar
    // qualquer coisa. O log por simbolo mantem o aviso visivel sem impedir que a
    // cena suba; `adb logcat -s MuLegacy` lista tudo que foi alcancado.
    //
    // O retorno e void*, nao void: muitas destas funcoes devolvem ponteiro ou
    // inteiro, e zerar o registrador de retorno e bem mais seguro do que deixar
    // lixo do quadro anterior virar um ponteiro.
    void* Unimplemented(const char* symbol, bool* reported)
    {
        if (!*reported)
        {
            *reported = true;
            __android_log_print(ANDROID_LOG_ERROR, "MuLegacy",
                                "subsistema nao portado alcancado: %s", symbol);
        }
        return 0;
    }
}

// funcoes ausentes: 59
//
// QUATRO STUBS SAIRAM DAQUI (2026-08-11): GetCheckSum, Util_CheckOption,
// CheckHack e TrayMode::SwitchState. Eles nasceram quando o alvo Android
// linkava so a camada Platform; agora que libmu_legacy_scene.a entra no link,
// as implementacoes REAIS existem e o stub virava `duplicate symbol` -- o link
// do arm64-v8a parava nos quatro. Quem regerar esta lista precisa filtrar
// contra os simbolos que a biblioteca de cena ja define, senao eles voltam.
extern "C" void* mu_missing_fn_1() { static bool reported = false; return Unimplemented("_Z12CloseMainExev", &reported); }  // CloseMainExe()
extern "C" void* mu_missing_fn_2() { static bool reported = false; return Unimplemented("_Z12KillGLWindowv", &reported); }  // KillGLWindow()
extern "C" void* mu_missing_fn_3() { static bool reported = false; return Unimplemented("_Z13DestroyWindowv", &reported); }  // DestroyWindow()
extern "C" void* mu_missing_fn_5() asm("_Z17TERRAIN_ATTRIBUTEff");
extern "C" void* mu_missing_fn_5() { static bool reported = false; return Unimplemented("_Z17TERRAIN_ATTRIBUTEff", &reported); }  // TERRAIN_ATTRIBUTE(float, float)
extern "C" void* mu_missing_fn_6() asm("_Z19RegisterClassObjectP9lua_State");
extern "C" void* mu_missing_fn_6() { static bool reported = false; return Unimplemented("_Z19RegisterClassObjectP9lua_State", &reported); }  // RegisterClassObject(lua_State*)
extern "C" void* mu_missing_fn_8() asm("_ZN10DOSConsole5WriteEbPKcz");
extern "C" void* mu_missing_fn_8() { static bool reported = false; return Unimplemented("_ZN10DOSConsole5WriteEbPKcz", &reported); }  // DOSConsole::Write(bool, char const*, ...)
extern "C" void* mu_missing_fn_9() asm("_ZN11CBannerInfo9SetBannerENSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEES6_b");
extern "C" void* mu_missing_fn_9() { static bool reported = false; return Unimplemented("_ZN11CBannerInfo9SetBannerENSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEES6_b", &reported); }  // CBannerInfo::SetBanner(std::__ndk1::basic_string<char, std::__ndk1::char_traits<char>, std::__ndk1::allocator<char> >, std::__ndk1::basic_string<char, std::__ndk1::char_traits<char>, std::__ndk1::allocator<char> >, bool)
extern "C" void* mu_missing_fn_10() asm("_ZN11CBannerInfoC1Ev");
extern "C" void* mu_missing_fn_10() { static bool reported = false; return Unimplemented("_ZN11CBannerInfoC1Ev", &reported); }  // CBannerInfo::CBannerInfo()
extern "C" void* mu_missing_fn_11() asm("_ZN11CBannerInfoD1Ev");
extern "C" void* mu_missing_fn_11() { static bool reported = false; return Unimplemented("_ZN11CBannerInfoD1Ev", &reported); }  // CBannerInfo::~CBannerInfo()
extern "C" void* mu_missing_fn_12() asm("_ZN12CListManager13GetScriptPathEv");
extern "C" void* mu_missing_fn_12() { static bool reported = false; return Unimplemented("_ZN12CListManager13GetScriptPathEv", &reported); }  // CListManager::GetScriptPath()
extern "C" void* mu_missing_fn_13() asm("_ZN12CListManager14LoadScriptListEb");
extern "C" void* mu_missing_fn_13() { static bool reported = false; return Unimplemented("_ZN12CListManager14LoadScriptListEb", &reported); }  // CListManager::LoadScriptList(bool)
extern "C" void* mu_missing_fn_14() asm("_ZN12CListManager18SetListManagerInfoE15_DownloaderTypePcS1_S1_S1_S1_16CListVersionInfoj");
extern "C" void* mu_missing_fn_14() { static bool reported = false; return Unimplemented("_ZN12CListManager18SetListManagerInfoE15_DownloaderTypePcS1_S1_S1_S1_16CListVersionInfoj", &reported); }  // CListManager::SetListManagerInfo(_DownloaderType, char*, char*, char*, char*, char*, CListVersionInfo, unsigned int)
extern "C" void* mu_missing_fn_15() asm("_ZN12CListManagerC2Ev");
extern "C" void* mu_missing_fn_15() { static bool reported = false; return Unimplemented("_ZN12CListManagerC2Ev", &reported); }  // CListManager::CListManager()
extern "C" void* mu_missing_fn_16() asm("_ZN12CListManagerD2Ev");
extern "C" void* mu_missing_fn_16() { static bool reported = false; return Unimplemented("_ZN12CListManagerD2Ev", &reported); }  // CListManager::~CListManager()
extern "C" void* mu_missing_fn_17() asm("_ZN12CShopPackage15GetPriceSeqNextERi");
extern "C" void* mu_missing_fn_17() { static bool reported = false; return Unimplemented("_ZN12CShopPackage15GetPriceSeqNextERi", &reported); }  // CShopPackage::GetPriceSeqNext(int&)
extern "C" void* mu_missing_fn_18() asm("_ZN12CShopPackage16SetPriceSeqFirstEv");
extern "C" void* mu_missing_fn_18() { static bool reported = false; return Unimplemented("_ZN12CShopPackage16SetPriceSeqFirstEv", &reported); }  // CShopPackage::SetPriceSeqFirst()
extern "C" void* mu_missing_fn_19() asm("_ZN12CShopPackage17GetProductSeqNextERi");
extern "C" void* mu_missing_fn_19() { static bool reported = false; return Unimplemented("_ZN12CShopPackage17GetProductSeqNextERi", &reported); }  // CShopPackage::GetProductSeqNext(int&)
extern "C" void* mu_missing_fn_20() asm("_ZN12CShopPackage18SetProductSeqFirstEv");
extern "C" void* mu_missing_fn_20() { static bool reported = false; return Unimplemented("_ZN12CShopPackage18SetProductSeqFirstEv", &reported); }  // CShopPackage::SetProductSeqFirst()
extern "C" void* mu_missing_fn_21() asm("_ZN12CShopPackageC1Ev");
extern "C" void* mu_missing_fn_21() { static bool reported = false; return Unimplemented("_ZN12CShopPackageC1Ev", &reported); }  // CShopPackage::CShopPackage()
extern "C" void* mu_missing_fn_22() asm("_ZN12CShopPackageD1Ev");
extern "C" void* mu_missing_fn_22() { static bool reported = false; return Unimplemented("_ZN12CShopPackageD1Ev", &reported); }  // CShopPackage::~CShopPackage()
extern "C" void* mu_missing_fn_23() asm("_ZN12CShopProductC1Ev");
extern "C" void* mu_missing_fn_23() { static bool reported = false; return Unimplemented("_ZN12CShopProductC1Ev", &reported); }  // CShopProduct::CShopProduct()
extern "C" void* mu_missing_fn_24() asm("_ZN12CShopProductD1Ev");
extern "C" void* mu_missing_fn_24() { static bool reported = false; return Unimplemented("_ZN12CShopProductD1Ev", &reported); }  // CShopProduct::~CShopProduct()
extern "C" void* mu_missing_fn_25() asm("_ZN13CKeyGenerater16GenerateKeyValueEj");
extern "C" void* mu_missing_fn_25() { static bool reported = false; return Unimplemented("_ZN13CKeyGenerater16GenerateKeyValueEj", &reported); }  // CKeyGenerater::GenerateKeyValue(unsigned int)
extern "C" void* mu_missing_fn_26() asm("_ZN13CShopCategory13AddPackageSeqEi");
extern "C" void* mu_missing_fn_26() { static bool reported = false; return Unimplemented("_ZN13CShopCategory13AddPackageSeqEi", &reported); }  // CShopCategory::AddPackageSeq(int)
extern "C" void* mu_missing_fn_27() asm("_ZN13CShopCategory15ClearPackageSeqEv");
extern "C" void* mu_missing_fn_27() { static bool reported = false; return Unimplemented("_ZN13CShopCategory15ClearPackageSeqEv", &reported); }  // CShopCategory::ClearPackageSeq()
extern "C" void* mu_missing_fn_28() asm("_ZN13CShopCategory15GetCategoryNextERi");
extern "C" void* mu_missing_fn_28() { static bool reported = false; return Unimplemented("_ZN13CShopCategory15GetCategoryNextERi", &reported); }  // CShopCategory::GetCategoryNext(int&)
extern "C" void* mu_missing_fn_29() asm("_ZN13CShopCategory16GetPackagSeqNextERi");
extern "C" void* mu_missing_fn_29() { static bool reported = false; return Unimplemented("_ZN13CShopCategory16GetPackagSeqNextERi", &reported); }  // CShopCategory::GetPackagSeqNext(int&)
extern "C" void* mu_missing_fn_30() asm("_ZN13CShopCategory16SetCategoryFirstEv");
extern "C" void* mu_missing_fn_30() { static bool reported = false; return Unimplemented("_ZN13CShopCategory16SetCategoryFirstEv", &reported); }  // CShopCategory::SetCategoryFirst()
extern "C" void* mu_missing_fn_31() asm("_ZN13CShopCategory17SetPackagSeqFirstEv");
extern "C" void* mu_missing_fn_31() { static bool reported = false; return Unimplemented("_ZN13CShopCategory17SetPackagSeqFirstEv", &reported); }  // CShopCategory::SetPackagSeqFirst()
extern "C" void* mu_missing_fn_32() asm("_ZN13CShopCategoryC1Ev");
extern "C" void* mu_missing_fn_32() { static bool reported = false; return Unimplemented("_ZN13CShopCategoryC1Ev", &reported); }  // CShopCategory::CShopCategory()
extern "C" void* mu_missing_fn_33() asm("_ZN13CShopCategoryD1Ev");
extern "C" void* mu_missing_fn_33() { static bool reported = false; return Unimplemented("_ZN13CShopCategoryD1Ev", &reported); }  // CShopCategory::~CShopCategory()
extern "C" void* mu_missing_fn_34() { static bool reported = false; return Unimplemented("_ZN14CSimpleModulus7DecryptEPvS0_i", &reported); }  // CSimpleModulus::Decrypt(void*, void*, int)
extern "C" void* mu_missing_fn_35() { static bool reported = false; return Unimplemented("_ZN14CSimpleModulus7EncryptEPvS0_i", &reported); }  // CSimpleModulus::Encrypt(void*, void*, int)
extern "C" void* mu_missing_fn_36() { static bool reported = false; return Unimplemented("_ZN14CSimpleModulusC1Ev", &reported); }  // CSimpleModulus::CSimpleModulus()
extern "C" void* mu_missing_fn_37() { static bool reported = false; return Unimplemented("_ZN14CSimpleModulusD1Ev", &reported); }  // CSimpleModulus::~CSimpleModulus()
extern "C" void* mu_missing_fn_38() { static bool reported = false; return Unimplemented("_ZN7CWsctlc10GetReadMsgEv", &reported); }  // CWsctlc::GetReadMsg()
extern "C" void* mu_missing_fn_39() { static bool reported = false; return Unimplemented("_ZN7CWsctlc11FDWriteSendEv", &reported); }  // CWsctlc::FDWriteSend()
extern "C" void* mu_missing_fn_40() { static bool reported = false; return Unimplemented("_ZN7CWsctlc12LogHexPrintSEPhi", &reported); }  // CWsctlc::LogHexPrintS(unsigned char*, int)
extern "C" void* mu_missing_fn_41() { static bool reported = false; return Unimplemented("_ZN7CWsctlc5CloseEv", &reported); }  // CWsctlc::Close()
extern "C" void* mu_missing_fn_42() { static bool reported = false; return Unimplemented("_ZN7CWsctlc5nRecvEv", &reported); }  // CWsctlc::nRecv()
extern "C" void* mu_missing_fn_43() { static bool reported = false; return Unimplemented("_ZN7CWsctlc6CreateEPvi", &reported); }  // CWsctlc::Create(void*, int)
extern "C" void* mu_missing_fn_44() { static bool reported = false; return Unimplemented("_ZN7CWsctlc7ConnectEPctj", &reported); }  // CWsctlc::Connect(char*, unsigned short, unsigned int)
extern "C" void* mu_missing_fn_45() { static bool reported = false; return Unimplemented("_ZN7CWsctlc7StartupEv", &reported); }  // CWsctlc::Startup()
extern "C" void* mu_missing_fn_46() { static bool reported = false; return Unimplemented("_ZN7CWsctlc9GetSocketEv", &reported); }  // CWsctlc::GetSocket()
extern "C" void* mu_missing_fn_47() { static bool reported = false; return Unimplemented("_ZN7CWsctlcC1Ev", &reported); }  // CWsctlc::CWsctlc()
extern "C" void* mu_missing_fn_48() { static bool reported = false; return Unimplemented("_ZN7CWsctlcD1Ev", &reported); }  // CWsctlc::~CWsctlc()
extern "C" void* mu_missing_fn_50() asm("_ZN8WZResult15GetErrorMessageEv");
extern "C" void* mu_missing_fn_50() { static bool reported = false; return Unimplemented("_ZN8WZResult15GetErrorMessageEv", &reported); }  // WZResult::GetErrorMessage()
extern "C" void* mu_missing_fn_51() asm("_ZN8WZResult18BuildSuccessResultEv");
extern "C" void* mu_missing_fn_51() { static bool reported = false; return Unimplemented("_ZN8WZResult18BuildSuccessResultEv", &reported); }  // WZResult::BuildSuccessResult()
extern "C" void* mu_missing_fn_52() asm("_ZN8WZResult9IsSuccessEv");
extern "C" void* mu_missing_fn_52() { static bool reported = false; return Unimplemented("_ZN8WZResult9IsSuccessEv", &reported); }  // WZResult::IsSuccess()
extern "C" void* mu_missing_fn_53() asm("_ZN8WZResult9SetResultEjjPcz");
extern "C" void* mu_missing_fn_53() { static bool reported = false; return Unimplemented("_ZN8WZResult9SetResultEjjPcz", &reported); }  // WZResult::SetResult(unsigned int, unsigned int, char*, ...)
extern "C" void* mu_missing_fn_54() asm("_ZN8WZResultC1Ev");
extern "C" void* mu_missing_fn_54() { static bool reported = false; return Unimplemented("_ZN8WZResultC1Ev", &reported); }  // WZResult::WZResult()
extern "C" void* mu_missing_fn_55() asm("_ZN8WZResultD1Ev");
extern "C" void* mu_missing_fn_55() { static bool reported = false; return Unimplemented("_ZN8WZResultD1Ev", &reported); }  // WZResult::~WZResult()
extern "C" void* mu_missing_fn_56() asm("_ZN8WZResultaSERKS_");
extern "C" void* mu_missing_fn_56() { static bool reported = false; return Unimplemented("_ZN8WZResultaSERKS_", &reported); }  // WZResult::operator=(WZResult const&)
extern "C" void* mu_missing_fn_57() asm("_ZN9CShopList11LoadPackageEPKc");
extern "C" void* mu_missing_fn_57() { static bool reported = false; return Unimplemented("_ZN9CShopList11LoadPackageEPKc", &reported); }  // CShopList::LoadPackage(char const*)
extern "C" void* mu_missing_fn_58() asm("_ZN9CShopList11LoadProductEPKc");
extern "C" void* mu_missing_fn_58() { static bool reported = false; return Unimplemented("_ZN9CShopList11LoadProductEPKc", &reported); }  // CShopList::LoadProduct(char const*)
extern "C" void* mu_missing_fn_59() asm("_ZN9CShopList12LoadCategroyEPKc");
extern "C" void* mu_missing_fn_59() { static bool reported = false; return Unimplemented("_ZN9CShopList12LoadCategroyEPKc", &reported); }  // CShopList::LoadCategroy(char const*)
extern "C" void* mu_missing_fn_60() asm("_ZN9CShopListC1Ev");
extern "C" void* mu_missing_fn_60() { static bool reported = false; return Unimplemented("_ZN9CShopListC1Ev", &reported); }  // CShopList::CShopList()
extern "C" void* mu_missing_fn_61() asm("_ZTv0_n24_NSt6__ndk113basic_istreamIcNS_11char_traitsIcEEED0Ev");
extern "C" void* mu_missing_fn_61() { static bool reported = false; return Unimplemented("_ZTv0_n24_NSt6__ndk113basic_istreamIcNS_11char_traitsIcEEED0Ev", &reported); }  // virtual thunk to std::__ndk1::basic_istream<char, std::__ndk1::char_traits<char> >::~basic_istream()
extern "C" void* mu_missing_fn_62() asm("_ZTv0_n24_NSt6__ndk113basic_istreamIcNS_11char_traitsIcEEED1Ev");
extern "C" void* mu_missing_fn_62() { static bool reported = false; return Unimplemented("_ZTv0_n24_NSt6__ndk113basic_istreamIcNS_11char_traitsIcEEED1Ev", &reported); }  // virtual thunk to std::__ndk1::basic_istream<char, std::__ndk1::char_traits<char> >::~basic_istream()

// dados ausentes: 50
// 64 KB por simbolo. O tamanho real e desconhecido aqui: o simbolo e
// so um nome, sem informacao de tipo. Com 256 bytes o app quebrava com
// SIGSEGV numa thread do JIT — escrita passando do fim de um destes
// blocos corrompia memoria vizinha. A folga custa BSS (zerada, sem
// impacto no tamanho do APK) e evita esse modo de falha.
extern "C" char mu_missing_data_0[65536] asm("Console");
extern "C" char mu_missing_data_0[65536] = {0};  // Console
extern "C" char mu_missing_data_1[65536] = {0};  // Destroy
extern "C" char mu_missing_data_2[65536] = {0};  // RandomTable
extern "C" char mu_missing_data_3[65536] = {0};  // Time_Effect
extern "C" char mu_missing_data_4[65536] asm("_ZN8SEASON3B11g_iItemInfoE");
extern "C" char mu_missing_data_4[65536] = {0};  // SEASON3B::g_iItemInfo
extern "C" char mu_missing_data_5[65536] asm("_ZN8SEASON3B13TextListColorE");
extern "C" char mu_missing_data_5[65536] = {0};  // SEASON3B::TextListColor
extern "C" char mu_missing_data_6[65536] asm("_ZN8SEASON3B15g_fScreenRate_xE");
extern "C" char mu_missing_data_6[65536] = {0};  // SEASON3B::g_fScreenRate_x
extern "C" char mu_missing_data_7[65536] asm("_ZN8SEASON3B15g_fScreenRate_yE");
extern "C" char mu_missing_data_7[65536] = {0};  // SEASON3B::g_fScreenRate_y
extern "C" char mu_missing_data_8[65536] asm("_ZN8SEASON3B17SelectedCharacterE");
extern "C" char mu_missing_data_8[65536] = {0};  // SEASON3B::SelectedCharacter
extern "C" char mu_missing_data_9[65536] asm("_ZN8SEASON3B20g_iCancelSkillTargetE");
extern "C" char mu_missing_data_9[65536] = {0};  // SEASON3B::g_iCancelSkillTarget
extern "C" char mu_missing_data_10[65536] asm("_ZN8SEASON3B21g_pUIJewelHarmonyinfoE");
extern "C" char mu_missing_data_10[65536] = {0};  // SEASON3B::g_pUIJewelHarmonyinfo
extern "C" char mu_missing_data_11[65536] asm("_ZN8SEASON3B22g_iLengthAuthorityCodeE");
extern "C" char mu_missing_data_11[65536] = {0};  // SEASON3B::g_iLengthAuthorityCode
extern "C" char mu_missing_data_12[65536] asm("_ZN8SEASON3B6MouseYE");
extern "C" char mu_missing_data_12[65536] = {0};  // SEASON3B::MouseY
extern "C" char mu_missing_data_13[65536] asm("_ZN8SEASON3B7TextNumE");
extern "C" char mu_missing_data_13[65536] = {0};  // SEASON3B::TextNum
extern "C" char mu_missing_data_14[65536] asm("_ZN8SEASON3B8ItemHelpE");
extern "C" char mu_missing_data_14[65536] = {0};  // SEASON3B::ItemHelp
extern "C" char mu_missing_data_15[65536] asm("_ZN8SEASON3B8TextBoldE");
extern "C" char mu_missing_data_15[65536] = {0};  // SEASON3B::TextBold
extern "C" char mu_missing_data_16[65536] asm("_ZN8SEASON3B8TextListE");
extern "C" char mu_missing_data_16[65536] = {0};  // SEASON3B::TextList
extern "C" char mu_missing_data_17[65536] asm("_ZN8SEASON4A18IntensityTransformE");
extern "C" char mu_missing_data_17[65536] = {0};  // SEASON4A::IntensityTransform
extern "C" char mu_missing_data_18[65536] asm("_ZN8SEASON4A18g_iLimitAttackTimeE");
extern "C" char mu_missing_data_18[65536] = {0};  // SEASON4A::g_iLimitAttackTime
extern "C" char mu_missing_data_19[65536] asm("_ZTI12CListManager");
extern "C" char mu_missing_data_19[65536] = {0};  // typeinfo for CListManager
extern "C" char mu_missing_data_20[65536] asm("_ZTV11CBannerInfo");
extern "C" char mu_missing_data_20[65536] = {0};  // vtable for CBannerInfo
extern "C" char mu_missing_data_21[65536] asm("_ZTV12CShopPackage");
extern "C" char mu_missing_data_21[65536] = {0};  // vtable for CShopPackage
extern "C" char mu_missing_data_22[65536] asm("_ZTV12CShopProduct");
extern "C" char mu_missing_data_22[65536] = {0};  // vtable for CShopProduct
extern "C" char mu_missing_data_23[65536] asm("_ZTV13CShopCategory");
extern "C" char mu_missing_data_23[65536] = {0};  // vtable for CShopCategory
extern "C" char mu_missing_data_24[65536] = {0};  // ashies
extern "C" char mu_missing_data_25[65536] = {0};  // gProtect
extern "C" char mu_missing_data_26[65536] = {0};  // gTrayMode
extern "C" char mu_missing_data_27[65536] asm("g_KeyGenerater");
extern "C" char mu_missing_data_27[65536] = {0};  // g_KeyGenerater
extern "C" char mu_missing_data_28[65536] = {0};  // g_aszMLSelection
extern "C" char mu_missing_data_29[65536] = {0};  // g_bEnterPressed
extern "C" char mu_missing_data_30[65536] = {0};  // g_hDC
extern "C" char mu_missing_data_31[65536] = {0};  // g_hFixFont
extern "C" char mu_missing_data_32[65536] = {0};  // g_hFont
extern "C" char mu_missing_data_33[65536] = {0};  // g_hFontBig
extern "C" char mu_missing_data_34[65536] = {0};  // g_hFontBold
extern "C" char mu_missing_data_35[65536] = {0};  // g_hInst
extern "C" char mu_missing_data_36[65536] = {0};  // g_iChatInputType
extern "C" char mu_missing_data_37[65536] = {0};  // g_iNoMouseTime
extern "C" char mu_missing_data_38[65536] = {0};  // g_pChatRoomSocketList
extern "C" char mu_missing_data_39[65536] = {0};  // g_pSinglePasswdInputBox
extern "C" char mu_missing_data_40[65536] = {0};  // g_pSingleTextInputBox
extern "C" char mu_missing_data_41[65536] = {0};  // g_pTimer
extern "C" char mu_missing_data_42[65536] = {0};  // g_pUIManager
extern "C" char mu_missing_data_43[65536] = {0};  // g_pUIMapName
extern "C" char mu_missing_data_44[65536] = {0};  // g_strSelectedML
extern "C" char mu_missing_data_45[65536] = {0};  // m_CameraOnOff
extern "C" char mu_missing_data_46[65536] = {0};  // m_ExeVersion
extern "C" char mu_missing_data_47[65536] = {0};  // m_ID
extern "C" char mu_missing_data_48[65536] = {0};  // m_Resolution
extern "C" char mu_missing_data_49[65536] = {0};  // weather
