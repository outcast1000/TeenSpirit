#include "stdafx.h"
#include "FileVersion.h"
#include <vector>

#pragma comment(lib, "version.lib")

BOOL GetFileVersion(FileVersion& fv, HMODULE module/* = 0*/)
{
	if (module == 0)
		module = AfxGetInstanceHandle();
	TCHAR szModuleFileName[MAX_PATH];
	if (GetModuleFileName(module, szModuleFileName, MAX_PATH) == 0)
		return FALSE;
	DWORD dwHandle = 0;
	DWORD dwDataSize = ::GetFileVersionInfoSize(szModuleFileName, &dwHandle);
	if (dwDataSize == 0)
		return FALSE;
	std::vector<BYTE> data(dwDataSize);
	if (!::GetFileVersionInfo(szModuleFileName, 0, dwDataSize, &data[0]))
		return FALSE;
	UINT nQuerySize = 0;
	VS_FIXEDFILEINFO* pVsffi = NULL;
	if (!::VerQueryValue(&data[0], _T("\\"), (void**)&pVsffi, &nQuerySize) || pVsffi == NULL)
		return FALSE;
	fv.Major = HIWORD(pVsffi->dwFileVersionMS);
	fv.Minor = LOWORD(pVsffi->dwFileVersionMS);
	fv.Build = HIWORD(pVsffi->dwFileVersionLS);
	fv.SubBuild = LOWORD(pVsffi->dwFileVersionLS);
	return TRUE;
}
