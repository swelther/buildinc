// BuildInc.cpp : Increments file versions in Windows executables.
//

#include "stdafx.h"

#include "BuildInc.h"
#include "IniFile.h"

using namespace std;

BOOL g_bVerbose = FALSE;
BOOL g_bDebugInfo = FALSE;
BOOL g_bNamePrinted = FALSE;
int g_nFileSize = 0;

//-----------------------------------------------------------------------------
void PrintName()
{
	if (!g_bNamePrinted)
	{
		g_bNamePrinted = TRUE;

		TCHAR sFilename[MAX_PATH];
		CModuleVersion moduleVersion;

		// Get filename
		::GetModuleFileName(NULL, sFilename, MAX_PATH);

		moduleVersion.GetFileVersionInfo(sFilename);

		wcout << L"BuildInc V" << (LPCTSTR)moduleVersion.GetValue(_T("ProductVersion"));

		#ifdef _WIN64
		wcout << " x64 Edition";
		#else
		//wcout << " x86";
		#endif

		#ifdef _DEBUG
		wcout << " DEBUG ";
		#endif
		wcout << endl;

		wcout << "Copyright (C) Sebastian Welther 2007-2011." << endl;
	}
}

//-----------------------------------------------------------------------------
void PrintHelpText()
{
	PrintName();

	// Print help text
	wcout << endl;

	wcout << L"BuildInc [/V {1|2|3}] /D directory [/B [Binary] /I ini file] /A xyz{$D|$T|$E} [/VERBOSE]" << endl << endl;

	wcout << L"  /V\t\t1: Increments only FileVersion (Default)." << endl;
	wcout << L"  \t\t2: Increments only ProductVersion." << endl;
	wcout << L"  \t\t3: Increments FileVersion and ProductVersion." << endl;

	wcout << L"  /A\t\tAdds string in brackets behind FileVersion (text)" << endl;
	wcout << L"\t\tand removes the old string." << endl;
	wcout << L"  \t\t$D: Adds current date in local format." << endl;
	wcout << L"  \t\t$T: Adds current time in local format." << endl;
	wcout << L"  \t\t$E: Adds content of environment `buildinc`." << endl;

	wcout << L"  /D\t\tDirectory to use. Increments version information in all" << endl;
	wcout << L"\t\t.rc and .rc2 files, depending on the /V settings." << endl;

	wcout << L"  /B\t\tChanges file version directly in binary file." << endl;
	wcout << L"\t\tDoes not change any version in .rc and .rc2 files." << endl;
	wcout << L"\t\tMust be used with Option /I." << endl;

	wcout << L"  /I\t\tFile name of the ini file in which the file version is" << endl;
	wcout << L"\t\tsaved and incremented." << endl;
	wcout << L"\t\tMust be used with Option /B." << endl;

	wcout << L"  /VERBOSE\tShows program and progress information." << endl;
	wcout << L"\t\tWithout only errors are reported." << endl;
}

//-----------------------------------------------------------------------------
int GetDezimalPlace(int nNumber)
{
	CString sString;

	sString.Format(_T("%i"), nNumber);
	return sString.GetLength();
}

//-----------------------------------------------------------------------------
CString GetString(int nNumber)
{
	CString sString;

	sString.Format(_T("%i"), nNumber);
	return sString;
}

