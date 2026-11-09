#include "stdafx.h"
#include "OnlineInfoProvider.h"
#include "WebPageUtilities.h"
#include "ApiKeys.h"
#include <WinInet.h>
#include <mutex>
#include <nlohmann/json.hpp>

using nlohmann::json;

//==============================================================
//		Helpers
//==============================================================

namespace
{
	const DWORD cMaxDownload = 20 * 1024 * 1024;

	//Safe accessors: the APIs omit fields or send null.
	std::string JsonString(const json& obj, const char* key)
	{
		if (obj.is_object())
		{
			json::const_iterator it = obj.find(key);
			if (it != obj.end() && it->is_string())
				return it->get<std::string>();
		}
		return std::string();
	}
	const json* JsonChild(const json& obj, const char* key)
	{
		if (obj.is_object())
		{
			json::const_iterator it = obj.find(key);
			if (it != obj.end())
				return &(*it);
		}
		return NULL;
	}
	json Parse(const std::string& text)
	{
		return json::parse(text, nullptr, false);	//no exceptions; is_discarded() on error
	}
	BOOL Contains(const std::vector<std::wstring>& v, const std::wstring& s)
	{
		for (size_t i = 0; i < v.size(); i++)
			if (v[i] == s)
				return TRUE;
		return FALSE;
	}
	//Lucene phrase escaping (MusicBrainz search syntax).
	std::wstring LuceneEscape(LPCWSTR s)
	{
		std::wstring r;
		for (; *s; s++)
		{
			if (*s == L'\\' || *s == L'"')
				r += L'\\';
			r += *s;
		}
		return r;
	}
	//MusicBrainz allows one request per second per client.
	void ThrottleMusicBrainz()
	{
		static std::mutex mx;
		static ULONGLONG lastTick = 0;
		std::lock_guard<std::mutex> lk(mx);
		ULONGLONG now = GetTickCount64();
		if (lastTick != 0 && now - lastTick < 1100)
			Sleep((DWORD)(1100 - (now - lastTick)));
		lastTick = GetTickCount64();
	}
}

BOOL OnlineInfo::HttpGet(LPVOID hInternet, const std::wstring& url, std::string& body, DWORD* status)
{
	body.clear();
	if (status != NULL)
		*status = 0;
	if (hInternet == NULL)
		return FALSE;
	HINTERNET hUrl = InternetOpenUrlW((HINTERNET)hInternet, url.c_str(),
		L"Accept: application/json, image/*;q=0.9, */*;q=0.5\r\n", (DWORD)-1,
		INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI, 0);
	if (hUrl == NULL)
	{
		TRACE(_T("@1 OnlineInfo::HttpGet failed to open '%s'\r\n"), url.c_str());
		return FALSE;
	}
	DWORD code = 0;
	DWORD len = sizeof(code);
	HttpQueryInfoW(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &code, &len, NULL);
	if (status != NULL)
		*status = code;
	BOOL bOK = (code == 200);
	if (bOK)
	{
		char bf[16384];
		DWORD read = 0;
		while (InternetReadFile(hUrl, bf, sizeof(bf), &read) && read > 0)
		{
			body.append(bf, read);
			if (body.size() > cMaxDownload)
			{
				bOK = FALSE;
				break;
			}
		}
	}
	InternetCloseHandle(hUrl);
	if (!bOK)
	{
		TRACE(_T("@3 OnlineInfo::HttpGet %d '%s'\r\n"), (INT)code, url.c_str());
		body.clear();
	}
	return bOK;
}

BOOL OnlineInfo::IsImageData(const std::string& d)
{
	if (d.size() < 1000)	//placeholders / error pixels
		return FALSE;
	const unsigned char* p = (const unsigned char*)d.data();
	return (p[0] == 0xFF && p[1] == 0xD8) ||								//JPEG
		(p[0] == 0x89 && p[1] == 'P' && p[2] == 'N' && p[3] == 'G') ||	//PNG
		(p[0] == 'G' && p[1] == 'I' && p[2] == 'F') ||						//GIF
		(p[0] == 'B' && p[1] == 'M');										//BMP
}

BOOL OnlineInfo::HttpGetImage(LPVOID hInternet, const std::wstring& url, LPCTSTR fileName)
{
	std::string data;
	if (!HttpGet(hInternet, url, data) || !IsImageData(data))
		return FALSE;
	HANDLE h = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD written = 0;
	BOOL bOK = WriteFile(h, data.data(), (DWORD)data.size(), &written, NULL) && written == data.size();
	CloseHandle(h);
	return bOK;
}

