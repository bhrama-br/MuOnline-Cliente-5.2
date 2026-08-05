//////////////////////////////////////////////////////////////////////////
//  
//  GuildCache.cpp
//  
//  ³»  ¿ë : ±æµåÁ¤º¸ Ä³½Ì
//  
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UIManager.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "GuildCache.h"

CGuildCache g_GuildCache;

CGuildCache::CGuildCache()
{
	Reset();
}

CGuildCache::~CGuildCache()
{
}

void CGuildCache::Reset()
{
	m_dwCurrIndex = 0;
	for( int i=0 ; i<MAX_MARKS; ++i )
		GuildMark[i].Key = -1;
}

int CGuildCache::GetGuildMarkIndex( int nGuildKey )
{
	for( int i=0 ; i<=(int)m_dwCurrIndex ; ++i )
	{
		if( GuildMark[i].Key == nGuildKey )
			return i;
	}
	return -1;
}

BOOL CGuildCache::IsExistGuildMark( int nGuildKey )
{
	if( GetGuildMarkIndex( nGuildKey ) == -1 )
		return FALSE;
	else
		return TRUE;
}

int CGuildCache::MakeGuildMarkIndex( int nGuildKey )
{
	if( m_dwCurrIndex >= MAX_MARKS )
	{
		assert( !"\xB1\xE6\xB5\xE5\xB8\xB6\xC5\xA9 \xB9\xF6\xC6\xDB\xC3\xCA\xB0\xFA" );
		return -1;
	}

	GuildMark[m_dwCurrIndex].Key = nGuildKey;
	return m_dwCurrIndex++;
}

int CGuildCache::SetGuildMark( int nGuildKey, BYTE* UnionName, BYTE* GuildName, BYTE* Mark )
{
	int nIndex = GetGuildMarkIndex( nGuildKey );
	if( nIndex != -1 )
	{
		memcpy( GuildMark[nIndex].UnionName, UnionName, 8 );
		GuildMark[nIndex].UnionName[8] = NULL;
		memcpy( GuildMark[nIndex].GuildName, GuildName, 8 );
		GuildMark[nIndex].GuildName[8] = NULL;
		for( int i=0 ; i<64 ; ++i )
		{
			if( i%2 == 0 )
				GuildMark[nIndex].Mark[i] = ( Mark[i/2]>>4 )&0x0f;
			else
				GuildMark[nIndex].Mark[i] = Mark[i/2]&0x0f;
		}
	}
	else
		assert( !"\xBE\xF8\xB4\xC2 \xB1\xE6\xB5\xE5\xB8\xB6\xC5\xA9" );

	return nIndex;
}