//-----------------------------------------------------------------------------
BOOL UpdateVersion(CString *psFileData, int nStart, int nLength, char cDelim, LPCTSTR lpcszAddParameters = NULL, BOOL bSearchForDelim2 = TRUE, BOOL bInc = TRUE)
{
	// FileVersionText
	int nFirst=0, nLast=0, i=nStart+nLength;
	char cDelim2;

	if (bSearchForDelim2 == TRUE)
	{
		if (cDelim == '.')
			cDelim2 = '\"';
		else
			cDelim2 = '\r';

		// Get first '"'.
		if (cDelim == '.')
			while (psFileData->GetAt(i) != cDelim2 && i<psFileData->GetLength())
			{
				TCHAR d = psFileData->GetAt(i);
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
				// This .rc-File is from MSVC6, ignore the \0 before the delimiter
				i--;
				break;
			}
		}

		nLast = i+1;
	}
	else
	{
		nFirst = 0;
		nLast = psFileData->GetLength()-1;
	}

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
	int nVersion = _ttoi(psFileData->Mid(i, nLast-i));
	int nFileSizeChange = 0;

	if (bInc == TRUE)
	{
		// Delete old value
		psFileData->Delete(i, GetDezimalPlace(nVersion));

		// Insert new value
		psFileData->Insert(i, GetString(++nVersion));

		nFileSizeChange = (GetDezimalPlace(nVersion) - GetDezimalPlace(nVersion-1));
		g_nFileSize += nFileSizeChange;

		// Important if we add a carry in the numbertext
		nLast += nFileSizeChange;
	}

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

		int nDate = sAddParameters.Find(_T("$D"));
		if (nDate != -1)
		{
			// Get date
			sAddParameters.Delete(nDate, 2);
//			sTemp = actDate.Format(LOCALE_NOUSEROVERRIDE | VAR_DATEVALUEONLY);
			sTemp = actDate.Format(_T("%d%m%y"));
			sTemp.Remove('.');
			sTemp.Remove(',');
			sAddParameters.Insert(nDate, sTemp);
		}

		int nTime = sAddParameters.Find(_T("$T"));
		if (nTime != -1)
		{
			// Get time
			sAddParameters.Delete(nTime, 2);
//			sTemp = actDate.Format(LOCALE_NOUSEROVERRIDE | VAR_TIMEVALUEONLY);
			sTemp = actDate.Format(_T("%H%M"));
			sTemp.Remove('.');
			sTemp.Remove(',');
			sTemp.Remove(':');
			sAddParameters.Insert(nTime, sTemp);
		}

		int nEnv = sAddParameters.Find(_T("$E"));
		if (nEnv != -1)
		{
			// Get environment text
			sAddParameters.Delete(nEnv, 2);
			sTemp.GetEnvironmentVariable(_T("buildinc"));
			sAddParameters.Insert(nEnv, sTemp);
		}

		// Insert text
		psFileData->Insert(nLast-1, _T(" (")+sAddParameters+_T(")"));
		g_nFileSize += sAddParameters.GetLength()+3;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
CString GetDosError(int nError)
{
	TCHAR szError[1025];
	_wcserror_s(szError, 1024, nError);

	return CString(szError);
}

//-----------------------------------------------------------------------------
BOOL ProcessFile(LPCTSTR lpcszFile, int nIncType, LPCTSTR lpcszAddParameters = NULL)
{
//	cout << "Processing file " << lpcszFile << endl;

	DWORD dwFileSize = 0;
	DWORD dwBytesRead = 0;
	//LPTSTR lpszData = NULL;
	int nFileVersionT = 0;
	int nFileVersion = 0;
	int nProductVersionT = 0;
	int nProductVersion = 0;
	int nFileStartPosition = 0;
	CString sFileData;

	// Open file
	HANDLE hFile = ::CreateFile(lpcszFile, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		PrintName();
		wcout << L"CreateFile failed. GetLastError reports: " << ::GetLastError() << endl;
		return FALSE;
	}

	// Get file size
	dwFileSize = ::GetFileSize(hFile, NULL);
	if (dwFileSize == INVALID_FILE_SIZE)
	{
		PrintName();
		wcout << L"GetFileSize failed. GetLastError reports: " << ::GetLastError() << endl;
		return FALSE;
	}

	// Close file handle
	::CloseHandle(hFile);

	// Open file
	FILE *pFile;
	if (_tfopen_s(&pFile, lpcszFile, _T("r+t, ccs=UNICODE")) != 0)
	{
		PrintName();
		wcout << L"fopen failed: " << errno << L" (" << (LPCTSTR)GetDosError(errno) << ")";
		return FALSE;
	}

	// Get start position
	nFileStartPosition = ftell(pFile);

	CStdioFile inputFile(pFile);
	CString sLine;

	// Read all data
	while (inputFile.ReadString(sLine) == TRUE)
	{
		sFileData += sLine;
		sFileData += _T("\n");
	}

	// Search for *VERSIONS
	nFileVersionT = sFileData.Find(BI_FILEVERSIONT);

	if (nFileVersionT > -1 && (nIncType == BI_INCFILEVERSION || nIncType == BI_INCFILEPRODVERSION))
		if (UpdateVersion(&sFileData, nFileVersionT, _tcslen(BI_FILEVERSIONT), '.', lpcszAddParameters) == FALSE)
			return FALSE;

	nFileVersion = sFileData.Find(BI_FILEVERSION);

	if (nFileVersion > -1 && (nIncType == BI_INCFILEVERSION || nIncType == BI_INCFILEPRODVERSION))
		if (UpdateVersion(&sFileData, nFileVersion, _tcslen(BI_FILEVERSION), ',') == FALSE)
			return FALSE;

	nProductVersionT = sFileData.Find(BI_PRODUCTVERSIONT);

	if (nProductVersionT > -1 && (nIncType == BI_INCPRODUCTVERSION || nIncType == BI_INCFILEPRODVERSION))
		if (UpdateVersion(&sFileData, nProductVersionT, _tcslen(BI_PRODUCTVERSIONT), '.') == FALSE)
			return FALSE;

	nProductVersion = sFileData.Find(BI_PRODUCTVERSION);

	if (nProductVersion > -1 && (nIncType == BI_INCPRODUCTVERSION || nIncType == BI_INCFILEPRODVERSION))
		if (UpdateVersion(&sFileData, nProductVersion, _tcslen(BI_PRODUCTVERSION), ',') == FALSE)
			return FALSE;

	// Seek to file begin and write changes
	inputFile.Seek(nFileStartPosition, CFile::begin);
	inputFile.WriteString(sFileData);
	inputFile.Close();

	return TRUE;
}

