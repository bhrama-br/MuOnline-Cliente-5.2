// Globais que o `Winmain` define e que os modulos do jogo esperam, compartilhados
// entre Web e Android.
//
// POR QUE COMPARTILHADO: estas definicoes viviam so em `web/WebGameGlobals.cpp`.
// No Android os mesmos simbolos eram atendidos pelo arquivo de stubs gerado, que
// da a cada um um bloco ZERADO -- e zero nem sempre e um valor valido:
//
//   g_strSelectedML = ""  -> todo caminho "Data\Local\<idioma>\..." virava
//                            "Data\Local\\...", o arquivo nao existia, e o
//                            cliente chamava SendMessage(WM_DESTROY). O sintoma
//                            era um SIGSEGV nos stubs de encerramento, tres
//                            passos depois da causa.
//   g_pTimer = NULL       -> Input.cpp o desreferencia sem testar.
//
// Ou seja: duplicar os globais por plataforma fez o Android divergir em silencio.
// Uma unica definicao evita a classe inteira do problema.
//
// Nao compila no Windows: la o dono e o `Winmain`.

#if !defined(_WIN32)

#include "../stdafx.h"
#include "../Utilities/Log/ErrorReport.h"
#include "../Protect.h"
#include "../UIWindows.h"
#include "../UIGuildInfo.h"
#include "../_define.h"
#include "../Time/Timer.h"
#include "../WindowTray.h"
#include "../UIMapName.h"
#include "../ZzzScene.h"          // szServerIpAddress, g_ServerPort
#include "../CrowdLod.h"          // secao [Render] do MainInfo.ini
#include "LegacyFileAccess.h"
#include "LegacySceneBringup.h"   // declara CarregarMainInfo
#include "../wsclientinline.h"    // SendCheck, usado por CheckHack

HWND g_hWnd = NULL;

// Subsistema de protecao (Themida/anti-hack), Windows-only.
//
// Ficava NULO, com a justificativa de que CGlobalBitmap::LoadImage so o
// desreferencia quando o arquivo tem cabecalho de protecao -- o que e verdade,
// mas era uma leitura incompleta do codigo: UpdateSceneState faz
//
//   sprintf(GrabFileName, gProtect->m_MainInfo.ScreenShotPath, ...)
//
// TODO QUADRO, fora do `if (GrabEnable)`. Com o ponteiro nulo isso e SIGSEGV
// lendo 0xb0f (o deslocamento de ScreenShotPath) assim que o laco de render
// comeca. Nao ha "so desreferencia se": ha um caminho incondicional.
//
// Protect.cpp traria o construtor, mas usa CreateFile/ReadFile e le um arquivo
// principal que este alvo nao tem -- e mesmo compilando, ReadMainFile falharia e
// ScreenShotPath ficaria vazio. Entao aqui vai armazenamento zerado com um
// caminho de captura valido, que e o unico campo que o caminho de render usa.
//
// O vptr fica nulo (o destrutor de CProtect e virtual). Aceitavel porque nada
// chama metodo virtual nem faz delete neste alvo: quem destruia era o Winmain.
namespace
{
    alignas(CProtect) unsigned char g_armazenamentoProtect[sizeof(CProtect)] = {0};

    CProtect* PrepararProtect()
    {
        CProtect* protecao = reinterpret_cast<CProtect*>(g_armazenamentoProtect);
        // 5 especificadores, na ordem que UpdateSceneState passa:
        // mes, dia, hora, minuto, contador. Cabe nos 50 bytes do campo.
        const char* padrao = "ScreenShots/Screen(%02d-%02d-%02d-%02d)-%03d.jpg";
        strncpy(protecao->m_MainInfo.ScreenShotPath, padrao,
                sizeof(protecao->m_MainInfo.ScreenShotPath) - 1);
        return protecao;
    }
}
CProtect* gProtect = PrepararProtect();

