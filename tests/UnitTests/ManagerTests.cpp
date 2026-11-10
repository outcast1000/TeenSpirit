//	CollectionManager (collection scans on the background thread, messages
//	delivered on the UI thread) and LocalPictureManager (artist/album pictures
//	from the database, the picture storage and the album folders).
#include "stdafx.h"
#include "TestFramework.h"
#include "CollectionManager.h"
#include "LocalPictureManager.h"
#include "StateManager.h"
#include "SQLManager.h"
#include "AppOptions.h"
#include "AppSettings/IniAppSettings.h"
#include "ITranslation/EditableTranslation.h"
#include "InformationProviders/InfoProviderFactory.h"
#include "FileTagger.h"
#include "resource.h"
#include <shlwapi.h>
#include <vector>
#include <algorithm>

namespace
{
	void Touch(const std::wstring& path, const char* content = "x")
	{
		HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD written = 0;
		if (h != INVALID_HANDLE_VALUE)
		{
			WriteFile(h, content, (DWORD)strlen(content), &written, NULL);
			CloseHandle(h);
		}
	}

	//An empty folder <TEMP>\jaangle-tests\<name>\ (emptied if it exists).
	std::wstring FreshDir(LPCWSTR name)
	{
		std::wstring dir = TempPath(name);
		std::wstring from = dir + L'\0';
		SHFILEOPSTRUCTW op = {0};
		op.wFunc = FO_DELETE;
		op.pFrom = from.c_str();
		op.fFlags = FOF_NO_UI;
		SHFileOperationW(&op);
		CreateDirectoryW(dir.c_str(), NULL);
		return dir + L"\\";
	}

	//Tags from the file name: "<title>.<ext>".
	class FakeTagger: public FileTagger
	{
	public:
		FakeTagger(): m_readDepth(-1)	{}
		virtual UINT Read(LPCTSTR fileName, TagInfo& ti)
		{
			TCHAR title[MAX_PATH];
			_tcsncpy_s(title, PathFindFileName(fileName), _TRUNCATE);
			PathRemoveExtension(title);
			ti.Title = title;
			ti.Artist = L"Scan Artist";
			ti.Album = L"Scan Album";
			ti.Length = 100;
			return TagInfo_Title | TagInfo_Artist | TagInfo_Album | TagInfo_Length;
		}
		virtual UINT Write(LPCTSTR, TagInfo&)					{return 0;}
		virtual BOOL ClearTags(LPCTSTR)							{return FALSE;}
		virtual void SetConfig(Config conf, INT val)			{if (conf == CONF_ReadDepth) m_readDepth = val;}
		virtual INT GetConfig(Config conf)						{return conf == CONF_ReadDepth ? m_readDepth : 0;}
		INT m_readDepth;
	};

	struct CollectionHostLog
	{
		std::vector<UINT> stateMessages;
		std::vector<std::wstring> notifications;
		BOOL bNotifiedOffThread;
		FakeTagger* pTagger;
		CollectionHostLog(): bNotifiedOffThread(FALSE), pTagger(NULL)	{}
		INT Count(UINT msg) const	{return (INT)std::count(stateMessages.begin(), stateMessages.end(), msg);}
		INT Starting(LPCWSTR prefix) const
		{
			INT n = 0;
			for (size_t i = 0; i < notifications.size(); i++)
				if (notifications[i].compare(0, wcslen(prefix), prefix) == 0)
					n++;
			return n;
		}
	};

	class FakeCollectionHost: public CollectionManager::Host
	{
	public:
		FakeCollectionHost(CollectionHostLog& log): m_log(log), m_thread(GetCurrentThreadId())	{}
		virtual void SendStateMessage(UINT msg)		{m_log.stateMessages.push_back(msg);}
		virtual void NotifyUser(LPCTSTR msg)
		{
			if (GetCurrentThreadId() != m_thread)
				m_log.bNotifiedOffThread = TRUE;
			m_log.notifications.push_back(msg);
		}
		virtual HINTERNET GetInternetHandle()		{return NULL;}
		virtual FileTagger* CreateFileTagger()		{m_log.pTagger = new FakeTagger; return m_log.pTagger;}
	private:
		CollectionHostLog& m_log;
		DWORD m_thread;
	};

