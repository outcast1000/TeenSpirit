//	Minimal test framework shared by the UnitTests suites.
#ifndef _TestFramework_h_
#define _TestFramework_h_

#include <string>
#include <cstdio>

extern INT gChecks;
extern INT gFailures;

#define CHECK(cond) \
	do { gChecks++; if (!(cond)) { gFailures++; wprintf(L"  FAIL %S(%d): %S\n", __FILE__, __LINE__, #cond); } } while (0)

//Path of a scratch file in %TEMP%\jaangle-tests\ (the folder is created).
std::wstring TempPath(LPCWSTR name);

//Suites
void RunDatabaseTests();
void RunJobQueueTests();
void RunLastFMTests();
void RunWebServicesTests();
void RunOnlineProvidersTests();
void RunTaggerTests();
void RunAppServicesTests();
void RunSkinTests();

#endif