namespace
{
    // Copia `valor` num campo de tamanho fixo, sempre terminando em '\0'.
    void CopiarCampo(char* destination, size_t capacidade, const std::string& value)
    {
        if (capacidade == 0) return;
        const size_t quantos = (value.size() < capacidade - 1) ? value.size() : capacidade - 1;
        memcpy(destination, value.c_str(), quantos);
        destination[quantos] = '\0';
    }

    // Corta espacos e tabulacoes das duas pontas. O MainInfo.ini usa tabulacoes
    // para alinhar os valores, entao sem isto o IP viria com tabulacao no meio.
    std::string Aparar(const std::string& texto)
    {
        size_t start = 0;
        while (start < texto.size() && (texto[start] == ' ' || texto[start] == '\t' ||
                                         texto[start] == '\r' || texto[start] == '\n'))
            ++start;
        size_t fim = texto.size();
        while (fim > start && (texto[fim - 1] == ' ' || texto[fim - 1] == '\t' ||
                               texto[fim - 1] == '\r' || texto[fim - 1] == '\n'))
            --fim;
        return texto.substr(start, fim - start);
    }
}

namespace Platform
{
    void CarregarMainInfo()
    {
        // Lista de personagens da Season 13 e o PADRAO deste projeto: e o que o
        // MainInfo.ini do servidor e o do Web dizem, e e o caminho que a UI deve usar.
        // Fixado aqui antes de ler o arquivo para valer TAMBEM quando o arquivo nao
        // existe ou nao traz a chave -- o `gProtect` do port aponta para memoria
        // zerada, e zero significaria cair na lista antiga sem ninguem pedir.
        // O arquivo continua podendo sobrescrever.
        gProtect->m_MainInfo.CharListS13 = 1;

        FILE* file = Platform::LegacyFileOpen("MainInfo.ini", "rb");
        if (file == NULL)
        {
            // Sem hardcode de endereco: trocar de servidor nao deve exigir
            // recompilar, e um IP embutido em codigo compartilhado seria pior do que
            // uma falha visivel aqui.
            fprintf(stderr, "[Platform] MainInfo.ini nao encontrado; "
                            "sem endereco de servidor (o login nao vai conectar)\n");
            return;
        }

        char line[512];
        while (fgets(line, sizeof(line), file) != NULL)
        {
            std::string texto = Aparar(line);
            if (texto.empty() || texto[0] == ';' || texto[0] == '[') continue;

            const size_t igual = texto.find('=');
            if (igual == std::string::npos) continue;

            const std::string key = Aparar(texto.substr(0, igual));
            const std::string value = Aparar(texto.substr(igual + 1));

            if      (key == "IpAddress")        CopiarCampo(gProtect->m_MainInfo.IpAddress, sizeof(gProtect->m_MainInfo.IpAddress), value);
            else if (key == "IpAddressPort")    gProtect->m_MainInfo.IpAddressPort = (WORD)atoi(value.c_str());
            else if (key == "ClientVersion")    CopiarCampo(gProtect->m_MainInfo.ClientVersion, sizeof(gProtect->m_MainInfo.ClientVersion), value);
            else if (key == "ClientSerial")     CopiarCampo(gProtect->m_MainInfo.ClientSerial, sizeof(gProtect->m_MainInfo.ClientSerial), value);
            else if (key == "WindowName")       CopiarCampo(gProtect->m_MainInfo.WindowName, sizeof(gProtect->m_MainInfo.WindowName), value);
            else if (key == "ScreenShotPath")   CopiarCampo(gProtect->m_MainInfo.ScreenShotPath, sizeof(gProtect->m_MainInfo.ScreenShotPath), value);
            else if (key == "CharListSeason13") gProtect->m_MainInfo.CharListS13 = atoi(value.c_str());
            else if (key == "OnlyCryptedLua")   gProtect->m_MainInfo.LuaCrypt = (BYTE)atoi(value.c_str());
            // O PrivateCode e usado como divisor em CLuaDecrypt
            // (`n % strlen(m_PrivateCode)`): vazio seria divisao por zero no caminho
            // de script cifrado.
            else if (key == "PrivateCode")      CopiarCampo(gProtect->m_MainInfo.m_PrivateCode, sizeof(gProtect->m_MainInfo.m_PrivateCode), value);
            // Chaves da secao [Render] (Crowd LOD). Este e o UNICO canal de
            // configuracao que Web e Android tem: nenhum dos dois recebe linha de
            // comando, e sem arquivo eles ficariam presos ao default compilado --
            // calibrar LOD por dispositivo exigiria recompilar.
            //
            // No PC este caminho nao existe: la o MainInfo vem de um struct binario
            // cifrado (CProtect::ReadMainFile, Data\Configs\Configs.xtm), nao de
            // texto. O canal do PC e a linha de comando.
            else if (CrowdLod::ApplyIniKey(key.c_str(), value.c_str())) { }
        }
        fclose(file);

        // As duas atribuicoes que o Winmain faz em 1097-1102. `Version` e `Serial`
        // NAO entram aqui: WSclient.cpp:129-130 ja os define com exatamente os
        // valores deste arquivo, entao repetir a derivacao so criaria duas fontes
        // para a mesma verdade.
        if (gProtect->m_MainInfo.IpAddress[0] != '\0')
        {
            if (szServerIpAddress == NULL) szServerIpAddress = new char[32];
            memset(szServerIpAddress, 0, 32);
            memcpy(szServerIpAddress, gProtect->m_MainInfo.IpAddress,
                   sizeof(gProtect->m_MainInfo.IpAddress));
        }
        if (gProtect->m_MainInfo.IpAddressPort != 0)
            g_ServerPort = gProtect->m_MainInfo.IpAddressPort;

        fprintf(stderr, "[Platform] MainInfo: servidor %s:%d, versao %s, LuaCrypt=%d\n",
                gProtect->m_MainInfo.IpAddress, (int)g_ServerPort,
                gProtect->m_MainInfo.ClientVersion, (int)gProtect->m_MainInfo.LuaCrypt);
    }
}

