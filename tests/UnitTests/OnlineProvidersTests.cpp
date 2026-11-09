//	Online info providers (Cover Art Archive, LRCLIB, Last.fm, Deezer).
//	The parsers are tested offline with canned API responses. A live smoke test
//	against the real services runs only with JAANGLE_LIVE_TESTS=1 (and Last.fm
//	only with JAANGLE_LASTFM_API_KEY set).
#include "stdafx.h"
#include "TestFramework.h"
#include "InformationProviders/OnlineInfoProvider.h"
#include <WinInet.h>

static void TestParsers()
{
	wprintf(L"TestOnlineProviderParsers\n");

	//--- MusicBrainz release-group search: only good matches, in order.
	std::vector<std::wstring> ids;
	CoverArtArchiveProvider::ParseReleaseGroups(
		"{\"release-groups\":["
		"{\"id\":\"1b022e01-4da6-387b-8658-8678046e4cef\",\"score\":100,\"title\":\"Nevermind\"},"
		"{\"id\":\"aaaaaaaa-0000-0000-0000-000000000001\",\"score\":95},"
		"{\"id\":\"bbbbbbbb-0000-0000-0000-000000000002\",\"score\":40},"
		"{\"score\":100},"
		"{\"id\":\"cccccccc-0000-0000-0000-000000000003\",\"score\":91}]}", ids, 2);
	CHECK(ids.size() == 2 && ids[0] == L"1b022e01-4da6-387b-8658-8678046e4cef" && ids[1] == L"aaaaaaaa-0000-0000-0000-000000000001");
	ids.clear();
	CoverArtArchiveProvider::ParseReleaseGroups("not json", ids, 3);
	CoverArtArchiveProvider::ParseReleaseGroups("{\"error\":\"x\"}", ids, 3);
	CHECK(ids.empty());
	std::wstring url = CoverArtArchiveProvider::BuildSearchURL(L"AC/DC", L"Back in \"Black\"");
	CHECK(url.find(L"https://musicbrainz.org/ws/2/release-group/?fmt=json") == 0);
	CHECK(url.find(L"AC/DC") == std::wstring::npos && url.find(L" ") == std::wstring::npos);
	CHECK(url.find(L"%5C%22Black%5C%22") != std::wstring::npos);	//escaped quotes inside the phrase

	//--- LRCLIB search: exact matches first, instrumental and empty skipped, \n -> \r\n, no duplicates.
	std::vector<std::wstring> lyrics;
	LrclibProvider::ParseSearch(
		"[{\"artistName\":\"Other\",\"trackName\":\"Lithium\",\"plainLyrics\":\"cover version\"},"
		"{\"artistName\":\"NIRVANA\",\"trackName\":\"lithium\",\"plainLyrics\":\"I'm so happy\\ncause today\"},"
		"{\"artistName\":\"Nirvana\",\"trackName\":\"Lithium\",\"plainLyrics\":\"I'm so happy\\ncause today\"},"
		"{\"artistName\":\"Nirvana\",\"trackName\":\"Lithium\",\"instrumental\":true,\"plainLyrics\":\"x\"},"
		"{\"artistName\":\"Nirvana\",\"trackName\":\"Lithium\",\"plainLyrics\":null}]",
		L"Nirvana", L"Lithium", lyrics);
	CHECK(lyrics.size() == 2);
	CHECK(lyrics.size() == 2 && lyrics[0] == L"I'm so happy\r\ncause today" && lyrics[1] == L"cover version");
	lyrics.clear();
	LrclibProvider::ParseSearch("{\"code\":404}", L"a", L"b", lyrics);
	CHECK(lyrics.empty());

	//--- Last.fm: bio content to text; empty/attribution-only and errors give nothing.
	std::wstring page;
	std::wstring bio = LastFmApiProvider::ParseInfo(
		"{\"artist\":{\"name\":\"Nirvana\",\"url\":\"https://www.last.fm/music/Nirvana\",\"bio\":{"
		"\"content\":\"Nirvana was an American rock band &amp; more.<br>Formed in 1987.\\n\\n\\n"
		"<a href=\\\"https://www.last.fm/music/Nirvana\\\">Read more on Last.fm</a>. User-contributed text is available under the Creative Commons By-SA License.\"}}}",
		TRUE, &page);
	CHECK(page == L"https://www.last.fm/music/Nirvana");
	CHECK(bio.find(L"Nirvana was an American rock band & more.\r\nFormed in 1987.") == 0);
	CHECK(bio.find(L"<") == std::wstring::npos && bio.find(L"\r\n\r\n\r\n") == std::wstring::npos);
	CHECK(bio.find(L"Read more on Last.fm") != std::wstring::npos);	//attribution (CC BY-SA) kept
	CHECK(LastFmApiProvider::ParseInfo("{\"artist\":{\"bio\":{\"content\":\" <a href=\\\"x\\\">Read more on Last.fm</a>\"}}}", TRUE, NULL).empty());
	CHECK(LastFmApiProvider::ParseInfo("{\"error\":6,\"message\":\"The artist you supplied could not be found\"}", TRUE, NULL).empty());
	CHECK(LastFmApiProvider::ParseInfo("{\"album\":{\"wiki\":{\"content\":\"An album.\"}}}", FALSE, NULL) == L"An album.");

	//--- Deezer: exact name first, generic placeholders skipped, picture_big fallback.
	std::vector<std::wstring> pics;
	DeezerProvider::ParseArtists(
		"{\"data\":["
		"{\"name\":\"Nirvana Tribute\",\"picture_xl\":\"https://e-cdns-images.dzcdn.net/images/artist/111/1000x1000.jpg\"},"
		"{\"name\":\"nirvana\",\"picture_xl\":\"https://e-cdns-images.dzcdn.net/images/artist/222/1000x1000.jpg\"},"
		"{\"name\":\"Nirvana\",\"picture_xl\":\"https://e-cdns-images.dzcdn.net/images/artist//1000x1000.jpg\"},"
		"{\"name\":\"Nirvana (UK)\",\"picture_big\":\"https://e-cdns-images.dzcdn.net/images/artist/333/500x500.jpg\"}]}",
		L"Nirvana", pics, 3);
	CHECK(pics.size() == 3);
	CHECK(pics.size() == 3 && pics[0].find(L"/222/") != std::wstring::npos && pics[1].find(L"/111/") != std::wstring::npos
		&& pics[2].find(L"/333/") != std::wstring::npos);

	//--- Helpers
	CHECK(OnlineInfo::HtmlToText(L"  a<b>b</b>&quot;c&quot;  ") == L"ab\"c\"");
	CHECK(OnlineInfo::SameText(L"\x0386\x039B\x03A6\x0391", L"\x03AC\x03BB\x03C6\x03B1"));	//ΆΛΦΑ / άλφα
	CHECK(OnlineInfo::UserLanguageCode().size() == 2);
	CHECK(!OnlineInfo::IsImageData(std::string("<html>404</html>")));
	std::string jpeg(2000, '\0');
	jpeg[0] = (char)0xFF;
	jpeg[1] = (char)0xD8;
	CHECK(OnlineInfo::IsImageData(jpeg));

	//--- Requests the provider cannot handle are refused
	LrclibProvider lr;
	IInfoProvider::Request req(IInfoProvider::SRV_ArtistBio);
	req.artist = L"Nirvana";
	CHECK(!lr.OpenRequest(req));
	IInfoProvider::Request incomplete(IInfoProvider::SRV_TrackLyrics);
	incomplete.artist = L"Nirvana";
	CHECK(!lr.OpenRequest(incomplete));	//no title
	LastFmApiProvider lf;
	IConfigurable::SettingInfo si;
	CHECK(lf.GetSettingInfo(0, si) && si.type == IConfigurable::COVT_LPCTSTR && !lf.GetSettingInfo(1, si));
	lf.SetLPCTSTRSetting(0, L"key123");
	IInfoProvider* pClone = lf.Clone();
	CHECK(wcscmp(pClone->GetLPCTSTRSetting(0), L"key123") == 0);	//settings survive Clone
	delete pClone;
}

