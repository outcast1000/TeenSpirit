//	/*
// 	*
// 	* Copyright (C) 2003-2010 Alexandros Economou
//	*
//	* This file is part of Jaangle (http://www.jaangle.com)
// 	*
// 	* This Program is free software; you can redistribute it and/or modify
// 	* it under the terms of the GNU General Public License as published by
// 	* the Free Software Foundation; either version 2, or (at your option)
// 	* any later version.
// 	*
// 	* This Program is distributed in the hope that it will be useful,
// 	* but WITHOUT ANY WARRANTY; without even the implied warranty of
// 	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// 	* GNU General Public License for more details.
// 	*
// 	* You should have received a copy of the GNU General Public License
// 	* along with GNU Make; see the file COPYING. If not, write to
// 	* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
// 	* http://www.gnu.org/copyleft/gpl.html
// 	*
//	*/ 

#include "stdafx.h"
#include "LastFMProvider.h"
#include <memory>
#include "cStringUtils.h"
#include "stlStringUtils.h"
#include "WebPageUtilities.h"

#ifdef _USRDLL
std::vector<IInfoProvider*>	s_createdIPs;
extern "C" __declspec(dllexport) UINT GetInfoProviderCount()
{
	return 1;
}
extern "C" __declspec(dllexport) IInfoProvider* CreateInfoProvider(UINT idx)
{
	if (idx == 0)
	{
		IInfoProvider* pIP = new LastFMProvider;
		s_createdIPs.push_back(pIP);
		return pIP;
	}
	return NULL;
}
extern "C" __declspec(dllexport) BOOL DeleteInfoProvider(IInfoProvider* pIP)
{
	std::vector<IInfoProvider*>::iterator it = s_createdIPs.begin();
	for (; it != s_createdIPs.end(); it++)
	{
		if (*it == pIP)
		{
			delete pIP;
			s_createdIPs.erase(it);
			return TRUE;
		}
	}
	return FALSE;
}
#endif

LPCTSTR cLastFMServers[] =
{
	_T("English"),
	_T("Deutsch"),
	_T("Español"),
	_T("Français"),
	_T("Italiano"),
	_T("Polski"),
	_T("Português"),
	_T("Svenska"),
	_T("Türkçe"),
	_T("Руccкий"),
	_T("日本語"),
	_T("简体中文"),
	NULL
};

LPCTSTR cLastFMServersURL[] = 
{
	_T("www.last.fm"),
	_T("www.lastfm.de"),
	_T("www.lastfm.es"),
	_T("www.lastfm.fr"),
	_T("www.lastfm.it"),
	_T("www.lastfm.pl"),
	_T("www.lastfm.com.br"),
	_T("www.lastfm.se"),
	_T("www.lastfm.com.tr"),
	_T("www.lastfm.ru"),
	_T("www.lastfm.jp"),
	_T("cn.last.fm"),
	NULL
};

LPCTSTR cLastFMProviderSettings[] = 
{
	_T("Language"),
	NULL
};


#ifdef _UNITTESTING
const int lastSizeOf = 668;

