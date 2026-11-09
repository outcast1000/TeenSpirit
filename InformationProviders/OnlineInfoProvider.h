//	Base class and helpers for the info providers that use public web APIs
//	(JSON over HTTPS): Cover Art Archive, LRCLIB, Last.fm and Deezer.
//
//	A provider implements Fetch(), which turns a request into candidate
//	results: texts (biography, lyrics...) or image URLs. The base class does
//	the IInfoProvider plumbing; image candidates are downloaded to a temporary
//	file when they are returned and must really be images.
//
//	The parsers are static functions so they can be tested without network.
#ifndef _OnlineInfoProvider_h_
#define _OnlineInfoProvider_h_

#include "InfoProvider.h"
#include <string>
#include <vector>

namespace OnlineInfo
{
	//HTTP GET on the shared WinInet session. Returns TRUE with the body only for HTTP 200.
	//status (optional) receives the HTTP status (0 when the request did not complete).
	BOOL HttpGet(LPVOID hInternet, const std::wstring& url, std::string& body, DWORD* status = NULL);
	//Downloads an image to fileName. FALSE unless it is HTTP 200 and a JPEG/PNG/GIF/BMP of a useful size.
	BOOL HttpGetImage(LPVOID hInternet, const std::wstring& url, LPCTSTR fileName);
	BOOL IsImageData(const std::string& data);

	std::wstring Utf8ToWide(const std::string& s);
	//Percent-encoded UTF-8 (for query string values).
	std::wstring UrlEncode(LPCWSTR value);
	//Case-insensitive (all scripts) equality.
	BOOL SameText(const std::wstring& a, const std::wstring& b);
	//Removes HTML tags, decodes entities, normalizes line breaks and trims.
	std::wstring HtmlToText(const std::wstring& html);
	//Two-letter ISO 639-1 code of the user's UI language ("en" when unknown).
	std::wstring UserLanguageCode();
}

class OnlineInfoProvider : public IInfoProvider
{
public:
	struct Candidate
	{
		Candidate():bImage(FALSE)	{}
		BOOL bImage;
		std::wstring value;			//Text, or image URL
		std::wstring source;		//Shown as additional info (e.g. the page it came from)
	};

	OnlineInfoProvider();
	virtual ~OnlineInfoProvider();

	//IInfoProvider
	virtual BOOL OpenRequest(const Request& request);
	virtual BOOL GetNextResult(Result& result);
	virtual const Request& GetRequest()							{return m_request;}
	virtual BOOL AddResult(const Result& result)				{return FALSE;}
	virtual BOOL DeleteResult()									{return FALSE;}
	virtual void SetInternetHandle(LPVOID hInternet)			{m_hNet = hInternet;}
	virtual LPVOID GetInternetHandle() const					{return m_hNet;}
	virtual LPCTSTR GetModuleInfo(ModuleInfo mi) const;

	//IConfigurable (no settings by default)
	virtual BOOL GetSettingInfo(INT settingIndex, IConfigurable::SettingInfo& setting) const	{return FALSE;}
	virtual INT GetIntSetting(INT settingIndex) const					{return 0;}
	virtual LPCTSTR GetLPCTSTRSetting(INT settingIndex) const			{return NULL;}
	virtual void SetIntSetting(INT settingIndex, INT intVal)			{}
	virtual void SetLPCTSTRSetting(INT settingIndex, LPCTSTR strVal)	{}
	virtual void UpdateConfiguration()									{}

protected:
	//Network work happens here (called on the info download worker thread).
	virtual BOOL Fetch(const Request& request, std::vector<Candidate>& candidates) = 0;
	//IPI_UniqueID, IPI_Name, IPI_Description, IPI_HomePage
	virtual LPCTSTR GetInfo(ModuleInfo mi) const = 0;
	//Clone helper: copies the internet handle and the settings.
	IInfoProvider* FinishClone(IInfoProvider* pClone) const;

	LPVOID m_hNet;

private:
	Request m_request;
	std::wstring m_artist, m_album, m_title;
	std::vector<Candidate> m_candidates;
	size_t m_next;
	BOOL m_bFetched;
	std::wstring m_resultMain, m_resultSource;
	TCHAR m_tempFile[MAX_PATH];
};

//=== Album covers: MusicBrainz release-group search + Cover Art Archive.
class CoverArtArchiveProvider : public OnlineInfoProvider
{
public:
	virtual IInfoProvider* Clone() const		{return FinishClone(new CoverArtArchiveProvider);}
	virtual BOOL CanHandle(ServiceEnum service) const	{return service == SRV_AlbumImage;}
	//MusicBrainz release-group IDs of good matches (score >= 90), best first.
	static void ParseReleaseGroups(const std::string& json, std::vector<std::wstring>& ids, size_t maxIDs);
	static std::wstring BuildSearchURL(LPCWSTR artist, LPCWSTR album);
protected:
	virtual BOOL Fetch(const Request& request, std::vector<Candidate>& candidates);
	virtual LPCTSTR GetInfo(ModuleInfo mi) const;
};

//=== Lyrics: LRCLIB (lrclib.net).
class LrclibProvider : public OnlineInfoProvider
{
public:
	virtual IInfoProvider* Clone() const		{return FinishClone(new LrclibProvider);}
	virtual BOOL CanHandle(ServiceEnum service) const	{return service == SRV_TrackLyrics;}
	//Plain lyrics of the search results, exact artist/title matches first.
	static void ParseSearch(const std::string& json, LPCWSTR artist, LPCWSTR title, std::vector<std::wstring>& lyrics);
protected:
	virtual BOOL Fetch(const Request& request, std::vector<Candidate>& candidates);
	virtual LPCTSTR GetInfo(ModuleInfo mi) const;
};

//=== Artist biographies and album descriptions: Last.fm API 2.0 (needs an API key).
class LastFmApiProvider : public OnlineInfoProvider
{
public:
	LastFmApiProvider();
	virtual IInfoProvider* Clone() const		{return FinishClone(new LastFmApiProvider);}
	virtual BOOL CanHandle(ServiceEnum service) const;
	//Setting 0: API key (https://www.last.fm/api/account/create)
	virtual BOOL GetSettingInfo(INT settingIndex, IConfigurable::SettingInfo& setting) const;
	virtual LPCTSTR GetLPCTSTRSetting(INT settingIndex) const;
	virtual void SetLPCTSTRSetting(INT settingIndex, LPCTSTR strVal);
	//The "content" text of artist.getinfo (bio) or album.getinfo (wiki); empty when none.
	static std::wstring ParseInfo(const std::string& json, BOOL bArtist, std::wstring* pageURL);
protected:
	virtual BOOL Fetch(const Request& request, std::vector<Candidate>& candidates);
	virtual LPCTSTR GetInfo(ModuleInfo mi) const;
private:
	std::wstring m_apiKey;
};

//=== Artist pictures: Deezer public API.
class DeezerProvider : public OnlineInfoProvider
{
public:
	virtual IInfoProvider* Clone() const		{return FinishClone(new DeezerProvider);}
	virtual BOOL CanHandle(ServiceEnum service) const	{return service == SRV_ArtistImage;}
	//Picture URLs of the matching artists (exact name matches first; placeholders skipped).
	static void ParseArtists(const std::string& json, LPCWSTR artist, std::vector<std::wstring>& pictures, size_t maxPictures);
protected:
	virtual BOOL Fetch(const Request& request, std::vector<Candidate>& candidates);
	virtual LPCTSTR GetInfo(ModuleInfo mi) const;
};

#endif
