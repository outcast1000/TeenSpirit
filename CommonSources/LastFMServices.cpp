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
#include "StdAfx.h"
#include "LastFMServices.h"
#include "cMD5.h"
#include "SystemTimeFunctions.h"
#include "WebPageUtilities.h"
#include "stlStringUtils.h"

LPCTSTR sProtocolVer = _T("1.2.1");
LPCTSTR sClientID = _T("tsp");
LPCTSTR sClientVer = _T("1.0");
const INT sMaxTracksOnQueue = 10;

LastFMServices::LastFMServices(void):
m_state(ST_UnInitialized),
m_error(ERR_None),
m_hNet(0),
m_pThread(NULL),
m_bExitThread(FALSE),
m_lastHandShakeTickCount(0)
{
}

LastFMServices::~LastFMServices(void)
{
	//=== PrgAPI calls StopWorker first and does not delete us if it failed.
	if (!StopWorker(30000))
		TRACE(_T("@0 LastFMServices::~LastFMServices. Worker thread did not exit\r\n"));
}

BOOL LastFMServices::StopWorker(DWORD timeoutMs)
{
	CWinThread* pThread = NULL;
	{
		CSingleLock lock(&m_cs, TRUE);
		m_bExitThread = TRUE;	//=== Submit no longer starts a thread
		pThread = m_pThread;
	}
	m_wakeEvent.SetEvent();
	if (pThread == NULL)
		return TRUE;
	//The thread is never terminated. Network calls in progress return as soon as
	//the shared internet handle is closed (PrgAPI does that before stopping us).
	if (WaitForSingleObject(pThread->m_hThread, timeoutMs) != WAIT_OBJECT_0)
		return FALSE;
	delete pThread;
	CSingleLock lock(&m_cs, TRUE);
	m_pThread = NULL;
	return TRUE;
}

void LastFMServices::Init(HINTERNET hNet)
{
	ASSERT(hNet != NULL);
	m_hNet = hNet;
	m_state = ST_Idle;
}

void LastFMServices::SetUserName(LPCTSTR username)
{
	ASSERT(username != NULL);
	//=== The worker thread reads the credentials (HandShake): change them under the lock
	CSingleLock lock(&m_cs, TRUE);
	if (username != NULL)
		m_username = username;
	else
		m_username.clear();

}
void LastFMServices::SetMD5Password(LPCTSTR MD5password)
{
	ASSERT(MD5password != NULL);
	CSingleLock lock(&m_cs, TRUE);
	if (MD5password != NULL)
		m_MD5password = MD5password;
	else
		m_MD5password.clear();
}

BOOL LastFMServices::CheckLogin()
{
	ASSERT(!m_username.empty());
	ASSERT(!m_MD5password.empty());
	if (m_username.empty() || m_MD5password.empty())
		return FALSE;
	std::basic_string<TCHAR> ses, now, sub;
	if (HandShake(ses, now, sub))
	{
		ASSERT(!ses.empty());
		return TRUE;
	}
	return FALSE;
}




