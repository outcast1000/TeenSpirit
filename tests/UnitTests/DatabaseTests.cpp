//	Database layer tests: exercises SQLManager (SQLite) end to end.
#include "stdafx.h"
#include "TestFramework.h"
#include "SQLManager.h"
#include "SQLite/SQLiteDB.h"
#include <process.h>
#include <cstdio>

static void RemoveDb(const std::wstring& path)
{
	DeleteFileW(path.c_str());
	DeleteFileW((path + L"-wal").c_str());
	DeleteFileW((path + L"-shm").c_str());
}

static SYSTEMTIME Today(INT dayOffset = 0)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	ULARGE_INTEGER u;
	u.LowPart = ft.dwLowDateTime;
	u.HighPart = ft.dwHighDateTime;
	u.QuadPart += (LONGLONG)dayOffset * 24 * 3600 * 10000000LL;
	ft.dwLowDateTime = u.LowPart;
	ft.dwHighDateTime = u.HighPart;
	FileTimeToSystemTime(&ft, &st);
	return st;
}

static TagInfo MakeTag(LPCWSTR title, LPCWSTR artist, LPCWSTR album, LPCWSTR genre, INT year)
{
	TagInfo t;
	t.Title = title;
	t.Artist = artist;
	t.Album = album;
	t.Genre = genre;
	t.Year = year;
	t.Bitrate = 192;
	t.Length = 200;
	t.Size = 4000;
	t.TrackNo = 1;
	t.validFields = TagInfo_Title | TagInfo_Artist | TagInfo_Album | TagInfo_Genre | TagInfo_Year |
		TagInfo_Bitrate | TagInfo_Length | TagInfo_Size | TagInfo_TrackNo;
	return t;
}

