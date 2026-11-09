//	LastFMServices worker thread: starts on demand and is joined (not
//	terminated) on destruction. No network traffic: without credentials the
//	worker never connects.
#include "stdafx.h"
#include "TestFramework.h"
#include "LastFMServices.h"

void RunLastFMTests()
{
	wprintf(L"TestLastFMThreadShutdown\n");
	HINTERNET hNet = InternetOpen(_T("UnitTests"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	CHECK(hNet != NULL);
	for (INT round = 0; round < 3; round++)
	{
		LastFMServices* pLFM = new LastFMServices;
		pLFM->Init(hNet);
		LastFMSubmitTrack track;
		track.artist = L"Artist";
		track.track = L"Track";
		track.length = 200;
		for (INT i = 0; i < 20; i++)	//also from the queue limit path
			pLFM->Submit(track);
		Sleep(round * 300);				//destroy while idle / waiting
		ULONGLONG t0 = GetTickCount64();
		delete pLFM;
		CHECK(GetTickCount64() - t0 < 2000);
	}
	//Destroying an instance that never started its thread
	ULONGLONG t0 = GetTickCount64();
	delete new LastFMServices;
	CHECK(GetTickCount64() - t0 < 100);
	InternetCloseHandle(hNet);
}
