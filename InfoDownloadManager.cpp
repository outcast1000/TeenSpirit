#include "StdAfx.h"
#include "InfoDownloadManager.h"

#include "InformationProviders/InfoProviderFactory.h"
#include "InformationProviders/InfoProviderJob.h"

#include "SQLManager.h"
#include "AppStrings.h"

#include "resource.h"

#include "TagInfo.h"
#include "ReEntryLock.h"


#include "SystemTimeFunctions.h"

#include "StateManager.h"

#define TABLES_NAME_MAX_LEN 100
 
InfoDownloadManager::InfoDownloadManager(InfoProviderFactory& ipf, SQLManager& sqlManager, AppOptions& options,
		ITranslation& translation, std::unique_ptr<Host> host):
m_IPFactory(ipf),
m_sqlManager(sqlManager),
m_options(options),
m_translation(translation),
m_host(std::move(host)),
m_pMTQueue(NULL),
m_bWorkStopped(FALSE),
m_bWorkerStillRunning(FALSE),
m_bAcceptAll(FALSE),
m_bCancelAll(FALSE),
m_bReEntranceProtection(FALSE),
m_bOldInfoRequestsRead(FALSE)
{
}

InfoDownloadManager::~InfoDownloadManager()
{
	TRACEST(_T("InfoDownloadManager::~InfoDownloadManager"));
	if (StopBackgroundWork())
		delete m_pMTQueue;
	else
	{
		//=== A download thread still runs and posts to the queue: leave it (and the host) alive
		TRACE(_T("@0 InfoDownloadManager::~InfoDownloadManager. A download is still running\r\n"));
		m_host.release();
	}
}

MultiThreadJobQueue& InfoDownloadManager::GetQueue()
{
	if (m_pMTQueue == NULL)
	{
		m_pMTQueue = new MultiThreadJobQueue;
		m_pMTQueue->SetMaxThreads(3);
		m_pMTQueue->SetAssignJobDelay(1000);
	}
	return *m_pMTQueue;
}

LPCTSTR InfoDownloadManager::Str(INT resID)
{
	return AppStrings::Get(m_translation, resID);
}

