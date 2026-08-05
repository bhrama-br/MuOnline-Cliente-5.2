

#ifndef _LAUNCHERHELPER_H_
#define _LAUNCHERHELPER_H_

#pragma once

#pragma warning(disable : 4786)
#include <string>

#pragma pack(push, 1)
typedef struct __LAUNCHINFO {
	std::string ip;			//. ip address to connect
	unsigned long port;		//. port number to connect
} WZLAUNCHINFO, *LPWZLAUNCHINFO;
#pragma pack(pop)

bool wzRegisterConnectionKey();		//. Register connection key.
void wzUnregisterConnectionKey();	//. Unregister connection key
unsigned long wzGetConnectionKey();
/* Connection Key°¡ ¾ø´Ù¸é ½ÇÆÐÇÑ´Ù.(return 0xFFFFFFFF) */

bool wzPushLaunchInfo(const WZLAUNCHINFO& LaunchInfo);
/* Á¢¼ÓÅ°°¡ ¾ø°Å³ª Á¢¼ÓÅ°°¡ µî·ÏµÈÈÄ 5ÃÊ ÀÌ³»¿¡ ÇÔ¼ö°¡ È£ÃâµÇÁö ¸øÇßÀ»°æ¿ì ½ÇÆÐÇÑ´Ù. */
/* ÇÔ¼ö°¡ È£ÃâµÈ µÚ¿¡´Â Á¢¼ÓÅ°°¡ µî·ÏÇØÁ¦µÈ´Ù */

bool wzPopLaunchInfo(WZLAUNCHINFO& LaunchInfo);				
/* LaunchInfo°¡ ¾ø´Ù¸é ½ÇÆÐÇÑ´Ù. */
/* ÇÔ¼ö°¡ È£ÃâµÈ µÚ¿¡´Â Á¢¼ÓÁ¤º¸°¡ »èÁ¦µÈ´Ù */

/*
	// example

	//. Mu online launcher side
	WZLAUNCHINFO LaunchInfo;
	LaunchInfo.ip = "10.1.210.3";
	LaunchInfo.port = 44405;
	if(wzPushLaunchInfo(LaunchInfo)) {		//. encryption management
		//. success
		//. launching Mu update application
	}

	//. Mu online application side
	WZLAUNCHINFO LaunchInfo;
	if(wzPopLaunchInfo(LaunchInfo)) {
		//. success
		//. connect to LaunchInfo.ip:LaunchInfo.port
	}
	else {
		//. failed.
		//. exit process
	}

*/

#endif // _LAUNCHERHELPER_H_