std::wstring OnlineInfo::Utf8ToWide(const std::string& s)
{
	if (s.empty())
		return std::wstring();
	INT n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (INT)s.size(), NULL, 0);
	std::wstring w(n, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.data(), (INT)s.size(), &w[0], n);
	return w;
}

std::wstring OnlineInfo::UrlEncode(LPCWSTR value)
{
	std::wstring encoded;
	URLEncodeW(encoded, value != NULL ? value : L"");
	return encoded;
}

BOOL OnlineInfo::SameText(const std::wstring& a, const std::wstring& b)
{
	return CompareStringOrdinal(a.c_str(), (INT)a.size(), b.c_str(), (INT)b.size(), TRUE) == CSTR_EQUAL;
}

std::wstring OnlineInfo::HtmlToText(const std::wstring& html)
{
	std::wstring text;
	text.reserve(html.size());
	for (size_t i = 0; i < html.size(); i++)
	{
		if (html[i] == L'<')
		{
			size_t end = html.find(L'>', i);
			if (end == std::wstring::npos)
				break;
			std::wstring tag = html.substr(i + 1, end - i - 1);
			if (_wcsnicmp(tag.c_str(), L"br", 2) == 0 || _wcsnicmp(tag.c_str(), L"/p", 2) == 0)
				text += L"\n";
			i = end;
			continue;
		}
		text += html[i] == L'\r' ? L'\n' : html[i];
	}
	ReplaceHtmlEntities(text);
	//Normalize: no blank runs of more than one empty line, CRLF line ends, trimmed.
	std::wstring out;
	INT newlines = 0;
	for (size_t i = 0; i < text.size(); i++)
	{
		if (text[i] == L'\n')
		{
			newlines++;
			continue;
		}
		if (newlines > 0 && !out.empty())
			out += newlines > 1 ? L"\r\n\r\n" : L"\r\n";
		newlines = 0;
		out += text[i];
	}
	size_t first = out.find_first_not_of(L" \t");
	size_t last = out.find_last_not_of(L" \t");
	return first == std::wstring::npos ? std::wstring() : out.substr(first, last - first + 1);
}

std::wstring OnlineInfo::UserLanguageCode()
{
	WCHAR name[LOCALE_NAME_MAX_LENGTH];
	if (LCIDToLocaleName(MAKELCID(GetUserDefaultUILanguage(), SORT_DEFAULT), name, LOCALE_NAME_MAX_LENGTH, 0) > 0)
	{
		std::wstring s(name);
		size_t dash = s.find(L'-');
		if (dash != std::wstring::npos)
			s.resize(dash);
		if (s.size() == 2)
			return s;
	}
	return L"en";
}

//==============================================================
//		OnlineInfoProvider
//==============================================================

OnlineInfoProvider::OnlineInfoProvider():
m_hNet(NULL),
m_request(SRV_First),
m_next(0),
m_bFetched(FALSE)
{
	TCHAR dir[MAX_PATH];
	m_tempFile[0] = 0;
	if (GetTempPath(MAX_PATH, dir) > 0)
		GetTempFileName(dir, _T("tsi"), 0, m_tempFile);	//unique per instance
}

OnlineInfoProvider::~OnlineInfoProvider()
{
	if (m_tempFile[0] != 0)
		DeleteFile(m_tempFile);
}

IInfoProvider* OnlineInfoProvider::FinishClone(IInfoProvider* pClone) const
{
	pClone->SetInternetHandle(GetInternetHandle());
	IConfigurableHelper::TransferConfiguration(*this, *pClone);
	return pClone;
}

LPCTSTR OnlineInfoProvider::GetModuleInfo(ModuleInfo mi) const
{
	switch (mi)
	{
	case IPI_Author:		return _T("TeenSpirit contributors");
	case IPI_VersionStr:	return _T("1");
	case IPI_VersionNum:	return _T("1");
	case IPI_Email:			return _T("");
	default:				return GetInfo(mi);
	}
}