LPCTSTR InfoDownloadManager::GetDefaultProvider(IInfoProvider::ServiceEnum service)
{
	//=== The built-in providers (InfoProviderFactory). These used to name the
	//=== retired services (LAFM, AMZN, GOLY), so every request fell back to a lookup.
	switch (service)
	{
	case IInfoProvider::SRV_ArtistBio:
	case IInfoProvider::SRV_AlbumReview:
		return _T("LFMA");		//Last.fm
	case IInfoProvider::SRV_ArtistImage:
		return _T("DEEZ");		//Deezer
	case IInfoProvider::SRV_AlbumImage:
		return _T("CAAR");		//Cover Art Archive
	case IInfoProvider::SRV_TrackLyrics:
		return _T("LRCL");		//LRCLIB
	}
	return NULL;
}
BOOL InfoDownloadManager::StopBackgroundWork()
{
	//Shutdown only reports a still-running thread the first time (it forgets
	//the threads), so remember the result.
	if (!m_bWorkStopped && m_pMTQueue != NULL)
	{
		m_bWorkStopped = TRUE;
		m_bWorkerStillRunning = !m_pMTQueue->Shutdown(MultiThreadJobQueue::cDefaultShutdownTimeout);
	}
	return !m_bWorkerStillRunning;
}
BOOL InfoDownloadManager::RequestArtistInfo(ArtistRecord& rec, LPCTSTR providerName, BOOL bForce, BOOL bSync)
{
	//TRACEST(_T("InfoDownloadManager::RequestArtistInfo"));
	ASSERT(rec.IsValid());
	if (!rec.IsValid())
		return FALSE;
	if (rec.name.empty())						return FALSE;
	if (rec.name[0] == '[')						return FALSE;
	IInfoProvider::Request req(IInfoProvider::SRV_ArtistBio);
	req.artist = rec.name.c_str();
	return RequestInfo(req, rec.ID, providerName, bForce, bSync, OPT_IM_AutoDlArtistBio, IIT_ArtistBio);
}
BOOL InfoDownloadManager::RequestArtistPic(ArtistRecord& rec, LPCTSTR providerName, BOOL bForce, BOOL bSync)
{
	//TRACEST(_T("InfoDownloadManager::RequestArtistPic"));
	ASSERT(rec.IsValid());
	if (!rec.IsValid())
		return FALSE;
	if (rec.name.empty())						return FALSE;
	if (rec.name[0] == '[')						return FALSE;
	IInfoProvider::Request req(IInfoProvider::SRV_ArtistImage);
	req.artist = rec.name.c_str();
	return RequestInfo(req, rec.ID, providerName, bForce, bSync, OPT_IM_AutoDlArtistPicture, IIT_ArtistPicture);
}
BOOL InfoDownloadManager::RequestAlbumInfo(AlbumRecord& rec, LPCTSTR artist, LPCTSTR providerName, BOOL bForce, BOOL bSync)
{
	//TRACEST(_T("InfoDownloadManager::RequestAlbumInfo"));
	ASSERT(artist != NULL);
	ASSERT(rec.IsValid());
	if (!rec.IsValid())							return FALSE;
	if (rec.name.empty())						return FALSE;
	if (rec.name[0] == '[')						return FALSE;
	if (artist == NULL)							return FALSE;
	if (artist[0] == '[')						return FALSE;
	IInfoProvider::Request req(IInfoProvider::SRV_AlbumReview);
	req.artist = artist;
	req.album = rec.name.c_str();
	return RequestInfo(req, rec.ID, providerName, bForce, bSync, OPT_IM_AutoDlAlbumReview, IIT_AlbumReview);
}
BOOL InfoDownloadManager::RequestAlbumPic(AlbumRecord& rec, LPCTSTR artist, LPCTSTR providerName, BOOL bForce, BOOL bSync)
{
	//TRACEST(_T("InfoDownloadManager::RequestAlbumPic"));
	ASSERT(rec.IsValid());
	if (!rec.IsValid())							return FALSE;
	if (rec.name.empty())						return FALSE;
	if (rec.name[0] == '[')						return FALSE;
	if (artist == NULL)							return FALSE;
	if (artist[0] == '[')						return FALSE;
	IInfoProvider::Request req(IInfoProvider::SRV_AlbumImage);
	req.artist = artist;
	req.album = rec.name.c_str();
	return RequestInfo(req, rec.ID, providerName, bForce, bSync, OPT_IM_AutoDlAlbumPicture, IIT_AlbumPicture);
}
BOOL InfoDownloadManager::RequestLyrics(TrackRecord& rec, LPCTSTR artist, LPCTSTR providerName, BOOL bForce, BOOL bSync)
{
	//TRACEST(_T("InfoDownloadManager::RequestLyrics"));
	ASSERT(artist != NULL);
	ASSERT(rec.IsValid());
	if (!rec.IsValid())							return FALSE;

	if (rec.name.empty())						return FALSE;
	if (rec.name[0] == '[')						return FALSE;
	if (artist == NULL)							return FALSE;
	if (artist[0] == '[')						return FALSE;

	m_bCancelAll = FALSE;
	IInfoProvider::Request req(IInfoProvider::SRV_TrackLyrics);
	req.artist = artist;
	req.title = rec.name.c_str();
	return RequestInfo(req, rec.ID, providerName, bForce, bSync, OPT_Unknown, IIT_TrackLyrics);
}
BOOL InfoDownloadManager::RequestTablatures(TrackRecord& rec, LPCTSTR artist, LPCTSTR providerName, BOOL bForce, BOOL bSync)
{
	//TRACEST(_T("InfoDownloadManager::RequestTablatures"));
	ASSERT(artist != NULL);
	ASSERT(rec.IsValid());
	if (!rec.IsValid())							return FALSE;

	if (rec.name.empty())						return FALSE;
	if (rec.name[0] == '[')						return FALSE;
	if (artist == NULL)							return FALSE;
	if (artist[0] == '[')						return FALSE;

	IInfoProvider::Request req(IInfoProvider::SRV_TrackTablatures);
	req.artist = artist;
	req.title = rec.name.c_str();
	return RequestInfo(req, rec.ID, providerName, bForce, bSync, OPT_Unknown, IIT_TrackTablatures);
}

