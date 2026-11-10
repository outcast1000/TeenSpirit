//	The real Windows version. GetVersion/GetVersionEx are deprecated, and on Windows 8.1+ they
//	report 6.2 to applications whose manifest doesn't list the newer OS versions.
#ifndef _OSVersion_h_
#define _OSVersion_h_

#include <windows.h>

inline BOOL GetOSVersion(OSVERSIONINFOEXW& osvi)
{
	ZeroMemory(&osvi, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	typedef LONG (WINAPI* RtlGetVersionFn)(OSVERSIONINFOEXW*);
	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	RtlGetVersionFn fn = ntdll ? (RtlGetVersionFn)GetProcAddress(ntdll, "RtlGetVersion") : NULL;
	return fn != NULL && fn(&osvi) == 0;	//STATUS_SUCCESS
}

#endif