// Leitura de opcao de linha de comando ("/xValor"), definida em Winmain.cpp:1238.
//
// Como o Winmain nao compila fora do Windows, o simbolo ficava ausente. No Web isso
// nao da erro de link (funcoes ausentes sao toleradas) e sim um abort em execucao,
// no momento em que alguem a chama -- ProtocolSend.cpp e WSclient.cpp a declaram.
//
// Copia fiel do original -- o laco dele termina em todos os casos: quando o strchr
// nao acha nada lpFound vira NULL e a condicao sai; quando acha '/' com outra letra,
// o strchr seguinte parte de lpFound+1 e progride.
//
// A unica diferenca e uma guarda para lpszCommandLine nulo, que aqui pode acontecer:
// no Windows quem chama passa o GetCommandLine, e neste alvo nao ha linha de comando.
BOOL Util_CheckOption(char* lpszCommandLine, unsigned char cOption, char* lpszString)
{
    if (lpszCommandLine == NULL) return FALSE;

    unsigned char cComp[2];
    cComp[0] = cOption;
    cComp[1] = cOption;
    if (islower((int)cOption))      cComp[1] = (unsigned char)toupper((int)cOption);
    else if (isupper((int)cOption)) cComp[1] = (unsigned char)tolower((int)cOption);

    unsigned char* lpFound = (unsigned char*)lpszCommandLine;
    while (lpFound != NULL)
    {
        lpFound = (unsigned char*)strchr((char*)(lpFound + 1), (int)'/');
        if (lpFound != NULL && (*(lpFound + 1) == cComp[0] || *(lpFound + 1) == cComp[1]))
        {
            if (lpszString != NULL)
            {
                int nCount = 0;
                for (unsigned char* lpSeek = lpFound + 2; *lpSeek != ' ' && *lpSeek != '\0'; ++lpSeek)
                    ++nCount;
                memcpy(lpszString, lpFound + 2, (size_t)nCount);
                lpszString[nCount] = '\0';
            }
            return TRUE;
        }
    }
    return FALSE;
}