void InfoDownloadManager::ResetArtistCache(ArtistRecord& rec)
{
	TRACEST(_T("InfoDownloadManager::ResetArtistCache"));
	//=== Reset Session Cache
	IInfoProvider::Request request(IInfoProvider::SRV_ArtistBio);
	request.artist = rec.name.c_str();
	UINT hash = CalcRequestHash(request);
	std::set<UINT>::iterator it = m_RequestHashes.find(hash);
	if (it != m_RequestHashes.end())
		m_RequestHashes.erase(it);
	request.service = IInfoProvider::SRV_ArtistImage;
	hash = CalcRequestHash(request);
	it = m_RequestHashes.find(hash);
	if (it != m_RequestHashes.end())
		m_RequestHashes.erase(it);
	////=== Reset DB Cache
	//SQLManager* pSM = PRGAPI()->GetSQLManager();
	//pSM->DeleteInfoRecordResult(rec.ID, IIT_ArtistPicture);
	//pSM->DeleteInfoRecordResult(rec.ID, IIT_ArtistBio);
}
void InfoDownloadManager::ResetAlbumCache(AlbumRecord& rec, LPCTSTR artist)
{
	TRACEST(_T("InfoDownloadManager::ResetAlbumCache"));
	//=== Reset Session Cache
	IInfoProvider::Request request(IInfoProvider::SRV_AlbumReview);
	request.artist = artist;
	request.album = rec.name.c_str();
	UINT hash = CalcRequestHash(request);
	std::set<UINT>::iterator it = m_RequestHashes.find(hash);
	if (it != m_RequestHashes.end())
		m_RequestHashes.erase(it);
	request.service = IInfoProvider::SRV_AlbumImage;
	hash = CalcRequestHash(request);
	it = m_RequestHashes.find(hash);
	if (it != m_RequestHashes.end())
		m_RequestHashes.erase(it);
	//=== Reset DB Cache
	//SQLManager* pSM = PRGAPI()->GetSQLManager();
	//pSM->DeleteInfoRecordResult(rec.ID, IIT_AlbumPicture);
	//pSM->DeleteInfoRecordResult(rec.ID, IIT_AlbumReview);
}

void InfoDownloadManager::ResetTrackCache(TrackRecord& rec, LPCTSTR artist)
{
	TRACEST(_T("InfoDownloadManager::ResetTrackCache"));
	//=== Reset Session Cache
	IInfoProvider::Request request(IInfoProvider::SRV_TrackLyrics);
	request.artist = artist;
	request.title = rec.name.c_str();
	UINT hash = CalcRequestHash(request);
	std::set<UINT>::iterator it = m_RequestHashes.find(hash);
	if (it != m_RequestHashes.end())
		m_RequestHashes.erase(it);
	request.service = IInfoProvider::SRV_TrackTablatures;
	hash = CalcRequestHash(request);
	it = m_RequestHashes.find(hash);
	if (it != m_RequestHashes.end())
		m_RequestHashes.erase(it);
	//=== Reset DB Cache
	//SQLManager* pSM = PRGAPI()->GetSQLManager();
	//pSM->DeleteInfoRecordResult(rec.ID, IIT_TrackLyrics);
	//pSM->DeleteInfoRecordResult(rec.ID, IIT_TrackTablatures);

}



UINT InfoDownloadManager::CalcStringHash(LPCTSTR str)
{
	UINT hash = 0;
	if (str != NULL)
	{
		UINT paramsLen = _tcslen(str);
		for (UINT i = 0; i < paramsLen; i++)
			hash += str[i] * i;
	}
	return hash;
}

