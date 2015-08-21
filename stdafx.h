// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WINVER 0x0400						// Change this to the appropriate value to target other versions of Windows.
#define _WIN32_WINNT 0x0400			// Change this to the appropriate value to target other versions of Windows.
#define _WIN32_WINDOWS 0x0400		// Change this to the appropriate value to target Windows Me or later.
#define _WIN32_IE 0x0600				// Change this to the appropriate value to target other versions of IE.

#include <iostream>
#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <atlbase.h>
#include <atlstr.h>
#include <atlcomtime.h>

#define BI_EXTENSION1				"*.rc"
#define BI_EXTENSION2				"*.rc2"

#define BI_FILEVERSIONT			"VALUE \"FileVersion\""
#define BI_FILEVERSION			"FILEVERSION"

#define BI_PRODUCTVERSIONT	"VALUE \"ProductVersion\""
#define BI_PRODUCTVERSION		"PRODUCTVERSION"

#include "ModulVer.h"
