// BuildInc.cpp : Increments file versions in Windows executables.
//

#include "stdafx.h"

using namespace std;

BOOL g_bNamePrinted = FALSE;
int g_nFileSize = 0;

//-----------------------------------------------------------------------------
void PrintName()
{
	if (!g_bNamePrinted)
	{
		g_bNamePrinted = TRUE;

		char sFilename[MAX_PATH];
		CModuleVersion moduleVersion;

		::GetModuleFileName(NULL, sFilename, MAX_PATH);

		moduleVersion.GetFileVersionInfoA(sFilename);

		cout << "BuildInc V" << moduleVersion.GetValue("ProductVersion");

		#ifdef _WIN64
		cout << " x64 Edition";
		#else
		//cout << " x86";
		#endif

		#ifdef _DEBUG
		cout << " DEBUG ";
		#endif
		cout << endl;

		cout << "Copyright (C) Sebastian Welther 2007-2008." << endl;
	}
}

//-----------------------------------------------------------------------------
void PrintHelpText()
{
	PrintName();
	
	// Print help text
	cout << endl;
	
	cout << "BuildInc [/V {1|2|3}] /D directory /A xyz{$D|$T|$E} [/VERBOSE]" << endl << endl;
	
	cout << "/V\t\t1: Increments only FileVersion (Default)." << endl;
	cout << "  \t\t2: Increments only ProductVersion." << endl;
	cout << "  \t\t3: Increments FileVersion and ProductVersion." << endl;

	cout << "/A\t\tAdds string in brackets behind FileVersion (text)" << endl;
	cout << "\t\tand removes the old string." << endl;
	cout << "  \t\t$D: Adds current date in local format." << endl;
	cout << "  \t\t$T: Adds current time in local format." << endl;
	cout << "  \t\t$E: Adds content of environment `buildinc`." << endl;

	cout << "/D\t\tDirectory to use. Increments version information in all" << endl;
	cout << "\t\t.rc and .rc2 files, depending on the /V settings." << endl;

	cout << "/VERBOSE\tShows program and progress information." << endl;
	cout << "\t\tWithout only errors are reported." << endl;
}

//-----------------------------------------------------------------------------
int GetDezimalPlace(int nNumber)
{
	CString sString;
	
	sString.Format("%i", nNumber);
	return sString.GetLength();
}

//-----------------------------------------------------------------------------
CString GetString(int nNumber)
{
	CString sString;
	
	sString.Format("%i", nNumber);
	return sString;
}

//-----------------------------------------------------------------------------
BOOL UpdateVersion(CString *psFileData, int nStart, int nLength, char cDelim, LPCSTR lpcszAddParameters = NULL)
{
	// FileVersionText
	int nFirst=0, nLast=0, i=nStart+nLength;
	char cDelim2;

	if (cDelim == '.')
		cDelim2 = '\"';
	else
		cDelim2 = '\r';

	// Get first '"'.
	if (cDelim == '.')
		while (psFileData->GetAt(i) != cDelim2 && i<psFileData->GetLength())
		{
			char d = psFileData->GetAt(i);
			i++;
		}
	else
		nFirst = nStart;

	nFirst = i;
	i++;

	// Get last '"'.
	while (psFileData->GetAt(i) != cDelim2 && i<psFileData->GetLength())
	{
		i++;
		
		// Check for MSVC 6
		if (psFileData->GetAt(i-1) == '\\')
		{
			// This .rc file is from MSVC6, ignore the \0 before the delimiter
			i--;
			break;
		}
	}
	
	nLast = i+1;

	i = nFirst+1;
	int nPoints = 0;
	
//	char c;
	
	// Get the first, second and third point "."
	for (; nPoints<3; i++)
	{
//		c = psFileData->GetAt(i);
//		cout << c << endl;
	
		if (psFileData->GetAt(i) == cDelim)
			nPoints++;
	}

	// We got the version
	int nVersion = atoi(psFileData->Mid(i, nLast-i));

	// Delete old value		
	psFileData->Delete(i, GetDezimalPlace(nVersion));
	
	// Insert new value
	psFileData->Insert(i, GetString(++nVersion));

	int nFileSizeChange = (GetDezimalPlace(nVersion) - GetDezimalPlace(nVersion-1));
	g_nFileSize += nFileSizeChange;

	// Important if we add a carry in the number text
	nLast += nFileSizeChange;

	// Insert (text)
	if (lpcszAddParameters != NULL)
	{
		int nFirstBracket = -1;
		int nLastBracket = -1;
	
		CString sTest;

		sTest = psFileData->Mid(nFirst, nLast-nFirst+nFileSizeChange);

		// Are there some ( or ) ?
		for (int i=nFirst; i<=nLast+nFileSizeChange; i++)
		{
			if (psFileData->GetAt(i) == '(')
				nFirstBracket = i;

			if (psFileData->GetAt(i) == ')')
				nLastBracket = i;
		}

		if (nFirstBracket != -1 && nLastBracket != -1)
		{
			// Remove space before bracket if exist
			if (psFileData->GetAt(nFirstBracket-1) == ' ')
				nFirstBracket--;

			// Remove text between brackets
			psFileData->Delete(nFirstBracket, nLastBracket-nFirstBracket+1);
			g_nFileSize -= nLastBracket-nFirstBracket+1;
			nLast -= nLastBracket-nFirstBracket+1;
		}

		// Make new text
		CString sNewText, sAddParameters = lpcszAddParameters;
		CString sTemp;
		COleDateTime actDate = COleDateTime::GetCurrentTime();
		
		int nDate = sAddParameters.Find("$D");
		if (nDate != -1)
		{
			// Get date
			sAddParameters.Delete(nDate, 2);
//			sTemp = actDate.Format(LOCALE_NOUSEROVERRIDE | VAR_DATEVALUEONLY);
			sTemp = actDate.Format("%d%m%y");
			sTemp.Remove('.');
			sTemp.Remove(',');
			sAddParameters.Insert(nDate, sTemp);
		}

		int nTime = sAddParameters.Find("$T");
		if (nTime != -1)
		{
			// Get time
			sAddParameters.Delete(nTime, 2);
//			sTemp = actDate.Format(LOCALE_NOUSEROVERRIDE | VAR_TIMEVALUEONLY);
			sTemp = actDate.Format("%H%M");
			sTemp.Remove('.');
			sTemp.Remove(',');
			sTemp.Remove(':');
			sAddParameters.Insert(nTime, sTemp);
		}

		int nEnv = sAddParameters.Find("$E");
		if (nEnv != -1)
		{
			// Get environment text
			sAddParameters.Delete(nEnv, 2);
			sTemp.GetEnvironmentVariable("buildinc");
			sAddParameters.Insert(nEnv, sTemp);
		}
		
		// Insert text
		psFileData->Insert(nLast-1, " ("+sAddParameters+")");
		g_nFileSize += sAddParameters.GetLength()+3;
	}
	
	return TRUE;
}