UINT InfoDownloadManager::CalcRequestHash(const IInfoProvider::Request& request)
{
	UINT hash = (UINT) request.service;
	hash += CalcStringHash(request.artist);
	hash += CalcStringHash(request.album);
	hash += CalcStringHash(request.title);
	return hash;
}

BOOL InfoDownloadManager::IsFirstRequest(const IInfoProvider::Request& request)
{
	ASSERT(request.IsValid());
	UINT hash = CalcRequestHash(request);
	//TRACE(_T("InfoDownloadManager::IsFirstRequestInThisSession Art: %s Hash: %d\r\n"), request.artist, hash);

	if (m_bOldInfoRequestsRead == FALSE)
	{
		TRACEST(_T("InfoDownloadManager::IsFirstRequest Getting old requests"));
		m_bOldInfoRequestsRead = TRUE;
		SQLManager* pSM = &m_sqlManager;
		UINT reqLimit = GetCurrentUnixTimeStamp() - 10 * 24 * 3600;
		pSM->DeleteInfoRequestsOlderThan(reqLimit);
		pSM->GetInfoRequests(m_RequestHashes);
	}
	if (m_RequestHashes.find(hash) != m_RequestHashes.end())
		return FALSE;
	m_sqlManager.AddInfoRequest(GetCurrentUnixTimeStamp(), hash);
	m_RequestHashes.insert(hash);
	return TRUE;
}