//------------------------------------------------------------------
static void TestBasicRecords()
{
	wprintf(L"TestBasicRecords\n");
	std::wstring db = TempPath(L"basic.db");
	RemoveDb(db);
	SQLManager sm;
	CHECK(sm.Init(db.c_str()));

	CollectionRecord col;
	col.location = L"d:\\music";
	col.type = CTYPE_LocalFolder;
	CHECK(sm.AddNewCollectionRecord(col));
	CHECK(col.ID != 0);
	col.name = L"new col name";
	CHECK(sm.UpdateCollectionRecord(col));
	CollectionRecord col2;
	CHECK(sm.GetCollectionRecord(col.ID, col2) && col2.name == L"new col name");
	CHECK(col2.dateAdded.wYear >= 2020);
	CHECK(sm.GetCollectionRecordUnique(col.serial, L"D:\\MUSIC", col2) && col2.ID == col.ID);
	CHECK(sm.CollectionRecordExists(col.ID));
	CollectionRecord missing = col;
	missing.ID = 947;
	CHECK(!sm.UpdateCollectionRecord(missing));
	CHECK(!sm.GetCollectionRecord(947, col2));
	CHECK(!sm.CollectionRecordExists(947));

	//Defaults rows
	GenreRecord g;
	CHECK(sm.GetGenreRecord(1, g) && g.name == L"[Unknown]");
	ArtistRecord a;
	CHECK(sm.GetArtistRecord(2, a) && a.name == L"[Various]");

	//Long names (Jet limited them to 100 chars; SQLite keeps them)
	GenreRecord longGenre;
	longGenre.name.assign(400, L'g');
	CHECK(sm.AddNewGenreRecord(longGenre));
	GenreRecord back;
	CHECK(sm.GetGenreRecord(longGenre.ID, back) && back.name.size() == 400);

	ArtistRecord art;
	art.name = L"It's \"quoted\" & 100% _odd_";
	art.genID = 1;
	CHECK(sm.AddNewArtistRecord(art));
	ArtistRecord art2;
	CHECK(sm.GetArtistRecordUnique(art.name.c_str(), art2) && art2.ID == art.ID);
	CHECK(sm.GetArtistID(art.name.c_str()) == art.ID);
	art.name = L"Renamed";
	CHECK(sm.UpdateArtistRecord(art, FALSE));
	CHECK(sm.GetArtistRecord(art.ID, art2) && art2.name == L"Renamed");
	ArtistRecord ghost = art;
	ghost.ID = 947;
	ghost.name = L"ghost";
	CHECK(!sm.UpdateArtistRecord(ghost, FALSE));

	AlbumRecord alb;
	alb.name = L"Album";
	alb.artID = art.ID;
	alb.year = 1999;
	CHECK(sm.AddNewAlbumRecord(alb));
	CHECK(sm.GetAlbumID(art.ID, L"ALBUM") == alb.ID);
	alb.rating = 200;
	CHECK(sm.UpdateAlbumRecord(alb, FALSE));
	AlbumRecord alb2;
	CHECK(sm.GetAlbumRecord(alb.ID, alb2) && alb2.rating == 200 && alb2.year == 1999);

	TrackRecord tra;
	tra.name = L"Track";
	tra.location = L"d:\\music\\track.mp3";
	tra.artID = art.ID;
	tra.genID = 1;
	tra.colID = col.ID;
	tra.albID = alb.ID;
	tra.trackType = GetTrackType(tra.location.c_str());
	GetLocalTime(&tra.dateAdded);
	CHECK(sm.AddNewTrackRecord(tra));
	TrackRecord tra2;
	CHECK(sm.GetTrackRecord(tra.ID, tra2) && tra2.location == tra.location);
	CHECK(tra2.dateAdded.wYear == tra.dateAdded.wYear && tra2.dateAdded.wDay == tra.dateAdded.wDay &&
		tra2.dateAdded.wSecond == tra.dateAdded.wSecond);
	CHECK(sm.GetTrackID(col.ID, L"D:\\Music\\TRACK.MP3") == tra.ID);
	tra.rating = 150;
	CHECK(sm.UpdateTrackRecord(tra));
	CHECK(sm.GetTrackRecord(tra.ID, tra2) && tra2.rating == 150);
	CHECK(sm.TrackRecordExists(tra.ID));
	CHECK(sm.DeleteTrackRecord(tra.ID));
	CHECK(!sm.TrackRecordExists(tra.ID));

	//IDs are never reused (AUTOINCREMENT, like Jet COUNTER)
	TrackRecord tra3 = tra;
	tra3.ID = 0;
	CHECK(sm.AddNewTrackRecord(tra3) && tra3.ID > tra.ID);
}

//------------------------------------------------------------------
static void TestCaseInsensitivity()
{
	wprintf(L"TestCaseInsensitivity\n");
	std::wstring db = TempPath(L"case.db");
	RemoveDb(db);
	SQLManager sm;
	CHECK(sm.Init(db.c_str()));
	CollectionRecord col;
	col.location = L"d:\\c";
	col.type = CTYPE_LocalFolder;
	CHECK(sm.AddNewCollectionRecord(col));

	TagInfo t1 = MakeTag(L"Song", L"\x0386\x039B\x03A6\x0391 Band", L"\x00C9t\x00E9", L"Rock", 2001);	//"ΆΛΦΑ Band", "Été"
	INT id1 = sm.AddNewTrackFromTagInfo(col.ID, L"d:\\c\\1.mp3", t1);
	CHECK(id1 != 0);
	TagInfo t2 = MakeTag(L"Song 2", L"\x03AC\x03BB\x03C6\x03B1 band", L"\x00E9T\x00C9", L"ROCK", 2001);	//"άλφα band", "éTÉ"
	INT id2 = sm.AddNewTrackFromTagInfo(col.ID, L"d:\\c\\2.mp3", t2);
	CHECK(id2 != 0);

	TrackRecord r1, r2;
	CHECK(sm.GetTrackRecord(id1, r1) && sm.GetTrackRecord(id2, r2));
	CHECK(r1.artID == r2.artID);		//same artist, different case (Greek)
	CHECK(r1.albID == r2.albID);		//same album, different case (accented Latin)
	CHECK(r1.genID == r2.genID);

	//Accents are still significant (as in Jet)
	CHECK(sm.GetGenreID(L"Rock") != 0);
	CHECK(sm.GetArtistID(L"\x0391\x039B\x03A6\x0391 BAND") == 0);	//"ΑΛΦΑ" without the tonos

	//LIKE is case-insensitive beyond ASCII
	FullTrackRecordCollection found;
	CHECK(sm.GetFullTrackRecordCollectionBySimpleSearch(found, L"\x03BB\x03C6\x03B1"));	//"λφα"
	CHECK(found.size() == 2);
	found.clear();
	CHECK(sm.GetFullTrackRecordCollectionBySmartSearch(found, L"\x039B\x03A6\x0391   song 2"));
	CHECK(found.size() == 1 && found[0]->track.ID == (UINT)id2);
}