static BOOL LiveFirst(IInfoProvider& p, IInfoProvider::Request& req, IInfoProvider::Result& res)
{
	return p.OpenRequest(req) && p.GetNextResult(res) && res.IsValid();
}

static BOOL IsImageFile(LPCWSTR path)
{
	HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return FALSE;
	std::string data(GetFileSize(h, NULL), '\0');
	DWORD read = 0;
	BOOL ok = !data.empty() && ReadFile(h, &data[0], (DWORD)data.size(), &read, NULL) && read == data.size();
	CloseHandle(h);
	return ok && OnlineInfo::IsImageData(data);
}

static void TestLive()
{
	wprintf(L"TestOnlineProvidersLive\n");
	HINTERNET hNet = InternetOpenW(L"Jaangle-UnitTests (+https://github.com/outcast1000/TeenSpirit)",
		INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	CHECK(hNet != NULL);
	IInfoProvider::Result res;

	CoverArtArchiveProvider caa;
	caa.SetInternetHandle(hNet);
	IInfoProvider::Request album(IInfoProvider::SRV_AlbumImage);
	album.artist = L"Nirvana";
	album.album = L"Nevermind";
	CHECK(LiveFirst(caa, album, res) && IsImageFile(res.main));
	wprintf(L"  cover: %s\n", res.additionalInfo ? res.additionalInfo : L"-");

	LrclibProvider lrc;
	lrc.SetInternetHandle(hNet);
	IInfoProvider::Request track(IInfoProvider::SRV_TrackLyrics);
	track.artist = L"Nirvana";
	track.title = L"Smells Like Teen Spirit";
	CHECK(LiveFirst(lrc, track, res) && wcslen(res.main) > 200);
	if (res.main)
		wprintf(L"  lyrics: %.40s...\n", res.main);

	DeezerProvider dz;
	dz.SetInternetHandle(hNet);
	IInfoProvider::Request artist(IInfoProvider::SRV_ArtistImage);
	artist.artist = L"Nirvana";
	CHECK(LiveFirst(dz, artist, res) && IsImageFile(res.main));

	WCHAR key[100];
	if (GetEnvironmentVariableW(L"JAANGLE_LASTFM_API_KEY", key, 100) > 0)
	{
		LastFmApiProvider lf;
		lf.SetInternetHandle(hNet);
		lf.SetLPCTSTRSetting(0, key);
		IInfoProvider::Request bio(IInfoProvider::SRV_ArtistBio);
		bio.artist = L"Nirvana";
		CHECK(LiveFirst(lf, bio, res) && wcslen(res.main) > 200);
	}
	else
		wprintf(L"  (Last.fm skipped: JAANGLE_LASTFM_API_KEY not set)\n");

	//No match: no result, no error
	IInfoProvider::Request nothing(IInfoProvider::SRV_TrackLyrics);
	nothing.artist = L"zz-no-such-artist-qq";
	nothing.title = L"zz-no-such-title-qq";
	CHECK(lrc.OpenRequest(nothing) && !lrc.GetNextResult(res));
	InternetCloseHandle(hNet);
}

void RunOnlineProvidersTests()
{
	TestParsers();
	WCHAR v[8];
	if (GetEnvironmentVariableW(L"JAANGLE_LIVE_TESTS", v, 8) > 0 && v[0] == L'1')
		TestLive();
}
