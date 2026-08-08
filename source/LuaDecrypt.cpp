// FileProtect.cpp: implementation of the CLuaDecrypt class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LuaDecrypt.h"
#include "Platform/LegacyFileAccess.h"

#include <stdlib.h>

CLuaDecrypt gFileProtectLua;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLuaDecrypt::CLuaDecrypt() // OK
{
	this->m_buff = 0;
	this->m_size = 0;
	memset(this->m_path, 0, sizeof(this->m_path));
}

CLuaDecrypt::~CLuaDecrypt() // OK
{

}

bool CLuaDecrypt::LoadFile(char* path) // OK
{
	// stdio em vez de CreateFile/ReadFile.
	//
	// Fora do Windows CreateFileA e um stub que SEMPRE devolve
	// INVALID_HANDLE_VALUE (escolha deliberada: falhar em vez de simular sucesso).
	// Com isso LoadFile falhava sempre, DecryptFile devolvia 0, ConvertMainFilePath
	// devolvia string VAZIA -- e luaL_loadbuffer aceita um trecho vazio sem erro.
	// Resultado: todo script Lua carregava "com sucesso" definindo nada, e o jogo
	// reclamava depois, longe da causa:
	//
	//   luacall_Generic_Call error running function 'LoadImages':
	//     'attempt to call a nil value'
	//
	// LegacyFileOpen ainda normaliza a barra invertida e aciona a busca de asset sob
	// demanda; no Windows e repasse direto para fopen, e stdio le o arquivo igual.
	// O modo e "rb": o original pedia GENERIC_READ|GENERIC_WRITE, mas nada aqui
	// escreve, e exigir permissao de escrita so faria falhar sem motivo.
	FILE* file = Platform::LegacyFileOpen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		return 0;
	}
	const long size = ftell(file);
	if (size < 0)
	{
		fclose(file);
		return 0;
	}
	rewind(file);

	// malloc, nao new[]: DecryptFile chama realloc neste ponteiro e, no caminho de
	// erro, free. Misturar new[] com realloc/free e comportamento indefinido -- ja
	// era assim antes, e como esta funcao esta sendo reescrita, o par certo fica
	// coerente com quem a usa (ver tambem DeleteTemporaryFile).
	if (this->m_buff != 0)
	{
		free(this->m_buff);
		this->m_buff = 0;
	}

	this->m_size = (DWORD)size;
	this->m_buff = (char*)malloc(this->m_size > 0 ? this->m_size : 1);
	if (this->m_buff == 0)
	{
		fclose(file);
		return 0;
	}

	if (this->m_size > 0 && fread(this->m_buff, 1, this->m_size, file) != this->m_size)
	{
		fclose(file);
		free(this->m_buff);
		this->m_buff = 0;
		this->m_size = 0;
		return 0;
	}

	fclose(file);
	return 1;
}
bool CLuaDecrypt::DecryptFile(char* path) // OK
{
	if (this->LoadFile(path) == 0)
	{
		return 0;
	}

	if (this->m_size < sizeof(gFileProtectLuaHeader))
	{
		return !gProtect->m_MainInfo.LuaCrypt;
	}

	if (memcmp(this->m_buff, gFileProtectLuaHeader, sizeof(gFileProtectLuaHeader)) != 0)
	{
		return !gProtect->m_MainInfo.LuaCrypt;
	}

	for (int n = 0; n < ((int)(this->m_size - sizeof(gFileProtectLuaHeader))); n++)
	{
		this->m_buff[sizeof(gFileProtectLuaHeader) + n] -= gFileProtectLuaXorTable[gProtect->m_MainInfo.m_PrivateCode[n % strlen(gProtect->m_MainInfo.m_PrivateCode)] % sizeof(gFileProtectLuaXorTable)];
		this->m_buff[sizeof(gFileProtectLuaHeader) + n] ^= gFileProtectLuaXorTable[n % sizeof(gFileProtectLuaXorTable)];
	}

	const size_t headerSize = sizeof(gFileProtectLuaHeader);
	const size_t decryptedSize = this->m_size - headerSize;
	std::memmove(this->m_buff, this->m_buff + headerSize, decryptedSize);

	char* resizedBuffer = static_cast<char*>(std::realloc(this->m_buff, decryptedSize));
	if (resizedBuffer == nullptr)
	{

		free(this->m_buff);
		this->m_buff = nullptr;
		this->m_size = 0;
		return 0;
	}

	this->m_buff = resizedBuffer;
	this->m_size = decryptedSize;

	return 1;
}

void CLuaDecrypt::DeleteTemporaryFile() // OK
{
	// free, nao delete[]: DecryptFile realoca este ponteiro com realloc, e liberar
	// com delete[] o que veio de realloc e comportamento indefinido. Agora LoadFile
	// aloca com malloc, entao o par esta coerente do inicio ao fim.
	free(this->m_buff);

	this->m_buff = 0;

	this->m_size = 0;

	memset(this->m_path, 0, sizeof(this->m_path));
}

std::string CLuaDecrypt::ConvertMainFilePath(char* path) // OK
{
	if (this->DecryptFile(path) == 0)
	{
		return "";
	}

	std::string result(this->m_buff, this->m_size);

	return result;
}