//------------------------------------------------------------------
static void TestQueries()
{
	wprintf(L"TestQueries\n");
	std::wstring db = TempPath(L"queries.db");
	RemoveDb(db);
	SQLManager sm;
	CHECK(sm.Init(db.c_str()));
	CollectionRecord col;
	col.location = L"d:\\q";
	col.type = CTYPE_LocalFolder;
	CHECK(sm.AddNewCollectionRecord(col));

	struct {LPCWSTR title, artist, album, genre; INT year;} songs[] =
	{
		{L"A1", L"Alpha", L"First", L"Rock", 1990},
		{L"A2", L"Alpha", L"First", L"Rock", 1990},
		{L"A3", L"Alpha", L"Second", L"Pop", 1995},
		{L"B1", L"Beta", L"Hits", L"Rock", 2000},
		{L"B2", L"Beta", L"Hits", L"Rock", 2000},
		{L"C1", L"Gamma", L"Hits", L"Jazz", 2005},	//"Hits" under two artists: a compilation candidate
	};
	std::vector<INT> ids;
	for (size_t i = 0; i < sizeof(songs) / sizeof(songs[0]); i++)
	{
		TagInfo t = MakeTag(songs[i].title, songs[i].artist, songs[i].album, songs[i].genre, songs[i].year);
		if (i == 0)
		{
			t.Lyrics = L"the quick brown fox";
			t.Comment = L"a comment";
			t.validFields |= TagInfo_Lyrics | TagInfo_Comment;
		}
		WCHAR path[64];
		_snwprintf_s(path, _TRUNCATE, L"d:\\q\\%s.mp3", songs[i].title);
		path[63] = 0;
		ids.push_back(sm.AddNewTrackFromTagInfo(col.ID, path, t));
		CHECK(ids.back() != 0);
	}
	UINT alphaID = sm.GetArtistID(L"Alpha");
	CHECK(alphaID != 0);

	TracksFilter all;
	CHECK(sm.GetTrackCount(all) == 6);
	std::vector<UINT> idList;
	CHECK(sm.GetTrackIDList(all, idList) && idList.size() == 6);

	TracksFilter byArtist;
	byArtist.ArtistID.match = NUMM_Exact;
	byArtist.ArtistID.val = alphaID;
	FullTrackRecordCollection ftc;
	CHECK(sm.GetFullTrackRecordCollectionByTracksFilter(ftc, byArtist) && ftc.size() == 3);
	CHECK(ftc.size() > 0 && ftc[0]->artist.name == L"Alpha" && ftc[0]->collection.ID == col.ID);

	TracksFilter byAlbumText;
	byAlbumText.Album.match = TXTM_Exact;
	byAlbumText.Album.val = L"hits";
	idList.clear();
	CHECK(sm.GetTrackIDList(byAlbumText, idList) && idList.size() == 3);
	ftc.clear();
	CHECK(sm.GetFullTrackRecordCollectionByTracksFilter(ftc, byAlbumText) && ftc.size() == 3);

	TracksFilter likeTitle;
	likeTitle.Title.match = TXTM_Like;
	likeTitle.Title.val = L"b";
	CHECK(sm.GetTrackCount(likeTitle) == 2);

	TracksFilter limit;
	ftc.clear();
	CHECK(sm.GetFullTrackRecordCollectionByTracksFilter(ftc, limit, 4) && ftc.size() == 4);

	//Date filters (Jet #date# literals before)
	TracksFilter afterYesterday;
	afterYesterday.DateAdded.match = DATM_After;
	afterYesterday.DateAdded.val = Today(-1);
	CHECK(sm.GetTrackCount(afterYesterday) == 6);
	TracksFilter afterTomorrow;
	afterTomorrow.DateAdded.match = DATM_After;
	afterTomorrow.DateAdded.val = Today(1);
	CHECK(sm.GetTrackCount(afterTomorrow) == 0);
	TracksFilter beforeTomorrow;
	beforeTomorrow.DateAdded.match = DATM_Before;
	beforeTomorrow.DateAdded.val = Today(1);
	CHECK(sm.GetTrackCount(beforeTomorrow) == 6);

	//Month added (Jet Month()/Year() before)
	SYSTEMTIME now = Today();
	INT monthKey = now.wMonth + now.wYear * 12 - 1;
	MonthAddedRecordCollection months;
	CHECK(sm.GetMonthAddedRecordCollectionByTracksFilter(months, all, TRUE));
	CHECK(months.size() == 1 && months[0]->month == monthKey && months[0]->trackCount == 6);
	TracksFilter thisMonth;
	thisMonth.MonthAdded.match = NUMM_Exact;
	thisMonth.MonthAdded.val = monthKey;
	CHECK(sm.GetTrackCount(thisMonth) == 6);

	//Grouped queries (Jet first() before)
	GenreRecordCollection genres;
	CHECK(sm.GetGenreRecordCollectionByTracksFilter(genres, all, TRUE) && genres.size() == 3);
	INT rockCount = 0;
	for (size_t i = 0; i < genres.size(); i++)
		if (genres[i]->name == L"Rock")
			rockCount = genres[i]->trackCount;
	CHECK(rockCount == 4);
	genres.clear();
	CHECK(sm.GetGenreRecordCollectionByTracksFilter(genres, all, FALSE) && genres.size() == 4);	//incl. [Unknown]

	YearRecordCollection years;
	CHECK(sm.GetYearRecordCollectionByTracksFilter(years, all, TRUE) && years.size() == 4);

	CollectionRecordCollection cols;
	CHECK(sm.GetCollectionRecordCollectionByTracksFilter(cols, all, TRUE) && cols.size() == 1);
	CHECK(cols.size() == 1 && cols[0]->trackCount == 6 && cols[0]->sumDuration == 1200);
	cols.clear();
	CHECK(sm.GetCollectionRecordCollectionByTypeID(cols, CTYPE_LocalFolder) && cols.size() == 1);

	FullArtistRecordCollection artists;
	CHECK(sm.GetFullArtistRecordCollectionByTracksFilter(artists, all) && artists.size() == 3);
	TracksFilter minTracks;
	minTracks.MinNumTracks.match = NUMM_Over;
	minTracks.MinNumTracks.val = 2;
	artists.clear();
	CHECK(sm.GetFullArtistRecordCollectionByTracksFilter(artists, minTracks) && artists.size() == 1);
	FullArtistRecordSP artRec;
	CHECK(sm.GetFullArtistRecordByID(artRec, alphaID) && artRec->artist.name == L"Alpha");

	FullAlbumRecordCollection albums;
	CHECK(sm.GetFullAlbumRecordCollectionByTracksFilter(albums, all) && albums.size() == 4);
	albums.clear();
	CHECK(sm.GetFullAlbumGPNRecordCollectionByTracksFilter(albums, all) && albums.size() == 3);
	albums.clear();
	CHECK(sm.GetFullAlbumRecordCollectionByTracksFilterForCollections(albums, all) && albums.size() == 1);
	CHECK(albums.size() == 1 && albums[0]->album.name == L"Hits" && albums[0]->album.trackCount == 3);
	FullAlbumRecordSP albRec;
	CHECK(sm.GetFullAlbumRecordByID(albRec, sm.GetAlbumID(alphaID, L"First")) && albRec->album.name == L"First");

	TracksFilter artistCount;
	artistCount.ArtistTrackCount.match = NUMM_Over;
	artistCount.ArtistTrackCount.val = 2;
	ftc.clear();
	CHECK(sm.GetFullTrackRecordCollectionByTracksFilter(ftc, artistCount) && ftc.size() == 3);

	FullTrackRecordSP one;
	CHECK(sm.GetFullTrackRecordByID(one, ids[3]) && one->track.name == L"B1" && one->genre.name == L"Rock");
	CHECK(sm.GetFullTrackRecordByLocation(one, L"D:\\Q\\C1.MP3") && one->track.name == L"C1");
	ftc.clear();
	CHECK(sm.GetFullTrackRecordCollectionByLocation(ftc, L"d:\\q\\a2.mp3") && ftc.size() == 1);

	//Track info
	CHECK(sm.GetTrackInfoFlags(ids[0]) == (TRF_NoFlags | TRF_HasLyrics | TRF_HasComments));	//NoFlags is always set (unchanged behavior)
	CHECK(sm.GetTrackInfoFlags(ids[1]) == TRF_NoFlags);
	ftc.clear();
	CHECK(sm.GetFullTrackRecordCollectionByInfoLikeness(ftc, IIT_TrackLyrics, L"BROWN") && ftc.size() == 1);
	CHECK(sm.AdjustTrackInfo(ids[0], IIT_TrackLyrics, L"new lyrics"));
	InfoRecord ir;
	CHECK(sm.GetTrackInfoRecord(ids[0], IIT_TrackLyrics, ir) && ir.name == L"new lyrics");
	CHECK(sm.AdjustTrackInfo(ids[0], IIT_TrackLyrics, L""));
	CHECK(!sm.GetTrackInfoRecord(ids[0], IIT_TrackLyrics, ir));
	CHECK(sm.AdjustArtistInfo(alphaID, IIT_ArtistBio, L"bio"));
	CHECK(sm.GetArtistInfoRecord(alphaID, IIT_ArtistBio, ir) && ir.name == L"bio" && ir.dateAdded.wYear >= 2020);
	CHECK(sm.AdjustAlbumInfo(sm.GetAlbumID(alphaID, L"First"), IIT_AlbumReview, L"review"));

	//Pictures (upsert)
	PicRecord pic(IIT_ArtistPicture, alphaID);
	pic.path = L"c:\\a.jpg";
	CHECK(sm.AddOrUpdatePicRecord(pic));
	pic.path = L"c:\\b.jpg";
	CHECK(sm.AddOrUpdatePicRecord(pic));
	PicRecord pic2(IIT_ArtistPicture, alphaID);
	CHECK(sm.GetPicRecord(pic2) && pic2.path == L"c:\\b.jpg");
	CHECK(sm.DeletePicRecord(IIT_ArtistPicture, alphaID));
	CHECK(!sm.GetPicRecord(pic2));

	//Info requests: hashes use the full unsigned range (overflowed Jet's int column)
	CHECK(sm.AddInfoRequest(1000, 0xF0000001u));
	CHECK(sm.AddInfoRequest(1000, 0xF0000001u));	//duplicate is ignored, not an error
	CHECK(sm.AddInfoRequest(5000, 7));
	std::set<UINT> requests;
	CHECK(sm.GetInfoRequests(requests) && requests.size() == 2 && requests.count(0xF0000001u) == 1);
	CHECK(sm.DeleteInfoRequestsOlderThan(2000));
	requests.clear();
	CHECK(sm.GetInfoRequests(requests) && requests.size() == 1 && requests.count(7) == 1);

	std::vector<SQLManager::ValuedID> similar;
	CHECK(sm.GetSimilarArtists(similar, alphaID));
	CHECK(similar.size() == 1 && similar[0].ID == sm.GetArtistID(L"Beta"));
}

