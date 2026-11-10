//	InfoDownloadManager: requests (options, "already tried", name clean-up),
//	downloads on the job threads through a fake provider, and the handling of
//	the results on this thread (database, pictures, lyrics confirmation, messages).
#include "stdafx.h"
#include "TestFramework.h"
#include "InfoDownloadManager.h"
#include "InformationProviders/InfoProviderFactory.h"
#include "StateManager.h"
#include "SQLManager.h"
#include "AppOptions.h"
#include "AppSettings/IniAppSettings.h"
#include "ITranslation/EditableTranslation.h"
#include "resource.h"
#include <map>
#include <vector>
#include <deque>

namespace
{
	//Results per service, shared by the prototype and its clones (one per download).
	struct FakeResults
	{
		std::map<INT, std::vector<std::wstring> > results;	//by ServiceEnum
		INT requests;
		std::wstring lastArtist, lastAlbum, lastTitle;
		FakeResults(): requests(0)	{}
	};
	FakeResults gFake;

	class FakeProvider: public IInfoProvider
	{
	public:
		FakeProvider(): m_next(0), m_hNet(NULL)	{}
		virtual IInfoProvider* Clone() const						{return new FakeProvider;}
		virtual BOOL OpenRequest(const Request& request)
		{
			m_request = request;
			m_artist = request.artist ? request.artist : L"";
			m_album = request.album ? request.album : L"";
			m_title = request.title ? request.title : L"";
			m_request.artist = m_artist.c_str();
			m_request.album = request.album ? m_album.c_str() : NULL;
			m_request.title = request.title ? m_title.c_str() : NULL;
			m_next = 0;
			gFake.requests++;
			gFake.lastArtist = m_artist;
			gFake.lastAlbum = m_album;
			gFake.lastTitle = m_title;
			return TRUE;
		}
		virtual BOOL GetNextResult(Result& result)
		{
			const std::vector<std::wstring>& r = gFake.results[m_request.service];
			if (m_next >= r.size())
				return FALSE;
			m_current = r[m_next++];
			result.service = m_request.service;
			result.main = m_current.c_str();
			result.additionalInfo = L"Fake";
			return TRUE;
		}
		virtual const Request& GetRequest()							{return m_request;}
		virtual BOOL AddResult(const Result&)						{return FALSE;}
		virtual BOOL DeleteResult()									{return FALSE;}
		virtual BOOL CanHandle(ServiceEnum)	const					{return TRUE;}
		virtual LPCTSTR GetModuleInfo(ModuleInfo mi) const
		{
			switch (mi)
			{
			case IPI_UniqueID:		return L"FAKE";
			case IPI_Name:			return L"Fake";
			case IPI_VersionNum:	return L"1";
			}
			return L"";
		}
		virtual void SetInternetHandle(LPVOID hInternet)			{m_hNet = hInternet;}
		virtual LPVOID GetInternetHandle() const					{return m_hNet;}
		virtual BOOL GetSettingInfo(INT, SettingInfo&) const		{return FALSE;}
		virtual INT GetIntSetting(INT) const						{return 0;}
		virtual LPCTSTR GetLPCTSTRSetting(INT) const				{return NULL;}
		virtual void SetIntSetting(INT, INT)						{}
		virtual void SetLPCTSTRSetting(INT, LPCTSTR)				{}
		virtual void UpdateConfiguration()							{}
	private:
		Request m_request;
		std::wstring m_artist, m_album, m_title, m_current;
		size_t m_next;
		LPVOID m_hNet;
	};

	struct HostLog
	{
		std::vector<UINT> stateMessages;
		std::vector<std::pair<std::wstring, SEVERITY> > notifications;
		std::vector<std::wstring> artistPictures, albumPictures;
		std::vector<std::wstring> taggedPaths, taggedLyrics;
		std::deque<InfoDownloadManager::Host::Answer> answers;
		std::wstring editedText;		//replaces the text on Accept when set
		INT confirmations;
		std::wstring lastConfirmText, lastTypeName;
		BOOL bRejectFirstPicture;
		HostLog(): confirmations(0), bRejectFirstPicture(FALSE)	{}
	};