BOOL OnlineInfoProvider::OpenRequest(const Request& request)
{
	if (!request.IsValid() || !CanHandle(request.service))
		return FALSE;
	m_artist = request.artist != NULL ? request.artist : _T("");
	m_album = request.album != NULL ? request.album : _T("");
	m_title = request.title != NULL ? request.title : _T("");
	m_request = Request(request.service);
	m_request.artist = m_artist.c_str();
	m_request.album = request.album != NULL ? m_album.c_str() : NULL;
	m_request.title = request.title != NULL ? m_title.c_str() : NULL;
	m_candidates.clear();
	m_next = 0;
	m_bFetched = FALSE;
	return TRUE;
}

BOOL OnlineInfoProvider::GetNextResult(Result& result)
{
	if (m_request.service == SRV_First)
		return FALSE;
	if (!m_bFetched)
	{
		m_bFetched = TRUE;
		if (!Fetch(m_request, m_candidates))
			m_candidates.clear();
	}
	while (m_next < m_candidates.size())
	{
		const Candidate& c = m_candidates[m_next++];
		if (c.bImage)
		{
			if (m_tempFile[0] == 0 || !OnlineInfo::HttpGetImage(m_hNet, c.value, m_tempFile))
				continue;
			m_resultMain = m_tempFile;
		}
		else
		{
			if (c.value.empty())
				continue;
			m_resultMain = c.value;
		}
		m_resultSource = c.source;
		result.service = m_request.service;
		result.main = m_resultMain.c_str();
		result.additionalInfo = m_resultSource.empty() ? NULL : m_resultSource.c_str();
		return TRUE;
	}
	return FALSE;
}

//==============================================================
//		Cover Art Archive (album covers)
//==============================================================

std::wstring CoverArtArchiveProvider::BuildSearchURL(LPCWSTR artist, LPCWSTR album)
{
	std::wstring query(L"releasegroup:\"");
	query += LuceneEscape(album);
	query += L"\" AND artist:\"";
	query += LuceneEscape(artist);
	query += L"\"";
	return L"https://musicbrainz.org/ws/2/release-group/?fmt=json&limit=5&query=" + OnlineInfo::UrlEncode(query.c_str());
}

void CoverArtArchiveProvider::ParseReleaseGroups(const std::string& text, std::vector<std::wstring>& ids, size_t maxIDs)
{
	json j = Parse(text);
	const json* groups = JsonChild(j, "release-groups");
	if (groups == NULL || !groups->is_array())
		return;
	for (size_t i = 0; i < groups->size() && ids.size() < maxIDs; i++)
	{
		const json& g = (*groups)[i];
		const json* score = JsonChild(g, "score");
		std::string id = JsonString(g, "id");
		if (id.empty() || score == NULL || !score->is_number() || score->get<INT>() < 90)
			continue;
		ids.push_back(OnlineInfo::Utf8ToWide(id));
	}
}

BOOL CoverArtArchiveProvider::Fetch(const Request& request, std::vector<Candidate>& candidates)
{
	//MusicBrainz answers 503 when the rate limit (per IP, so shared with any other
	//client behind the same address) is exceeded: back off and try again.
	const std::wstring url = BuildSearchURL(request.artist, request.album);
	std::string body;
	DWORD status = 0;
	BOOL bOK = FALSE;
	for (INT attempt = 0; attempt < 3 && !bOK; attempt++)
	{
		if (attempt > 0)
		{
			if (status != 503)
				break;
			Sleep(2000 * attempt);
		}
		ThrottleMusicBrainz();
		bOK = OnlineInfo::HttpGet(m_hNet, url, body, &status);
	}
	if (!bOK)
		return FALSE;
	std::vector<std::wstring> ids;
	ParseReleaseGroups(body, ids, 3);
	for (size_t i = 0; i < ids.size(); i++)
	{
		Candidate c;
		c.bImage = TRUE;
		c.value = L"https://coverartarchive.org/release-group/" + ids[i] + L"/front-500";
		c.source = L"https://musicbrainz.org/release-group/" + ids[i];
		candidates.push_back(c);
	}
	return TRUE;
}

LPCTSTR CoverArtArchiveProvider::GetInfo(ModuleInfo mi) const
{
	switch (mi)
	{
	case IPI_UniqueID:		return _T("CAAR");
	case IPI_Name:			return _T("Cover Art Archive");
	case IPI_Description:	return _T("Album covers from the Cover Art Archive (MusicBrainz)");
	case IPI_HomePage:		return _T("https://coverartarchive.org");
	}
	return NULL;
}

//==============================================================
//		LRCLIB (lyrics)
//==============================================================