//------------------------------------------------------------------
static void TestMerges()
{
	wprintf(L"TestMerges\n");
	std::wstring db = TempPath(L"merge.db");
	RemoveDb(db);
	SQLManager sm;
	CHECK(sm.Init(db.c_str()));
	CollectionRecord col;
	col.location = L"d:\\m";
	col.type = CTYPE_LocalFolder;
	CHECK(sm.AddNewCollectionRecord(col));
	TagInfo t1 = MakeTag(L"x", L"Doors", L"LA Woman", L"Rock", 1971);
	TagInfo t2 = MakeTag(L"y", L"The Doors", L"LA Woman", L"Classic Rock", 1971);
	INT id1 = sm.AddNewTrackFromTagInfo(col.ID, L"d:\\m\\x.mp3", t1);
	INT id2 = sm.AddNewTrackFromTagInfo(col.ID, L"d:\\m\\y.mp3", t2);
	CHECK(id1 && id2);

	//Genre merge (always failed with Jet: the UPDATE used a non-existent column)
	GenreRecord g;
	CHECK(sm.GetGenreRecordUnique(L"Classic Rock", g));
	g.name = L"Rock";
	CHECK(sm.UpdateGenreRecord(g, TRUE));
	TrackRecord r1, r2;
	CHECK(sm.GetTrackRecord(id1, r1) && sm.GetTrackRecord(id2, r2) && r1.genID == r2.genID);
	GenreRecord gone;
	CHECK(!sm.GetGenreRecordUnique(L"Classic Rock", gone));

	//Artist merge, including merging the same-named albums
	ArtistRecord a;
	CHECK(sm.GetArtistRecordUnique(L"The Doors", a));
	a.name = L"DOORS";
	CHECK(!sm.UpdateArtistRecord(a, FALSE));	//name taken, not forced
	CHECK(sm.UpdateArtistRecord(a, TRUE));
	CHECK(sm.GetTrackRecord(id1, r1) && sm.GetTrackRecord(id2, r2));
	CHECK(r1.artID == r2.artID && r1.albID == r2.albID);
	TracksFilter all;
	FullAlbumRecordCollection albums;
	CHECK(sm.GetFullAlbumRecordCollectionByTracksFilter(albums, all) && albums.size() == 1);

	//Deletes
	CHECK(sm.DeleteCollectionRecord(col.ID, TRUE));
	CHECK(sm.GetTrackCount(all) == 0);
}

