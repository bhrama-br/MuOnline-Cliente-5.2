#ifndef _IEXPLORER_H_
#define _IEXPLORER_H_

#pragma once

#include "Platform/PlatformShell.h"

namespace leaf {
	/* Open URL for default web-browser    */
	/* This function return process handle */
	inline bool OpenExplorer(const std::string& url)
	{
		return Platform::OpenExternalUrl(url.c_str());
	}
}


#endif // _IEXPLORER_H_