//-----------------------------------------------------------------------------
BOOL ProcessBinaryFile(LPCTSTR lpcszFile, int nIncType, BI_BinaryVersion *pVersion, LPCTSTR lpcszAddParameters = NULL)
{
	HANDLE hResource;
	WCHAR szLanguage[9];
	WCHAR szLanguageSmall[6];
	VS_FIXEDFILEINFO *pFixedFileInfo;

	BYTE *pVersionInfo = NULL;
	BYTE *pVersionInfoNew = NULL;
	DWORD dwDummyHandle;
	DWORD dwLength;
	int dwDiff=0;

	dwLength = ::GetFileVersionInfoSize(lpcszFile, &dwDummyHandle);
	if (dwLength <= 0)
	{
		wcout << L"GetFileVersionInfoSize failed with error " << (LPCTSTR)FormatLastError(::GetLastError()) << L" (" << ::GetLastError() << L")" << endl;
		return FALSE;
	}

	pVersionInfo = new BYTE[dwLength*2]; // allocate version info, extra size for byte insert
	pVersionInfoNew = new BYTE[dwLength*2];

	if (!::GetFileVersionInfo(lpcszFile, 0, dwLength, pVersionInfo))
	{
		wcout << L"GetFileVersionInfo failed with error " << (LPCTSTR)FormatLastError(::GetLastError()) << L" (" << ::GetLastError() << L")" << endl;
		delete[] pVersionInfo;
		delete[] pVersionInfoNew;
		return FALSE;
	}

	// Copy bytes
	memcpy(pVersionInfoNew, pVersionInfo, dwLength);

	WORD wLength, wValueLength, wType;
	WORD *pwVersionInfoLength = (WORD*)pVersionInfoNew;
	WCHAR *pszKey;
	BYTE *pStringFileInfo=NULL;

	memcpy(&wLength, pVersionInfo+(0*sizeof(WORD)), sizeof(WORD));
	memcpy(&wValueLength, pVersionInfo+(1*sizeof(WORD)), sizeof(WORD));
	memcpy(&wType, pVersionInfo+(2*sizeof(WORD)), sizeof(WORD));
	pszKey = (WCHAR*)(pVersionInfo+(3*sizeof(WORD)));
	UINT nPadding1, nPadding2, nPadding3, nPadding4, nPadding5, nPaddingFor;

	if (g_bDebugInfo == TRUE)
	{
		wcout << L"Length: " << wLength << endl;
		wcout << L"ValueLength: " << wValueLength << endl;
		wcout << L"Type: " << wType << endl;
		wcout << L"Key: " << pszKey << endl;
	}

	// Check pszKey
	if (_tcscmp(pszKey, _T("VS_VERSION_INFO")) != 0)
	{
		// No version info
		wcout << L"No VS_VERSION_INFO found." << endl;
		delete[] pVersionInfo;
		delete[] pVersionInfoNew;
		return FALSE;
	}

	// Binary version info?
	/*if (wType == 0)
	{
		// Binary version info
		wcout << L"Binary version info found" << endl;
		delete[] pVersionInfo;
		delete[] pVersionInfoNew;
		return FALSE;
	}*/

	// Padding starts after strlen(pszKey)
	nPadding1 = (_tcslen(pszKey)+1)*sizeof(WCHAR) + (4*sizeof(WORD));
	if (nPadding1%4 > 0)
		nPadding1 = nPadding1+(4-(nPadding1%4));

	if (wValueLength > 0)
	{
		pFixedFileInfo = (VS_FIXEDFILEINFO*)(pVersionInfoNew+nPadding1);

		// Check signature
		if (pFixedFileInfo->dwSignature == 0xFEEF04BD)
		{
			if (g_bDebugInfo == TRUE)
			{
				wcout << L"FileVersion: " << HIWORD(pFixedFileInfo->dwFileVersionMS) << L"." << LOWORD(pFixedFileInfo->dwFileVersionMS);
				wcout << L"." << HIWORD(pFixedFileInfo->dwFileVersionLS) << L"." << LOWORD(pFixedFileInfo->dwFileVersionLS) << endl;

				wcout << L"ProductVersion: " << HIWORD(pFixedFileInfo->dwProductVersionMS) << L"." << LOWORD(pFixedFileInfo->dwProductVersionMS);
				wcout << L"." << HIWORD(pFixedFileInfo->dwProductVersionLS) << L"." << LOWORD(pFixedFileInfo->dwProductVersionLS) << endl;
			}

			// Increment fileversion?
			if (nIncType == BI_INCFILEVERSION || nIncType == BI_INCFILEPRODVERSION)
			{
				pFixedFileInfo->dwFileVersionMS = MAKELONG(pVersion->nVersion2, pVersion->nVersion1);
				pFixedFileInfo->dwFileVersionLS = MAKELONG(pVersion->nVersion4, pVersion->nVersion3);
			}

			// Increment productversion
			if (nIncType == BI_INCPRODUCTVERSION || nIncType == BI_INCFILEPRODVERSION)
			{
				pFixedFileInfo->dwProductVersionMS = MAKELONG(pVersion->nVersion2, pVersion->nVersion1);
				pFixedFileInfo->dwProductVersionLS = MAKELONG(pVersion->nVersion4, pVersion->nVersion3);
			}
		}
		else
		{
			if (g_bVerbose == TRUE)
				wcout << L"No VS_FIXEDFILEINFO signature found." << endl;
		}
	}

	// Exists some StringFileInfo?
	nPadding2 = nPadding1 + wValueLength;
	if (nPadding2%4 > 0)
		nPadding2 = nPadding2 + (4-(nPadding2%4));
	pStringFileInfo = pVersionInfo+nPadding2;

	WORD wLengthString = (WORD)(*(pStringFileInfo+(0*sizeof(WORD))));
	WORD wValueLengthString = (WORD)(*(pStringFileInfo+(1*sizeof(WORD))));
	WORD wTypeString = (WORD)(*(pStringFileInfo+(2*sizeof(WORD))));
	WCHAR *pszStringKey = (WCHAR*)(pStringFileInfo+(3*sizeof(WORD)));

	WORD *pwStringFileInfoLength = (WORD*)(pVersionInfoNew+nPadding2);

	// StringFileInfo
	if (_tcscmp(pszStringKey, L"StringFileInfo") == 0 && wTypeString == 1)
	{
		nPadding3 = (_tcslen(pszStringKey)+1)*sizeof(WCHAR)+(3*sizeof(WORD));
		if (nPadding3%4 > 0)
			nPadding3 = nPadding3 + (4-(nPadding3%4));

		WORD wStringTableLength = (WORD)(*(pStringFileInfo+nPadding3));
		WORD wStringTableValueLength = (WORD)(*(pStringFileInfo+nPadding3+(1*sizeof(WORD))));
		WORD wStringTableType = (WORD)(*(pStringFileInfo+nPadding3+(2*sizeof(WORD))));
		WCHAR *pszStringTableKey = (WCHAR*)(pStringFileInfo+nPadding3+(3*sizeof(WORD)));

		WORD *pwStringTableLength = (WORD*)(pVersionInfoNew+nPadding2+nPadding3);

		memcpy(szLanguage, pszStringTableKey, 9*sizeof(TCHAR));
		memcpy(szLanguageSmall, szLanguage, 8);
		szLanguageSmall[4] = L'\0';



		//LANG_GERMAN
		//SUBLANG_GERMAN
		//LANG_NEUTRAL
		//szLanguage == 0407 04b0
		long lLang = wcstol(szLanguageSmall, L'\0', 16);
		WORD w1 = PRIMARYLANGID(lLang);
		WORD w2 = SUBLANGID(lLang);
		WORD w3 = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);



		// Next padding
		nPadding4 = nPadding3 + (_tcslen(szLanguage)+1)*sizeof(WCHAR) + (3*sizeof(WORD));
		if (nPadding4%4 > 0)
			nPadding4 = nPadding4 + (4-(nPadding4%4));

		nPaddingFor = nPadding4;

		for (;pStringFileInfo+(nPaddingFor+(3*sizeof(WORD)))<pVersionInfo+dwLength;)
		{
			WORD wStringLength = (WORD)(*(pStringFileInfo+nPaddingFor));
			WORD wStringValueLength = (WORD)(*(pStringFileInfo+nPaddingFor+(1*sizeof(WORD))));
			WORD wStringType = (WORD)(*(pStringFileInfo+nPaddingFor+(2*sizeof(WORD))));

			WORD *pwStringLength = (WORD*)((pVersionInfoNew+nPadding2+dwDiff+nPaddingFor+(0*sizeof(WORD))));
			WORD *pwStringValueLength = (WORD*)((pVersionInfoNew+nPadding2+dwDiff+nPaddingFor+(1*sizeof(WORD))));

			WCHAR *pszStringKey = (WCHAR*)(pStringFileInfo+nPaddingFor+(3*sizeof(WORD)));

			// Padding for value
			nPadding5 = nPaddingFor + (_tcslen(pszStringKey)+1)*sizeof(WCHAR) + (3*sizeof(WORD));
			if (nPadding5%4 > 0)
				nPadding5 = nPadding5 + (4-(nPadding5%4));

			// Value
			WCHAR *pszStringValue = (WCHAR*)(pStringFileInfo+nPadding5);

			if (wStringType == 1)
			{
				if (_tcscmp(pszStringKey, L"FileVersion") == 0 && (nIncType == BI_INCFILEVERSION || nIncType == BI_INCFILEPRODVERSION))
				{
					// Change FileVersion
					int nOldSize=0, nNewSize=0;
					CString sNew, sOriginal;
					sNew = pVersion->sFileVersion;//pszStringValue;
					sOriginal = pszStringValue;
					nOldSize = sOriginal.GetLength();

					// Add following empty chars
					/*sNew += L"  ";

					if (UpdateVersion(&sNew, 0, 0, '.', lpcszAddParameters, FALSE) == FALSE)
					{
						// UpdateVersion failed
						wcout << L"UpdateVersion failed." << endl;
						delete[] pVersionInfo;
						delete[] pVersionInfoNew;
						return FALSE;
					}

					// Delete last empty char
					sNew.Delete(sNew.GetLength()-2, 2);*/

					nNewSize = sNew.GetLength();

					DWORD dwOffset = (pStringFileInfo+nPadding5)-pVersionInfo;

					if (nOldSize == nNewSize)
					{
						// Same size, copy string
						memcpy(pVersionInfoNew+(dwOffset+dwDiff), (LPCTSTR)sNew, nOldSize*sizeof(TCHAR));
					}
					else // Size differs
					{
						// Change in backup
						memset(pVersionInfoNew+dwOffset+dwDiff, 0, dwLength-((pVersionInfoNew+dwOffset+dwDiff)-pVersionInfoNew));
						memcpy(pVersionInfoNew+dwOffset+dwDiff, (LPCTSTR)sNew, sNew.GetLength()*sizeof(TCHAR));

						// Change value length
						*pwStringValueLength += (nNewSize-nOldSize);
						*pwStringLength += (nNewSize-nOldSize)*sizeof(TCHAR);

						// Copy rest
						DWORD dwNewPadding = (pVersionInfoNew+(dwOffset+((sNew.GetLength()+1)*sizeof(TCHAR))))-pVersionInfoNew;
						AlignOn4Byte(&dwNewPadding);
						BYTE *pOffsetInNew = pVersionInfoNew+dwNewPadding;

						DWORD dwOriginalPadding = dwOffset+((sOriginal.GetLength()+1)*sizeof(TCHAR));
						AlignOn4Byte(&dwOriginalPadding);
						BYTE *pOffsetInOriginal = pVersionInfo+dwOriginalPadding;

						memcpy(pOffsetInNew, pOffsetInOriginal, dwLength-(pOffsetInOriginal-pVersionInfo));

						// Calc diff
						dwDiff += (dwNewPadding-dwOriginalPadding);
					}
				}
				else if (_tcscmp(pszStringKey, L"ProductVersion") == 0 && (nIncType == BI_INCPRODUCTVERSION || nIncType == BI_INCFILEPRODVERSION))
				{
					// Change ProductVersion
					int nOldSize=0, nNewSize=0;
					CString sNew, sOriginal;
					sNew = pVersion->sProductVersion; //pszStringValue;
					sOriginal = pszStringValue;
					nOldSize = sOriginal.GetLength();

					// Add following empty chars
					/*sNew += L"  ";

					if (UpdateVersion(&sNew, 0, 0, '.', NULL, FALSE) == FALSE)
					{
						// UpdateVersion failed
						wcout << L"UpdateVersion failed." << endl;
						delete[] pVersionInfo;
						delete[] pVersionInfoNew;
						return FALSE;
					}

					// Delete last empty char
					sNew.Delete(sNew.GetLength()-2, 2);*/
					nNewSize = sNew.GetLength();

					DWORD dwOffset = (pStringFileInfo+nPadding5)-pVersionInfo;

					if (nOldSize == nNewSize)
					{
						// Same size, copy string
						memcpy(pVersionInfoNew+(dwOffset+dwDiff), (LPCTSTR)sNew, nOldSize*sizeof(TCHAR));
					}
					else
					{
						// Size differs, change in backup
						memset(pVersionInfoNew+dwOffset+dwDiff, 0, dwLength-((pVersionInfoNew+dwOffset+dwDiff)-pVersionInfoNew));
						memcpy(pVersionInfoNew+dwOffset+dwDiff, (LPCTSTR)sNew, sNew.GetLength()*sizeof(TCHAR));

						// Change value length
						*pwStringValueLength += (nNewSize-nOldSize);
						*pwStringLength += (nNewSize-nOldSize)*sizeof(TCHAR);

						// Copy rest
						DWORD dwNewPadding = (pVersionInfoNew+(dwOffset+((sNew.GetLength()+1)*sizeof(TCHAR))))-pVersionInfoNew;
						AlignOn4Byte(&dwNewPadding);
						BYTE *pOffsetInNew = pVersionInfoNew+dwDiff+dwNewPadding;

						DWORD dwOriginalPadding = dwOffset+((sOriginal.GetLength()+1)*sizeof(TCHAR));
						AlignOn4Byte(&dwOriginalPadding);
						BYTE *pOffsetInOriginal = pVersionInfo+dwOriginalPadding;

						memcpy(pOffsetInNew, pOffsetInOriginal, dwLength-(pOffsetInOriginal-pVersionInfo));

						// Calc diff
						dwDiff += (dwNewPadding-dwOriginalPadding);
					}
				}

				if (g_bDebugInfo == TRUE)
					wcout << pszStringKey << L": " << pszStringValue << endl;
			}

			// Next pair
			nPaddingFor = nPadding5 + (_tcslen(pszStringValue)+1)*sizeof(WCHAR);
			if (nPaddingFor%4 > 0)
				nPaddingFor = nPaddingFor + (4-(nPaddingFor%4));
		}

		// Resize values
		*pwStringTableLength += (WORD)dwDiff;
		*pwStringFileInfoLength += (WORD)dwDiff;
		*pwVersionInfoLength += (WORD)dwDiff;
	}

	// Write the changes to binary
	hResource = ::BeginUpdateResource(lpcszFile, FALSE);
	if (hResource == NULL)
	{
		wcout << L"BeginUpdateResource failed with error " << (LPCTSTR)FormatLastError(::GetLastError()) << L" (" << ::GetLastError() << L")" << endl;
		delete[] pVersionInfo;
		delete[] pVersionInfoNew;
		return FALSE;
	}

	if (::UpdateResource(hResource, RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO), (WORD)wcstol(szLanguageSmall, L'\0', 16), pVersionInfoNew, dwLength+dwDiff) == FALSE)
	{
		wcout << L"UpdateResource failed with error " << (LPCTSTR)FormatLastError(::GetLastError()) << L" (" << ::GetLastError() << L")" << endl;
		delete[] pVersionInfo;
		delete[] pVersionInfoNew;
		return FALSE;
	}

	if (::EndUpdateResource(hResource, FALSE) == FALSE)
	{
		wcout << L"EndUpdateResource failed with error " << (LPCTSTR)FormatLastError(::GetLastError()) << L" (" << ::GetLastError() << L")" << endl;
		delete[] pVersionInfo;
		delete[] pVersionInfoNew;
		return FALSE;
	}

	delete[] pVersionInfo;
	delete[] pVersionInfoNew;
	return TRUE;
}