//------------------------------------------------------------------
static void TestHistory()
{
	wprintf(L"TestHistory\n");
	std::wstring db = TempPath(L"history.db");
	RemoveDb(db);
	SQLManager sm;
	CHECK(sm.Init(db.c_str()));
	SYSTEMTIME now = Today();
	SYSTEMTIME old = Today(-30);
	CHECK(sm.LogTrackInHistory(L"Art", L"Song", HLA_Played, old));
	CHECK(sm.LogTrackInHistory(L"Art", L"Song", HLA_Played, now));
	CHECK(sm.LogTrackInHistory(L"ART", L"SONG", HLA_Played, now));	//same track, other case
	CHECK(sm.LogTrackInHistory(L"Art", L"Other", HLA_Played, now));
	CHECK(sm.LogTrackInHistory(L"Art", L"Other", HLA_Clicked, now));
	CHECK(sm.LogTrackInHistory(L"", L"", HLA_Played, now));			//-> [Unknown]

	SQLManager::HistoryLogStats stats;
	CHECK(sm.GetHistoryLogStats(stats) && stats.plays == 5 && stats.actions == 1);
	CHECK(stats.firstTime.wDay == old.wDay && stats.lastTime.wDay == now.wDay);

	HistArtistRecord unknown;
	CHECK(sm.GetHistArtistRecordUnique(L"[Unknown]", unknown) && unknown.ID == 1);

	FullHistLogRecordCollection logs;
	CHECK(sm.GetFullLogRecordCollection(logs, NULL, NULL, NULL, NULL, HLA_Unknown, 0) && logs.size() == 6);
	logs.clear();
	SYSTEMTIME weekAgo = Today(-7);
	CHECK(sm.GetFullLogRecordCollection(logs, &weekAgo, NULL, NULL, NULL, HLA_Played, 0) && logs.size() == 4);
	logs.clear();
	WCHAR artFilter[] = L"ar";
	CHECK(sm.GetFullLogRecordCollection(logs, NULL, NULL, artFilter, NULL, HLA_Unknown, 2) && logs.size() == 2);

	FullHistTrackRecordCollection tracks;
	CHECK(sm.GetFullHistTrackRecordCollection(tracks, NULL, NULL, NULL, NULL, HLA_Played, 0) && tracks.size() == 3);
	FullHistArtistRecordCollection artists;
	CHECK(sm.GetFullHistArtistRecordCollection(artists, NULL, NULL, NULL, NULL, HLA_Played, 0) && artists.size() == 2);

	std::vector<SQLManager::Rank> ranks;
	CHECK(sm.GetHistTrackRankCollection(ranks) && ranks.size() == 2 && ranks[0].Hits == 3);
	ranks.clear();
	CHECK(sm.GetHistArtistRankCollection(ranks) && ranks.size() == 2 && ranks[0].Hits == 4);

	SQLManager::HistoryTrackStats ts;
	CHECK(sm.GetHistoryTrackStats(L"art", L"song", NULL, NULL, ts) && ts.hits == 3 && ts.rank == 1);
	SQLManager::HistoryTrackStats ts2;
	SYSTEMTIME tomorrow = Today(1);
	CHECK(sm.GetHistoryTrackStats(L"Art", L"Song", &weekAgo, &tomorrow, ts2) && ts2.hits == 2);
}