bool ashies = false;          // efeito de cinzas do clima
int  weather = 0;             // indice de clima atual
// Modo de entrada de texto. 1 e o valor que Winmain.cpp fixa no Windows
// (Winmain.cpp:1190, incondicional -- nem vem do registro), e e o modo em que a UI
// usa CUITextInputBox.
//
// Estava 0 aqui, e isso deixava os campos de Conta e Senha do login INVISIVEIS de
// um jeito que nao parecia relacionado: CLoginWin::SetPosition so posiciona as
// caixas quando g_iChatInputType == 1, entao com 0 elas continuavam desenhando na
// origem da tela em vez de dentro da janela de login.
int  g_iChatInputType = 1;

// Fontes. Preenchidas por gCreateFont.SetFont, chamado de
// Platform/LegacySceneBringup.cpp quando a resolucao e aplicada.
HFONT g_hFixFont   = NULL;
HFONT g_hFont      = NULL;
HFONT g_hFontBold  = NULL;
HFONT g_hFontBig   = NULL;

// Instancia do modulo Win32: usada para recursos e classe de janela, nenhuma das
// duas existe aqui. Os pontos que a consomem tratam handle nulo.
HINSTANCE g_hInst = NULL;

// Idioma dos dados de Data/Local, nas duas formas que o cliente usa. No PC vem do
// registro ("LangSelection"); o Winmain cai em "Eng" quando a chave nao existe, e
// esse e o padrao aqui. String vazia faz DEZENAS de arquivos falharem de uma vez.
char        g_aszMLSelection[MAX_LANGUAGE_NAME_LENGTH] = { 'E', 'n', 'g', '\0' };
std::string g_strSelectedML = "Eng";

// Indice de resolucao do lancador. A resolucao real vem da area de desenho (ver
// LegacySceneBringup.cpp), entao aqui so precisa ser um valor valido.
int m_Resolution = 0;

// Objetos que o Winmain cria junto da janela; os usos testam o ponteiro.
CChatRoomSocketList* g_pChatRoomSocketList = NULL;
CUIManager*          g_pUIManager          = NULL;
CUIMapName*          g_pUIMapName          = NULL;

char m_ID[11] = {0};          // ID de login digitado (MAX_ID_SIZE + 1)
char m_ExeVersion[11] = {0};  // versao enviada ao servidor no login

// Tabela de angulos usada por efeitos (ZzzObject.cpp indexa `%100`).
// Platform/LegacyGlobalAllocations.cpp a preenche.
int RandomTable[100] = {0};

// Relogio de efeitos. O Winmain o avanca a cada quadro; enquanto o laco de quadro
// nao fizer o mesmo, os efeitos ficam parados -- visivel, nao silencioso.
float Time_Effect = 0.f;

int m_CameraOnOff = 0;        // camera livre de depuracao, desligada

// Contador de alta resolucao. Input.cpp o desreferencia SEM testar nulo, entao
// precisa existir de fato. O Winmain tambem o cria na inicializacao estatica.
CTimer* g_pTimer = new CTimer();

// `Destroy` e uma FLAG, nao uma funcao, apesar do nome: o laco do Winmain a
// consulta para saber se deve encerrar.
bool Destroy = false;

HDC  g_hDC = NULL;            // contexto GDI; nao existe aqui
bool g_bEnterPressed = false; // estado do Enter, para o chat
int  g_iNoMouseTime = 0;      // segundos sem mouse; >31 fecha a janela no PC

// Onde o botao esquerdo foi solto (Winmain.cpp:551). O window proc usa isto para
// caducar o `MouseLButtonPop` quando o cursor sai do lugar do clique; a traducao de
// mouse em AtualizarMouseLegado faz o mesmo.
int  g_iMousePopPosition_x = 0;
int  g_iMousePopPosition_y = 0;

// Icone de bandeja do Windows. A classe so guarda estado, entao construi-la e
// inofensivo e evita portar WindowTray.cpp, que e Shell_NotifyIcon puro.
TrayMode gTrayMode;

