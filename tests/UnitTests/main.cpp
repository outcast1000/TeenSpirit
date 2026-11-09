//	UnitTests: database layer, job queue and thread shutdown tests.
//
//	Run: build\out\Win32\<Configuration>\UnitTests.exe   (exit code = failures)
#include "stdafx.h"
#include "TestFramework.h"

INT gChecks = 0;
INT gFailures = 0;

//Hooks used by the UNITTEST macro (UnitTest.h) in the code under test.
void UnitTestFailed(LPCSTR message, LPCSTR file)
{
	gChecks++;
	gFailures++;
	wprintf(L"  FAIL (UNITTEST) %S: %S\n", file, message);
}
void UnitTestSuccess(LPCSTR message, LPCSTR file)
{
	gChecks++;
}

std::wstring TempPath(LPCWSTR name)
{
	WCHAR dir[MAX_PATH];
	GetTempPathW(MAX_PATH, dir);
	std::wstring p(dir);
	p += L"jaangle-tests\\";
	CreateDirectoryW(p.c_str(), NULL);
	return p + name;
}

int wmain()
{
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
		return 100;
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	RunDatabaseTests();
	RunJobQueueTests();
	RunLastFMTests();
	RunWebServicesTests();
	RunOnlineProvidersTests();

	wprintf(L"\n%d checks, %d failures\n", gChecks, gFailures);
	CoUninitialize();
	return gFailures;
}
