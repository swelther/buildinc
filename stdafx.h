// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WINVER 0x0600						// Change this to the appropriate value to target other versions of Windows.
#define _WIN32_WINNT 0x0600			// Change this to the appropriate value to target other versions of Windows.
#define _WIN32_WINDOWS 0x0600		// Change this to the appropriate value to target Windows Me or later.
#define _WIN32_IE 0x0600				// Change this to the appropriate value to target other versions of IE.

#include <afx.h>

#include <iostream>
#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <atlbase.h>
#include <atlstr.h>
#include <atlcomtime.h>

#define BI_EXTENSION1				_T("*.rc")
#define BI_EXTENSION2				_T("*.rc2")
#define BI_EXTENSION3				_T("*.*");
#define BI_EXTENSION4				_T("*.exe");
#define BI_EXTENSION5				_T("*.dll");
#define BI_EXTENSION6				_T("*.ocx");

#define BI_FILEVERSIONT			_T("VALUE \"FileVersion\"")
#define BI_FILEVERSION			_T("FILEVERSION")

#define BI_PRODUCTVERSIONT	_T("VALUE \"ProductVersion\"")
#define BI_PRODUCTVERSION		_T("PRODUCTVERSION")

#include "ModulVer.h"
