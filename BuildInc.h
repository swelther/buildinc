#pragma once

// IncTypes
#define BI_INCFILEVERSION				1
#define	BI_INCPRODUCTVERSION		2
#define BI_INCFILEPRODVERSION		3

struct BI_BinaryVersion
{
public:
	int nVersion1;
	int nVersion2;
	int nVersion3;
	int nVersion4;
	CString sFileVersion;
	CString sProductVersion;
};

BOOL ProcessIniFile(LPCTSTR lpszIniFile, BI_BinaryVersion *pVersion, LPCTSTR lpcszAddParameters = NULL);

void AlignOn4Byte(DWORD *pnPointer)
{
	DWORD nPadding = *pnPointer;

	if (nPadding%4 > 0)
		nPadding = nPadding+(4-(nPadding%4));

	*pnPointer = nPadding;
};

CString FormatLastError(DWORD dwMessageId)
{
	LPVOID lpMsgBuf;
	CString sMessage;

	// Get Errortext
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
								0, dwMessageId, 0, (LPTSTR) &lpMsgBuf, 0, NULL);

	sMessage = (LPTSTR)lpMsgBuf;
	LocalFree(lpMsgBuf);

	return sMessage;
}
