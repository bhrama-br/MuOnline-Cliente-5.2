// Alocacao dos vetores globais do cliente, fora do Windows.
//
// `Winmain.cpp` aloca um conjunto de vetores globais antes de qualquer
// carregamento (GateAttribute, SkillAttribute, ItemAttribute, CharactersClient,
// CharacterMachine...). Como o Winmain e o ponto de entrada Win32, ele nao
// compila em Web/Android -- e sem essas alocacoes os ponteiros ficam NULOS.
//
// O sintoma disso NAO parece um ponteiro nulo. `OpenGateScript` faz
// `memcpy(&GateAttribute[i], ...)`, entao com GateAttribute nulo a escrita cai
// no endereco 0 e o que o Emscripten reporta e:
//
//   Aborted(Stack overflow! Stack cookie has been overwritten at 0x00000004)
//
// O endereco 4 e onde mora o cookie da pilha, nao onde estava o defeito. Custou
// tres hipoteses erradas (pilha pequena, memoria insuficiente, bug no audio)
// antes de o rastro por arquivo apontar `Data/Gate.bmd`.
//
// DUAS DIFERENCAS DELIBERADAS em relacao ao Winmain, ambas ligadas a ofuscacao
// anti-hack, que nao faz sentido dentro do sandbox do navegador:
//
//   1. O Winmain aloca blocos maiores (`MAX_ITEM+1024`) e faz o ponteiro real
//      apontar para um deslocamento ALEATORIO dentro deles, para dificultar
//      varredura de memoria. Aqui a alocacao e exata: o deslocamento aleatorio
//      so gastaria memoria e tornaria as falhas nao reproduziveis.
//   2. `RendomMemoryDump` (bloco isca de tamanho aleatorio) nao existe.
//
// TODO(Platform): o certo e o Winmain passar a chamar esta mesma funcao, para
// nao haver duas listas para manter em sincronia. Isso muda codigo que o PC
// executa em producao, entao fica para uma mudanca propria.

#if !defined(_WIN32)

#include "../stdafx.h"
#include "../ZzzInfomation.h"
#include "../ZzzCharacter.h"
#include "../w_MapProcess.h"
#include "../w_PetProcess.h"
#include "../_GlobalFunctions.h"
#include "../MultiLanguage.h"
#include "../ZzzOpenData.h"   // extern g_strSelectedML
#include "LegacySceneBringup.h"   // CarregarMainInfo
#include "../UIWindows.h"         // CChatRoomSocketList
#include "../UIManager.h"         // CUIManager
#include "../UIMapName.h"         // CUIMapName

extern CChatRoomSocketList* g_pChatRoomSocketList;
extern CUIManager*          g_pUIManager;
extern CUIMapName*          g_pUIMapName;

// RandomTable e declarada em Winmain.h, que arrasta a cadeia Win32 do ponto de
// entrada; repetir a declaracao aqui evita incluir aquele cabecalho inteiro.
extern int RandomTable[];

// Chaves de pacote. Definidas em WSclient.cpp; WSclient.h tambem arrasta a cadeia
// Win32, entao a declaracao e repetida aqui.
#include "../SimpleModulus.h"
extern CSimpleModulus g_SimpleModulusCS;
extern CSimpleModulus g_SimpleModulusSC;

#include <stdlib.h>
#include <string.h>