// Minimizar para a bandeja com F12. Nao existe bandeja nem janela para esconder aqui,
// entao e um no-op.
//
// So passou a ser exigida pelo link quando GetAsyncKeyState virou funcao de verdade:
// `ZzzScene.cpp:1178` chama isto sob `GetKeyState(VK_F12) & 0x8000`, e antes essa
// condicao era impossivel. E um bom exemplo de como um stub silencioso esconde a
// dependencia seguinte.
void TrayMode::SwitchState() {}

// As caixas de texto sao criadas pelo Winmain junto da janela.
class CUITextInputBox;
CUITextInputBox* g_pSinglePasswdInputBox = NULL;
CUITextInputBox* g_pSingleTextInputBox   = NULL;

// Verificacao periodica de presenca, definida em Winmain.cpp:213.
//
// O nome engana: nao inspeciona nada localmente. Com NEW_PROTOCOL_SYSTEM desligado
// (que e o caso deste cliente, Defined_Global.h:6) o corpo e um unico SendCheck() --
// o pacote 0xC1/0x0E que diz ao servidor de jogo que o cliente esta vivo.
//
// TranslateProtocol o chama ao tratar a resposta do login. Como Winmain nao compila
// aqui, o simbolo ficava ausente: o link so avisava (ERROR_ON_UNDEFINED_SYMBOLS=0) e
// o cliente abortava em pleno voo com "missing function: _Z9CheckHackv" no primeiro
// pacote recebido depois de enviar a conta e a senha.
void CheckHack(void)
{
    SendCheck();
}

// Soma de verificacao do arquivo do anti-cheat, pedida pelo servidor (WSclient.cpp
// 11763) e definida em Winmain.cpp:338-400. Mesmo algoritmo, byte a byte.
//
// Nesta instalacao o arquivo Data/Local/Gameguard.csr NAO existe, e nesse caso o
// original devolve 0 -- ou seja, o PC ja responde zero hoje. A funcao e portada
// inteira mesmo assim para o resultado continuar igual se o arquivo aparecer.
namespace
{
    WORD DecryptChecksumKey(WORD source)
    {
        const WORD acumulado = (WORD)(source ^ 0xB479);
        return (WORD)(((acumulado >> 10) << 4) | (acumulado & 0xF));
    }

    DWORD GenerateChecksum(const BYTE* buffer, DWORD size, WORD key)
    {
        const DWORD dwKey = (DWORD)key;
        DWORD resultado = dwKey << 9;
        if (size < 4) return resultado;
        for (DWORD lidos = 0; lidos <= size - 4; lidos += 4)
        {
            DWORD value;
            memcpy(&value, buffer + lidos, sizeof(DWORD));

            switch ((lidos / 4 + key) % 3)
            {
            case 0: resultado ^= value; break;
            case 1: resultado += value; break;
            case 2: resultado <<= (value % 11); resultado ^= value; break;
            }

            // Sempre verdadeiro, ja que o passo e 4; preservado como no original.
            if (0 == (lidos % 4))
                resultado ^= ((dwKey + resultado) >> ((lidos / 4) % 16 + 3));
        }
        return resultado;
    }
}

DWORD GetCheckSum(WORD wKey)
{
    wKey = DecryptChecksumKey(wKey);

    FILE* file = Platform::LegacyFileOpen("data\\local\\Gameguard.csr", "rb");
    if (file == NULL) return 0;

    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0) { fclose(file); return 0; }

    BYTE* conteudo = new BYTE[(size_t)size];
    const size_t lidos = fread(conteudo, 1, (size_t)size, file);
    fclose(file);

    const DWORD sum = GenerateChecksum(conteudo, (DWORD)lidos, wKey);
    delete [] conteudo;
    return sum;
}

// TODO(Platform): pertencem ao Winmain / camada de janela. Nao ha janela para
// destruir nem processo para encerrar.
void KillGLWindow() {}
void DestroyWindow() {}
void CloseMainExe()  {}

#endif  // !_WIN32
