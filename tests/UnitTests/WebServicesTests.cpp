//	"Search on the web" templates (WebServices\*.wsxml): they load, and the
//	placeholders expand to correctly encoded URLs.
#include "stdafx.h"
#include "TestFramework.h"
#include "ServicesRegistrationSrv.h"
#include "WebPageUtilities.h"

namespace
{
	class CaptureViewer : public ServicesRegistrationSrv::IWebViewer
	{
	public:
		virtual BOOL OpenURL(LPCTSTR path)	{url = path; return TRUE;}
		std::wstring url;
	};

	std::wstring RepoWebServicesDir()
	{
		//The tests run from build\out\<Platform>\<Configuration>\.
		WCHAR exe[MAX_PATH];
		GetModuleFileNameW(NULL, exe, MAX_PATH);
		std::wstring dir(exe);
		for (INT i = 0; i < 5; i++)
			dir.resize(dir.find_last_of(L'\\'));
		return dir + L"\\WebServices\\";
	}

	BOOL FindService(ServicesRegistrationSrv& srv, LPCWSTR name, ServiceTypeEnum type, ServiceInfo& out)
	{
		for (UINT i = 0; i < srv.GetServicesCount(); i++)
		{
			ServiceInfo si;
			if (srv.GetServiceInfo(i, si) && wcscmp(si.Name, name) == 0 && si.CanHandle(type))
			{
				out = si;
				return TRUE;
			}
		}
		return FALSE;
	}
}

void RunWebServicesTests()
{
	wprintf(L"TestWebServices\n");
	ServicesRegistrationSrv srv;
	std::wstring dir = RepoWebServicesDir();
	CHECK(srv.LoadServices(dir.c_str()) == 10);	//files
	CHECK(srv.GetServicesCount() == 27);

	BOOL allHttps = TRUE;
	for (UINT i = 0; i < srv.GetServicesCount(); i++)
	{
		ServiceInfo si;
		CHECK(srv.GetServiceInfo(i, si));
		if (wcsncmp(si.Action, L"web@https://", 12) != 0)
			allHttps = FALSE;
	}
	CHECK(allHttps);

	ServiceInfo discogs;
	CHECK(FindService(srv, L"Discogs", STYPE_ArtistInfo, discogs));
	CHECK(discogs.hIcon != NULL);
	CaptureViewer viewer;
	ServicesExecuteInfo inf;
	inf.artist = L"AC/DC & \x0391";	//"AC/DC & Α"
	CHECK(srv.Execute(discogs, inf, &viewer));
	wprintf(L"  %s\n", viewer.url.c_str());
	CHECK(viewer.url.find(L"https://www.discogs.com/search/?q=") == 0);
	CHECK(viewer.url.find(L"&type=artist") != std::wstring::npos);
	CHECK(viewer.url.find(L"%artist%") == std::wstring::npos);
	CHECK(viewer.url.find(L"AC/DC") == std::wstring::npos);	//'/' must be encoded
	CHECK(viewer.url.find(L" ") == std::wstring::npos);

	ServiceInfo genius;
	CHECK(FindService(srv, L"Genius (lyrics)", STYPE_TrackInfo, genius));
	ServiceInfo none;
	CHECK(!FindService(srv, L"Genius (lyrics)", STYPE_ArtistInfo, none));	//track only
	inf.artist = L"Nirvana";
	inf.title = L"Lithium";
	CHECK(srv.Execute(genius, inf, &viewer));
	CHECK(viewer.url == L"https://genius.com/search?q=Nirvana+Lithium");

	//URLEncodeW: UTF-8 of the whole string, so surrogate pairs survive
	std::wstring enc;
	CHECK(URLEncodeW(enc, L"AC/DC & \x0391"));
	CHECK(enc == L"AC%2FDC%20%26%20%CE%91");
	enc.clear();
	CHECK(URLEncodeW(enc, L"a\xD83C\xDFB5z"));	//U+1F3B5 (musical note)
	CHECK(enc == L"a%F0%9F%8E%B5z");
	enc.clear();
	CHECK(URLEncodeW(enc, L""));
	CHECK(enc.empty());
}