//-----------------------------------------------------------------------------
BOOL ProcessIniFile(LPCTSTR lpszIniFile, BI_BinaryVersion *pVersion, LPCTSTR lpcszAddParameters /*= NULL*/)
{
	CIniFile iniFile;
	CStringA sIniFile(lpszIniFile);

	// Get version from ini file
	std::string sValue;
	sValue = iniFile.GetValue(std::string("Version"), std::string("Buildinc"), std::string(sIniFile));
	if (sValue.empty() == true)
		return FALSE;

	// Tokenize
	int nPos=0;
	CStringA sTemp(sValue.c_str());

	pVersion->nVersion1 = atoi(sTemp.Tokenize(".", nPos));
	pVersion->nVersion2 = atoi(sTemp.Tokenize(".", nPos));
	pVersion->nVersion3 = atoi(sTemp.Tokenize(".", nPos));
	pVersion->nVersion4 = atoi(sTemp.Tokenize(".", nPos));
	pVersion->nVersion4++;

	pVersion->sFileVersion.Format(_T("%u.%u.%u.%u"), pVersion->nVersion1, pVersion->nVersion2, pVersion->nVersion3, pVersion->nVersion4);
	pVersion->sProductVersion = pVersion->sFileVersion;
	pVersion->sFileVersion += L"  ";

	if (UpdateVersion(&pVersion->sFileVersion, 0, 0, '.', lpcszAddParameters, FALSE, FALSE) == FALSE)
	{
		// UpdateVersion failed
		wcout << L"UpdateVersion failed." << endl;
		return FALSE;
	}

	// Delete last empty char
	pVersion->sFileVersion.Delete(pVersion->sFileVersion.GetLength()-2, 2);

	// Save changed version
	iniFile.SetValue(std::string("Version"), std::string(CStringA(pVersion->sProductVersion)), std::string("Buildinc"), std::string(sIniFile));

	return TRUE;
}