	class FakeHost: public InfoDownloadManager::Host
	{
	public:
		explicit FakeHost(HostLog& log): m_log(log)	{}
		virtual void SendStateMessage(UINT msg)						{m_log.stateMessages.push_back(msg);}
		virtual void NotifyUser(LPCTSTR msg, SEVERITY sev)			{m_log.notifications.push_back(std::make_pair(std::wstring(msg), sev));}
		virtual BOOL AddArtistPicture(const ArtistRecord&, LPCTSTR path)
		{
			m_log.artistPictures.push_back(path);
			if (m_log.bRejectFirstPicture && m_log.artistPictures.size() == 1)
				return FALSE;		//"already have it"
			return TRUE;
		}
		virtual BOOL AddAlbumPicture(const FullAlbumRecord&, LPCTSTR path)	{m_log.albumPictures.push_back(path); return TRUE;}
		virtual void TagTrack(LPCTSTR path, TagInfo& ti)			{m_log.taggedPaths.push_back(path); m_log.taggedLyrics.push_back(ti.Lyrics);}
		virtual Answer ConfirmTrackInfo(InfoItemTypeEnum, LPCTSTR typeName, LPCTSTR, LPCTSTR, std::tstring& text)
		{
			m_log.confirmations++;
			m_log.lastConfirmText = text;
			m_log.lastTypeName = typeName;
			Answer a = ANS_Cancel;
			if (!m_log.answers.empty())
			{
				a = m_log.answers.front();
				m_log.answers.pop_front();
			}
			if ((a == ANS_Accept || a == ANS_AcceptAll) && !m_log.editedText.empty())
				text = m_log.editedText;
			return a;
		}
	private:
		HostLog& m_log;
	};

	//Lets the job threads finish, handling the results as the UI timer does.
	BOOL WaitDone(InfoDownloadManager& idm)
	{
		for (INT i = 0; i < 1000; i++)
		{
			idm.ConsumeResult();
			if (idm.GetPendingJobs() == 0)
			{
				//Handling the last results can queue the job again (e.g. Retry): wait for that too
				idm.ConsumeResult();
				if (idm.GetPendingJobs() == 0)
					return TRUE;
			}
			Sleep(10);
		}
		return FALSE;
	}

	INT Count(const std::vector<UINT>& v, UINT x)
	{
		INT n = 0;
		for (size_t i = 0; i < v.size(); i++)
			if (v[i] == x)
				n++;
		return n;
	}

	std::wstring Info(SQLManager& sm, UINT id, InfoItemTypeEnum iit)
	{
		InfoRecord ir;
		BOOL bOk = FALSE;
		switch (iit)
		{
		case IIT_ArtistBio:		bOk = sm.GetArtistInfoRecord(id, iit, ir); break;
		case IIT_AlbumReview:	bOk = sm.GetAlbumInfoRecord(id, iit, ir); break;
		default:				bOk = sm.GetTrackInfoRecord(id, iit, ir); break;
		}
		return bOk ? ir.name : L"(none)";
	}

	void TestDefaultProviders()
	{
		CHECK(wcscmp(InfoDownloadManager::GetDefaultProvider(IInfoProvider::SRV_ArtistBio), L"LFMA") == 0);
		CHECK(wcscmp(InfoDownloadManager::GetDefaultProvider(IInfoProvider::SRV_ArtistImage), L"DEEZ") == 0);
		CHECK(wcscmp(InfoDownloadManager::GetDefaultProvider(IInfoProvider::SRV_AlbumReview), L"LFMA") == 0);
		CHECK(wcscmp(InfoDownloadManager::GetDefaultProvider(IInfoProvider::SRV_AlbumImage), L"CAAR") == 0);
		CHECK(wcscmp(InfoDownloadManager::GetDefaultProvider(IInfoProvider::SRV_TrackLyrics), L"LRCL") == 0);
		CHECK(InfoDownloadManager::GetDefaultProvider(IInfoProvider::SRV_TrackTablatures) == NULL);
		//The defaults exist and can handle their service (they named retired services)
		InfoProviderFactory ipf;
		for (INT srv = IInfoProvider::SRV_ArtistBio; srv <= IInfoProvider::SRV_TrackLyrics; srv++)
		{
			LPCTSTR id = InfoDownloadManager::GetDefaultProvider((IInfoProvider::ServiceEnum)srv);
			IInfoProvider* pIP = id != NULL ? ipf.CreateInfoProviderByUniqueID(id) : NULL;
			CHECK(pIP != NULL && pIP->CanHandle((IInfoProvider::ServiceEnum)srv));
			delete pIP;
		}
	}

