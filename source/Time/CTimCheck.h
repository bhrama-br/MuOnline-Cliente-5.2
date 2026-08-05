#pragma once

#include <VECTOR>
#include "Singleton.h"

struct TimeCheck
{
	int		iIndex;			// ½Ã°£ °ü·Ã ¹øÈ£
	int		iBackupTime;	// ½Ã°£ °ü·Ã ¹é¾÷
	bool	bTimeCheck;		// ½Ã°£ °ü·Ã Ã¼Å©	
};

class CTimeCheck : public Singleton <CTimeCheck>
{
public:
	std::vector<TimeCheck> stl_Time;
	std::vector<TimeCheck>::iterator stl_Time_I;

	CTimeCheck();
	virtual ~CTimeCheck();

	int	 CheckIndex(int index);
	bool GetTimeCheck(int index, int DelayTime);
	void DeleteTimeIndex(int index);
};

#define g_Time CTimeCheck::GetSingleton()