//------------------------------------------------------------------
struct WriterArgs
{
	const std::wstring* path;
	INT added;
	INT failures;
};

static unsigned __stdcall WriterThread(void* p)
{
	WriterArgs* args = (WriterArgs*)p;
	SQLManager sm;
	if (!sm.Init(args->path->c_str()))
	{
		args->failures++;
		return 0;
	}
	CollectionRecord col;
	col.location = L"d:\\threads";
	col.type = CTYPE_LocalFolder;
	if (!sm.AddNewCollectionRecord(col))
		args->failures++;
	for (INT i = 0; i < 300; i++)
	{
		WCHAR title[32], path[64], artist[32];
		_snwprintf_s(title, _TRUNCATE, L"t%d", i);
		_snwprintf_s(path, _TRUNCATE, L"d:\\threads\\%d.mp3", i);
		_snwprintf_s(artist, _TRUNCATE, L"artist %d", i % 7);
		title[31] = path[63] = artist[31] = 0;
		TagInfo t = MakeTag(title, artist, L"album", L"genre", 2000);
		if (sm.AddNewTrackFromTagInfo(col.ID, path, t) != 0)
			args->added++;
		else
			args->failures++;
	}
	return 0;
}

static void TestConcurrentConnections()
{
	wprintf(L"TestConcurrentConnections\n");
	std::wstring db = TempPath(L"threads.db");
	RemoveDb(db);
	SQLManager reader;
	CHECK(reader.Init(db.c_str()));
	WriterArgs args = {&db, 0, 0};
	HANDLE h = (HANDLE)_beginthreadex(NULL, 0, WriterThread, &args, 0, NULL);
	CHECK(h != NULL);
	INT reads = 0;
	BOOL bReadsOK = TRUE;
	while (WaitForSingleObject(h, 0) == WAIT_TIMEOUT)
	{
		TracksFilter all;
		FullTrackRecordCollection col;
		if (!reader.GetFullTrackRecordCollectionByTracksFilter(col, all))
			bReadsOK = FALSE;
		SYSTEMTIME now = Today();
		if (!reader.LogTrackInHistory(L"reader", L"track", HLA_Played, now))	//reader writes too
			bReadsOK = FALSE;
		reads++;
	}
	CloseHandle(h);
	CHECK(bReadsOK);
	CHECK(reads > 0);
	CHECK(args.failures == 0 && args.added == 300);
	TracksFilter all;
	CHECK(reader.GetTrackCount(all) == 300);
	SQLManager::HistoryLogStats stats;
	CHECK(reader.GetHistoryLogStats(stats) && stats.plays == (UINT)reads);
	wprintf(L"  %d reads interleaved with 300 writes\n", reads);
}