BOOL InfoDownloadManager::RequestInfo(IInfoProvider::Request& request, 
			UINT helpID, LPCTSTR providerName, BOOL bForce, BOOL bSync, 
			AppOptionsEnum optionToCheck, InfoItemTypeEnum iit)
{
	ASSERT(request.IsValid());
	if (!bForce)
	{
		//=== Check if the user has select not to download pictures/info
		if (optionToCheck > 0 && !m_options.Get(optionToCheck))
			return FALSE;	//No Need
		//=== Check if we have already tried this request in this session
		if (!IsFirstRequest(request))
		{
			return FALSE;
		}
		//=== Check if we have previously tried this request and we have failed (Database)
		//if (!IsFirstRequestInTheLastSessions())
		//InfoRecordRequest irr(helpID, iit);
		//if (pSM->GetInfoRecordRequestRecord(irr))
		//{
		//	UINT curTime = GetCurrentUnixTimeStamp();
		//	if (curTime - irr.requestTimeStamp < 10 * 24 * 3600)
		//	{
		//		return FALSE;//=== We have checked for such info less than 10 days before and it failed
		//	}
		//}
		//=== Result: Proceed to download.
	}
	TRACEST(_T("InfoDownloadManager::RequestInfo"), iit);

	//=== Clean Up the request
	static LPCTSTR const trimChars = _T(" ");
	std::tstring art;
	if (request.artist != NULL)
	{
		static LPCTSTR const cutChars = _T("()[]<>");
		//TRACE(_T("@2 InfoDownloadManager::RequestInfo. Filtering :'%s'\r\n"), request.artist);
		art = request.artist;
		//=== trim left
		art.erase(0, art.find_first_not_of(trimChars));
		//=== trim "the "
		if (_tcsnicmp(art.c_str(), _T("the "), 4) == 0)
			art.erase(0, 4);
		//=== Cut when find the first of the cut chars
		size_t pos = art.find_first_of(cutChars);
		if (pos != std::tstring::npos)
			art.erase(pos);
		//=== trim right
		art.erase(art.find_last_not_of(trimChars) + 1);
		//=== Filter Out the request if less than 3 chars
		if (art.size() < 3)
		{
			TRACE(_T("@2 InfoDownloadManager::RequestInfo. Artist Filtered Out '%s'\r\n"), request.artist);
			return FALSE;
		}
#ifdef _DEBUG
		if (_tcslen(request.artist) != art.size())
			TRACE(_T("@3 InfoDownloadManager::RequestInfo. Artist Filtered TO :'%s'\r\n"), art.c_str());
#endif
		request.artist = art.c_str();
	}
	std::tstring alb;
	if (request.album != NULL)
	{
		static LPCTSTR const cutChars = _T("()[]-<>");
		//TRACE(_T("@2 InfoDownloadManager::RequestInfo. Filtering :'%s'\r\n"), request.album);
		alb = request.album;
		//=== trim left
		alb.erase(0, alb.find_first_not_of(trimChars));
		//=== Cut when find the first of the cut chars
		size_t pos = alb.find_first_of(cutChars);
		if (pos != std::tstring::npos)
			alb.erase(pos);
		//=== trim right
		alb.erase(alb.find_last_not_of(trimChars) + 1);
		//=== Filter Out the request if less than 3 chars
		if (alb.size() < 3)
		{
			TRACE(_T("@2 InfoDownloadManager::RequestInfo. Album Filtered Out '%s'\r\n"), request.album);
			return FALSE;
		}
#ifdef _DEBUG
		if (_tcslen(request.album) != alb.size())
			TRACE(_T("@3 InfoDownloadManager::RequestInfo. Album Filtered TO :'%s'\r\n"), alb.c_str());
#endif
		request.album = alb.c_str();
	}
	std::tstring tit;
	if (request.title != NULL)
	{
		static LPCTSTR const cutChars = _T("()[]-<>");
		//TRACE(_T("@2 InfoDownloadManager::RequestInfo. Filtering :'%s'\r\n"), request.title);
		tit = request.title;
		//=== trim left
		tit.erase(0, tit.find_first_not_of(trimChars));
		//=== Cut when find the first of the cut chars
		size_t pos = tit.find_first_of(cutChars);
		if (pos != std::tstring::npos)
			tit.erase(pos);
		//=== trim right
		tit.erase(tit.find_last_not_of(trimChars) + 1);
		//=== Filter Out the request if less than 3 chars
		if (tit.size() < 3)
		{
			TRACE(_T("@2 InfoDownloadManager::RequestInfo. Title Filtered Out '%s'\r\n"), request.title);
			return FALSE;
		}
#ifdef _DEBUG
		if (_tcslen(request.title) != tit.size())
			TRACE(_T("@3 InfoDownloadManager::RequestInfo. Title Filtered TO :'%s'\r\n"), tit.c_str());
#endif
		request.title = tit.c_str();
	}


	if (providerName == NULL)
		providerName = GetDefaultProvider(request.service);

	IInfoProvider* pIP = NULL;
	if (providerName != NULL)
		pIP = m_IPFactory.CreateInfoProviderByUniqueID(providerName);
	if (pIP == NULL)
		pIP = m_IPFactory.CreateInfoProviderByService(request.service);
	if (pIP != NULL)
	{
		if (pIP->OpenRequest(request))
		{
			JobSP job(InfoProviderJob::CreateInfoProviderJob(pIP, helpID));
			if (job.get())
			{
				if (bSync)
				{
					//=== Run it here; the next ConsumeResult handles it (the result used to be dropped)
					job->Execute();
					m_syncFinished.push_back(job);
					return TRUE;
				}
				return GetQueue().AddJob(job);
			}
		}
		delete pIP;
	}
	TRACE(_T("@2 InfoDownloadManager::RequestInfo. Cannot create this job.\r\n"));
	return FALSE;
}

void InfoDownloadManager::NotifyUserForSuccess(LPCTSTR infoType, LPCTSTR reqInfo, LPCTSTR addInfo)
{
	ASSERT(infoType != NULL && reqInfo != NULL && addInfo != NULL);
	if (infoType != NULL && reqInfo != NULL && addInfo != NULL)
	{
		TCHAR msg[1000];
		_sntprintf(msg, 1000,
			_T("%s | %s: %s | %s | %s: %s"),
			Str(IDS_SUCCESS),
			Str(IDS_TYPE), infoType,
			reqInfo,
			Str(IDS_PROVIDER), addInfo);
		msg[999] = 0;
		m_host->NotifyUser(msg, SEV_InfoL1);
	}
}