	//Waits for the scans, consuming their results as the UI timer does.
	BOOL WaitIdle(CollectionManager& cm, BOOL bConsume = TRUE)
	{
		for (INT i = 0; i < 1000; i++)
		{
			if (bConsume)
				cm.ConsumeResult();
			if (cm.GetPendingJobs() == 0 && (!bConsume || cm.GetState() == CollectionManager::S_Idle))
				return TRUE;
			Sleep(10);
		}
		return FALSE;
	}

	INT TrackCount(SQLManager& sm, UINT collectionID)
	{
		TrackRecordCollection col;
		TracksFilter tf;
		tf.CollectionID.match = NUMM_Exact;
		tf.CollectionID.val = collectionID;
		sm.GetTrackRecordCollectionByTracksFilter(col, tf);
		return (INT)col.size();
	}

	CollectionRecordSP FindCollection(SQLManager& sm, const std::wstring& dir)
	{
		CollectionRecordCollection col;
		sm.GetCollectionRecordCollectionByTypeID(col, CollectionManager::GetCollectionType(dir.c_str()));
		for (size_t i = 0; i < col.size(); i++)
			if (_wcsicmp(col[i]->location.c_str(), dir.c_str()) == 0)
				return col[i];
		return CollectionRecordSP();
	}

	void TestCollectionManager()
	{
		std::wstring db = TempPath(L"collections.db");
		DeleteFileW(db.c_str());
		SQLManager sm;
		CHECK(sm.Init(db.c_str()));
		std::wstring ini = TempPath(L"collections.ini");
		DeleteFileW(ini.c_str());
		IniAppSettings settings;
		settings.Load(ini.c_str());
		AppOptions options(settings);
		options.Read();
		options.Set(OPT_TAG_SafetyOption, 42);
		EditableTranslation tr;
		tr.SetString(IDS_ADDING, L"Adding");
		tr.SetString(IDS_UPDATING, L"Updating");
		tr.SetString(IDS_REMOVE, L"Remove");

		std::wstring dir = FreshDir(L"collection");
		CreateDirectoryW((dir + L"sub").c_str(), NULL);
		Touch(dir + L"One.mp3");
		Touch(dir + L"Two.mp3");
		Touch(dir + L"sub\\Three.flac");
		Touch(dir + L"notes.txt");			//not a track
		std::wstring location = dir.substr(0, dir.size() - 1);

		CollectionHostLog log;
		{
			CollectionManager cm(sm, options, tr, std::unique_ptr<CollectionManager::Host>(new FakeCollectionHost(log)));
			CHECK(CollectionManager::GetCollectionType(location.c_str()) == CTYPE_LocalFolder);
			CHECK(!cm.CollectionExists(location.c_str()));

			//Add: the scan runs on the job thread
			CHECK(cm.AddCollection(location.c_str(), L"Test collection"));
			CHECK(cm.CollectionExists(location.c_str()));
			CHECK(log.Count(SM_CollectionManagerEvent) == 1);
			CollectionRecordSP rec = FindCollection(sm, location);
			CHECK(rec.get() != NULL);
			if (rec.get() == NULL)
				return;
			CHECK(cm.IsCollectionPending(*rec) && cm.GetState() == CollectionManager::S_Working);
			CHECK(!cm.UpdateCollection(rec, TRUE, TRUE, TRUE));	//already being scanned
			CHECK(WaitIdle(cm));
			CHECK(!cm.IsCollectionPending(*rec));
			CHECK(TrackCount(sm, rec->ID) == 3);
			CHECK(log.pTagger != NULL && log.pTagger->m_readDepth == 42);	//OPT_TAG_SafetyOption
			CHECK(log.Count(SM_CollectionManagerEvent) == 2 && log.Count(SM_DatabaseUpdated) == 1);
			CHECK(log.Starting(L"Adding: ") == 3);
			CHECK(std::find(log.notifications.begin(), log.notifications.end(), L"Adding: " + dir + L"One.mp3") != log.notifications.end());
			CHECK(!log.bNotifiedOffThread);		//messages reach the host on the UI thread

			//Update existing files, then remove a deleted one
			log.notifications.clear();
			CHECK(cm.UpdateCollection(rec, FALSE, TRUE, FALSE));
			CHECK(WaitIdle(cm));
			CHECK(log.Starting(L"Updating: ") == 3 && log.Starting(L"Adding: ") == 0);
			DeleteFileW((dir + L"Two.mp3").c_str());
			log.notifications.clear();
			CHECK(cm.UpdateCollection(rec, FALSE, FALSE, TRUE));
			CHECK(WaitIdle(cm));
			CHECK(TrackCount(sm, rec->ID) == 2);
			CHECK(log.notifications.size() == 1 && log.notifications[0] == L"Remove: " + dir + L"Two.mp3");

			//A big scan only keeps the latest messages until they are consumed
			for (INT i = 0; i < 60; i++)
				Touch(dir + L"Extra" + std::to_wstring(i) + L".mp3");
			log.notifications.clear();
			CHECK(cm.UpdateCollection(rec, TRUE, FALSE, FALSE));
			CHECK(WaitIdle(cm, FALSE));
			CHECK(log.notifications.empty());
			cm.ConsumeResult();
			CHECK(log.notifications.size() == CollectionManager::cMaxPendingMessages);
			CHECK(!log.notifications.empty() && log.notifications.back().find(L"Extra") != std::wstring::npos);
			CHECK(TrackCount(sm, rec->ID) == 62);
			CHECK(cm.GetState() == CollectionManager::S_Idle);

			CHECK(cm.RenameCollection(rec->ID, L"Renamed"));
			CollectionRecord renamed;
			CHECK(sm.GetCollectionRecord(rec->ID, renamed) && renamed.name == L"Renamed");
			CHECK(cm.RemoveCollection(rec->ID));
			CHECK(!sm.GetCollectionRecord(rec->ID, renamed));
			CHECK(TrackCount(sm, rec->ID) == 0);
			CHECK(!cm.CollectionExists(location.c_str()));
		}
		//Adding a folder that doesn't exist fails without starting a scan.
		CollectionHostLog log2;
		CollectionManager cm2(sm, options, tr, std::unique_ptr<CollectionManager::Host>(new FakeCollectionHost(log2)));
		CHECK(!cm2.AddCollection(TempPath(L"no-such-folder").c_str(), L"x"));
		CHECK(log2.stateMessages.empty() && cm2.GetState() == CollectionManager::S_Idle);
	}