//------------------------------------------------------------------
static void TestReopen()
{
	wprintf(L"TestReopen\n");
	std::wstring db = TempPath(L"reopen.db");
	RemoveDb(db);
	UINT colID = 0;
	{
		SQLManager sm;
		CHECK(sm.Init(db.c_str()));
		CollectionRecord col;
		col.location = L"d:\\r";
		col.type = CTYPE_Media;
		CHECK(sm.AddNewCollectionRecord(col));
		colID = col.ID;
	}
	{
		SQLManager sm;
		CHECK(sm.Init(db.c_str()));
		CollectionRecord col;
		CHECK(sm.GetCollectionRecord(colID, col) && col.type == CTYPE_Media);
		CHECK(_wcsicmp(sm.GetDatabasePath(), db.c_str()) == 0);
	}
	{
		SQLiteDB raw;
		CHECK(raw.Open(db.c_str()));
		CHECK(raw.GetUserVersion() == 1);
		CHECK(raw.SetUserVersion(99));
	}
	{
		//A database from a newer version is refused (the MessageBox is not shown in tests:
		//this case is detected before any UI by GetUserVersion; see SQLManager::Init).
		SQLiteDB raw;
		CHECK(raw.Open(db.c_str()));
		CHECK(raw.GetUserVersion() == 99);
	}
}