BOOL InfoDownloadManager::HandleArtistInfo(UINT artID, const IInfoProvider::Result& result)
{
	//TRACEST(_T("InfoDownloadManager::HandleArtistInfo"));
	ASSERT(artID != 0 && result.IsValid());
	SQLManager* pSM = &m_sqlManager;
	ArtistRecord ar;
	if (pSM->GetArtistRecord(artID, ar))
	{
		pSM->AdjustArtistInfo(artID, IIT_ArtistBio, result.main);
		m_host->SendStateMessage(SM_InfoChanged);
		NotifyUserForSuccess(Str(IDS_ARTISTBIO),  
			ar.name.c_str(), 
			result.additionalInfo);
	}
	return FALSE;
}
BOOL InfoDownloadManager::HandleArtistPic(UINT artID, const IInfoProvider::Result& result)
{
	//TRACEST(_T("InfoDownloadManager::HandleArtistPic"));
	ASSERT(artID != 0 && result.IsValid());
	ArtistRecord ar;
	if (m_sqlManager.GetArtistRecord(artID, ar))
	{
		if (!m_host->AddArtistPicture(ar, result.main))
		{
			//It happens when the same picture exists
			return TRUE;//Request more results
		}
		m_host->SendStateMessage(SM_PictureChanged);
		NotifyUserForSuccess(Str(IDS_ARTISTPICTURE),  
			ar.name.c_str(), 
			result.additionalInfo);
	}
	return FALSE;//DO NOT REQUEST more results
}
BOOL InfoDownloadManager::HandleAlbumInfo(UINT albID, const IInfoProvider::Result& result)
{
	//TRACEST(_T("InfoDownloadManager::HandleAlbumInfo"));
	ASSERT(albID != 0 && result.IsValid());
	SQLManager* pSM = &m_sqlManager;
	FullAlbumRecordSP rec;
	if (pSM->GetFullAlbumRecordByID(rec, albID))
	{
		pSM->AdjustAlbumInfo(albID, IIT_AlbumReview, result.main);
		m_host->SendStateMessage(SM_InfoChanged);
		TCHAR msg[500];
		_sntprintf(msg, 500, _T("%s: %s"), rec->artist.name.c_str(), rec->album.name.c_str());
		NotifyUserForSuccess(Str(IDS_ALBUMREVIEW),  
			msg, 
			result.additionalInfo);
	}
	return FALSE;
}
BOOL InfoDownloadManager::HandleAlbumPic(UINT albID, const IInfoProvider::Result& result)
{
	//TRACEST(_T("InfoDownloadManager::HandleAlbumPic"));
	ASSERT(albID != 0 && result.IsValid());
	FullAlbumRecordSP rec;
	if (m_sqlManager.GetFullAlbumRecordByID(rec, albID))
	{
		if (!m_host->AddAlbumPicture(*rec, result.main))
		{
			//It happens when the same picture exists
			return TRUE;//Request more results
		}
		m_host->SendStateMessage(SM_PictureChanged);
		TCHAR msg[500];
		_sntprintf(msg, 500, _T("%s: %s"), rec->artist.name.c_str(), rec->album.name.c_str());
		NotifyUserForSuccess(Str(IDS_ALBUMPICTURE),  
			msg, 
			result.additionalInfo);
	}
	return FALSE;//DO NOT REQUEST more results
}
BOOL InfoDownloadManager::HandleTrackInfo(UINT traID, const IInfoProvider::Result& result, InfoItemTypeEnum iit)
{
	//TRACEST(_T("InfoDownloadManager::HandleTrackInfo"));
	ASSERT(traID != 0 && result.IsValid());
	if (m_bCancelAll)
		return FALSE;
	SQLManager* pSM = &m_sqlManager;
	FullTrackRecordSP tr;
	if (pSM->GetFullTrackRecordByID(tr, traID))
	{
		if (iit != IIT_TrackLyrics && iit != IIT_TrackTablatures)
		{
			ASSERT(0);
			return FALSE;
		}
		BOOL bRefuse = FALSE;
		std::tstring info = result.main;
		if (!m_bAcceptAll)
		{
			TCHAR msg[1000];
			_sntprintf(msg, 1000,
				_T("%s: %s - %s\r\n%s: %s"),
				Str(IDS_TITLE), tr->track.name.c_str(), tr->artist.name.c_str(),
				Str(IDS_PROVIDER), result.additionalInfo);
			msg[999] = 0;
			switch (m_host->ConfirmTrackInfo(iit, Str(iit == IIT_TrackLyrics ? IDS_LYRICS : IDS_TABLATURES),
				msg, result.additionalInfo, info))
			{
			case Host::ANS_AcceptAll:
				m_bAcceptAll = TRUE;
				break;
			case Host::ANS_Accept:
				break;
			case Host::ANS_Retry:
				return TRUE;
			case Host::ANS_Cancel:
				bRefuse = TRUE;
				break;
			case Host::ANS_CancelAll:
				if (m_pMTQueue != NULL)
					m_pMTQueue->AbortJobs();
				m_bCancelAll = TRUE;
				bRefuse = TRUE;
				break;
			}

		}
		if (!bRefuse)
		{
			pSM->AdjustTrackInfo(traID, iit, info.c_str());
			if (iit == IIT_TrackLyrics)
			{
				TagInfo ti;
				ti.Lyrics = info;
				ti.validFields = TagInfo_Lyrics;
				m_host->TagTrack(tr->track.location.c_str(), ti);
			}
			m_host->SendStateMessage(SM_InfoChanged);
			TCHAR msg[500];
			_sntprintf(msg, 500, _T("%s: %s"), tr->artist.name.c_str(), tr->track.name.c_str());
			msg[499] = 0;
			NotifyUserForSuccess(AppStrings::ForInfoItemType(m_translation, iit),  
				msg, 
				result.additionalInfo);
		}
	}
	return FALSE;
}