//-----------------------------------------------------------------------------
BOOL ProcessDirectory(LPCTSTR lpszDirectory, LPCTSTR lpszExtension, BOOL bVerbose, int nIncMode, int nIncType, CString sIniFile, CString sAddParameters, BI_BinaryVersion *pBinaryVersionInfo)
{
	TCHAR szPath[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = NULL;
	int nLastError = 0;

	::memset(&FindFileData, NULL, sizeof FindFileData);

	// Create search path
	if (::PathCombine(szPath, lpszDirectory, lpszExtension) == NULL)
	{
		// Error
		PrintName();
		wcout << L"PathCombine failed. Exiting..." << endl;

		::FindClose(hFind);
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
		wcout << L"FindFirstFile failed. GetLastError reports: " << nLastError << ". Exiting..." << endl;

		::FindClose(hFind);
		return -1;
	}
	else if (nLastError == 2)
	{
		// No file found, exit
		if (!bVerbose)
		{
			PrintName();
			wcout << endl << L"No file found, exiting.." << endl;
		}

		::FindClose(hFind);
		return 1;
	}

	// Make filestring
	if (::PathCombine(szPath, lpszDirectory, FindFileData.cFileName) == NULL)
	{
		// Error
		PrintName();
		wcout << L"PathCombine failed. Exiting..." << endl;
		::FindClose(hFind);

		return -1;
	}

	if (!bVerbose)
		wcout << L"Processing file " << szPath << endl;

	// Process first file
	if (nIncMode == 1)
	{
		// "old" behavior
		ProcessFile(szPath, nIncType, sAddParameters.IsEmpty() == FALSE ? (LPCTSTR)sAddParameters : NULL);
	}
	else
	{
		// Edit binary resource
		ProcessBinaryFile(szPath, nIncType, pBinaryVersionInfo, sAddParameters.IsEmpty() == FALSE ? (LPCTSTR)sAddParameters : NULL);
	}

	// Proceed files
	while (::FindNextFile(hFind, &FindFileData))
	{
		// Make filestring
		if (::PathCombine(szPath, lpszDirectory, FindFileData.cFileName) == NULL)
		{
			// Error
			PrintName();
			wcout << L"PathCombine failed. Exiting..." << endl;
			::FindClose(hFind);

			return -1;
		}

		if (!bVerbose)
			wcout << L"Processing file " << szPath << endl;

		// Get file and work on it.
		if (nIncMode == 1)
		{
			// "old" behavior
			ProcessFile(szPath, nIncType, sAddParameters.IsEmpty() == FALSE ? (LPCTSTR)sAddParameters : NULL);
		}
		else
		{
			// Edit binary resource
			ProcessBinaryFile(szPath, nIncType, pBinaryVersionInfo, sAddParameters.IsEmpty() == FALSE ? (LPCTSTR)sAddParameters : NULL);
		}
	}

	if (::GetLastError() == ERROR_NO_MORE_FILES)
	{
		// No more files, exit.
		//cout << "No more files..." << endl;
		::FindClose(hFind);

		return 0;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	BOOL bError = FALSE;
	BOOL bVerbose = TRUE;
	int nIncType = 1;
	int nIncMode = 1;
	CString sDirectory;
	CString sAddParameters;
	CString sIniFile;
	CString sBinary;

	int nLastError = 0;

	BI_BinaryVersion binaryVersionInfo;

	// No arguments?
	if (argc == 1)
	{
		// Display helptext
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
		if (_tcscmp(argv[i], _T("/V")) == 0)
		{
			if (argv[i+1][0] > '0' && argv[i+1][0] < '4')
			{
				nIncType = argv[i+1][0]-'0';

				// Skip next value
				i++;
				continue;
			}
			else
			{
				// Error using /V
				PrintName();
				wcout << endl << L"Wrong parameter with option /V." << endl;

				bError = TRUE;
			}
		}

		// Option /D
		if (_tcscmp(argv[i], _T("/D")) == 0)
		{
			if (i+1 <= argc)
			{
				sDirectory = argv[i+1];

				// Skip next value
				i++;
				continue;
			}
			else
			{
				// Error using /D
				PrintName();
				wcout << endl << L"Missing directory with option /D." << endl;

				bError = TRUE;
			}
		}

		// Option /A
		if (_tcscmp(argv[i], _T("/A")) == 0)
		{
			if (i+1 <= argc)
			{
				sAddParameters = argv[i+1];

				// Skip next value
				i++;
				continue;
			}
			else
			{
				// Error using /A
				PrintName();
				wcout << endl << L"Missing parameter with option /A." << endl;

				bError = TRUE;
			}
		}

		// Option /B
		if (_tcscmp(argv[i], _T("/B")) == 0)
		{
			nIncMode = 2;

			if (i+1 <= argc && argv[i+1][0] != L'/')
			{
				sBinary = argv[i+1];

				// Skip next value
				i++;
				continue;
			}
		}

		// Option /I
		if (_tcscmp(argv[i], _T("/I")) == 0)
		{
			if (i+1 <= argc)
			{
				sIniFile = argv[i+1];

				// Skip next value
				i++;
				continue;
			}
			else
			{
				// Error using /I
				PrintName();
				wcout << endl << L"No ini file provided." << endl;

				bError = TRUE;
			}
		}

		// Option /VERBOSE
		if (_tcscmp(argv[i], _T("/VERBOSE")) == 0)
		{
			bVerbose = FALSE;
			g_bVerbose = TRUE;
		}

		// Option /DEBUG
		if (_tcscmp(argv[i], _T("/DEBUG")) == 0)
		{
			g_bDebugInfo = TRUE;
		}
	}

	if (!bVerbose)
		PrintName();

	// Missing /D?
	if (sDirectory.IsEmpty())
	{
		PrintName();
		wcout << endl << L"Missing option /D." << endl;

		bError = TRUE;
	}

	// Check directory
	if (!sDirectory.IsEmpty() && !::PathIsDirectory(sDirectory))
	{
		PrintName();
		wcout << endl << L"Invalid directory provided." << endl;

		wcout << (LPCTSTR)sDirectory;

		bError = TRUE;
	}

	// Commandline errors?
	if (bError)
	{
		return -1;
	}

	// Make searchstring
	if (sBinary.IsEmpty() == true)
	{
		if (nIncMode == 1)
		{
			sBinary = BI_EXTENSION1;
			ProcessDirectory(sDirectory, sBinary, bVerbose, nIncMode, nIncType, sIniFile, sAddParameters, NULL);
		}
		else if (nIncMode == 2)
		{
			// Get the inifile version
			ProcessIniFile(sIniFile, &binaryVersionInfo, sAddParameters.IsEmpty() == FALSE ? (LPCTSTR)sAddParameters : NULL);

			// Process binaries
			sBinary = BI_EXTENSION4;
			ProcessDirectory(sDirectory, sBinary, bVerbose, nIncMode, nIncType, sIniFile, sAddParameters, &binaryVersionInfo);

			sBinary = BI_EXTENSION5;
			ProcessDirectory(sDirectory, sBinary, bVerbose, nIncMode, nIncType, sIniFile, sAddParameters, &binaryVersionInfo);

			sBinary = BI_EXTENSION6;
			ProcessDirectory(sDirectory, sBinary, bVerbose, nIncMode, nIncType, sIniFile, sAddParameters, &binaryVersionInfo);
		}
	}

	return 0;
}