void LrclibProvider::ParseSearch(const std::string& text, LPCWSTR artist, LPCWSTR title, std::vector<std::wstring>& lyrics)
{
	json j = Parse(text);
	if (!j.is_array())
		return;
	//Two passes: exact artist + title matches first, then the rest.
	for (INT pass = 0; pass < 2; pass++)
	{
		for (size_t i = 0; i < j.size() && lyrics.size() < 3; i++)
		{
			const json& r = j[i];
			const json* instrumental = JsonChild(r, "instrumental");
			if (instrumental != NULL && instrumental->is_boolean() && instrumental->get<bool>())
				continue;
			std::wstring text = OnlineInfo::Utf8ToWide(JsonString(r, "plainLyrics"));
			if (text.empty())
				continue;
			BOOL bExact = OnlineInfo::SameText(OnlineInfo::Utf8ToWide(JsonString(r, "artistName")), artist) &&
				OnlineInfo::SameText(OnlineInfo::Utf8ToWide(JsonString(r, "trackName")), title);
			if ((pass == 0) != (bExact != FALSE))
				continue;
			//LRCLIB uses \n line ends; the info pane expects \r\n.
			std::wstring crlf;
			for (size_t k = 0; k < text.size(); k++)
			{
				if (text[k] == L'\n' && (k == 0 || text[k - 1] != L'\r'))
					crlf += L'\r';
				crlf += text[k];
			}
			if (!Contains(lyrics, crlf))
				lyrics.push_back(crlf);
		}
	}
}

BOOL LrclibProvider::Fetch(const Request& request, std::vector<Candidate>& candidates)
{
	std::wstring url(L"https://lrclib.net/api/search?artist_name=");
	url += OnlineInfo::UrlEncode(request.artist);
	url += L"&track_name=";
	url += OnlineInfo::UrlEncode(request.title);
	std::string body;
	if (!OnlineInfo::HttpGet(m_hNet, url, body))
		return FALSE;
	std::vector<std::wstring> lyrics;
	ParseSearch(body, request.artist, request.title, lyrics);
	for (size_t i = 0; i < lyrics.size(); i++)
	{
		Candidate c;
		c.value = lyrics[i];
		c.source = L"https://lrclib.net";
		candidates.push_back(c);
	}
	return TRUE;
}

LPCTSTR LrclibProvider::GetInfo(ModuleInfo mi) const
{
	switch (mi)
	{
	case IPI_UniqueID:		return _T("LRCL");
	case IPI_Name:			return _T("LRCLIB");
	case IPI_Description:	return _T("Lyrics from lrclib.net");
	case IPI_HomePage:		return _T("https://lrclib.net");
	}
	return NULL;
}

//==============================================================
//		Last.fm API (artist biographies, album descriptions)
//==============================================================

LastFmApiProvider::LastFmApiProvider():
m_apiKey(_T(JAANGLE_LASTFM_API_KEY))
{
}

BOOL LastFmApiProvider::CanHandle(ServiceEnum service) const
{
	return service == SRV_ArtistBio || service == SRV_AlbumReview;
}

BOOL LastFmApiProvider::GetSettingInfo(INT settingIndex, IConfigurable::SettingInfo& setting) const
{
	if (settingIndex != 0)
		return FALSE;
	setting.type = IConfigurable::COVT_LPCTSTR;
	setting.name = _T("API key");
	setting.desc = _T("Last.fm API key (free: https://www.last.fm/api/account/create)");
	setting.availableValues = NULL;
	return TRUE;
}

LPCTSTR LastFmApiProvider::GetLPCTSTRSetting(INT settingIndex) const
{
	return settingIndex == 0 ? m_apiKey.c_str() : NULL;
}

void LastFmApiProvider::SetLPCTSTRSetting(INT settingIndex, LPCTSTR strVal)
{
	if (settingIndex == 0)
		m_apiKey = strVal != NULL ? strVal : _T("");
}

std::wstring LastFmApiProvider::ParseInfo(const std::string& text, BOOL bArtist, std::wstring* pageURL)
{
	json j = Parse(text);
	const json* item = JsonChild(j, bArtist ? "artist" : "album");
	if (item == NULL)
		return std::wstring();		//{"error":6,"message":"..."} etc.
	if (pageURL != NULL)
		*pageURL = OnlineInfo::Utf8ToWide(JsonString(*item, "url"));
	const json* wiki = JsonChild(*item, bArtist ? "bio" : "wiki");
	if (wiki == NULL)
		return std::wstring();
	std::wstring content = OnlineInfo::HtmlToText(OnlineInfo::Utf8ToWide(JsonString(*wiki, "content")));
	//Text that is only the attribution link means: nothing written yet.
	if (content.size() < 80 && content.find(L"Read more on Last.fm") != std::wstring::npos)
		return std::wstring();
	return content;
}