BOOL InfoDownloadManager::NextFinishedJob(JobSP& job)
{
	if (!m_syncFinished.empty())
	{
		job = m_syncFinished.front();
		m_syncFinished.pop_front();
		return TRUE;
	}
	return m_pMTQueue != NULL && m_pMTQueue->RetrieveFinishedJob(job);
}

void InfoDownloadManager::ConsumeResult()
{
	if (m_pMTQueue == NULL && m_syncFinished.empty())
		return;
	ReEntryLock lock(m_bReEntranceProtection);
	if (lock.WasAlreadyLocked())
	{
		TRACE(_T("@2InfoDownloadManager::ConsumeResult. Re-entrance protection\r\n"));
		return;
	}
	JobSP job;
	while (NextFinishedJob(job))
	{
		InfoProviderJob* pInfoJob = dynamic_cast<InfoProviderJob*>(job.get());
		ASSERT(pInfoJob != NULL);
		if (pInfoJob == NULL)
			continue;
		IInfoProvider::Result result;
		IInfoProvider::Request request;
		UINT itemID = pInfoJob->GetItemID();
		pInfoJob->GetLastRequest(request);
		BOOL bSuccess = pInfoJob->GetLastResult(result);
		if (bSuccess)
		{
			TRACEST(_T("InfoDownloadManager::ConsumeResult. Success (p=srv)"), result.service);
			InfoItemTypeEnum iit;
			BOOL bContinueGettingResults = FALSE;
			switch (result.service)
			{
			case IInfoProvider::SRV_ArtistBio:
				iit = IIT_ArtistBio;
				bContinueGettingResults = HandleArtistInfo(pInfoJob->GetItemID(), result);
				break;
			case IInfoProvider::SRV_ArtistImage:
				iit = IIT_ArtistPicture;
				bContinueGettingResults = HandleArtistPic(pInfoJob->GetItemID(), result);
				break;
			case IInfoProvider::SRV_AlbumReview:
				iit = IIT_AlbumReview;
				bContinueGettingResults = HandleAlbumInfo(pInfoJob->GetItemID(), result);
				break;
			case IInfoProvider::SRV_AlbumImage:
				iit = IIT_AlbumPicture;
				bContinueGettingResults = HandleAlbumPic(pInfoJob->GetItemID(), result);
				break;
			case IInfoProvider::SRV_TrackLyrics:
				iit = IIT_TrackLyrics;
				bContinueGettingResults = HandleTrackInfo(pInfoJob->GetItemID(), result, iit);
				break;
			case IInfoProvider::SRV_TrackTablatures:
				iit = IIT_TrackTablatures;
				bContinueGettingResults = HandleTrackInfo(pInfoJob->GetItemID(), result, iit);
				break;
			default:
				ASSERT(0);//None handle it
			}
			if (bContinueGettingResults)
				GetQueue().AddJob(job);
			//else
			//	pSM->DeleteInfoRecordResult(pInfoJob->GetItemID(), iit);
		}
		else
		{
			TRACEST(_T("InfoDownloadManager::ConsumeResult. Failure (p=srv)"), result.service);
			InfoItemTypeEnum iit = IIT_Unknown;
			TCHAR msg[500];
			msg[0] = 0;
			switch (request.service)
			{
			case IInfoProvider::SRV_ArtistBio:
				iit = IIT_ArtistBio;
				_sntprintf(msg, 500, _T("%s"), request.artist);
				break;
			case IInfoProvider::SRV_ArtistImage:
				{
					iit = IIT_ArtistPicture;
					_sntprintf(msg, 500, _T("%s"), request.artist);
					//Add a Download "FAILURE" record in the database
				}
				break;
			case IInfoProvider::SRV_AlbumReview:
				iit = IIT_AlbumReview;
				_sntprintf(msg, 500, _T("%s - %s"), 
					request.artist, 
					request.album);
				break;
			case IInfoProvider::SRV_AlbumImage:
				{
					iit = IIT_AlbumPicture;
					_sntprintf(msg, 500, _T("%s - %s"), 
						request.artist, 
						request.album);

				}
				break;
			case IInfoProvider::SRV_TrackLyrics:
				iit = IIT_TrackLyrics;
				_sntprintf(msg, 500, _T("%s - %s"), 
					request.artist, 
					request.title);
				break;
			case IInfoProvider::SRV_TrackTablatures:
				iit = IIT_TrackTablatures;
				_sntprintf(msg, 500, _T("%s - %s"), 
					request.artist, 
					request.title);
				break;
			}
			msg[499] = 0;	//=== _sntprintf does not terminate a truncated string
			ASSERT(iit != IIT_Unknown);
			if (iit != IIT_Unknown)
			{
				////===Add A Failure Record
				//InfoRecordRequest irr(itemID, iit);
				//irr.result = IRRR_GeneralFailure;
				//irr.requestTimeStamp = GetCurrentUnixTimeStamp();
				//pSM->AddOrUpdateInfoRecordRequestRecord(irr);

				TCHAR bf[1000];
				_sntprintf(bf, 1000, _T("%s | %s: %s | %s"),
					Str(IDS_FAILURE),
					Str(IDS_TYPE), AppStrings::ForInfoItemType(m_translation, iit),
					msg);
				bf[999] = 0;
				m_host->NotifyUser(bf, SEV_Warning);
			}
		}
	}
	//=== "Accept all" lasts for the current batch of lyrics/tablatures: until no
	//=== job is waiting, running or finished but not handled yet (one snapshot)
	if (m_syncFinished.empty())
	{
		MultiThreadJobQueue::JobStatisics stats;
		if (m_pMTQueue != NULL)
			stats = m_pMTQueue->GetStatistics();
		if (stats.waiting + stats.working + stats.finished == 0)
			m_bAcceptAll = FALSE;
	}
}

BOOL InfoDownloadManager::IsWorking()		
{
	if (m_pMTQueue != NULL)
	{
		MultiThreadJobQueue::JobStatisics stats = m_pMTQueue->GetStatistics();
		return (stats.waiting + stats.working) > 0;
	}
	return FALSE;
}

INT InfoDownloadManager::GetPendingJobs()
{
	if (m_pMTQueue != NULL)
	{
		MultiThreadJobQueue::JobStatisics stats = m_pMTQueue->GetStatistics();
		return stats.waiting + stats.working;
	}
	return 0;

}

