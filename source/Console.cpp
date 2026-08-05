#include "Stdafx.h"
#include <stdio.h>
#include "Console.h"
#ifdef _WIN32
// So o ramo Windows precisa: de procesos.h vem apenas _endthread, usado por
// Console__Start. O cabecalho e cheio de __declspec e anotacoes SAL, que o clang
// recusa sem -fms-extensions.
#include "procesos.h"
#else
#include <time.h>
#endif

#ifdef CONSOLE

DOSConsole Console;

void DOSConsole::Write(bool WriteToFile, const char* Format, ...)
{
	char Message[3072];
	va_list pArguments;
	va_start(pArguments, Format);
	// vsnprintf no lugar de vsprintf: Format vem de Lua.cpp, com texto de script,
	// e o buffer tem tamanho fixo.
	vsnprintf(Message, sizeof(Message), Format, pArguments);
	va_end(pArguments);

#ifdef _WIN32
	SYSTEMTIME t;
	GetLocalTime(&t);

	DWORD dwBytesWritten;
	HANDLE Handle = GetStdHandle(STD_OUTPUT_HANDLE);
	char currdate[11] = { 0 };
	sprintf_s(currdate, "(%02d:%02d:%02d)", t.wHour, t.wMinute, t.wSecond);
	char outputmsg[3072];
	wsprintf(outputmsg, "%s %s\n", currdate, Message);
	WriteFile(Handle, outputmsg, strlen(outputmsg), &dwBytesWritten, NULL);
#else
	// Fora do Windows nao ha console alocado: a saida vai para stderr, que o
	// navegador mostra no console da pagina e o Android encaminha ao logcat.
	// Mesmo formato de horario, para o log continuar comparavel com o do PC.
	time_t agora = time(NULL);
	struct tm local;
	localtime_r(&agora, &local);
	fprintf(stderr, "(%02d:%02d:%02d) %s\n",
	        local.tm_hour, local.tm_min, local.tm_sec, Message);
#endif
}


// A partir daqui e console de depuracao do Windows: janela alocada com
// AllocConsole e uma thread lendo stdin. Nao ha equivalente no navegador nem no
// Android, e o console nao participa do jogo em si -- so da entrada de comandos
// de depuracao. As versoes nao-Windows sao inertes, e Write (acima) segue
// funcionando, que e a parte de fato usada por Lua.cpp e ZzzScene.cpp.
#ifdef _WIN32

int DOSConsole::StdIn(char* Buffer)
{
	char inText[1024];
	memset(inText, 0x00, 1024);
	memset(Buffer, 0x00, 1024);
	DWORD dwBytesWritten;
	ReadFile(Handle(TRUE), inText, 1024, &dwBytesWritten, NULL);
	strncpy(Buffer, inText, strlen(inText) - 2);
	return dwBytesWritten;
}

HANDLE DOSConsole::Handle(BOOL Input)
{
	if (Input == TRUE)
	{
		return GetStdHandle(STD_INPUT_HANDLE);
	}
	else
	{

		return GetStdHandle(STD_OUTPUT_HANDLE);
	}
}

void DOSConsole::ChatCore(char* Input)
{
	char Temp[1024] = { 0 };

}



//void __stdcall Console__Start(PVOID pVoid)
void Console__Start(void * lpParam)
{
	char Temp[1024];
	char sBuff[255] = { 0 };
	AllocConsole();
	SetConsoleTitleA("RoxGaming Debug Console");
	while (true)
	{
		Console.StdIn(Temp);
		Console.ChatCore(Temp);
		Sleep(100);
	}
	_endthread();
}

void DOSConsole::SetName()
{
	SetConsoleTitleA("RoxGaming Debug Console");
}

void DOSConsole::Init()
{
	DWORD hTh;
	_beginthread(Console__Start, 0, NULL);
	Console.SpyChat = 0;
}

#else   // !_WIN32

int DOSConsole::StdIn(char* Buffer)
{
	if (Buffer != NULL) Buffer[0] = '\0';
	return 0;
}

HANDLE DOSConsole::Handle(BOOL Input)
{
	(void)Input;
	return NULL;
}

void DOSConsole::ChatCore(char* Input)
{
	(void)Input;
}

void DOSConsole::SetName() {}

void DOSConsole::Init()
{
	Console.SpyChat = 0;
}

#endif  // _WIN32
#endif  // CONSOLE