	void TestDownloads()
	{
		std::wstring db = TempPath(L"downloads.db");
		DeleteFileW(db.c_str());
		SQLManager sm;
		CHECK(sm.Init(db.c_str()));
		std::wstring ini = TempPath(L"downloads.ini");
		DeleteFileW(ini.c_str());
		IniAppSettings settings;
		settings.Load(ini.c_str());
		AppOptions options(settings);
		options.Read();
		EditableTranslation tr;
		tr.SetString(IDS_SUCCESS, L"OK");
		tr.SetString(IDS_FAILURE, L"Failed");
		tr.SetString(IDS_TYPE, L"Type");
		tr.SetString(IDS_PROVIDER, L"Provider");
		tr.SetString(IDS_TITLE, L"Title");
		tr.SetString(IDS_ARTISTBIO, L"Biography");
		tr.SetString(IDS_ALBUMREVIEW, L"Review");
		tr.SetString(IDS_ARTISTPICTURE, L"Artist picture");
		tr.SetString(IDS_LYRICS, L"Lyrics");

		//An artist, album and two tracks
		CollectionRecord col;
		col.location = L"C:\\DownloadTests";
		col.type = CTYPE_LocalFolder;
		CHECK(sm.AddNewCollectionRecord(col));
		TagInfo ti;
		ti.Artist = L"The Download Artist (Live)";
		ti.Album = L"Download Album [Remaster]";
		ti.Title = L"Download Song";
		ti.validFields = TagInfo_Artist | TagInfo_Album | TagInfo_Title;
		CHECK(sm.AddNewTrackFromTagInfo(col.ID, L"C:\\DownloadTests\\Song.mp3", ti));
		ti.Title = L"Second Song";
		CHECK(sm.AddNewTrackFromTagInfo(col.ID, L"C:\\DownloadTests\\Second.mp3", ti));
		FullTrackRecordCollection tracks;
		TracksFilter tf;
		tf.CollectionID.match = NUMM_Exact;
		tf.CollectionID.val = col.ID;
		CHECK(sm.GetFullTrackRecordCollectionByTracksFilter(tracks, tf) && tracks.size() == 2);
		if (tracks.size() != 2)
			return;
		if (tracks[0]->track.name != L"Download Song")
			std::swap(tracks[0], tracks[1]);
		ArtistRecord artist = tracks[0]->artist;
		AlbumRecord album = tracks[0]->album;
		TrackRecord song = tracks[0]->track, second = tracks[1]->track;
		LPCTSTR artistName = tracks[0]->artist.name.c_str();

		InfoProviderFactory ipf;
		CHECK(ipf.RegisterInfoProvider(new FakeProvider, TRUE));
		gFake = FakeResults();
		gFake.results[IInfoProvider::SRV_ArtistBio].push_back(L"A biography");
		gFake.results[IInfoProvider::SRV_ArtistImage].push_back(L"C:\\pics\\first.jpg");
		gFake.results[IInfoProvider::SRV_ArtistImage].push_back(L"C:\\pics\\second.jpg");
		gFake.results[IInfoProvider::SRV_AlbumImage].push_back(L"C:\\pics\\album.jpg");
		gFake.results[IInfoProvider::SRV_TrackLyrics].push_back(L"First lyrics");
		gFake.results[IInfoProvider::SRV_TrackLyrics].push_back(L"Better lyrics");

		HostLog log;
		{
			InfoDownloadManager idm(ipf, sm, options, tr, std::unique_ptr<InfoDownloadManager::Host>(new FakeHost(log)));

			//Artist biography: the names are cleaned up for the provider
			CHECK(idm.RequestArtistInfo(artist, L"FAKE"));
			CHECK(WaitDone(idm));
			CHECK(gFake.lastArtist == L"Download Artist");		//no "The ", nothing from '('
			CHECK(Info(sm, artist.ID, IIT_ArtistBio) == L"A biography");
			CHECK(Count(log.stateMessages, SM_InfoChanged) == 1);
			CHECK(log.notifications.size() == 1 && log.notifications[0].second == SEV_InfoL1
				&& log.notifications[0].first == std::wstring(L"OK | Type: Biography | ") + artistName + L" | Provider: Fake");

			//The same request isn't repeated (this session, and for 10 days in the database)...
			INT requests = gFake.requests;
			CHECK(!idm.RequestArtistInfo(artist, L"FAKE"));
			CHECK(gFake.requests == requests);
			//...unless forced
			CHECK(idm.RequestArtistInfo(artist, L"FAKE", TRUE));
			CHECK(WaitDone(idm));
			CHECK(gFake.requests == requests + 1);

			//Turned off in the options: only forced requests
			options.Set(OPT_IM_AutoDlAlbumPicture, 0);
			CHECK(!idm.RequestAlbumPic(album, artistName, L"FAKE"));
			CHECK(log.albumPictures.empty());
			CHECK(idm.RequestAlbumPic(album, artistName, L"FAKE", TRUE));
			CHECK(WaitDone(idm));
			CHECK(gFake.lastAlbum == L"Download Album");		//nothing from '['
			CHECK(log.albumPictures.size() == 1 && log.albumPictures[0] == L"C:\\pics\\album.jpg");
			CHECK(Count(log.stateMessages, SM_PictureChanged) == 1);

			//A picture we already have: the next result is tried
			log.bRejectFirstPicture = TRUE;
			CHECK(idm.RequestArtistPic(artist, L"FAKE"));
			CHECK(WaitDone(idm));
			CHECK(log.artistPictures.size() == 2 && log.artistPictures[1] == L"C:\\pics\\second.jpg");
			CHECK(Count(log.stateMessages, SM_PictureChanged) == 2);

			//Nothing found: a warning
			log.notifications.clear();
			CHECK(idm.RequestAlbumInfo(album, artistName, L"FAKE"));
			CHECK(WaitDone(idm));
			CHECK(log.notifications.size() == 1 && log.notifications[0].second == SEV_Warning
				&& log.notifications[0].first == L"Failed | Type: Review | Download Artist - Download Album");
			CHECK(Info(sm, album.ID, IIT_AlbumReview) == L"(none)");

			//Too short after the clean-up: not requested
			ArtistRecord shortName = artist;
			shortName.name = L"The AB";
			requests = gFake.requests;
			CHECK(!idm.RequestArtistInfo(shortName, L"FAKE", TRUE));
			CHECK(gFake.requests == requests);

			//Lyrics: asked, then stored (as edited) and written to the file
			log.answers.push_back(InfoDownloadManager::Host::ANS_Accept);
			log.editedText = L"Edited lyrics";
			CHECK(idm.RequestLyrics(song, artistName, L"FAKE"));
			CHECK(WaitDone(idm));
			CHECK(log.confirmations == 1 && log.lastConfirmText == L"First lyrics" && log.lastTypeName == L"Lyrics");
			CHECK(Info(sm, song.ID, IIT_TrackLyrics) == L"Edited lyrics");
			CHECK(log.taggedPaths.size() == 1 && log.taggedPaths[0] == L"C:\\DownloadTests\\Song.mp3"
				&& log.taggedLyrics[0] == L"Edited lyrics");
			log.editedText.clear();

			//Retry shows the provider's next result; Cancel stores nothing
			log.answers.push_back(InfoDownloadManager::Host::ANS_Retry);
			log.answers.push_back(InfoDownloadManager::Host::ANS_Cancel);
			CHECK(idm.RequestLyrics(second, artistName, L"FAKE"));
			CHECK(WaitDone(idm));
			CHECK(log.confirmations == 3 && log.lastConfirmText == L"Better lyrics");
			CHECK(Info(sm, second.ID, IIT_TrackLyrics) == L"(none)");
			CHECK(log.taggedPaths.size() == 1);

			//Accept all: the rest of the batch is accepted without asking...
			log.answers.push_back(InfoDownloadManager::Host::ANS_AcceptAll);
			CHECK(idm.RequestLyrics(song, artistName, L"FAKE", TRUE));
			CHECK(idm.RequestLyrics(second, artistName, L"FAKE", TRUE));
			CHECK(WaitDone(idm));
			CHECK(log.confirmations == 4);
			CHECK(Info(sm, second.ID, IIT_TrackLyrics) == L"First lyrics");
			//...but not the next batch
			log.answers.push_back(InfoDownloadManager::Host::ANS_Cancel);
			CHECK(idm.RequestLyrics(second, artistName, L"FAKE", TRUE));
			CHECK(WaitDone(idm));
			CHECK(log.confirmations == 5);

			//Synchronous: downloaded now, handled by the next ConsumeResult (it was dropped)
			gFake.results[IInfoProvider::SRV_ArtistBio][0] = L"Sync biography";
			requests = gFake.requests;
			CHECK(idm.RequestArtistInfo(artist, L"FAKE", TRUE, TRUE));
			CHECK(gFake.requests == requests + 1 && idm.GetPendingJobs() == 0);
			CHECK(Info(sm, artist.ID, IIT_ArtistBio) == L"A biography");
			idm.ConsumeResult();
			CHECK(Info(sm, artist.ID, IIT_ArtistBio) == L"Sync biography");
		}

		//A new session remembers the requests tried in the last 10 days
		HostLog log2;
		InfoDownloadManager idm2(ipf, sm, options, tr, std::unique_ptr<InfoDownloadManager::Host>(new FakeHost(log2)));
		INT requests = gFake.requests;
		CHECK(!idm2.RequestArtistPic(artist, L"FAKE"));
		CHECK(gFake.requests == requests);
	}
}

void RunDownloadTests()
{
	wprintf(L"Download tests\n");
	TestDefaultProviders();
	TestDownloads();
}