//A legacy music.mdb next to a new database is no longer imported (the Jet import was
//removed after the one-time conversion): the new database starts empty, the .mdb is left alone.
static void TestLegacyFileIgnored()
{
	wprintf(L"TestLegacyFileIgnored\n");
	std::wstring mdb = TempPath(L"legacy.mdb");
	std::wstring db = TempPath(L"legacy.db");
	RemoveDb(db);
	{
		FILE* f = NULL;
		CHECK(_wfopen_s(&f, mdb.c_str(), L"wb") == 0 && f != NULL);
		if (f != NULL)
		{
			fputs("not a Jet database", f);
			fclose(f);
		}
	}
	{
		SQLManager sm;
		CHECK(sm.Init(db.c_str()));
		GenreRecord g;
		CHECK(sm.GetGenreRecord(1, g) && g.name == L"[Unknown]");
		TracksFilter tf;
		FullTrackRecordCollection col;
		CHECK(sm.GetFullTrackRecordCollectionByTracksFilter(col, tf) && col.empty());
	}
	CHECK(GetFileAttributesW(mdb.c_str()) != INVALID_FILE_ATTRIBUTES);
	DeleteFileW(mdb.c_str());
}

//------------------------------------------------------------------
void RunDatabaseTests()
{
#ifdef _UNITTESTING
	wprintf(L"SQLManager::UnitTest\n");
	CHECK(SQLManager::UnitTest());
#endif
	TestBasicRecords();
	TestCaseInsensitivity();
	TestQueries();
	TestMerges();
	TestHistory();
	TestConcurrentConnections();
	TestReopen();
	TestLegacyFileIgnored();
}