BOOL LastFMProvider::UnitTest()
{
	if (lastSizeOf != sizeof(LastFMProvider))
		TRACE(_T("TestLastFMProvider. Object Size Changed. Was: %d - Is: %d\r\n"), lastSizeOf, sizeof(LastFMProvider));

	LastFMProvider g;
	HINTERNET hNet = InternetOpen(_T("UnitTest"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);
	g.SetInternetHandle(hNet);
	//Successful
	Request req(IInfoProvider::SRV_ArtistBio);
	req.artist = _T("Afghan whigs");
	Result res;
	UNITTEST(g.OpenRequest(req));
	while (g.GetNextResult(res))
	{
		MessageBox(0, 
			res.main, 
			res.additionalInfo, 
			0);
	}

	req.service = IInfoProvider::SRV_ArtistImage;
	UNITTEST(g.OpenRequest(req));
	LPCTSTR tmpFile = _T("c:\\test.jpg");
	while (g.GetNextResult(res))
	{
		if (MoveFile(res.main, tmpFile))
			ShellExecute(0, _T("open"), tmpFile, 0,0,0);
		int ret = MessageBox(0, _T("more pictures?"), _T("???"), MB_YESNO);
		VERIFY(DeleteFile(tmpFile));
		if (ret != IDYES)
			break;
	}

	//=== Check the fallback

	req.artist = _T("Ρόδες");
	req.service = IInfoProvider::SRV_ArtistBio;
	g.m_lastFMServer = 7;
	UNITTEST(g.OpenRequest(req));
	while (g.GetNextResult(res))
	{
		MessageBox(0, 
			res.main, 
			res.additionalInfo, 
			0);
	}
	g.m_lastFMServer = 0;


	//Misstagged
	req.service = IInfoProvider::SRV_ArtistBio;
	req.artist = _T("Ana Belen");
	UNITTEST(g.OpenRequest(req));
	while (g.GetNextResult(res))
	{
		MessageBox(0, 
			res.main, 
			res.additionalInfo, 
			0);
	}

	req.service = IInfoProvider::SRV_ArtistImage;
	UNITTEST(g.OpenRequest(req));
	while (g.GetNextResult(res))
	{
		if (MoveFile(res.main, tmpFile))
			ShellExecute(0, _T("open"), tmpFile, 0,0,0);
		int ret = MessageBox(0, _T("more pictures?"), _T("???"), MB_YESNO);
		VERIFY(DeleteFile(tmpFile));
		if (ret != IDYES)
			break;
	}

	//Bad
	req.service = IInfoProvider::SRV_ArtistBio;
	req.artist = _T("Γιάννης Αγγελάκας & οι επισκέπτες");
	UNITTEST(g.OpenRequest(req));
	while (g.GetNextResult(res))
	{
		MessageBox(0, 
			res.main, 
			res.additionalInfo, 
			0);
	}

	req.service = IInfoProvider::SRV_ArtistImage;
	UNITTEST(g.OpenRequest(req));
	while (g.GetNextResult(res))
	{
		if (MoveFile(res.main, tmpFile))
			ShellExecute(0, _T("open"), tmpFile, 0,0,0);
		int ret = MessageBox(0, _T("more pictures?"), _T("???"), MB_YESNO);
		VERIFY(DeleteFile(tmpFile));
		if (ret != IDYES)
			break;
	}


	g.SetInternetHandle(0);
	InternetCloseHandle(hNet);
	return TRUE;
}
#endif

//It may use info in various languages
static INT LastFMProviderInstances = 0;

LastFMProvider::LastFMProvider():
m_hNet(NULL),
m_lastFMServer(0),
m_curResult(-1),
m_request(SRV_First)
{
	//I ve change it because .tmp files were left in the app dir (when closing the application while downloading)
	TCHAR tmpPath[MAX_PATH];
	GetTempPath(MAX_PATH, tmpPath);
	_sntprintf(m_TempFile, MAX_PATH, _T("%slfm%d.tmp"), tmpPath, LastFMProviderInstances);
	LastFMProviderInstances++;
}

IInfoProvider* LastFMProvider::Clone() const
{
	IInfoProvider* pIP = new LastFMProvider;
	pIP->SetInternetHandle(GetInternetHandle());
	IConfigurableHelper::TransferConfiguration(*this, *pIP);
	return pIP;
}

LastFMProvider::~LastFMProvider()
{
	DeleteFile(m_TempFile);//Delete the file if it exists
}

BOOL LastFMProvider::CanHandle(ServiceEnum service) const
{
	switch (service)
	{
	case IInfoProvider::SRV_ArtistBio:
	case IInfoProvider::SRV_ArtistImage:
		return TRUE;
	}
	return FALSE;
}

BOOL LastFMProvider::OpenRequest(const Request& request)
{
	ASSERT(request.IsValid());
	if (!request.IsValid())
		return FALSE;

	switch (request.service)
	{
	case SRV_ArtistBio:
	case SRV_ArtistImage:
		m_artist = request.artist;
		m_request.artist = m_artist.c_str();
		break;
	default:
		return FALSE;
	}

	m_request.service = request.service;
	m_curResult = 0;
	return TRUE;
}

BOOL LastFMProvider::GetNextResult(Result& result)
{
	m_result.clear();
	result.service = m_request.service;
	switch (m_request.service)
	{
	case SRV_ArtistBio:
		GetArtistBio();
		break;
	case SRV_ArtistImage:
		GetArtistPicture();
		break;
	}
	if (m_result.empty())
		return FALSE;
	result.additionalInfo = cLastFMServersURL[m_lastFMServer];
	result.main = m_result.c_str();
	return TRUE;
}

BOOL LastFMProvider::GetLastFMServer(std::basic_string<TCHAR>& server, INT lastFMServer)
{
	ASSERT(m_lastFMServer >= 0 && m_lastFMServer < sizeof(cLastFMServersURL)/sizeof(LPCTSTR));
	if (m_lastFMServer >= 0 && m_lastFMServer < sizeof(cLastFMServersURL)/sizeof(LPCTSTR))
	{
		server = _T("http://");
		server += cLastFMServersURL[lastFMServer];
		return TRUE;
	}
	return FALSE;
}

//void LastFMProvider::FixURLParameter(std::tstring& str)
//{
//	//DWORD bfLen = 1000;
//	//TCHAR bf[1000];
//	//InternetCanonicalizeUrl(str.c_str(), bf, &bfLen, NULL);
//	//str = bf;
//
//	replace(str, _T("\""), _T("%22"));
//	replace(str, _T("&"), _T("%26"));
//	replace(str, _T(" "), _T("+"));
//}


BOOL LastFMProvider::IsWikiPageValid(LPCTSTR wikiPage)
{
	if (_tcsstr(wikiPage, _T("/+wiki/edit\">")) != NULL)
	{
		TRACE(_T("@2 LastFMProvider::IsWikiPageValid. Wiki requests addition."));
		return FALSE;
	}
	return TRUE;
}


BOOL LastFMProvider::GetArtistBio()
{
	if (m_curResult > 0)
		return FALSE;
	m_curResult++;
	std::basic_string<TCHAR> query;
	if (!GetLastFMServer(query, m_lastFMServer))
		return FALSE;
	std::basic_string<TCHAR> params = _T("/music/");
	std::basic_string<TCHAR> fixedParam;//(m_artist);
	URLEncode(fixedParam, m_artist.c_str());
	params += fixedParam;
	params += _T("/+wiki");

	query += params;

	std::wstring artistPage;
	if (!DownloadWebPageUnicode(artistPage, m_hNet, query.c_str()))
		return FALSE;
	if (!IsWikiPageValid(artistPage.c_str()))
	{
		if (m_lastFMServer == 0)
			return FALSE;
		if (!GetLastFMServer(query, 0))//Get The default server
			return FALSE;
		query += params;
		if (!DownloadWebPageUnicode(artistPage, m_hNet, query.c_str()))
			return FALSE;
		if (!IsWikiPageValid(artistPage.c_str()))
			return FALSE;
	}


	


	LPCTSTR start = _tcsstr(artistPage.c_str(), _T("<div id=\"wiki\">"));
	if (start == NULL)	
		return FALSE;
	start += 15;
	LPCTSTR end = _tcsstr(start, _T("</div"));
	if (end == NULL)	
		return FALSE;
	GetTextFromHtmlFragment(m_result, start, end);
	return TRUE;
}

BOOL LastFMProvider::GetArtistPicture()
{
	if (m_curResult == 0)
	{
		m_artistPictures.clear();
		std::basic_string<TCHAR> query;
		if (!GetLastFMServer(query, m_lastFMServer))
			return FALSE;
		query += _T("/music/");
		std::basic_string<TCHAR> fixedParam;//(m_artist);
		URLEncode(fixedParam, m_artist.c_str());
		//replace(fixedParam, _T("%20"), _T("+"));
		query += fixedParam;
		query += _T("/+images");

		std::string artistImagePage;
		if (!DownloadWebPage(artistImagePage, m_hNet, query.c_str()))
			return FALSE;
		const LPCSTR urlFind = "/126b/";
		const LPCSTR urlFindReplace = "/_/";
		LPCSTR Page = artistImagePage.c_str();
		LPCSTR start = Page;
		if (strstr(Page, "lastfm_bad_tag") != 0)
		{
			TRACE(_T("@1 LastFMProvider::GetArtistImage. Found bad artist tag\r\n"));
			start = NULL;
		}
		while (start != NULL)
		{
			start = strstr(start, urlFind);
			if (start != NULL)
			{
				start = strrchrex(Page, start, '"');
				if (start != NULL)
				{
					start++;
					LPCSTR end = strchr(start, '"');
					if (end != NULL)
					{
						const int urlSize = 1000;
						if (end - start < urlSize)
						{
							CHAR url[urlSize];
							strncpy(url, start, end - start);
							url[end - start] = 0;
							strReplaceInl(url, 500, urlFind, urlFindReplace);
							m_artistPictures.push_back(url);
							start = end;
						}
						else
						{
							start = NULL;
							TRACE(_T("@1 LastFMProvider::GetArtistImage. Parse Error: 2\r\n"));
						}
					}
					else
					{
						start = NULL;
						TRACE(_T("@1 LastFMProvider::GetArtistImage. Parse Error: 1\r\n"));
					}
				}
				else
					TRACE(_T("@1 LastFMProvider::GetArtistImage. Parse Error: 0\r\n"));
			}
			else
			{
				if (m_artistPictures.size() == 0)
					TRACE(_T("@1 LastFMProvider::GetArtistPicture. Can't find '%s'\r\n"), (LPCTSTR)CA2CT(urlFind));//Fixed 29-12-07
				else
					TRACE(_T("@3 LastFMProvider::GetArtistPicture. Found %d pictures\r\n"), m_artistPictures.size());//Fixed 29-12-07

			}
		}

	}
	else
		TRACE(_T("@3 LastFMProvider::ArtistPicture. Using Cached Page\r\n"));

	while (m_curResult < (INT)m_artistPictures.size())
	{
		if (DownloadToFile(m_TempFile, m_hNet, (LPCTSTR)CA2CT(m_artistPictures[m_curResult].c_str())))
		{
			//Get File size - If it is less than 2k then discard
			HANDLE f= CreateFile(m_TempFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (f != INVALID_HANDLE_VALUE)
			{
				DWORD fs = ::GetFileSize(f, NULL);
				CloseHandle(f);
				const INT cKeepStatsCleanFS1 = 13068;
				const INT cKeepStatsCleanFS2 = 13510;
				if (fs > 2000 && fs != cKeepStatsCleanFS1 && fs != cKeepStatsCleanFS2)
				{
					m_result = m_TempFile;
					m_curResult++;
					return TRUE;
				}
				else
					TRACE(_T("@3 LastFMProvider::ArtistPicture. Invalid Picture Size\r\n"));
			}
		}
		m_curResult++;
	}
	m_curResult++;
	return FALSE;
}


LPCTSTR LastFMProvider::GetModuleInfo(ModuleInfo mi) const
{
	switch (mi)
	{
	case IPI_UniqueID:		return _T("LAFM");
	case IPI_Name:			return _T("LastFM");
	case IPI_Author:		return _T("Alex Economou");
	case IPI_VersionStr:	return _T("2");
	case IPI_VersionNum:	return _T("2");
	case IPI_Description:	return _T("Downloads info from www.last.fm");
	case IPI_HomePage:		return _T("http://teenspirit.artificialspirit.com");
	case IPI_Email:			return _T("alex@artificialspirit.com");
	}
	return NULL;
}


//LPTSTR LastFMProvider::RetrieveBiography(LPSTR Page)
//{
//	ASSERT(Page != NULL);
//	if (strstr(Page, "Nothing has been written here yet.") != NULL)
//		return NULL;//Filter out empty pages
//	char* start = strstr(Page, "/forum/markup#artistwikitags");
//	if (start == NULL)	return NULL;
//	start = strstr(start++, "<div class=\"f\">");
//	if (start == NULL)	return NULL;
//	start = strstr(++start, "</div>");
//	if (start == NULL)	return NULL;
//	start = strstr(++start, "</div>");
//	if (start == NULL)	return NULL;
//	start = strstr(++start, "</div>");
//	if (start == NULL)	return NULL;
//	start += 6;
//	char* end = strstr(++start, "<div");
//	if (end == NULL)	return NULL;
//	return RetrieveHtmlText(start, end);
//}



BOOL LastFMProvider::GetURL(LPCSTR start, LPSTR bf, UINT bfLen)
{
	LPCSTR posEnd = strchr(start, '"');
	if (posEnd == NULL || posEnd - start >= (INT)bfLen)
		return FALSE;
	strncpy(bf, start, posEnd - start);
	bf[posEnd - start] = 0; 
	return TRUE;
}

BOOL LastFMProvider::GetSettingInfo(INT settingIndex, IConfigurable::SettingInfo& setting) const	
{
	if (settingIndex < sizeof(cLastFMProviderSettings) / sizeof(LPCTSTR) - 1)
		setting.name = cLastFMProviderSettings[settingIndex];

	switch (settingIndex)
	{
	case 0:
		setting.type = IConfigurable::COVT_Int;
		setting.availableValues = cLastFMServers;
		return TRUE;
	}
	return FALSE;
}
INT LastFMProvider::GetIntSetting(INT settingIndex) const				
{
	switch (settingIndex)
	{
	case 0:
		return m_lastFMServer;
	}
	ASSERT(0);
	return 0;
}

void LastFMProvider::SetIntSetting(INT settingIndex, INT intVal)			
{
	switch (settingIndex)
	{
	case 0:
		if (intVal >= 0 && intVal < sizeof(cLastFMServers) / sizeof(LPCTSTR))
			m_lastFMServer = intVal;
		break;
	default:
		ASSERT(0);
	}
}