BOOL LastFmApiProvider::Fetch(const Request& request, std::vector<Candidate>& candidates)
{
	if (m_apiKey.empty())
	{
		TRACE(_T("@2 LastFmApiProvider: no API key configured\r\n"));
		return FALSE;
	}
	const BOOL bArtist = request.service == SRV_ArtistBio;
	std::wstring base(L"https://ws.audioscrobbler.com/2.0/?format=json&autocorrect=1&method=");
	base += bArtist ? L"artist.getinfo" : L"album.getinfo";
	base += L"&api_key=" + OnlineInfo::UrlEncode(m_apiKey.c_str());
	base += L"&artist=" + OnlineInfo::UrlEncode(request.artist);
	if (!bArtist)
		base += L"&album=" + OnlineInfo::UrlEncode(request.album);
	//The user's language first, English as fallback.
	std::wstring lang = OnlineInfo::UserLanguageCode();
	for (INT attempt = 0; attempt < 2; attempt++)
	{
		std::wstring url(base);
		if (attempt == 0)
		{
			if (lang == L"en")
				continue;
			url += L"&lang=" + lang;
		}
		std::string body;
		if (!OnlineInfo::HttpGet(m_hNet, url, body))
			continue;
		Candidate c;
		c.value = ParseInfo(body, bArtist, &c.source);
		if (!c.value.empty())
		{
			candidates.push_back(c);
			break;
		}
	}
	return TRUE;
}

LPCTSTR LastFmApiProvider::GetInfo(ModuleInfo mi) const
{
	switch (mi)
	{
	case IPI_UniqueID:		return _T("LFMA");
	case IPI_Name:			return _T("Last.fm");
	case IPI_Description:	return _T("Artist biographies and album descriptions from the Last.fm API");
	case IPI_HomePage:		return _T("https://www.last.fm");
	}
	return NULL;
}

//==============================================================
//		Deezer (artist pictures)
//==============================================================

void DeezerProvider::ParseArtists(const std::string& text, LPCWSTR artist, std::vector<std::wstring>& pictures, size_t maxPictures)
{
	json j = Parse(text);
	const json* data = JsonChild(j, "data");
	if (data == NULL || !data->is_array())
		return;
	for (INT pass = 0; pass < 2; pass++)
	{
		for (size_t i = 0; i < data->size() && pictures.size() < maxPictures; i++)
		{
			const json& a = (*data)[i];
			BOOL bExact = OnlineInfo::SameText(OnlineInfo::Utf8ToWide(JsonString(a, "name")), artist);
			if ((pass == 0) != (bExact != FALSE))
				continue;
			std::string pic = JsonString(a, "picture_xl");
			if (pic.empty())
				pic = JsonString(a, "picture_big");
			//Artists without a picture get a generic one: ".../images/artist//..."
			if (pic.empty() || pic.find("/artist//") != std::string::npos)
				continue;
			std::wstring w = OnlineInfo::Utf8ToWide(pic);
			if (!Contains(pictures, w))
				pictures.push_back(w);
		}
	}
}

BOOL DeezerProvider::Fetch(const Request& request, std::vector<Candidate>& candidates)
{
	std::string body;
	if (!OnlineInfo::HttpGet(m_hNet, L"https://api.deezer.com/search/artist?limit=5&q=" + OnlineInfo::UrlEncode(request.artist), body))
		return FALSE;
	std::vector<std::wstring> pictures;
	ParseArtists(body, request.artist, pictures, 3);
	for (size_t i = 0; i < pictures.size(); i++)
	{
		Candidate c;
		c.bImage = TRUE;
		c.value = pictures[i];
		c.source = L"https://www.deezer.com";
		candidates.push_back(c);
	}
	return TRUE;
}

LPCTSTR DeezerProvider::GetInfo(ModuleInfo mi) const
{
	switch (mi)
	{
	case IPI_UniqueID:		return _T("DEEZ");
	case IPI_Name:			return _T("Deezer");
	case IPI_Description:	return _T("Artist pictures from Deezer");
	case IPI_HomePage:		return _T("https://www.deezer.com");
	}
	return NULL;
}