	class FakePictureHost: public LocalPictureManager::Host
	{
	public:
		FakePictureHost(INT& artistRequests, INT& albumRequests): m_artist(artistRequests), m_album(albumRequests)	{}
		virtual void RequestArtistPicture(ArtistRecord&)			{m_artist++;}
		virtual void RequestAlbumPicture(AlbumRecord&, LPCTSTR)		{m_album++;}
	private:
		INT& m_artist;
		INT& m_album;
	};

	void TestLocalPictureManager()
	{
		std::wstring db = TempPath(L"pictures.db");
		DeleteFileW(db.c_str());
		SQLManager sm;
		CHECK(sm.Init(db.c_str()));
		std::wstring storage = FreshDir(L"picstorage");
		std::wstring music = FreshDir(L"picmusic");
		std::wstring albumDir = music + L"Album\\";
		CreateDirectoryW(albumDir.c_str(), NULL);

		//An artist and album with a track in albumDir
		CollectionRecord col;
		col.location = music.substr(0, music.size() - 1);
		col.type = CTYPE_LocalFolder;
		CHECK(sm.AddNewCollectionRecord(col));
		TagInfo ti;
		ti.Artist = L"Picture Artist";
		ti.Album = L"Picture Album";
		ti.Title = L"Song";
		ti.validFields = TagInfo_Artist | TagInfo_Album | TagInfo_Title;
		CHECK(sm.AddNewTrackFromTagInfo(col.ID, (albumDir + L"Song.mp3").c_str(), ti));
		ArtistRecord artist;
		CHECK(sm.GetArtistRecordUnique(L"Picture Artist", artist));
		FullTrackRecordCollection tracks;
		TracksFilter tf;
		tf.Artist.val = L"Picture Artist";
		tf.Artist.match = TXTM_Exact;
		CHECK(sm.GetFullTrackRecordCollectionByTracksFilter(tracks, tf) && tracks.size() == 1);
		if (tracks.size() != 1)
			return;
		FullAlbumRecordSP album;
		CHECK(sm.GetFullAlbumRecordByID(album, tracks[0]->album.ID) && album.get() != NULL);
		if (album.get() == NULL)
			return;

		InfoProviderFactory providers;
		INT artistRequests = 0, albumRequests = 0;
		{
			LocalPictureManager lpm(sm, providers, std::unique_ptr<LocalPictureManager::Host>(new FakePictureHost(artistRequests, albumRequests)));
			CHECK(lpm.Init(storage.c_str()));
			//Downloads by service must go to an online provider, not to the local image providers
			for (INT srv = IInfoProvider::SRV_ArtistImage; srv <= IInfoProvider::SRV_AlbumImage; srv++)
			{
				if (srv != IInfoProvider::SRV_ArtistImage && srv != IInfoProvider::SRV_AlbumImage)
					continue;
				IInfoProvider* pIP = providers.CreateInfoProviderByService((IInfoProvider::ServiceEnum)srv);
				LPCTSTR id = pIP != NULL ? pIP->GetModuleInfo(IInfoProvider::IPI_UniqueID) : L"(none)";
				CHECK(pIP != NULL && (wcscmp(id, L"DEEZ") == 0 || wcscmp(id, L"CAAR") == 0));
				delete pIP;
			}

			//No pictures yet
			CHECK(lpm.GetMainArtistPicture(artist) == NULL);
			std::tstringvector pics;
			CHECK(!lpm.GetArtistPictures(artist, pics));

			//Drawing a missing picture asks the host for a download
			HDC hdc = CreateCompatibleDC(NULL);
			RECT rc = {0, 0, 64, 64};
			CHECK(!lpm.DrawThumbnail(IIT_ArtistPicture, artist.ID, hdc, rc));
			CHECK(artistRequests == 1);
			CHECK(!lpm.DrawThumbnail(IIT_AlbumPicture, album->album.ID, hdc, rc));
			CHECK(albumRequests == 1);
			DeleteDC(hdc);

			//A picture added by the user is copied to the storage and becomes the main one
			std::wstring source = music + L"artist.jpg";
			Touch(source, "not really a jpeg");
			lpm.ResetArtistCache(artist);
			CHECK(lpm.AddArtistPicture(artist, source.c_str()));
			pics.clear();
			CHECK(lpm.GetArtistPictures(artist, pics) && pics.size() == 1);
			std::wstring stored = pics.empty() ? L"" : pics[0];
			CHECK(stored.compare(0, storage.size(), storage) == 0);
			LPCTSTR main = lpm.GetMainArtistPicture(artist);
			CHECK(main != NULL && stored == main);
			PicRecord pr(IIT_ArtistPicture, artist.ID);
			CHECK(sm.GetPicRecord(pr) && pr.path == stored);		//remembered in the database

			//Deleted from disk: forgotten (session cache and database)
			lpm.RemoveArtistPicture(artist, stored.c_str());
			CHECK(PathFileExistsW(stored.c_str()) == FALSE);
			CHECK(lpm.GetMainArtistPicture(artist) == NULL);
			PicRecord gone(IIT_ArtistPicture, artist.ID);
			CHECK(!sm.GetPicRecord(gone));

			//Album pictures: images in the album folder, when enabled
			std::wstring front = albumDir + L"front.jpg";
			Touch(front, "front cover");
			lpm.EnableFolderImages(FALSE);
			lpm.ResetAlbumCache(*album);
			CHECK(lpm.GetMainAlbumPicture(*album) == NULL);
			lpm.EnableFolderImages(TRUE);
			lpm.ResetAlbumCache(*album);
			main = lpm.GetMainAlbumPicture(*album);
			CHECK(main != NULL && _wcsicmp(main, front.c_str()) == 0);
			PicRecord apr(IIT_AlbumPicture, album->album.ID);
			CHECK(sm.GetPicRecord(apr) && _wcsicmp(apr.path.c_str(), front.c_str()) == 0);

			//A main picture chosen by the user wins
			std::wstring chosen = albumDir + L"chosen.jpg";
			Touch(chosen, "chosen");
			lpm.SetMainAlbumPicture(*album, chosen.c_str());
			main = lpm.GetMainAlbumPicture(*album);
			CHECK(main != NULL && chosen == main);
		}
	}
}

void RunManagerTests()
{
	wprintf(L"Manager tests\n");
	ULONG_PTR gdiplusToken = 0;
	Gdiplus::GdiplusStartupInput gdiplusInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, NULL);
	TestCollectionManager();
	TestLocalPictureManager();
	Gdiplus::GdiplusShutdown(gdiplusToken);
}