void LastFMServices::Submit(LastFMSubmitTrack& track)
{
	ASSERT(m_state != ST_UnInitialized);
	//Check the track
	if (track.IsValid() && track.length >= 30)
	{
		if (track.unixTimeStamp == 0) // Automatically fix the "start playing" date if needed
		{
			track.unixTimeStamp = GetCurrentUnixTimeStamp();
			if (track.length < 480)
				track.unixTimeStamp -= (track.length/2);
			else
				track.unixTimeStamp -= 240;
		}
		CSingleLock lock(&m_cs, TRUE);
		if (sMaxTracksOnQueue > m_jobs.size())
			m_jobs.push_back(track);
		if (m_pThread == NULL && !m_bExitThread)
		{
			m_pThread = AfxBeginThread(LastFMServices::WorkingThreadST, this,
				THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
			if (m_pThread != NULL)
			{
				m_pThread->m_bAutoDelete = FALSE;	//We wait for it and delete it
				m_pThread->ResumeThread();
			}
		}
		m_wakeEvent.SetEvent();

	}



}


BOOL LastFMServices::IsTrackAvailable()
{
	CSingleLock lock(&m_cs, TRUE);
	return m_jobs.size() > 0;
}
//BOOL LastFM::GetTrack(LastFMSubmitTrack& track)
//{
//	CSingleLock lock(&m_cs);
//	if (m_jobs.size() > 0)
//	{
//		track = *m_jobs.begin();
//		return TRUE;
//	}
//	return FALSE;
//}
//void LastFM::PopTrack()
//{
//	CSingleLock lock(&m_cs);
//	if (m_jobs.begin() != m_jobs.end())
//		m_jobs.erase(m_jobs.begin());
//}
//BOOL LastFM::PushTrack(LastFMSubmitTrack& track)
//{
//	CSingleLock lock(&m_cs);
//	if (sMaxTracksOnQueue > m_jobs.size())
//	{
//		m_jobs.push_back(track);
//		return TRUE;
//	}
//	return FALSE;
//}


UINT LastFMServices::WorkingThreadST(LPVOID pParam)
{
	ASSERT(pParam != NULL);
	return ((LastFMServices*)pParam)->WorkingThread();
}
UINT LastFMServices::WorkingThread()
{
	std::basic_string<TCHAR> sessionID;
	std::basic_string<TCHAR> nowPlayingURL;
	std::basic_string<TCHAR> submissionURL;
	INT failCounter = 0;
	while (!m_bExitThread)
	{
		//Wake up when a track is queued, on exit, or every second to retry.
		WaitForSingleObject(m_wakeEvent.m_hObject, 1000);
		if (m_bExitThread)
			break;
		if (!IsTrackAvailable())
			continue;
		if (sessionID.empty())
		{
			if (GetTickCount() - m_lastHandShakeTickCount > 30000)
			{
				HandShake(sessionID, nowPlayingURL, submissionURL);
				m_lastHandShakeTickCount = GetTickCount();
			}
		}
		if (!sessionID.empty())
		{
			size_t submitted = 0;
			if (SubmitTracks(sessionID.c_str(), submissionURL.c_str(), submitted))
			{
				failCounter = 0;
				//Remove only what was sent: tracks may have been queued meanwhile.
				CSingleLock lock(&m_cs, TRUE);
				for (size_t i = 0; i < submitted && !m_jobs.empty(); i++)
					m_jobs.pop_front();
			}
			else
			{
				failCounter++;
				if (failCounter == 3)
					sessionID.clear();
			}
		}
	}
	TRACE(_T("LastFM::WorkingThread. Exit\r\n"));
	return 0;
}

BOOL LastFMServices::HandShake(std::basic_string<TCHAR>& sessionID, 
							   std::basic_string<TCHAR>& nowPlayingURL, 
							   std::basic_string<TCHAR>& submissionURL)
{
	sessionID.clear();
	nowPlayingURL.clear();
	submissionURL.clear();
	std::basic_string<TCHAR> username, MD5password;
	{
		CSingleLock lock(&m_cs, TRUE);
		username = m_username;
		MD5password = m_MD5password;
	}
	if (username.empty() || MD5password.empty())
	{
		//TRACE(_T("@4 LastFMServices::HandShake. Empty Authorization.\r\n"));
		m_error = ERR_BadAuth;
		return FALSE;
	}
	SYSTEMTIME st;
	GetSystemTime(&st);
	UINT ts = SystemTime2UnixTimeStamp(st);
	LPCTSTR auth = _T("auth");
	cMD5 md5;
	CHAR authStr[100];
	_snprintf_s(authStr, _TRUNCATE, "%s%u", (LPCSTR)CT2CA(MD5password.c_str()), ts);
	CStringA authToken(md5.CalcMD5FromString(authStr));
	authToken.MakeLower();


	//http://post.audioscrobbler.com/?hs=true&p=1.2.1&c=<client-id>&v=<client-ver>&u=<user>&t=<timestamp>&a=<auth>
	TCHAR url[2000];
	_sntprintf_s(url, _TRUNCATE, 
		_T("http://post.audioscrobbler.com/?hs=true&p=%s&c=%s&v=%s&u=%s&t=%u&a=%.32s"),
		sProtocolVer,
		sClientID,
		sClientVer,
		(LPCTSTR)CA2CT(FormEncode(username.c_str()).c_str()),
		ts,
		(LPCTSTR)CA2CT(authToken));

	TRACE(_T("About to request: '%s'\r\n"), url);
	m_state = ST_Connecting;

	std::string page;
	if (DownloadWebPage(page, m_hNet, url))
	{
		std::string delimiter = "\n";
		std::string line;
		INT pos = getToken(page, 0, delimiter, line);
		INT count = 0;
		while (pos != -1)
		{
			switch (count)
			{
			case 0:
				m_errorString = line;
				if (line == "OK")
					m_error = ERR_None;
				else if (line == "BANNED")
					m_error = ERR_Banned;
				else if (line == "BADAUTH")
					m_error = ERR_BadAuth;
				else if (line == "BADTIME")
					m_error = ERR_BadTime;
				else 
					m_error = ERR_Failed;
				break;
			case 1:
				sessionID = (LPCTSTR)CA2CT(line.c_str());
				break;
			case 2:
				nowPlayingURL = (LPCTSTR)CA2CT(line.c_str());
				break;
			case 3:
				submissionURL = (LPCTSTR)CA2CT(line.c_str());
				break;
			default:
				break;
			}
			if (m_error != ERR_None)
				break;
			count++;
			pos = getToken(page, pos, delimiter, line);
		}
		if (m_error == ERR_None)
		{
			TRACE(_T("@4 LastFMServices::HandShake. success.\r\n"));
			m_state = ST_Idle;
		}
		else
		{
			TRACE(_T("@4 LastFMServices::HandShake. fail: '%s'.\r\n"), (LPCTSTR)CA2CT(GetErrorString()));
			m_state = ST_Error;
		}
	}
	else //---Download page failed
	{
		m_error = ERR_CommunicationError;
		m_state = ST_Error;
		TRACE(_T("@4 LastFMServices::HandShake. fail: Communication Error.\r\n"));
	}
	return m_state == ST_Idle;

}


//=== Percent-encoded UTF-8 of src, for a form field or URL query value
//=== (all reserved characters such as & + % = are encoded).
std::string LastFMServices::FormEncode(LPCTSTR src)
{
	std::wstring encoded;
	if (src == NULL || !URLEncodeW(encoded, src))
		return std::string();
	std::string ret;
	ret.reserve(encoded.size());
	for (size_t i = 0; i < encoded.size(); i++)
		ret += (CHAR) encoded[i];	//=== ASCII only
	return ret;
}




BOOL LastFMServices::SubmitTracks(LPCTSTR sessionID, LPCTSTR submissionURL, size_t& submittedCount)
{
	submittedCount = 0;
	ASSERT(sessionID != NULL && submissionURL != NULL);
	if (!(sessionID != NULL && submissionURL != NULL))
		return FALSE;
	ASSERT(sessionID[0] != 0 && submissionURL[0]);
	if (!(sessionID[0] != 0 && submissionURL[0]))
		return FALSE;
	TRACEST(_T("LastFMServices::SubmitTracks"));
	std::string postData;
	const INT bfSize = 1000;
	postData.reserve(bfSize);
	CHAR bf[bfSize];
	//s=<sessionID>
	//The Session ID string returned by the handshake request. Required
	_snprintf_s(bf, _TRUNCATE, "s=%s&", (LPCSTR)CT2CA(sessionID));
	postData = bf;
	{
		CSingleLock lock(&m_cs, TRUE);
		submittedCount = m_jobs.size();
		std::list<LastFMSubmitTrack>::const_iterator it = m_jobs.begin();
		UINT i = 0;
		for (;it != m_jobs.end(); it++)
		{
			const LastFMSubmitTrack& track = *it;
			//a[0]=<artist>
			//The artist name. Required.
			_snprintf_s(bf, _TRUNCATE, "a[%d]=", i);
			bf[bfSize - 1] = 0;
			postData += bf;
			postData += FormEncode(track.artist.c_str());
			postData += '&';
			//t[0]=<track>
			//The track title. Required.
			_snprintf_s(bf, _TRUNCATE, "t[%d]=", i);
			bf[bfSize - 1] = 0;
			postData += bf;
			postData += FormEncode(track.track.c_str());
			postData += '&';
			//i[0]=<time>
			//The time the track started playing, in UNIX timestamp format. 
			//This must be in the UTC time zone, and is required.
			_snprintf_s(bf, _TRUNCATE, "i[%d]=%u&", i, track.unixTimeStamp);
			bf[bfSize - 1] = 0;
			postData += bf;
			//o[0]=<source>
			//The source of the track. Required
			_snprintf_s(bf, _TRUNCATE, "o[%d]=P&", i);
			postData += bf;
			//r[0]=<rating>
			//A single character denoting the rating of the track
			_snprintf_s(bf, _TRUNCATE, "r[%d]=&", i);
			bf[bfSize - 1] = 0;
			postData += bf;

			//l[0]=<secs>
			//The length of the track in seconds. Required when the source is P, optional otherwise.
			_snprintf_s(bf, _TRUNCATE, "l[%d]=%d&", i, track.length);
			bf[bfSize - 1] = 0;
			postData += bf;

			//b[0]=<album>
			//The album title, or an empty string if not known.
			_snprintf_s(bf, _TRUNCATE, "b[%d]=", i);
			bf[bfSize - 1] = 0;
			postData += bf;
			postData += FormEncode(track.album.c_str());
			postData += '&';

			//n[0]=<tracknumber>
			//The position of the track on the album, or an empty string if not known.
			_snprintf_s(bf, _TRUNCATE, "n[%d]=%d&", i, track.trackNo);
			bf[bfSize - 1] = 0;
			postData += bf;

			//m[0]=<mb-trackid>
			//The MusicBrainz Track ID, or an empty string if not known.
			_snprintf_s(bf, _TRUNCATE, "m[%d]=&", i);
			bf[bfSize - 1] = 0;
			postData += bf;
			i++;
		}
	}
	postData.erase(postData.size() - 1);

	TRACE(_T("@4 LastFMServices::SubmitTracks. postingData '%s'\n"), (LPCTSTR)CA2CT(postData.c_str()));
	std::string page;
	if (DownloadWebPagePost(page, m_hNet, submissionURL, postData.c_str()))
	{
		m_errorString = page;
		if (strncmp(page.c_str(), "OK", 2) == 0)
		{
			//TRACE(_T("@4 LastFMServices::SubmitTracks. Track Succesfully submitted.\n"));
			m_error = ERR_None;
			m_state = ST_Idle;
		}
		else 
		{
			m_state = ST_Error;
			if (strncmp(page.c_str(), "BADSESSION", 10) == 0)
				m_error = ERR_BadSession;
			else 
				m_error = ERR_Failed;
			TRACE(_T("@1 LastFMServices::SubmitTracks. Track Failed (%d):'%s'.\n"), m_error, (LPCTSTR)CA2CT(GetErrorString()));
			TRACE(_T("@1 ... postedData '%s'.\n"), (LPCTSTR)CA2CT(postData.c_str()));
		}
	}
	else
	{
		m_state = ST_Error;
		m_error = ERR_CommunicationError;
		TRACE(_T("@4 LastFMServices::SubmitTracks. DownloadWebPage Failed\n"));
	}
	if (m_state == ST_Error)
		return FALSE;
	return TRUE;
}