namespace Platform
{
    void AlocarGlobaisDoCliente()
    {
        // Idempotente: chamar duas vezes nao vaza nem realoca.
        if (GateAttribute != NULL) return;

        GateAttribute    = new GATE_ATTRIBUTE [MAX_GATES];
        SkillAttribute   = new SKILL_ATTRIBUTE[MAX_SKILLS];
        ItemAttribute    = new ITEM_ATTRIBUTE [MAX_ITEM + 1];
        CharactersClient = new CHARACTER      [MAX_CHARACTERS_CLIENT + 1];
        CharacterMachine = new CHARACTER_MACHINE;

        memset(GateAttribute,    0, sizeof(GATE_ATTRIBUTE)  * MAX_GATES);
        memset(SkillAttribute,   0, sizeof(SKILL_ATTRIBUTE) * MAX_SKILLS);
        memset(ItemAttribute,    0, sizeof(ITEM_ATTRIBUTE)  * MAX_ITEM);
        memset(CharacterMachine, 0, sizeof(CHARACTER_MACHINE));

        CharacterAttribute = &CharacterMachine->Character;
        CharacterMachine->Init();
        Hero = &CharactersClient[0];

        // Quanto isto custa, em bytes. Sem a medicao nao da para distinguir
        // "alocacao grande demais" de outra causa quando o sistema mata o
        // processo com SIGKILL, que nao deixa tombstone.
        {
            const double mb = (double)(
                sizeof(GATE_ATTRIBUTE)  * MAX_GATES +
                sizeof(SKILL_ATTRIBUTE) * MAX_SKILLS +
                sizeof(ITEM_ATTRIBUTE)  * (MAX_ITEM + 1) +
                sizeof(CHARACTER)       * (MAX_CHARACTERS_CLIENT + 1) +
                sizeof(CHARACTER_MACHINE)) / (1024.0 * 1024.0);
            fprintf(stderr, "[Platform] globais do cliente: %.1f MB "
                            "(CHARACTER=%zu bytes x %d)\n",
                    mb, sizeof(CHARACTER), MAX_CHARACTERS_CLIENT + 1);
        }

        // Tabela de angulos aleatorios usada por efeitos. O Winmain a preenche
        // logo apos semear o gerador.
        for (int i = 0; i < 100; ++i)
            RandomTable[i] = rand() % 360;

        // Os tres subsistemas que o Winmain cria em 1799-1803. Diferente dos
        // ponteiros de janela, estes NAO toleram nulo: cada acessor tem um
        // `assert`, e o caminho da cena de login passa pelos tres.
        g_BuffSystem = BuffStateSystem::Make();   // _GlobalFunctions.cpp:13
        g_MapProcess = MapProcess::Make();        // w_MapProcess.cpp:12
        g_petProcess = PetProcess::Make();        // w_PetProcess.cpp:74

        // CMultiLanguage, criada em Winmain.cpp:1616 logo apos ler o config.ini.
        //
        // O construtor e que registra o singleton (ms_Singleton = this), e o resto
        // do codigo o alcanca pela macro g_pMultiLanguage. Sem esta linha o
        // ponteiro fica nulo, e o CONSTRUTOR de CNewUIChatInputBox o dereferencia
        // na propria lista de inicializacao:
        //
        //   MAX_CHAT_SIZE_UTF16(MAX_CHAT_SIZE / g_pMultiLanguage->GetNumByteForOneCharUTF8())
        //
        // o que dava SIGSEGV lendo o endereco 0x8 ao montar a interface principal.
        // Nao ha objeto dono aqui: em Winmain o ponteiro `pMultiLanguage` existe
        // so para o SAFE_DELETE no encerramento, e nada mais o consulta.
        new CMultiLanguage(g_strSelectedML);

        // Endereco do servidor e demais campos do arquivo principal. Depois do
        // CMultiLanguage porque nada aqui depende da ordem, e antes de qualquer
        // tentativa de conexao -- que so acontece na cena de login.
        CarregarMainInfo();

        // Os tres objetos que o Winmain cria em 1786-1788. Eu havia assumido que os
        // usos testavam o ponteiro; NAO testam.
        //
        // `MoveMainScene` chama `g_pUIMapName->Init()` e `ShowMapName()` direto. Com o
        // ponteiro nulo isso nao estoura no wasm: escreve em enderecos baixos e segue.
        // O efeito aparecia longe da causa -- `m_mapImgPath[World]` sobre um std::map
        // invalido devolvia string VAZIA, e `LoadBitmap("")` cai no caminho de
        // integridade de assets, que faz ExitProcess. Ou seja: o cliente ENTRAVA no
        // mapa, desenhava tudo, e se encerrava sozinho 2 segundos depois.
        //
        // Aqui, e nao na inicializacao estatica, porque o construtor de CUIMapName monta
        // os caminhos das imagens com `g_strSelectedML` -- que precisa estar definido.
        if (g_pChatRoomSocketList == NULL) g_pChatRoomSocketList = new CChatRoomSocketList;
        if (g_pUIManager == NULL)          g_pUIManager          = new CUIManager;
        if (g_pUIMapName == NULL)          g_pUIMapName          = new CUIMapName;

        // Chaves do SimpleModulus, carregadas em Winmain.cpp:1604-1605.
        //
        // Sao as do canal cifrado C1/0xF1, que e por onde o LOGIN passa. Sem elas o
        // modulus fica todo zerado e CSimpleModulus::Encrypt divide por zero: o
        // cliente abortava com "remainder by zero" no instante em que o botao Connect
        // era clicado -- depois de ja ter escrito "Try to Login" no log, o que fazia
        // parecer problema de rede ou de senha.
        if (!g_SimpleModulusCS.LoadEncryptionKey((char*)"Data\\Enc1.dat") ||
            !g_SimpleModulusSC.LoadDecryptionKey((char*)"Data\\Dec2.dat"))
            fprintf(stderr, "[Platform] chaves SimpleModulus nao carregadas "
                            "(Data/Enc1.dat, Data/Dec2.dat)\n");
    }
}

#endif  // !_WIN32
