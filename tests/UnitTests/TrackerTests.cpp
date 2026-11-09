//	Trackers: MultiTracker sends plays to the local history and (lazily) to the
//	Last.fm scrobbler, as the options say.
#include "stdafx.h"
#include "TestFramework.h"
#include "Trackers/MultiTracker.h"
#include "AppOptions.h"
#include "AppSettings/IniAppSettings.h"
#include "SQLManager.h"
#include "SystemTimeFunctions.h"

namespace
{
	UINT Plays(SQLManager& sm)
	{
		SQLManager::HistoryLogStats stats;
		sm.GetHistoryLogStats(stats);
		return stats.plays;
	}

	void TestMultiTracker()
	{
		std::wstring db = TempPath(L"tracker.db");
		DeleteFileW(db.c_str());
		SQLManager sm;
		CHECK(sm.Init(db.c_str()));

		std::wstring ini = TempPath(L"tracker.ini");
		DeleteFileW(ini.c_str());
		IniAppSettings settings;
		settings.Load(ini.c_str());
		AppOptions options(settings);
		options.Read();
		CHECK(options.Get(OPT_GEN_NotifyHistory) == 1);		//defaults: history on,
		CHECK(options.Get(OPT_GEN_LastFMScrobbler) == 0);	//scrobbling off

		INT lastFMRequests = 0;
		MultiTracker tracker(options, sm, [&lastFMRequests]() -> LastFMServices*
		{
			lastFMRequests++;
			return NULL;	//not available (yet)
		});

		TrackerTrack track;
		track.artist = L"Tracker Artist";
		track.title = L"Tracker Song";
		track.length = 200;
		//2026-06-15 12:00:00 UTC: logged in local time
		const UINT when = 1781524800;

		tracker.SubmitTrack(ITracker::ACT_Started, track, when);
		CHECK(Plays(sm) == 0);								//only finished plays are logged
		tracker.SubmitTrack(ITracker::ACT_Played, track, when);
		CHECK(Plays(sm) == 1);
		CHECK(lastFMRequests == 0);							//never created while scrobbling is off

		FullHistLogRecordCollection logs;
		CHECK(sm.GetFullLogRecordCollection(logs, NULL, NULL, NULL, NULL, HLA_Played, 0) && logs.size() == 1);
		if (logs.size() == 1)
		{
			SYSTEMTIME utc = UnixTimeStampToSystemTime(when), local;
			SystemTimeToTzSpecificLocalTime(NULL, &utc, &local);
			const SYSTEMTIME& logged = logs[0]->log.dateAdded;
			CHECK(logged.wYear == local.wYear && logged.wMonth == local.wMonth && logged.wDay == local.wDay
				&& logged.wHour == local.wHour && logged.wMinute == local.wMinute);
			CHECK(logs[0]->artist.name == L"Tracker Artist" && logs[0]->track.name == L"Tracker Song");
		}

		//Options are read on every submit.
		options.Set(OPT_GEN_NotifyHistory, 0);
		tracker.SubmitTrack(ITracker::ACT_Played, track, when + 300);
		CHECK(Plays(sm) == 1);

		options.Set(OPT_GEN_LastFMScrobbler, 1);
		tracker.SubmitTrack(ITracker::ACT_Played, track, when + 600);
		CHECK(lastFMRequests == 1);
		tracker.SubmitTrack(ITracker::ACT_Played, track, when + 900);
		CHECK(lastFMRequests == 2);							//asked again while unavailable
		CHECK(Plays(sm) == 1);

		options.Set(OPT_GEN_NotifyHistory, 1);
		tracker.SubmitTrack(ITracker::ACT_Played, track, when + 1200);
		CHECK(Plays(sm) == 2);

		//No Last.fm provider at all
		MultiTracker noLastFM(options, sm, nullptr);
		noLastFM.SubmitTrack(ITracker::ACT_Played, track, when + 1500);
		CHECK(Plays(sm) == 3);
	}
}

void RunTrackerTests()
{
	wprintf(L"Tracker tests\n");
	TestMultiTracker();
}