//-----------------------------------------------------------------------------
BOOL ProcessFile(LPCSTR lpcszFile, int nIncType, LPCSTR lpcszAddParameters = NULL)
{
//	cout << "Processing file " << lpcszFile << endl;

	DWORD dwFileSize = 0;
	DWORD dwBytesRead = 0;
	LPSTR lpszData = NULL;
	int nFileVersionT = 0;
	int nFileVersion = 0;
	int nProductVersionT = 0;
	int nProductVersion = 0;
	CString sFileData;

	// Open file
	HANDLE hFile = ::CreateFile(lpcszFile, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		PrintName();
		cout << "CreateFile failed. GetLastError reports: " << ::GetLastError() << endl;
		return FALSE;
	}
	
	// Get file size
	dwFileSize = ::GetFileSize(hFile, NULL);
	if (dwFileSize == INVALID_FILE_SIZE)
	{
		PrintName();
		cout << "GetFileSize failed. GetLastError reports: " << ::GetLastError() << endl;
		return FALSE;
	}
	
	
//	cout << "FileSize: " << dwFileSize << endl;
	
//	::Sleep(20*1000);
	
	
	// Generate buffer
	lpszData = new char[dwFileSize];
	g_nFileSize = dwFileSize;

	// Read file
	if (::ReadFile(hFile, lpszData, dwFileSize, &dwBytesRead, NULL) == 0)
	{
		PrintName();
		cout << "ReadFile failed. GetLastError reports: " << ::GetLastError() << endl;
		return FALSE;
	}

	// Search for *VERSIONS
	sFileData = lpszData;
	delete lpszData;
	lpszData = NULL;

	nFileVersionT = sFileData.Find(BI_FILEVERSIONT);

	if (nFileVersionT > -1 && (nIncType == 1 || nIncType == 3))
		if (UpdateVersion(&sFileData, nFileVersionT, sizeof BI_FILEVERSIONT, '.', lpcszAddParameters) == FALSE)
			return FALSE;
	
	nFileVersion = sFileData.Find(BI_FILEVERSION);

	if (nFileVersion > -1 && (nIncType == 1 || nIncType == 3))
		if (UpdateVersion(&sFileData, nFileVersion, sizeof BI_FILEVERSION, ',') == FALSE)
			return FALSE;

	nProductVersionT = sFileData.Find(BI_PRODUCTVERSIONT);

	if (nProductVersionT > -1 && (nIncType == 2 || nIncType == 3))
		if (UpdateVersion(&sFileData, nProductVersionT, sizeof BI_PRODUCTVERSIONT, '.') == FALSE)
			return FALSE;

	nProductVersion = sFileData.Find(BI_PRODUCTVERSION);

	if (nProductVersion > -1 && (nIncType == 2 || nIncType == 3))
		if (UpdateVersion(&sFileData, nProductVersion, sizeof BI_PRODUCTVERSION, ',') == FALSE)
			return FALSE;

	// Write changes
	if (::SetFilePointer(hFile, 0, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		PrintName();
		cout << "SetFilePointer failed. GetLastError reports: " << ::GetLastError() << endl;
		return FALSE;
	}
	
	if (::WriteFile(hFile, sFileData, g_nFileSize, &dwBytesRead, NULL) == 0)
	{
		PrintName();
		cout << "WriteFile failed. GetLastError reports: " << ::GetLastError() << endl;
		return FALSE;
	}

	// Close file
	::CloseHandle(hFile);

	// Delete buffer
	delete lpszData;

	return TRUE;
}

//-----------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	BOOL bError = FALSE;
	BOOL bVerbose = TRUE;
	int nIncType = 1;
	CString sDirectory;
	CString sAddParameters;

	int nLastError = 0;
	char szPath[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = NULL;

	::memset(&FindFileData, NULL, sizeof FindFileData);

	// No arguments?
	if (argc == 1)
	{
		// Display help text
		PrintHelpText();

		return 0;
	}

	
	/*cout << "argc: " << argc << endl;

	for (int i=1; i<argc; i++)
	{
		cout << "i: " << i << " argv[" << i << "]: " << argv[i] << endl;
	}*/

//	cout << endl << "Waiting for debugger..." << endl;

//	::Sleep(20*1000);

	// Get arguments
	for (int i=1; i<argc; i++)
	{
		// Option /V
		if (strcmp(argv[i], "/V") == 0)
		{
			if (argv[i+1][0] > '0' && argv[i+1][0] < '4')
			{
				nIncType = argv[i+1][0]-'0';

				// Skip next value
				i++;
			}
			else
			{
				// Error using /V
				PrintName();
				cout << endl << "Wrong parameter with option /V." << endl;

				bError = TRUE;
			}
		}

		// Option /D
		if (strcmp(argv[i], "/D") == 0)
		{
			if (i+1 <= argc)
			{
				sDirectory = argv[i+1];

				// Skip next value
				i++;
			}
			else
			{
				// Error using /D
				PrintName();
				cout << endl << "Missing directory with option /D." << endl;

				bError = TRUE;
			}
		}

		// Option /A
		if (strcmp(argv[i], "/A") == 0)
		{
			if (i+1 <= argc)
			{
				sAddParameters = argv[i+1];

				// Skip next value
				i++;
			}
			else
			{
				// Error using /A
				PrintName();
				cout << endl << "Missing parameter with option /A." << endl;

				bError = TRUE;
			}
		}

		// Option /VERBOSE
		if (strcmp(argv[i], "/VERBOSE") == 0)
			bVerbose = FALSE;
	}

	if (!bVerbose)
		PrintName();

	// Missing /D?
	if (sDirectory.IsEmpty())
	{
		PrintName();
		cout << endl << "Missing option /D." << endl;

		bError = TRUE;
	}

	// Check directory
	if (!sDirectory.IsEmpty() && !::PathIsDirectory(sDirectory))
	{
		PrintName();
		cout << endl << "Invalid directory provided." << endl;

		cout << sDirectory;

		bError = TRUE;
	}

	// Command line errors?
	if (bError)
	{
		return -1;
	}
	
	// Make search string
	if (::PathCombine(szPath, sDirectory, BI_EXTENSION1) == NULL)
	{
		// Error
		PrintName();
		cout << "PathCombine failed. Exiting..." << endl;

		return -1;
	}
	
	//cout << szPath << endl;
	
	// Search for .rc*-files
	hFind = ::FindFirstFile(szPath, &FindFileData);
	nLastError = ::GetLastError();

	if (hFind == INVALID_HANDLE_VALUE && nLastError != 2)
	{
		// Error
		PrintName();
		cout << "FindFirstFile failed. GetLastError reports: " << nLastError << ". Exiting..." << endl;

		return -1;
	}
	else if (nLastError == 2)
	{
		// No file found, exit
		if (!bVerbose)
		{
			PrintName();
			cout << endl << "No .rc or rc2 file found. Exiting.";
		}
		
		return 1;
	}
	
	// Make file string
	if (::PathCombine(szPath, sDirectory, FindFileData.cFileName) == NULL)
	{
		// Error
		PrintName();
		cout << "PathCombine failed. Exiting..." << endl;

		return -1;
	}

	if (!bVerbose)
		cout << "Processing file " << szPath << endl;

	// Process first file
	ProcessFile(szPath, nIncType, !sAddParameters.IsEmpty() ? sAddParameters : NULL);

	// Proceed files
	while (::FindNextFile(hFind, &FindFileData))
	{
		// Make file string
		if (::PathCombine(szPath, sDirectory, FindFileData.cFileName) == NULL)
		{
			// Error
			PrintName();
			cout << "PathCombine failed. Exiting..." << endl;

			return -1;
		}

		if (!bVerbose)
			cout << "Processing file " << szPath << endl;

		// Get file and work on it.
		ProcessFile(szPath, nIncType);
	}

	if (::GetLastError() == ERROR_NO_MORE_FILES)
	{
		// No more files, exit.
		//cout << "No more files..." << endl;
		::FindClose(hFind);

		return 0;
	}	

	return 0;
}
