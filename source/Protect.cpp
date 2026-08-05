#include "stdafx.h"
#include "CCRC32.h"

CProtect* gProtect;

static BYTE bBuxCode[3] = { 0xF2, 0x95, 0x54 };

CProtect::CProtect() // OK
{
}

CProtect::~CProtect() // OK
{
}

bool CProtect::ReadMainFile(char* Path) // OK
{
	CCRC32 CRC32;

	if (CRC32.FileCRC(Path, &this->m_ClientFileCRC, 1024) == 0)
	{
		return 0;
	}

	HANDLE zFile = CreateFile(Path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, 0);

	if (zFile == INVALID_HANDLE_VALUE) { return 0; }

	if (GetFileSize(zFile, 0) != sizeof(MAIN_FILE_INFO))
	{
		CloseHandle(zFile);
		return 0;
	}

	DWORD OutSize = 0;

	if (ReadFile(zFile, &this->m_MainInfo, sizeof(MAIN_FILE_INFO), &OutSize, 0) == 0)
	{
		CloseHandle(zFile);
		return 0;
	}

	for (int n = 0; n < sizeof(MAIN_FILE_INFO); n++)
	{
		((BYTE*)&this->m_MainInfo)[n] ^= bBuxCode[n % 3];
	}

	CloseHandle(zFile);
	return 1;
}

bool CProtect::ReadCustomJewelConfig(char* name)
{
	HANDLE file = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, 0);

	if (file == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	if (GetFileSize(file, 0) != sizeof(LOAD_CUSTOM_JEWEL_INFO))
	{
		CloseHandle(file);
		return 0;
	}

	DWORD OutSize = 0;

	if (ReadFile(file, &this->m_CustomJewel, sizeof(LOAD_CUSTOM_JEWEL_INFO), &OutSize, 0) == 0)
	{
		CloseHandle(file);
		return 0;
	}

	for (int n = 0; n < sizeof(LOAD_CUSTOM_JEWEL_INFO); n++)
	{
		((BYTE*)&this->m_CustomJewel)[n] += (BYTE)(0xFC ^ HIBYTE(n));
		((BYTE*)&this->m_CustomJewel)[n] ^= (BYTE)(0xCA ^ LOBYTE(n));
	}

	CloseHandle(file);
	return 1;
}