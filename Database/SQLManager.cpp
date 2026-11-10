#include "stdafx.h"
#include "SQLManager.h"
#include <algorithm>
#include "SQLite/SQLiteDB.h"
#include "JetImporter.h"
#include "stlStringUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//#define USE_INTEGRITY_CHECKS
#define USE_CACHED_QUERIES


#ifdef _UNITTESTING
BOOL SQLManager::UnitTest()
{
	TRACE(_T("@2 SQLManager:Unit-Testing\r\n"));
	TCHAR dbPath[MAX_PATH];
	GetTempPath(MAX_PATH, dbPath);
	_tcscat_s(dbPath, _T("jaangle-unittest.db"));
	DeleteFile(dbPath);
	{
		SQLManager sm;
		UNITTEST(sm.Init(dbPath));
	}
	{
		SQLManager sm;
		UNITTEST(sm.Init(dbPath));

		CollectionRecord col;
		col.location = _T("d:\\mmm.mp3");
		col.type = CTYPE_LocalFolder;
		UNITTEST(sm.AddNewCollectionRecord(col));
		UNITTEST(col.IsValid());
		col.name = _T("new col name");
		UNITTEST(sm.UpdateCollectionRecord(col));
		UNITTEST(sm.GetCollectionRecord(col.ID, col));
		UNITTEST(col.name == _T("new col name"));
		CollectionRecord col2;
		UNITTEST(sm.GetCollectionRecordUnique(col.serial, col.location.c_str(), col2));
		UNITTEST(col2.name == _T("new col name"));
		UNITTEST(sm.CollectionRecordExists(col2.ID));
		col.ID = 947;
		UNITTEST(!sm.UpdateCollectionRecord(col) && _T("Record is not Existent"));
		UNITTEST(!sm.GetCollectionRecord(col.ID, col) && _T("Record is not Existent"));
		UNITTEST(!sm.CollectionRecordExists(col.ID) && _T("Record is not Existent"));

		SYSTEMTIME st;
		GetLocalTime(&st);
		UNITTEST(sm.LogTrackInHistory(_T("ArtName#2"), _T("TraName#2"), HLA_Clicked, st));
		UNITTEST(sm.LogTrackInHistory(_T("ArtName#2"), _T("TraName#2"), HLA_Played, st));
		SQLManager::HistoryLogStats	stats;
		UNITTEST(sm.GetHistoryLogStats(stats));
		UNITTEST(stats.actions == 1);
		UNITTEST(stats.plays == 1);
	}
	DeleteFile(dbPath);
	return TRUE;
}
#endif

//==============================================================
//		INITIALIZATION - DESTRUCTION
//==============================================================

SQLManager::~SQLManager()
{
	delete m_pDB;
}

namespace
{
	void DeleteDatabaseFiles(LPCTSTR path)
	{
		std::tstring p(path);
		DeleteFile(p.c_str());
		DeleteFile((p + _T("-wal")).c_str());
		DeleteFile((p + _T("-shm")).c_str());
	}
}

BOOL SQLManager::Init(LPCTSTR DatabasePath)
{
	TRACE(_T("@3 SQLManager::Init. DatabasePath: %s\r\n"), DatabasePath);
	ASSERT(m_pDB == NULL);//Initialize only once
	if (m_pDB != NULL || DatabasePath == NULL)
		return FALSE;
	m_dbPath = DatabasePath;
	const BOOL bExisted = PathFileExists(DatabasePath);

	m_pDB = new SQLiteDB;
	if (!m_pDB->Open(DatabasePath))
	{
		TRACE(_T("@0 SQLManager::Init. DB Connection Failed. Aborting.\r\n"));
		delete m_pDB;
		m_pDB = NULL;
		return FALSE;
	}

	INT version = m_pDB->GetUserVersion();
	if (version == 0)
	{
		//=== New database: create the schema, importing a legacy Jet database if present.
		std::tstring mdbPath(DatabasePath);
		std::tstring::size_type dot = mdbPath.find_last_of(_T('.'));
		if (dot != std::tstring::npos && mdbPath.find_last_of(_T("\\/")) < dot)
			mdbPath.resize(dot);
		mdbPath += _T(".mdb");
		const BOOL bImport = !bExisted && PathFileExists(mdbPath.c_str());

		BOOL bOK = m_pDB->BeginTransaction();
		bOK = bOK && CreateSchema(!bImport);
		if (bOK && bImport)
			bOK = ImportLegacyDatabase(mdbPath.c_str());
		bOK = bOK && m_pDB->SetUserVersion(cLatestSchemaVersion);
		if (bOK)
			bOK = m_pDB->CommitTransaction();
		else
			m_pDB->RollbackTransaction();
		if (!bOK)
		{
			TRACE(_T("@0 SQLManager::Init. Schema creation / import failed: %s\r\n"), m_pDB->GetLastError());
			delete m_pDB;
			m_pDB = NULL;
			if (!bExisted)
				DeleteDatabaseFiles(DatabasePath);
			return FALSE;
		}
		version = cLatestSchemaVersion;
	}
	if (version > cLatestSchemaVersion || version < 0)
	{
		::MessageBox(0,
			_T("The music database is not compatible with this version of the program.\r\n")
			_T("Delete it or get the latest program version."),
			_T("Database Error"),
			MB_OK);
		delete m_pDB;
		m_pDB = NULL;
		return FALSE;
	}
	//=== Future schema upgrades: if (version == 1) {...; version = 2;}
	return TRUE;
}

BOOL SQLManager::CreateSchema(BOOL bWithDefaultRows)
{
	//Text columns use NOCASE_U (Unicode, case-insensitive): Jet compared text
	//case-insensitively and the lookups by name/path rely on it.
	//AUTOINCREMENT: like Jet COUNTER, IDs of deleted rows are never reused.
	static LPCTSTR const schema =
		_T("CREATE TABLE Genres (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U);")
		_T("CREATE INDEX GenresName ON Genres (Name);")

		_T("CREATE TABLE Artists (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" GenreID INTEGER NOT NULL,")
		_T(" Rating INTEGER NOT NULL);")
		_T("CREATE INDEX ArtistsName ON Artists (Name);")

		_T("CREATE TABLE Albums (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" artID INTEGER NOT NULL,")
		_T(" Year INTEGER NOT NULL,")
		_T(" Rating INTEGER NOT NULL);")
		_T("CREATE INDEX AlbumsName ON Albums (Name);")
		_T("CREATE INDEX AlbumsArtID ON Albums (artID);")

		_T("CREATE TABLE Collections (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" PathName TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" Serial INTEGER NOT NULL,")		//Media Serial Number
		_T(" Type INTEGER NOT NULL,")		//CollectionTypesEnum
		_T(" DateAdded TEXT NOT NULL,")
		_T(" DateUpdated TEXT);")
		_T("CREATE INDEX CollectionsName ON Collections (Name);")

		_T("CREATE TABLE Tracks (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" Type INTEGER NOT NULL,")		//TrackTypesEnum
		_T(" DateAdded TEXT NOT NULL,")
		_T(" albID INTEGER NOT NULL,")
		_T(" artID INTEGER NOT NULL,")
		_T(" CollectionID INTEGER NOT NULL,")
		_T(" genreID INTEGER NOT NULL,")
		_T(" Path TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" Bitrate INTEGER,")				//In Kbps
		_T(" Duration INTEGER,")			//In seconds
		_T(" Size INTEGER,")
		_T(" TrackNo INTEGER,")
		_T(" Year INTEGER NOT NULL,")
		_T(" Rating INTEGER NOT NULL);")
		_T("CREATE INDEX TracksName ON Tracks (Name);")
		_T("CREATE INDEX TracksPath ON Tracks (Path);")
		_T("CREATE INDEX TracksAlbID ON Tracks (albID);")
		_T("CREATE INDEX TracksArtID ON Tracks (artID);")
		_T("CREATE INDEX TracksGenreID ON Tracks (genreID);")
		_T("CREATE INDEX TracksCollectionID ON Tracks (CollectionID);")

		_T("CREATE TABLE HistArtists (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U);")
		_T("CREATE INDEX HistArtistsName ON HistArtists (Name);")

		_T("CREATE TABLE HistTracks (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" artID INTEGER NOT NULL);")
		_T("CREATE INDEX HistTracksName ON HistTracks (Name);")
		_T("CREATE INDEX HistTracksArtID ON HistTracks (artID);")

		_T("CREATE TABLE HistLog (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" actID INTEGER NOT NULL,")
		_T(" DateAdded TEXT NOT NULL,")
		_T(" trackID INTEGER NOT NULL);")
		_T("CREATE INDEX HistLogTrackID ON HistLog (trackID);")

		_T("CREATE TABLE TracksInfo (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" trackID INTEGER NOT NULL,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" DateAdded TEXT NOT NULL,")
		_T(" Type INTEGER NOT NULL);")
		_T("CREATE INDEX TracksInfoTrackID ON TracksInfo (trackID);")

		_T("CREATE TABLE AlbumsInfo (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" albID INTEGER NOT NULL,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" DateAdded TEXT NOT NULL,")
		_T(" Type INTEGER NOT NULL);")
		_T("CREATE INDEX AlbumsInfoAlbID ON AlbumsInfo (albID);")

		_T("CREATE TABLE ArtistsInfo (")
		_T(" ID INTEGER PRIMARY KEY AUTOINCREMENT,")
		_T(" artID INTEGER NOT NULL,")
		_T(" Name TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" DateAdded TEXT NOT NULL,")
		_T(" Type INTEGER NOT NULL);")
		_T("CREATE INDEX ArtistsInfoArtID ON ArtistsInfo (artID);")

		_T("CREATE TABLE LocalPic (")
		_T(" parID INTEGER NOT NULL,")
		_T(" infoType INTEGER NOT NULL,")
		_T(" Path TEXT NOT NULL COLLATE NOCASE_U,")
		_T(" Type INTEGER NOT NULL);")
		_T("CREATE UNIQUE INDEX LocalPicID ON LocalPic (parID, infoType);")

		_T("CREATE TABLE InfoRequests (")
		_T(" requestTimeStamp INTEGER NOT NULL,")
		_T(" hash INTEGER NOT NULL);")
		_T("CREATE UNIQUE INDEX hashInfoRequests ON InfoRequests (hash);");

	static LPCTSTR const defaultRows =
		_T("INSERT INTO Genres (ID, Name) VALUES (1, '[Unknown]');")
		_T("INSERT INTO Artists (ID, Name, GenreID, Rating) VALUES (1, '[Unknown]', 1, 0);")
		_T("INSERT INTO Artists (ID, Name, GenreID, Rating) VALUES (2, '[Various]', 1, 0);")
		_T("INSERT INTO HistArtists (ID, Name) VALUES (1, '[Unknown]');");

	TRACE(_T("@2 SQLManager::CreateSchema\r\n"));
	if (m_pDB->Execute(schema) == -1)
		return FALSE;
	if (bWithDefaultRows && m_pDB->Execute(defaultRows) == -1)
		return FALSE;
	return TRUE;
}

BOOL SQLManager::ImportLegacyDatabase(LPCTSTR mdbPath)
{
	TRACE(_T("@2 SQLManager::ImportLegacyDatabase '%s'\r\n"), mdbPath);
	std::tstring error;
	if (JetImporter::Import(mdbPath, *m_pDB, error))
		return TRUE;
	std::tstring msg(_T("The existing music database could not be converted:\r\n"));
	msg += mdbPath;
	msg += _T("\r\n\r\n");
	msg += error;
	msg += _T("\r\n\r\nThe old database has not been modified.");
	::MessageBox(0, msg.c_str(), _T("Database Error"), MB_OK | MB_ICONERROR);
	return FALSE;
}

void SQLManager::InvalidateCaches()
{
	m_genreCache.ID = 0;
	m_albumCache.ID = 0;
	m_artistCache.ID = 0;
}

std::tstring SQLManager::SQLQuote(LPCTSTR value)
{
	std::tstring ret(_T("'"));
	if (value != NULL)
	{
		for (LPCTSTR p = value; *p != 0; p++)
		{
			if (*p == _T('\''))
				ret += _T('\'');
			ret += *p;
		}
	}
	ret += _T("'");
	return ret;
}

std::tstring SQLManager::SQLDate(const SYSTEMTIME& input)
{
	SYSTEMTIME day = input;
	day.wHour = day.wMinute = day.wSecond = day.wMilliseconds = 0;
	std::tstring ret(_T("'"));
	ret += SQLiteDB::FormatDate(day);
	ret += _T("'");
	return ret;
}


INT SQLManager::AddNewTrackFromTagInfo(UINT collectionID, LPCTSTR location, TagInfo& info)
{
	TRACEST(_T("SQLManager::AddNewTrackFromTagInfo"));
	ASSERT(collectionID != 0 && location != NULL);
	if (collectionID == 0)
	{
		TRACE(_T("@1 SQLManager::AddNewTrackFromTagInfo collectionID: 0\r\n"));
		return 0;
	}
	if (location == 0)
	{
		TRACE(_T("@1 SQLManager::AddNewTrackFromTagInfo location: 0\r\n"));
		return 0;
	}
	TrackRecord track;
	track.colID = collectionID;
	track.location = location;
	track.trackType = GetTrackType(location);
	::GetLocalTime(&track.dateAdded);
	if (info.IsValid(TagInfo_Bitrate))
		track.bitrate = info.Bitrate;
	if (info.IsValid(TagInfo_Length))
		track.duration = info.Length;
	if (info.IsValid(TagInfo_Title))
		track.name = info.Title;
	if (info.IsValid(TagInfo_Rating))
		track.rating = info.Rating;
	if (info.IsValid(TagInfo_Size))
		track.size = info.Size;
	if (info.IsValid(TagInfo_TrackNo))
		track.trackNo = info.TrackNo;
	if (info.IsValid(TagInfo_Year))
		track.year = info.Year;
	if (track.year < 1000)
		track.year = 0;

	//======Genres
	trim(info.Genre);
	LPCTSTR genreName = _T("");
	if (info.IsValid(TagInfo_Genre))
		genreName = info.Genre.c_str();
	GenreRecord genre;
	if (!GetGenreRecordUnique(genreName, genre))//This Genre Exists in Database
	{
		genre.name = genreName;
		if (!AddNewGenreRecord(genre))
			return 0;
	}
	track.genID = genre.ID;
	//=======Artists
	trim(info.Artist);
	LPCTSTR artistName = _T("");
	if (info.IsValid(TagInfo_Artist))
		artistName = info.Artist.c_str();
	ArtistRecord artist;
	if (!GetArtistRecordUnique(artistName, artist))//This Artist Exists in Database
	{
		artist.name = artistName;
		artist.genID = track.genID;
		if (!AddNewArtistRecord(artist))
			return 0;
	}
	track.artID = artist.ID;
	//=======Albums
	trim(info.Album);
	LPCTSTR albumName = _T("");
	if (info.IsValid(TagInfo_Album))
		albumName = info.Album.c_str();
	AlbumRecord album;
	if (!GetAlbumRecordUnique(track.artID, albumName, album))//This album Exists in Database
	{
		album.artID = track.artID;
		album.year = track.year;
		album.name = albumName;
		if (!AddNewAlbumRecord(album))
			return 0;
	}
	track.albID = album.ID;

	if (!AddNewTrackRecord(track))
		return 0;

	//=======Comment
	if (info.IsValid(TagInfo_Comment))
	{
		trim(info.Comment);
		if (!info.Comment.empty())
		{
			if (!AddNewTrackInfo(track.ID, IIT_TrackComment, info.Comment.c_str()))
				TRACE(_T("@1 SQLManager::AddNewTrackFromTagInfo Add Comments failed\r\n"));
		}
	}
	//=======Lyrics
	if (info.IsValid(TagInfo_Lyrics))
	{
		trim(info.Lyrics);
		if (!info.Lyrics.empty())
		{
			if (!AddNewTrackInfo(track.ID, IIT_TrackLyrics, info.Lyrics.c_str()))
				TRACE(_T("@1 SQLManager::AddNewTrackFromTagInfo Add Lyrics failed\r\n"));
		}
	}
	return track.ID;
}

BOOL SQLManager::UpdateTrackFromTagInfo(UINT ID, UINT collectionID, LPCTSTR location, TagInfo& info)
{
	ASSERT(ID != 0);
	ASSERT(collectionID != 0);
	ASSERT(location != NULL);
	TrackRecord track;
	if (!GetTrackRecord(ID, track))
		return FALSE;

	track.colID = collectionID;
	track.location = location;
	track.trackType = GetTrackType(location);
	//dateAdded stays the same
	if (info.IsValid(TagInfo_Bitrate))
		track.bitrate = info.Bitrate;
	if (info.IsValid(TagInfo_Length))
		track.duration = info.Length;
	if (info.IsValid(TagInfo_Title))
		track.name = info.Title;
	if (info.IsValid(TagInfo_Rating))
		track.rating = info.Rating;
	if (info.IsValid(TagInfo_Size))
		track.size = info.Size;
	if (info.IsValid(TagInfo_TrackNo))
		track.trackNo = info.TrackNo;
	if (info.IsValid(TagInfo_Year))
		track.year = info.Year;

	//======Genres
	if (info.IsValid(TagInfo_Genre))
	{
		trim(info.Genre);
		GenreRecord genre;
		if (!GetGenreRecordUnique(info.Genre.c_str(), genre))//This Genre Exists in Database
		{
			genre.name = info.Genre;
			if (!AddNewGenreRecord(genre))
				return FALSE;
		}
		track.genID = genre.ID;
	}
	//=======Artists
	if (info.IsValid(TagInfo_Artist))
	{
		trim(info.Artist);
		ArtistRecord artist;
		if (!GetArtistRecordUnique(info.Artist.c_str(), artist))//This Artist Exists in Database
		{
			artist.name = info.Artist;
			artist.genID = track.genID;
			if (!AddNewArtistRecord(artist))
				return FALSE;
		}
		track.artID = artist.ID;
		//=== Test the case that. We are changing the artist name "Doors"->"Nirvana"
		//		but not the Album Name.
		if (!info.IsValid(TagInfo_Album))
		{
			AlbumRecord existing;
			if (!GetAlbumRecord(track.albID, existing))
			{
				TRACE(_T("@1 SQLManager::UpdateTrackFromTagInfo. DB Inconsistent albID not found\r\n"));
				return 0;
			}
			if (existing.artID != artist.ID)
			{
				//=== We have changed Artist but not Album Name
				AlbumRecord album;
				if (!GetAlbumRecordUnique(track.artID, existing.name.c_str(), album))
				{
					//=== There is no Album with such name in this new artist
					album.artID = track.artID;
					album.year = existing.year;
					album.name = existing.name.c_str();
					if (!AddNewAlbumRecord(album))
						return FALSE;
				}
				track.albID = album.ID;
			}
		}
	}
	//=======Albums
	if (info.IsValid(TagInfo_Album))
	{
		trim(info.Album);
		AlbumRecord album;
		if (!GetAlbumRecordUnique(track.artID, info.Album.c_str(), album))//This album Exists in Database
		{
			album.artID = track.artID;
			album.year = info.Year;
			album.name = info.Album;
			if (!AddNewAlbumRecord(album))
				return FALSE;
		}
		track.albID = album.ID;
	}

	//=======Comment
	if (info.IsValid(TagInfo_Comment))
	{
		trim(info.Comment);
		AdjustTrackInfo(track.ID, IIT_TrackComment, info.Comment.c_str());
	}
	//=======Lyrics
	if (info.IsValid(TagInfo_Lyrics))
	{
		trim(info.Lyrics);
		AdjustTrackInfo(track.ID, IIT_TrackLyrics, info.Lyrics.c_str());
	}
	return UpdateTrackRecord(track);
}


//==============================================
//=================== SQL Strings / GetField functions
//==============================================

const TCHAR* const TrackSQL = _T("SELECT Tracks.ID,Tracks.Name,Tracks.Type,Tracks.DateAdded,")
_T("Tracks.albID,Tracks.artID,Tracks.CollectionID,Tracks.GenreID,Tracks.Path,")
_T("Tracks.Bitrate,Tracks.Duration,Tracks.Size,Tracks.TrackNo,")
_T("Tracks.Year,Tracks.Rating ")
_T("FROM Tracks ");

void SQLManager::GetTrackRecordFields(TrackRecord& rec, SQLiteStmt& rs, short& fieldID)
{
	rec.ID = rs.GetUINTFieldValue(fieldID++);
	rs.GetStringFieldValue(fieldID++, rec.name);
	rec.trackType = (TrackTypesEnum)rs.GetINTFieldValue(fieldID++);
	rec.dateAdded = rs.GetSYSTEMTIMEFieldValue(fieldID++);

	rec.albID = rs.GetINTFieldValue(fieldID++);
	rec.artID = rs.GetINTFieldValue(fieldID++);
	rec.colID = rs.GetINTFieldValue(fieldID++);
	rec.genID = rs.GetINTFieldValue(fieldID++);
	rs.GetStringFieldValue(fieldID++, rec.location);

	rec.bitrate = rs.GetINTFieldValue(fieldID++);
	rec.duration = rs.GetINTFieldValue(fieldID++);
	rec.size = rs.GetINTFieldValue(fieldID++);
	rec.trackNo = rs.GetINTFieldValue(fieldID++);

	rec.year = rs.GetINTFieldValue(fieldID++);
	rec.rating = rs.GetINTFieldValue(fieldID++);

	ASSERT(rec.IsValid());
}

const TCHAR* const AlbumSQL = _T("SELECT ")
_T("ID, Name, artID, Year, Rating ")
_T("FROM Albums ");

void SQLManager::GetAlbumRecordFields(AlbumRecord& album, SQLiteStmt& rs, short& startIdx)
{
	album.ID = rs.GetUINTFieldValue(startIdx++);
	rs.GetStringFieldValue(startIdx++, album.name);
	album.artID = rs.GetUINTFieldValue(startIdx++);
	album.year = rs.GetINTFieldValue(startIdx++);
	album.rating = rs.GetSHORTFieldValue(startIdx++);
	ASSERT(album.IsValid());
}

const TCHAR* const ArtistSQL = _T("SELECT ")
_T("ID, Name, GenreID, Rating ")
_T("FROM Artists ");

void SQLManager::GetArtistRecordFields(ArtistRecord& artist, SQLiteStmt& rs, short& startIdx)
{
	artist.ID = rs.GetUINTFieldValue(startIdx++);
	rs.GetStringFieldValue(startIdx++, artist.name);
	artist.genID = rs.GetUINTFieldValue(startIdx++);
	artist.rating = rs.GetSHORTFieldValue(startIdx++);
}

const TCHAR* const GenreSQL = _T("SELECT ")
_T("Genres.ID, Genres.Name ")
_T(", -1 ")
_T("FROM Genres ");

//Grouped queries select "bare" columns: SQLite takes all of them from the same
//row of each group, which is what Jet's first() did.
const TCHAR* const CountGenreSQL = _T("SELECT ")
_T("Genres.ID, Genres.Name")
_T(", count(*) ")
_T("FROM Tracks INNER JOIN Genres ON Tracks.genreID = Genres.ID ");

void SQLManager::GetGenreRecordFields(GenreRecord& genre, SQLiteStmt& rs, short& startIdx)
{
	genre.ID = rs.GetUINTFieldValue(startIdx++);
	rs.GetStringFieldValue(startIdx++, genre.name);
	ASSERT(genre.IsValid());
}

const TCHAR* const CollectionSQL = _T("SELECT ")
_T("Collections.ID, Collections.Name, Collections.PathName ")
_T(", Collections.Serial, Collections.Type ")
_T(", Collections.DateAdded, Collections.DateUpdated ")
_T(", 0, 0, 0 ")
_T("FROM Collections ");

const TCHAR* const CountCollectionSQL = _T("SELECT ")
_T("Collections.ID, Collections.Name, Collections.PathName ")
_T(", Collections.Serial, Collections.Type ")
_T(", Collections.DateAdded, Collections.DateUpdated ")
_T(", count(Tracks.ID), Sum(Tracks.Size), Sum(Tracks.Duration) ")
_T("FROM Collections LEFT JOIN Tracks ON Collections.ID = Tracks.CollectionID ");

void SQLManager::GetCollectionRecordFields(CollectionRecord& rec, SQLiteStmt& rs, short& startIdx)
{
	rec.ID = rs.GetINTFieldValue(startIdx++);
	rs.GetStringFieldValue(startIdx++, rec.name);
	rs.GetStringFieldValue(startIdx++, rec.location);
	rec.serial = rs.GetINTFieldValue(startIdx++);
	rec.type = (CollectionTypesEnum) rs.GetINTFieldValue(startIdx++);
	rec.dateAdded = rs.GetSYSTEMTIMEFieldValue(startIdx++);
	rec.dateUpdated = rs.GetSYSTEMTIMEFieldValue(startIdx++);
	ASSERT(rec.IsValid());
}

const TCHAR* const HistArtistSQL = _T("SELECT ")
_T("ID, Name ")
_T("FROM HistArtists ");

void SQLManager::GetHistArtistRecordFields(HistArtistRecord& rec, SQLiteStmt& rs, short& startIdx)
{
	rec.ID = rs.GetUINTFieldValue(startIdx++);
	rs.GetStringFieldValue(startIdx++, rec.name);
	ASSERT(rec.IsValid());
}

const TCHAR* const HistLogSQL = _T("SELECT ")
_T("ID, actID, DateAdded, trackID ")
_T("FROM HistLog ");

void SQLManager::GetHistLogRecordFields(HistLogRecord& rec, SQLiteStmt& rs, short& startIdx)
{
	rec.ID = rs.GetUINTFieldValue(startIdx++);
	rec.actID = (HistoryLogActionsEnum) rs.GetINTFieldValue(startIdx++);
	rec.dateAdded = rs.GetSYSTEMTIMEFieldValue(startIdx++);
	rec.trackID = rs.GetUINTFieldValue(startIdx++);
	ASSERT(rec.IsValid());
}

const TCHAR* const HistTrackSQL = _T("SELECT ")
_T("ID, Name, artID ")
_T("FROM HistTracks ");

void SQLManager::GetHistTrackRecordFields(HistTrackRecord& rec, SQLiteStmt& rs, short& fieldID)
{
	rec.ID = rs.GetUINTFieldValue(fieldID++);
	rs.GetStringFieldValue(fieldID++, rec.name);
	rec.artID = rs.GetUINTFieldValue(fieldID++);
	ASSERT(rec.IsValid());
}


//=============================================
//====================Retrieve Collections By SQL
//=============================================
BOOL SQLManager::GetTrackRecordCollectionBySQL(TrackRecordCollection& col, LPCTSTR sql, INT limitResults)
{
	ASSERT(sql != NULL && sql[0] != 0);
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(sql))
		return FALSE;
	INT countResults = 0;
	while (!rs.IsEOF())
	{
		TrackRecordSP rec(new TrackRecord);
		short startIdx = 0;
		GetTrackRecordFields(*rec, rs, startIdx);
		col.push_back(rec);
		countResults++;
		if (countResults == limitResults)
			return TRUE;
		rs.MoveNext();
	}
	return TRUE;
}

BOOL SQLManager::GetGenreRecordCollectionBySQL(GenreRecordCollection& col, LPCTSTR sql)
{
	ASSERT(sql != NULL && sql[0] != 0);
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(sql))
		return FALSE;
	while (!rs.IsEOF())
	{
		GenreRecordSP rec(new GenreRecord);
		short startIdx = 0;
		GetGenreRecordFields(*rec, rs, startIdx);
		rec->trackCount = rs.GetINTFieldValue(startIdx++);
		col.push_back(rec);
		rs.MoveNext();
	}
	return TRUE;
}

BOOL SQLManager::GetCollectionRecordCollectionBySQL(CollectionRecordCollection& col, LPCTSTR sql)
{
	ASSERT(sql != NULL && sql[0] != 0);
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(sql))
		return FALSE;
	while (!rs.IsEOF())
	{
		CollectionRecordSP rec(new CollectionRecord);
		short startIdx = 0;
		GetCollectionRecordFields(*rec, rs, startIdx);
		rec->trackCount = rs.GetUINTFieldValue(startIdx++);
		rec->sumSize = (UINT)rs.GetDOUBLEFieldValue(startIdx++);
		rec->sumDuration = (UINT)rs.GetDOUBLEFieldValue(startIdx++);
		col.push_back(rec);
		rs.MoveNext();
	}
	return TRUE;
}

BOOL SQLManager::GetYearRecordCollectionBySQL(YearRecordCollection& col, LPCTSTR sql)
{
	ASSERT(sql != NULL && sql[0] != 0);
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(sql))
		return FALSE;
	while (!rs.IsEOF())
	{
		YearRecordSP rec(new YearRecord);
		short startIdx = 0;
		rec->year = rs.GetINTFieldValue(startIdx++);
		rec->trackCount = rs.GetINTFieldValue(startIdx++);
		col.push_back(rec);
		rs.MoveNext();
	}
	return TRUE;
}

BOOL SQLManager::GetMonthAddedRecordCollectionBySQL(MonthAddedRecordCollection& col, LPCTSTR sql)
{
	ASSERT(sql != NULL && sql[0] != 0);
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(sql))
		return FALSE;
	while (!rs.IsEOF())
	{
		MonthAddedRecordSP rec(new MonthAddedRecord);
		short startIdx = 0;
		rec->month = rs.GetINTFieldValue(startIdx++);
		rec->trackCount = rs.GetINTFieldValue(startIdx++);
		col.push_back(rec);
		rs.MoveNext();
	}
	return TRUE;
}


BOOL SQLManager::GetFullTrackRecordCollectionBySQL(FullTrackRecordCollection& col, LPCTSTR sql, INT limitResults)
{
	ASSERT(sql != NULL && sql[0] != 0);
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(sql))
		return FALSE;
	INT countResults = 0;
	while (!rs.IsEOF())
	{
		FullTrackRecordSP rec(new FullTrackRecord);
		short fieldID = 0;
		GetTrackRecordFields(rec->track, rs, fieldID);
		GetAlbumRecordFields(rec->album, rs, fieldID);
		GetArtistRecordFields(rec->artist, rs, fieldID);
		GetGenreRecordFields(rec->genre, rs, fieldID);
		GetCollectionRecordFields(rec->collection, rs, fieldID);
		ASSERT(rec->IsValid());
		col.push_back(rec);
		countResults++;
		if (countResults == limitResults)
			return TRUE;
		rs.MoveNext();
	}
	return TRUE;
}

BOOL SQLManager::GetFullAlbumRecordCollectionBySQL(FullAlbumRecordCollection& col, LPCTSTR sql)
{
	ASSERT(sql != NULL && sql[0] != 0);
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(sql))
		return FALSE;
	while (!rs.IsEOF())
	{
		FullAlbumRecordSP rec(new FullAlbumRecord);
		short fieldID = 0;
		GetAlbumRecordFields(rec->album, rs, fieldID);
		GetArtistRecordFields(rec->artist, rs, fieldID);
		rec->album.trackCount = rs.GetINTFieldValue(fieldID++);
		col.push_back(rec);
		rs.MoveNext();
	}
	return TRUE;
}

BOOL SQLManager::GetFullArtistRecordCollectionBySQL(FullArtistRecordCollection& col, LPCTSTR sql)
{
	ASSERT(sql != NULL && sql[0] != 0);
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(sql))
		return FALSE;
	while (!rs.IsEOF())
	{
		FullArtistRecordSP rec(new FullArtistRecord);
		short fieldID = 0;
		GetArtistRecordFields(rec->artist, rs, fieldID);
		GetGenreRecordFields(rec->genre, rs, fieldID);
		rec->artist.trackCount = rs.GetINTFieldValue(fieldID++);
		col.push_back(rec);
		rs.MoveNext();
	}
	return TRUE;
}


//=============================================
//====================Pictures / Info requests
//=============================================

BOOL SQLManager::GetPicRecord(PicRecord& rec)
{
	ASSERT(rec.parID > 0);
	if (rec.parID == 0)
		return FALSE;
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(_T("SELECT path,type FROM LocalPic WHERE parID=? AND infoType=?")) &&
		rs.BindUInt(1, rec.parID) && rs.BindInt(2, rec.infoType) &&
		rs.Step() && !rs.IsEOF())
	{
		short fieldID = 0;
		rs.GetStringFieldValue(fieldID++, rec.path);
		rec.pictureType = (PictureTypeEnum)rs.GetINTFieldValue(fieldID++);
		ASSERT(rec.IsValid());
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::AddOrUpdatePicRecord(PicRecord& rec)
{
	ASSERT(rec.parID != NULL);
	if (rec.parID == 0)
		return FALSE;
	SQLiteStmt st(m_pDB);
	return st.Prepare(
			_T("INSERT INTO LocalPic (parID, infoType, Path, Type) VALUES (?,?,?,?) ")
			_T("ON CONFLICT(parID, infoType) DO UPDATE SET Path=excluded.Path, Type=excluded.Type")) &&
		st.BindUInt(1, rec.parID) && st.BindInt(2, rec.infoType) &&
		st.BindText(3, rec.path.c_str()) && st.BindInt(4, rec.pictureType) &&
		st.Exec() > 0;
}

BOOL SQLManager::DeletePicRecord(InfoItemTypeEnum iit, UINT parID)
{
	ASSERT(iit >= IIT_Unknown && iit < IIT_Last);
	ASSERT(m_pDB != NULL);
	SQLiteStmt st(m_pDB);
	return st.Prepare(_T("DELETE FROM LocalPic WHERE parID=? AND infoType=?")) &&
		st.BindUInt(1, parID) && st.BindInt(2, iit) && st.Exec() != -1;
}

BOOL SQLManager::AddInfoRequest(UINT timeStamp, UINT hash)
{
	SQLiteStmt st(m_pDB);
	return st.Prepare(_T("INSERT OR IGNORE INTO InfoRequests (requestTimeStamp,hash) VALUES (?,?)")) &&
		st.BindUInt(1, timeStamp) && st.BindUInt(2, hash) && st.Exec() != -1;
}

BOOL SQLManager::DeleteInfoRequestsOlderThan(UINT timeStampLimit)
{
	SQLiteStmt st(m_pDB);
	return st.Prepare(_T("DELETE FROM InfoRequests WHERE requestTimeStamp<?")) &&
		st.BindUInt(1, timeStampLimit) && st.Exec() != -1;
}

BOOL SQLManager::GetInfoRequests(std::set<UINT>& requestSet)
{
	SQLiteStmt rs(m_pDB);
	if (rs.Open(_T("SELECT hash FROM InfoRequests")))
	{
		while (!rs.IsEOF())
		{
			requestSet.insert(rs.GetUINTFieldValue(0));
			rs.MoveNext();
		}
	}
	return TRUE;
}


//=============================================
//====================Retrieve Records By ID
//=============================================

//===============Get-Xxx-Record: Gets a record from ID
//REQUIRE:	ID!=0
//RETURN:	FALSE if xxx does not exists || failure
//			TRUE on success.

#define GET_RECORD_BY_ID(baseSQL, readFields)							\
	SQLiteStmt rs(m_pDB);												\
	std::tstring sql(baseSQL);											\
	sql += _T(" WHERE ID=?");											\
	if (rs.Prepare(sql.c_str()) && rs.BindUInt(1, ID) && rs.Step() && !rs.IsEOF())	\
	{																	\
		short startIdx = 0;												\
		readFields(rec, rs, startIdx);									\
		return TRUE;													\
	}																	\
	return FALSE;

BOOL SQLManager::GetTrackRecord(UINT ID, TrackRecord& rec)
{
	ASSERT(ID > 0);
	GET_RECORD_BY_ID(TrackSQL, GetTrackRecordFields);
}
BOOL SQLManager::GetAlbumRecord(UINT ID, AlbumRecord& rec)
{
	ASSERT(ID > 0);
	GET_RECORD_BY_ID(AlbumSQL, GetAlbumRecordFields);
}
BOOL SQLManager::GetArtistRecord(UINT ID, ArtistRecord& rec)
{
	ASSERT(ID > 0);
	GET_RECORD_BY_ID(ArtistSQL, GetArtistRecordFields);
}
BOOL SQLManager::GetGenreRecord(UINT ID, GenreRecord& rec)
{
	ASSERT(ID > 0);
	GET_RECORD_BY_ID(GenreSQL, GetGenreRecordFields);
}
BOOL SQLManager::GetCollectionRecord(UINT ID, CollectionRecord& rec)
{
	ASSERT(ID > 0);
	GET_RECORD_BY_ID(CollectionSQL, GetCollectionRecordFields);
}
BOOL SQLManager::GetHistTrackRecord(UINT ID, HistTrackRecord& rec)
{
	ASSERT(ID > 0);
	GET_RECORD_BY_ID(HistTrackSQL, GetHistTrackRecordFields);
}
BOOL SQLManager::GetHistArtistRecord(UINT ID, HistArtistRecord& rec)
{
	ASSERT(ID > 0);
	GET_RECORD_BY_ID(HistArtistSQL, GetHistArtistRecordFields);
}
BOOL SQLManager::GetHistLogRecord(UINT ID, HistLogRecord& rec)
{
	GET_RECORD_BY_ID(HistLogSQL, GetHistLogRecordFields);
}

//=============================================
//====================Retrieve Records By Unique Key
//=============================================

//===============Get-Xxx-RecordUnique
//RETURN:	FALSE if xxx does not exists || failure
//			TRUE on success.
BOOL SQLManager::GetTrackRecordUnique(UINT colID, LPCTSTR location, TrackRecord& rec)
{
	ASSERT(colID > 0);
	ASSERT(location != NULL);
	if (location == NULL)
		return FALSE;
	std::tstring sql(TrackSQL);
	sql += _T(" WHERE collectionID=? AND path=?");
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(sql.c_str()) && rs.BindUInt(1, colID) && rs.BindText(2, location) &&
		rs.Step() && !rs.IsEOF())
	{
		short startIdx = 0;
		GetTrackRecordFields(rec, rs, startIdx);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::GetAlbumRecordUnique(UINT artID, LPCTSTR name, AlbumRecord& album)
{
	ASSERT(name != NULL);
	if (name == NULL)
		return FALSE;
	std::tstring fname(name);
	if (fname.empty())
		fname = TS_UnknownString;
#ifdef USE_CACHED_QUERIES
	if (m_albumCache.ID != 0 && m_albumCache.artID == artID && m_albumCache.name == fname)
	{
		album = m_albumCache;
		return TRUE;
	}
#endif // USE_CACHED_QUERIES
	std::tstring sql(AlbumSQL);
	sql += _T(" WHERE name=? AND artID=?");
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(sql.c_str()) && rs.BindText(1, fname.c_str()) && rs.BindUInt(2, artID) &&
		rs.Step() && !rs.IsEOF())
	{
		short startIdx = 0;
		GetAlbumRecordFields(album, rs, startIdx);
#ifdef USE_CACHED_QUERIES
		m_albumCache = album;
#endif // USE_CACHED_QUERIES
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::GetArtistRecordUnique(LPCTSTR name, ArtistRecord& artist)
{
	ASSERT(name != NULL);
	if (name == NULL)
		return FALSE;
	if (name[0] == 0)
	{
		artist.ID = 1;
		artist.name = name;
		return TRUE;
	}
#ifdef USE_CACHED_QUERIES
	if (m_artistCache.ID != 0 && m_artistCache.name == name)
	{
		artist = m_artistCache;
		return TRUE;
	}
#endif // USE_CACHED_QUERIES
	std::tstring sql(ArtistSQL);
	sql += _T(" WHERE name=?");
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(sql.c_str()) && rs.BindText(1, name) && rs.Step() && !rs.IsEOF())
	{
		short startIdx = 0;
		GetArtistRecordFields(artist, rs, startIdx);
#ifdef USE_CACHED_QUERIES
		m_artistCache = artist;
#endif // USE_CACHED_QUERIES
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::GetGenreRecordUnique(LPCTSTR name, GenreRecord& genre)
{
	ASSERT(name != NULL);
	if (name == NULL)
		return FALSE;
	if (name[0] == 0)
	{
		genre.ID = 1;
		genre.name = TS_UnknownString;
		return TRUE;
	}
#ifdef USE_CACHED_QUERIES
	if (m_genreCache.ID != 0 && m_genreCache.name == name)
	{
		genre = m_genreCache;
		return TRUE;
	}
#endif // USE_CACHED_QUERIES
	std::tstring sql(GenreSQL);
	sql += _T(" WHERE name=?");
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(sql.c_str()) && rs.BindText(1, name) && rs.Step() && !rs.IsEOF())
	{
		short startIdx = 0;
		GetGenreRecordFields(genre, rs, startIdx);
#ifdef USE_CACHED_QUERIES
		m_genreCache = genre;
#endif // USE_CACHED_QUERIES
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::GetCollectionRecordUnique(UINT serial, LPCTSTR location, CollectionRecord& rec)
{
	ASSERT(location != NULL);
	if (location == NULL)
		return FALSE;
	std::tstring sql(CollectionSQL);
	sql += _T(" WHERE serial=? AND pathName=?");
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(sql.c_str()) && rs.BindInt(1, (INT)serial) && rs.BindText(2, location) &&
		rs.Step() && !rs.IsEOF())
	{
		short startIdx = 0;
		GetCollectionRecordFields(rec, rs, startIdx);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::GetHistTrackRecordUnique(UINT artID, LPCTSTR name, HistTrackRecord& rec)
{
	ASSERT(artID != 0 && name != NULL);
	if (artID == 0 || name == NULL)
		return FALSE;
	std::tstring sql(HistTrackSQL);
	sql += _T(" WHERE artID=? AND name=?");
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(sql.c_str()) && rs.BindUInt(1, artID) && rs.BindText(2, name) &&
		rs.Step() && !rs.IsEOF())
	{
		short startIdx = 0;
		GetHistTrackRecordFields(rec, rs, startIdx);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::GetHistArtistRecordUnique(LPCTSTR name, HistArtistRecord& rec)
{
	ASSERT(name != NULL);
	if (name == NULL)
		return FALSE;
	std::tstring sql(HistArtistSQL);
	sql += _T(" WHERE name=?");
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(sql.c_str()) && rs.BindText(1, name) && rs.Step() && !rs.IsEOF())
	{
		short startIdx = 0;
		GetHistArtistRecordFields(rec, rs, startIdx);
		return TRUE;
	}
	return FALSE;
}


//===============Get-Xxx-ID: Gets the ID
//RETURN:	0 if xxx does not exists || failure
//			Database ID on success.
UINT SQLManager::GetTrackID(UINT colID, LPCTSTR location)
{
	TrackRecord rec;
	return GetTrackRecordUnique(colID, location, rec) ? rec.ID : 0;
}
UINT SQLManager::GetAlbumID(UINT artID, LPCTSTR name)
{
	ASSERT(name != NULL);
	if (name == NULL)
		return 0;
	std::tstring fname(name);
	if (fname.empty())
		fname = TS_UnknownString;
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(_T("SELECT ID FROM Albums WHERE name=? AND artID=?")) &&
		rs.BindText(1, fname.c_str()) && rs.BindUInt(2, artID) && rs.Step() && !rs.IsEOF())
		return rs.GetUINTFieldValue(0);
	return 0;
}
UINT SQLManager::GetArtistID(LPCTSTR name)
{
	ASSERT(name != NULL);
	if (name == NULL)
		return 0;
	if (name[0] == 0)
		return 1;
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(_T("SELECT ID FROM Artists WHERE name=?")) &&
		rs.BindText(1, name) && rs.Step() && !rs.IsEOF())
		return rs.GetUINTFieldValue(0);
	return 0;
}
UINT SQLManager::GetGenreID(LPCTSTR name)
{
	ASSERT(name != NULL);
	if (name == NULL)
		return 0;
	if (name[0] == 0)
		return 1;
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(_T("SELECT ID FROM Genres WHERE name=?")) &&
		rs.BindText(1, name) && rs.Step() && !rs.IsEOF())
		return rs.GetUINTFieldValue(0);
	return 0;
}
UINT SQLManager::GetCollectionID(UINT serial, LPCTSTR location)
{
	CollectionRecord rec;
	return GetCollectionRecordUnique(serial, location, rec) ? rec.ID : 0;
}


//=============================================
//====================AddNew Records
//=============================================

//===============AddNew-Xxx-Record: Adds a new Xxx record
//REQUIRE:	xxx with ID==0 && name != "" && foreign keys valid
//RETURN:	FALSE if xxx exists || failure
//			TRUE on success. Modifies ID with the new ID
//INSERT ... RETURNING ID gives the new ID atomically, even when the
//connection is shared between threads.

BOOL SQLManager::AddNewTrackRecord(TrackRecord& rec)
{
	ASSERT(rec.ID == 0);
	ASSERT(!rec.location.empty());
	ASSERT(rec.artID != 0);
	ASSERT(rec.albID != 0);
	ASSERT(rec.genID != 0);
	ASSERT(rec.colID != 0);
	ASSERT(rec.trackType != TTYPE_Unsupported);
	if (rec.ID != 0 || rec.location.empty() || rec.artID == 0 || rec.albID == 0 ||
		rec.genID == 0 || rec.colID == 0 || rec.trackType == TTYPE_Unsupported)
		return FALSE;
	if (rec.name.empty())
		rec.name = TS_UnknownString;
#ifdef USE_INTEGRITY_CHECKS
	TrackRecord existing;
	if (GetTrackRecordUnique(rec.colID, rec.location.c_str(), existing))
		return FALSE;
	if (!ArtistRecordExists(rec.artID))			return FALSE;
	if (!AlbumRecordExists(rec.albID))			return FALSE;
	if (!GenreRecordExists(rec.genID))			return FALSE;
	if (!CollectionRecordExists(rec.colID))		return FALSE;
#endif // USE_INTEGRITY_CHECKS
	SQLiteStmt st(m_pDB);
	if (st.Prepare(_T("INSERT INTO Tracks (Name,Type,DateAdded,albID,artID,CollectionID,genreID,")
			_T("Path,Bitrate,Duration,Size,TrackNo,Year,Rating) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?) RETURNING ID")) &&
		st.BindText(1, rec.name.c_str()) && st.BindInt(2, rec.trackType) &&
		st.BindDate(3, rec.dateAdded) && st.BindUInt(4, rec.albID) &&
		st.BindUInt(5, rec.artID) && st.BindUInt(6, rec.colID) &&
		st.BindUInt(7, rec.genID) && st.BindText(8, rec.location.c_str()) &&
		st.BindInt(9, rec.bitrate) && st.BindInt(10, rec.duration) &&
		st.BindInt(11, rec.size) && st.BindInt(12, rec.trackNo) &&
		st.BindInt(13, rec.year) && st.BindInt(14, rec.rating) &&
		st.Step() && !st.IsEOF())
	{
		m_changes.tracks.additions++;
		rec.ID = st.GetUINTFieldValue(0);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::AddNewAlbumRecord(AlbumRecord& rec)
{
	ASSERT(rec.ID == 0);
	ASSERT(rec.artID != 0);
	if (rec.name.empty())
		rec.name = TS_UnknownString;
#ifdef USE_CACHED_QUERIES
	m_albumCache.ID = 0;
#endif // USE_CACHED_QUERIES
	if (rec.ID != 0 || rec.artID == 0)
		return FALSE;
#ifdef USE_INTEGRITY_CHECKS
	AlbumRecord existing;
	if (GetAlbumRecordUnique(rec.artID, rec.name.c_str(), existing))
		return FALSE;
	if (!ArtistRecordExists(rec.artID))			return FALSE;
#endif // USE_INTEGRITY_CHECKS
	SQLiteStmt st(m_pDB);
	if (st.Prepare(_T("INSERT INTO Albums (Name,artID,Year,Rating) VALUES (?,?,?,?) RETURNING ID")) &&
		st.BindText(1, rec.name.c_str()) && st.BindUInt(2, rec.artID) &&
		st.BindInt(3, rec.year) && st.BindInt(4, rec.rating) &&
		st.Step() && !st.IsEOF())
	{
		m_changes.albums.additions++;
		rec.ID = st.GetUINTFieldValue(0);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::AddNewArtistRecord(ArtistRecord& rec)
{
	ASSERT(rec.ID == 0);
	ASSERT(!rec.name.empty());
	ASSERT(rec.genID != 0);
	if (rec.ID != 0 || rec.genID == 0 || rec.name.empty())
		return FALSE;
#ifdef USE_CACHED_QUERIES
	m_artistCache.ID = 0;
#endif // USE_CACHED_QUERIES
#ifdef USE_INTEGRITY_CHECKS
	ArtistRecord existing;
	if (GetArtistRecordUnique(rec.name.c_str(), existing))
		return FALSE;
	if (!GenreRecordExists(rec.genID))			return FALSE;
#endif // USE_INTEGRITY_CHECKS
	SQLiteStmt st(m_pDB);
	if (st.Prepare(_T("INSERT INTO Artists (Name,GenreID,Rating) VALUES (?,?,?) RETURNING ID")) &&
		st.BindText(1, rec.name.c_str()) && st.BindUInt(2, rec.genID) &&
		st.BindInt(3, rec.rating) && st.Step() && !st.IsEOF())
	{
		m_changes.artists.additions++;
		rec.ID = st.GetUINTFieldValue(0);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::AddNewGenreRecord(GenreRecord& rec)
{
	ASSERT(rec.ID == 0);
	ASSERT(!rec.name.empty());
	if (rec.ID != 0 || rec.name.empty())
		return FALSE;
#ifdef USE_CACHED_QUERIES
	m_genreCache.ID = 0;
#endif // USE_CACHED_QUERIES
#ifdef USE_INTEGRITY_CHECKS
	GenreRecord existing;
	if (GetGenreRecordUnique(rec.name.c_str(), existing))
		return FALSE;
#endif // USE_INTEGRITY_CHECKS
	SQLiteStmt st(m_pDB);
	if (st.Prepare(_T("INSERT INTO Genres (Name) VALUES (?) RETURNING ID")) &&
		st.BindText(1, rec.name.c_str()) && st.Step() && !st.IsEOF())
	{
		m_changes.genres.additions++;
		rec.ID = st.GetUINTFieldValue(0);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::AddNewCollectionRecord(CollectionRecord& rec)
{
	ASSERT(rec.ID == 0);
	ASSERT(!rec.location.empty());
	ASSERT(rec.type > CTYPE_Unknown && rec.type < CTYPE_Last);
	if(rec.ID) return FALSE;
	if(rec.location.empty()) return FALSE;
	if(!(rec.type > CTYPE_Unknown && rec.type < CTYPE_Last)) return FALSE;
	if (rec.name.empty())
		rec.name = rec.location;
#ifdef USE_INTEGRITY_CHECKS
	CollectionRecord existing;
	if (GetCollectionRecordUnique(rec.serial, rec.location.c_str(), existing))
		return FALSE;
#endif // USE_INTEGRITY_CHECKS
	::GetLocalTime(&rec.dateAdded);
	::GetLocalTime(&rec.dateUpdated);
	SQLiteStmt st(m_pDB);
	if (st.Prepare(_T("INSERT INTO Collections (Name,PathName,Serial,Type,DateAdded,DateUpdated) ")
			_T("VALUES (?,?,?,?,?,?) RETURNING ID")) &&
		st.BindText(1, rec.name.c_str()) && st.BindText(2, rec.location.c_str()) &&
		st.BindInt(3, (INT)rec.serial) && st.BindInt(4, rec.type) &&
		st.BindDate(5, rec.dateAdded) && st.BindDate(6, rec.dateUpdated) &&
		st.Step() && !st.IsEOF())
	{
		m_changes.collections.additions++;
		rec.ID = st.GetUINTFieldValue(0);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::AddNewHistArtistRecord(HistArtistRecord& rec)
{
	ASSERT(rec.ID == 0 && !rec.name.empty());
	if(rec.ID != 0 || rec.name.empty())
		return FALSE;
#ifdef USE_INTEGRITY_CHECKS
	HistArtistRecord existing;
	if (GetHistArtistRecordUnique(rec.name.c_str(), existing))
		return FALSE;
#endif // USE_INTEGRITY_CHECKS
	SQLiteStmt st(m_pDB);
	if (st.Prepare(_T("INSERT INTO HistArtists (Name) VALUES (?) RETURNING ID")) &&
		st.BindText(1, rec.name.c_str()) && st.Step() && !st.IsEOF())
	{
		rec.ID = st.GetUINTFieldValue(0);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::AddNewHistTrackRecord(HistTrackRecord& rec)
{
	ASSERT(rec.ID == 0 && !rec.name.empty() && rec.artID != 0);
	if(rec.ID != 0 || rec.name.empty() || rec.artID == 0)
		return FALSE;
#ifdef USE_INTEGRITY_CHECKS
	HistTrackRecord existing;
	if (GetHistTrackRecordUnique(rec.artID, rec.name.c_str(), existing))
		return FALSE;
#endif // USE_INTEGRITY_CHECKS
	SQLiteStmt st(m_pDB);
	if (st.Prepare(_T("INSERT INTO HistTracks (Name,artID) VALUES (?,?) RETURNING ID")) &&
		st.BindText(1, rec.name.c_str()) && st.BindUInt(2, rec.artID) &&
		st.Step() && !st.IsEOF())
	{
		rec.ID = st.GetUINTFieldValue(0);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::AddNewHistLogRecord(HistLogRecord& rec)
{
	ASSERT(rec.ID == 0 && rec.actID > HLA_Unknown && rec.actID < HLA_Last && rec.trackID);
	if(!(rec.ID == 0 && rec.actID > HLA_Unknown && rec.actID < HLA_Last && rec.trackID))
		return FALSE;
	SQLiteStmt st(m_pDB);
	if (st.Prepare(_T("INSERT INTO HistLog (actID,trackID,DateAdded) VALUES (?,?,?) RETURNING ID")) &&
		st.BindInt(1, rec.actID) && st.BindUInt(2, rec.trackID) &&
		st.BindDate(3, rec.dateAdded) && st.Step() && !st.IsEOF())
	{
		rec.ID = st.GetUINTFieldValue(0);
		return TRUE;
	}
	return FALSE;
}


//===============Update-Xxx-Record: Updates a Xxx record
//REQUIRE:	xxx with ID!=0 && name != "" && foreign keys valid
//RETURN:	FALSE on failure (including: the record does not exist)
//			TRUE on success.
BOOL SQLManager::UpdateTrackRecord(TrackRecord& rec)
{
	ASSERT(rec.ID > 0);//0:Invalid
	ASSERT(!rec.location.empty());
	ASSERT(rec.artID != 0);
	ASSERT(rec.albID != 0);
	ASSERT(rec.genID != 0);
	ASSERT(rec.colID != 0);
	ASSERT(rec.trackType > TTYPE_Unsupported && rec.trackType < TTYPE_Last);
	if (rec.ID == 0 || rec.location.empty() || rec.artID == 0 || rec.albID == 0 ||
		rec.genID == 0 || rec.colID == 0 || rec.trackType == TTYPE_Unsupported)
		return FALSE;
	if (rec.name.empty())
		rec.name = TS_UnknownString;
#ifdef USE_INTEGRITY_CHECKS
	if (!ArtistRecordExists(rec.artID))			return FALSE;
	if (!AlbumRecordExists(rec.albID))			return FALSE;
	if (!GenreRecordExists(rec.genID))			return FALSE;
	if (!CollectionRecordExists(rec.colID))		return FALSE;
#endif // USE_INTEGRITY_CHECKS

	TrackRecord existing;
	GetTrackRecordUnique(rec.colID, rec.location.c_str(), existing);
	if (existing.ID == 0 || existing.ID == rec.ID)
	{
		SQLiteStmt st(m_pDB);
		if (st.Prepare(_T("UPDATE Tracks SET Name=?,Type=?,DateAdded=?,albID=?,artID=?,CollectionID=?,")
				_T("genreID=?,Path=?,Bitrate=?,Duration=?,Size=?,TrackNo=?,Year=?,Rating=? WHERE ID=?")) &&
			st.BindText(1, rec.name.c_str()) && st.BindInt(2, rec.trackType) &&
			st.BindDate(3, rec.dateAdded) && st.BindUInt(4, rec.albID) &&
			st.BindUInt(5, rec.artID) && st.BindUInt(6, rec.colID) &&
			st.BindUInt(7, rec.genID) && st.BindText(8, rec.location.c_str()) &&
			st.BindInt(9, rec.bitrate) && st.BindInt(10, rec.duration) &&
			st.BindInt(11, rec.size) && st.BindInt(12, rec.trackNo) &&
			st.BindInt(13, rec.year) && st.BindInt(14, rec.rating) &&
			st.BindUInt(15, rec.ID) && st.Exec() > 0)
		{
			m_changes.tracks.updates++;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL SQLManager::UpdateAlbumRecord(AlbumRecord& rec, BOOL bForce)
{
	ASSERT(rec.ID != 0);
	ASSERT(rec.artID != 0);
	ASSERT(rec.IsValid());
	if (!(rec.artID != 0))
		return FALSE;
	m_albumCache.ID = 0;
	if (rec.name.empty())
		rec.name = TS_UnknownString;
	//We must check for name duplication
	AlbumRecord existing;
	GetAlbumRecordUnique(rec.artID, rec.name.c_str(), existing);
	if (existing.ID == 0 || existing.ID == rec.ID)
	{
		TRACE(_T("@4 SQLManager::UpdateAlbumRecord. Updating. Existing: %d\r\n"), existing.ID);
		SQLiteStmt st(m_pDB);
		if (st.Prepare(_T("UPDATE Albums SET Name=?,artID=?,Year=?,Rating=? WHERE ID=?")) &&
			st.BindText(1, rec.name.c_str()) && st.BindUInt(2, rec.artID) &&
			st.BindInt(3, rec.year) && st.BindInt(4, rec.rating) &&
			st.BindUInt(5, rec.ID) && st.Exec() > 0)
		{
			m_changes.albums.updates++;
			return TRUE;
		}
	}
	else if (bForce)
	{
		TRACE(_T("@4 SQLManager::UpdateAlbumRecord. There is already a record that has the same name\r\n"));
		//There is already a record that has the same name as this one.
		//FORCE WILL Keep the integrity of the database by
		//	1. Update all Track Records to the existing ID
		CString sql;
		sql.Format(_T("UPDATE Tracks SET albID=%u WHERE albID=%u"), existing.ID, rec.ID);
		m_pDB->Execute(sql);
		//	2. Delete all the info related to this record
		DeleteAllAlbumInfo(rec.ID);
		//	3. Delete This record
		m_changes.albums.removals++;
		sql.Format(_T("DELETE FROM Albums WHERE ID=%u"), rec.ID);
		m_pDB->Execute(sql);
		rec = existing;
		return TRUE;
	}
	TRACE(_T("@1 SQLManager::UpdateAlbumRecord. Not forced. Do nothing\r\n"));
	return FALSE;
}

BOOL SQLManager::UpdateArtistRecord(ArtistRecord& rec, BOOL bForce)
{
	ASSERT(rec.ID > 2);
	ASSERT(rec.genID != 0);
	ASSERT(rec.IsValid());
	//We must check for name duplication
	if (!(rec.ID > 2 && rec.genID != 0 && rec.IsValid()))
		return FALSE;
	m_artistCache.ID = 0;
	ArtistRecord existing;
	GetArtistRecordUnique(rec.name.c_str(), existing);
	if (existing.ID == 0 || existing.ID == rec.ID)
	{
		TRACE(_T("@4 SQLManager::UpdateArtistRecord. Updating. Existing: %d\r\n"), existing.ID);
		SQLiteStmt st(m_pDB);
		if (st.Prepare(_T("UPDATE Artists SET Name=?,GenreID=?,Rating=? WHERE ID=?")) &&
			st.BindText(1, rec.name.c_str()) && st.BindUInt(2, rec.genID) &&
			st.BindInt(3, rec.rating) && st.BindUInt(4, rec.ID) && st.Exec() > 0)
		{
			m_changes.artists.updates++;
			return TRUE;
		}
	}
	else if (bForce)
	{
		TRACE(_T("@4 SQLManager::UpdateArtistRecord. There is already a record that has the same name\r\n"));
		//There is already a record that has the same name as this one.
		//FORCE WILL Keep the integrity of the database by
		//	1. Update all Track Records to the existing ID
		CString sql;
		sql.Format(_T("UPDATE Tracks SET artID=%u WHERE artID=%u"), existing.ID, rec.ID);
		m_changes.tracks.updates += m_pDB->Execute(sql);
		//	2. Move the albums to the existing artist (merging albums with the same name)
		std::vector<AlbumRecord> albums;
		{
			SQLiteStmt rs(m_pDB);
			std::tstring asql(AlbumSQL);
			asql += _T(" WHERE artID=?");
			if (rs.Prepare(asql.c_str()) && rs.BindUInt(1, rec.ID) && rs.Step())
			{
				while (!rs.IsEOF())
				{
					AlbumRecord al;
					short idx = 0;
					GetAlbumRecordFields(al, rs, idx);
					albums.push_back(al);
					rs.MoveNext();
				}
			}
		}
		for (size_t i = 0; i < albums.size(); i++)
		{
			albums[i].artID = existing.ID;
			UpdateAlbumRecord(albums[i], TRUE);
		}
		//	3. Delete all the info related to this record
		DeleteAllArtistInfo(rec.ID);
		//	4. Delete This record
		sql.Format(_T("DELETE FROM Artists WHERE ID=%u"), rec.ID);
		m_changes.artists.removals += m_pDB->Execute(sql);
		rec = existing;
		return TRUE;
	}
	TRACE(_T("@1 SQLManager::UpdateArtistRecord. Not forced. Do nothing\r\n"));
	return FALSE;
}

BOOL SQLManager::UpdateGenreRecord(GenreRecord& rec, BOOL bForce)
{
	ASSERT(rec.ID != 0);
	ASSERT(rec.name.empty() == FALSE);
	m_genreCache.ID = 0;
	//We must check for name duplication
	GenreRecord existing;
	GetGenreRecordUnique(rec.name.c_str(), existing);
	if (existing.ID == 0 || existing.ID == rec.ID)
	{
		SQLiteStmt st(m_pDB);
		if (st.Prepare(_T("UPDATE Genres SET Name=? WHERE ID=?")) &&
			st.BindText(1, rec.name.c_str()) && st.BindUInt(2, rec.ID) && st.Exec() > 0)
		{
			m_changes.genres.updates++;
			return TRUE;
		}
	}
	else if (bForce)
	{
		//There is already a record that has the same name as this one.
		//FORCE WILL Keep the integrity of the database by
		//	1. Update all Track Records to the existing ID
		CString sql;
		sql.Format(_T("UPDATE Tracks SET genreID=%u WHERE genreID=%u"), existing.ID, rec.ID);
		m_pDB->Execute(sql);
		//	2. Update all Artists Records the same way
		sql.Format(_T("UPDATE Artists SET GenreID=%u WHERE GenreID=%u"), existing.ID, rec.ID);
		m_pDB->Execute(sql);
		//	3. Delete This record
		sql.Format(_T("DELETE FROM Genres WHERE ID=%u"), rec.ID);
		m_changes.genres.removals++;
		m_pDB->Execute(sql);
		rec = existing;
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::UpdateCollectionRecord(CollectionRecord& rec)
{
	ASSERT(rec.ID != 0);
	ASSERT(rec.location.empty() == FALSE);
	if (rec.name.empty())
		rec.name = rec.location;
	ASSERT(rec.type > CTYPE_Unknown && rec.type < CTYPE_Last);
	//We must check for name duplication
	CollectionRecord existing;
	GetCollectionRecordUnique(rec.serial, rec.location.c_str(), existing);
	if (existing.ID == 0 || existing.ID == rec.ID)
	{
		SQLiteStmt st(m_pDB);
		if (st.Prepare(_T("UPDATE Collections SET Name=?,PathName=?,Serial=?,Type=?,DateAdded=?,DateUpdated=? WHERE ID=?")) &&
			st.BindText(1, rec.name.c_str()) && st.BindText(2, rec.location.c_str()) &&
			st.BindInt(3, (INT)rec.serial) && st.BindInt(4, rec.type) &&
			st.BindDate(5, rec.dateAdded) && st.BindDate(6, rec.dateUpdated) &&
			st.BindUInt(7, rec.ID) && st.Exec() > 0)
		{
			m_changes.collections.updates++;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL SQLManager::UpdateHistTrackRecord(HistTrackRecord& rec)
{
	ASSERT(0);
	return FALSE;
}
BOOL SQLManager::UpdateHistLogRecord(HistLogRecord& rec)
{
	ASSERT(0);
	return FALSE;
}
BOOL SQLManager::UpdateHistArtistRecord(HistArtistRecord& rec)
{
	ASSERT(0);
	return FALSE;
}


//===============Xxx-RecordExists: Test if a record exist
//REQUIRE:	ID!=0
//RETURN:	FALSE if xxx does not exists || failure
//			TRUE on success.
BOOL SQLManager::RecordExists(LPCTSTR sql, UINT ID)
{
	ASSERT(ID > 0);
	std::tstring q(sql);
	q += _T(" WHERE ID=?");
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(q.c_str()) && rs.BindUInt(1, ID) && rs.Step() && !rs.IsEOF())
		return TRUE;
	return FALSE;
}
BOOL SQLManager::TrackRecordExists(UINT ID)			{return RecordExists(TrackSQL, ID);}
BOOL SQLManager::AlbumRecordExists(UINT ID)			{return RecordExists(AlbumSQL, ID);}
BOOL SQLManager::ArtistRecordExists(UINT ID)		{return RecordExists(ArtistSQL, ID);}
BOOL SQLManager::GenreRecordExists(UINT ID)			{return RecordExists(GenreSQL, ID);}
BOOL SQLManager::CollectionRecordExists(UINT ID)	{return RecordExists(CollectionSQL, ID);}
BOOL SQLManager::HistTrackRecordExists(UINT ID)		{return RecordExists(HistTrackSQL, ID);}
BOOL SQLManager::HistArtistRecordExists(UINT ID)	{return RecordExists(HistArtistSQL, ID);}
BOOL SQLManager::HistLogRecordExists(UINT ID)		{return RecordExists(HistLogSQL, ID);}


BOOL SQLManager::UpdateFullArtistRecord(FullArtistRecord& artist, BOOL bForce)
{
	//======Genres
	GenreRecord genre;
	if (!GetGenreRecordUnique(artist.genre.name.c_str(), genre))
	{
		genre.name = artist.genre.name.c_str();
		if (!AddNewGenreRecord(genre))
			return FALSE;
	}
	artist.artist.genID = artist.genre.ID = genre.ID;
	//=======Artists
	return UpdateArtistRecord(artist.artist, bForce);
}

BOOL SQLManager::UpdateFullAlbumRecord(FullAlbumRecord& album, BOOL bForce)
{
	//======Artist
	ArtistRecord artist;
	if (!GetArtistRecordUnique(album.artist.name.c_str(), artist))
	{
		artist.name = album.artist.name.c_str();
		if (!AddNewArtistRecord(artist))
			return FALSE;
	}
	album.album.artID = album.artist.ID = artist.ID;
	//=======Album
	return UpdateAlbumRecord(album.album, bForce);
}

BOOL SQLManager::UpdateFullTrackRecord(FullTrackRecord& track)
{
	ASSERT(track.track.ID != 0);
	ASSERT(track.track.colID == track.collection.ID);
	ASSERT(track.track.colID != 0);
	ASSERT(!track.track.location.empty());
	//======Genres
	GenreRecord genre;
	if (!GetGenreRecordUnique(track.genre.name.c_str(), genre))
	{
		genre.name = track.genre.name.c_str();
		if (!AddNewGenreRecord(genre))
			return FALSE;
	}
	track.track.genID = track.genre.ID = genre.ID;
	//======Artist
	ArtistRecord artist;
	if (!GetArtistRecordUnique(track.artist.name.c_str(), artist))
	{
		artist.name = track.artist.name.c_str();
		artist.genID = genre.ID;
		if (!AddNewArtistRecord(artist))
			return FALSE;
	}
	track.track.artID = track.artist.ID = artist.ID;

	//======Album
	AlbumRecord album;
	if (!GetAlbumRecordUnique(artist.ID, track.album.name.c_str(), album))
	{
		album.name = track.album.name.c_str();
		album.artID = artist.ID;
		if (!AddNewAlbumRecord(album))
			return FALSE;
	}
	track.track.albID = track.album.ID = album.ID;
	return UpdateTrackRecord(track.track);
}


//==============================================================
//		TRACKS TABLE
//==============================================================

BOOL SQLManager::GetTrackRecordCollectionByTracksFilter(TrackRecordCollection& col, TracksFilter& Filter)
{
	CString Where(GetWhereSQL(Filter));
	CString SQL;
	if (!Where.IsEmpty())
		SQL.Format(_T("%s WHERE %s"), TrackSQL, &((LPCTSTR)Where)[4]);
	else
		SQL = TrackSQL;
	return GetTrackRecordCollectionBySQL(col, SQL);
}


const TCHAR* const FullTrackSQL = _T("SELECT ")
					_T("Tracks.ID,Tracks.Name,Tracks.Type,Tracks.DateAdded,")
					_T("Tracks.albID,Tracks.artID,Tracks.CollectionID,Tracks.GenreID,Tracks.Path,")
					_T("Tracks.Bitrate,Tracks.Duration,Tracks.Size,Tracks.TrackNo,")
					_T("Tracks.Year,Tracks.Rating,")
					_T("Albums.ID,Albums.Name,Albums.artID,Albums.Year,Albums.Rating,")
					_T("Artists.ID,Artists.Name,Artists.GenreID,Artists.Rating,")
					_T("Genres.ID,Genres.Name,")
					_T("Collections.ID,Collections.Name,Collections.PathName,Collections.Serial,")
					_T("Collections.Type,Collections.DateAdded,Collections.DateUpdated ")
					_T("FROM Tracks INNER JOIN Albums ON Tracks.albID=Albums.ID ")
					_T("INNER JOIN Genres ON Tracks.GenreID=Genres.ID ")
					_T("INNER JOIN Artists ON Tracks.artID=Artists.ID ")
					_T("INNER JOIN Collections ON Tracks.collectionID=Collections.ID ");

//(month + year * 12 - 1) of a date column: the "month added" key.
#define MONTH_ADDED_SQL(col) _T("(CAST(strftime('%m',") col _T(") AS INTEGER) + CAST(strftime('%Y',") col _T(") AS INTEGER) * 12 - 1)")


BOOL SQLManager::GetFullTrackRecordByID(FullTrackRecordSP& rec, UINT id)
{
	TracksFilter tf;
	tf.TrackID.match = NUMM_Exact;
	tf.TrackID.val = id;
	FullTrackRecordCollection col;
	if (GetFullTrackRecordCollectionByTracksFilter(col, tf))
	{
		if (col.size() == 1)
		{
			rec = col[0];
			return TRUE;
		}
	}
	return FALSE;
}

BOOL SQLManager::GetFullTrackRecordByLocation(FullTrackRecordSP& rec, LPCTSTR Location, UINT CollectionID)
{
	ASSERT(Location != NULL);
	if (Location != NULL)
	{
		TracksFilter tf;
		tf.Location.match = TXTM_Exact;
		tf.Location.val = Location;
		if (CollectionID != 0)
		{
			tf.CollectionID.match = NUMM_Exact;
			tf.CollectionID.val = CollectionID;
		}
		FullTrackRecordCollection col;
		if (GetFullTrackRecordCollectionByTracksFilter(col, tf))
		{
			if (col.size() > 0)
			{
				rec = col[0];
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL SQLManager::GetFullTrackRecordCollectionByLocation(FullTrackRecordCollection& col, LPCTSTR Location)
{
	TRACEST(_T("SQLManager::GetFullTrackRecordCollectionByLocation"));
	ASSERT(Location != NULL);
	if (Location != NULL)
	{
		TracksFilter tf;
		tf.Location.match = TXTM_Exact;
		tf.Location.val = Location;
		if (GetFullTrackRecordCollectionByTracksFilter(col, tf))
			return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::GetFullTrackRecordCollectionByTracksFilter(FullTrackRecordCollection& col, TracksFilter& Filter, INT limitResults)
{
	CString Where(GetWhereSQL(Filter));
	if (Filter.MinNumTracks.match == NUMM_Disabled)
	{
		if (Filter.ArtistID.match == NUMM_Disabled)
			AddSQLCountParameter(Where, Filter.ArtistTrackCount, _T("Tracks.artID"));
		if (Filter.AlbumID.match == NUMM_Disabled)
			AddSQLCountParameter(Where, Filter.AlbumTrackCount, _T("Tracks.albID"));
		if (Filter.GenreID.match == NUMM_Disabled)
			AddSQLCountParameter(Where, Filter.GenreTrackCount, _T("Tracks.genreID"));
		if (Filter.CollectionID.match == NUMM_Disabled)
			AddSQLCountParameter(Where, Filter.CollectionTrackCount, _T("Tracks.collectionID"));
		if (Filter.Year.match == NUMM_Disabled)
			AddSQLCountParameter(Where, Filter.YearTrackCount, _T("Tracks.Year"));
		if (Filter.MonthAdded.match == NUMM_Disabled)
			AddSQLCountParameter(Where, Filter.MonthAddedTrackCount, MONTH_ADDED_SQL(_T("Tracks.DateAdded")));
	}
	CString SQL;
	if (!Where.IsEmpty())
		SQL.Format(_T("%s WHERE %s"), FullTrackSQL, &((LPCTSTR)Where)[4]);
	else
		SQL = FullTrackSQL;
	return GetFullTrackRecordCollectionBySQL(col, SQL, limitResults);
}

BOOL SQLManager::GetFullTrackRecordCollectionBySimpleSearch(FullTrackRecordCollection& col, LPCTSTR searchStr, INT limitResults)
{
	TRACEST(_T("SQLManager::GetFullTrackRecordCollectionBySimpleSearch"), searchStr);
	ASSERT(searchStr != NULL);
	if (searchStr[0] == 0)
		return TRUE;//Does not touch the collection (Leave it empty usually - except if the user wanted to append)
	std::tstring pattern(_T("%"));
	pattern += searchStr;
	pattern += _T("%");
	std::tstring quoted = SQLQuote(pattern.c_str());

	std::tstring SQL(FullTrackSQL);
	SQL += _T(" WHERE (Tracks.Name LIKE ");
	SQL += quoted;
	SQL += _T(") OR (Artists.Name LIKE ");
	SQL += quoted;
	SQL += _T(") OR (Albums.Name LIKE ");
	SQL += quoted;
	SQL += _T(") OR (Tracks.Path LIKE ");
	SQL += quoted;
	SQL += _T(")");
	return GetFullTrackRecordCollectionBySQL(col, SQL.c_str(), limitResults);
}

void SQLManager::AddSmartSearchCondition(std::tstring& SQL, LPCTSTR condition)
{
	std::tstring pattern(_T("%"));
	pattern += condition;
	pattern += _T("%");
	SQL += _T(" (Tracks.Name || Artists.Name || Albums.Name || Tracks.Path) LIKE ");
	SQL += SQLQuote(pattern.c_str());
	SQL += _T(" ");
}

BOOL SQLManager::GetFullTrackRecordCollectionBySmartSearch(FullTrackRecordCollection& col, LPCTSTR searchStr, INT limitResults)
{
	TRACEST(_T("SQLManager::GetFullTrackRecordCollectionBySmartSearch"), searchStr);
	ASSERT(searchStr != NULL);
	if (searchStr[0] == 0)
		return TRUE;//Does not touch the collection (Leave it empty usually - except if the user wanted to append)
	std::tstring searchF(searchStr);
	trim(searchF);
	if (searchF.empty())
		return TRUE;

	//Every space-separated word must match.
	std::tstring SQL(FullTrackSQL);
	SQL += _T(" WHERE ");
	BOOL bFirst = TRUE;
	std::tstring::size_type pos = 0;
	while (pos < searchF.size())
	{
		std::tstring::size_type end = searchF.find(_T(' '), pos);
		if (end == std::tstring::npos)
			end = searchF.size();
		if (end > pos)
		{
			if (!bFirst)
				SQL += _T(" AND ");
			AddSmartSearchCondition(SQL, searchF.substr(pos, end - pos).c_str());
			bFirst = FALSE;
		}
		pos = end + 1;
	}
	return GetFullTrackRecordCollectionBySQL(col, SQL.c_str(), limitResults);
}

BOOL SQLManager::GetFullTrackRecordCollectionByInfoLikeness(FullTrackRecordCollection& col, InfoItemTypeEnum iit, LPCTSTR searchString)
{
	ASSERT(searchString != NULL);
	if (searchString[0] == 0)	return FALSE;
	std::tstring pattern(_T("%"));
	pattern += searchString;
	pattern += _T("%");
	CString SQL;
	SQL.Format(_T("%s INNER JOIN TracksInfo ON TracksInfo.trackID=Tracks.ID ")
		_T("WHERE TracksInfo.Name LIKE %s AND TracksInfo.Type=%d"),
		FullTrackSQL, SQLQuote(pattern.c_str()).c_str(), iit);
	return GetFullTrackRecordCollectionBySQL(col, SQL);
}

//==============================================================
//		ARTISTS TABLE
//==============================================================

const TCHAR* const FullArtistSQL = _T("SELECT ")
_T("Artists.ID, Artists.Name, Artists.GenreID, Artists.Rating ")
_T(",Genres.ID,Genres.Name ")
_T(", -1 ")
_T("FROM Artists INNER JOIN Genres ON Artists.GenreID=Genres.ID ");

const TCHAR* const CountFullArtistSQL = _T("SELECT ")
_T("Artists.ID, Artists.Name, Artists.GenreID, Artists.Rating ")
_T(",Genres.ID,Genres.Name,")
_T("count(*) ")
_T("FROM Tracks INNER JOIN Artists ON Tracks.artID = Artists.ID ")
_T("INNER JOIN Genres ON Artists.genreID = Genres.ID ");

BOOL SQLManager::GetFullArtistRecordByID(FullArtistRecordSP& rec, UINT id)
{
	ASSERT(id > 0);
	FullArtistRecordCollection col;
	CString SQL;
	SQL.Format(_T("%s WHERE Artists.ID=%u"), FullArtistSQL, id);
	if (GetFullArtistRecordCollectionBySQL(col, SQL))
	{
		ASSERT(col.size() == 1);
		if (col.size() == 1)
		{
			rec = col[0];
			return TRUE;
		}
	}
	return FALSE;
}

BOOL SQLManager::GetFullArtistRecordCollectionByTracksFilter(FullArtistRecordCollection& col, TracksFilter& Filter)
{
	std::tstring SQL(CountFullArtistSQL);
	CString Where(GetWhereSQL(Filter));
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}
	SQL += _T(" GROUP BY Tracks.artID ");
	SQL += GetHavingSQL(Filter.MinNumTracks);
	return GetFullArtistRecordCollectionBySQL(col, SQL.c_str());
}


//==============================================================
//		ALBUMS TABLE
//==============================================================

const TCHAR* const FullAlbumSQL = _T("SELECT ")
_T("Albums.ID, Albums.Name, Albums.artID, Albums.Year, Albums.Rating ")
_T(", Artists.ID, Artists.Name, Artists.GenreID, Artists.Rating ")
_T(", -1 ")
_T("FROM Albums INNER JOIN Artists ON Albums.artID=Artists.ID ");

const TCHAR* const CountFullAlbumSQL = _T("SELECT ")
_T("Albums.ID, Albums.Name, Albums.artID, Albums.Year, Albums.Rating ")
_T(", Artists.ID, Artists.Name, Artists.GenreID, Artists.Rating")
_T(", count(*) ")
_T("FROM Tracks INNER JOIN Albums ON Tracks.albID = Albums.ID ")
_T("INNER JOIN Artists ON Albums.artID = Artists.ID ");

BOOL SQLManager::GetFullAlbumRecordByID(FullAlbumRecordSP& rec, UINT id)
{
	ASSERT(id > 0);
	if (id == 0)
		return FALSE;
	FullAlbumRecordCollection col;
	CString SQL;
	SQL.Format(_T("%s WHERE Albums.ID=%u"), FullAlbumSQL, id);
	if (GetFullAlbumRecordCollectionBySQL(col, SQL))
	{
		ASSERT(col.size() == 1);
		if (col.size() == 1)
		{
			rec = col[0];
			return TRUE;
		}
	}
	return FALSE;
}

BOOL SQLManager::GetFullAlbumRecordCollectionByTracksFilter(FullAlbumRecordCollection& col, TracksFilter& Filter)
{
	std::tstring SQL(CountFullAlbumSQL);
	CString Where(GetWhereSQL(Filter));
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}
	SQL += _T(" GROUP BY Tracks.albID ");
	SQL += GetHavingSQL(Filter.MinNumTracks);
	return GetFullAlbumRecordCollectionBySQL(col, SQL.c_str());
}

BOOL SQLManager::GetFullAlbumGPNRecordCollectionByTracksFilter(FullAlbumRecordCollection& col, TracksFilter& Filter)
{
	std::tstring SQL(CountFullAlbumSQL);
	CString Where(GetWhereSQL(Filter));
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}
	SQL += _T(" GROUP BY Albums.name ");
	SQL += GetHavingSQL(Filter.MinNumTracks);
	return GetFullAlbumRecordCollectionBySQL(col, SQL.c_str());
}

//Albums whose name appears under more than one artist (compilations).
BOOL SQLManager::GetFullAlbumRecordCollectionByTracksFilterForCollections(FullAlbumRecordCollection& col, TracksFilter& Filter)
{
	//Columns: album (5) + an empty artist (4) + track count.
	std::tstring SQL = _T("SELECT ")
		_T("Albums.ID, Albums.Name, Albums.artID, Albums.Year, Albums.Rating, 0, '', 0, 0, count(*) ")
		_T("FROM Tracks INNER JOIN Albums ")
		_T("ON Tracks.albID = Albums.ID ")
		_T("WHERE substr(Albums.Name,1,1)<>'[' ");
	CString Where(GetWhereSQL(Filter));
	if (!Where.IsEmpty())
		SQL += Where;
	SQL += _T(" GROUP BY Albums.Name HAVING MAX(Tracks.artID) <> MIN(Tracks.artID) ");
	return GetFullAlbumRecordCollectionBySQL(col, SQL.c_str());
}


//==============================================================
//		GENRES TABLE
//==============================================================

BOOL SQLManager::GetGenreRecordCollectionByTracksFilter(GenreRecordCollection& col, TracksFilter& Filter, BOOL bCountTracks)
{
	std::tstring SQL(bCountTracks ? CountGenreSQL : GenreSQL);
	CString Where(GetWhereSQL(Filter));
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}
	if (bCountTracks)
	{
		SQL += _T(" GROUP BY Tracks.genreID ");
		SQL += GetHavingSQL(Filter.MinNumTracks);
	}
	return GetGenreRecordCollectionBySQL(col, SQL.c_str());
}

//==============================================================
//		COLLECTIONS TABLE
//==============================================================

BOOL SQLManager::GetCollectionRecordCollectionByTracksFilter(CollectionRecordCollection& col, TracksFilter& Filter, BOOL bCountTracks)
{
	std::tstring SQL(bCountTracks ? CountCollectionSQL : CollectionSQL);
	CString Where(GetWhereSQL(Filter));
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}
	if (bCountTracks)
	{
		SQL += _T(" GROUP BY Collections.ID ");
		SQL += GetHavingSQL(Filter.MinNumTracks);
	}
	return GetCollectionRecordCollectionBySQL(col, SQL.c_str());
}

BOOL SQLManager::GetCollectionRecordCollectionByTypeID(CollectionRecordCollection& col, CollectionTypesEnum type)
{
	CString SQL;
	SQL.Format(_T("%s WHERE Type=%d"), CollectionSQL, type);
	return GetCollectionRecordCollectionBySQL(col, SQL);
}

//==============================================================
//		YEARS
//==============================================================

const TCHAR* const CountYearSQL = _T("SELECT ")
_T("Tracks.Year ")
_T(", count(*) ")
_T("FROM Tracks ");

BOOL SQLManager::GetYearRecordCollectionByTracksFilter(YearRecordCollection& col, TracksFilter& Filter, BOOL bCountTracks)
{
	ASSERT(bCountTracks);
	std::tstring SQL(CountYearSQL);
	CString Where(GetWhereSQL(Filter));
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}
	if (bCountTracks)
	{
		SQL += _T(" GROUP BY Tracks.Year ");
		SQL += GetHavingSQL(Filter.MinNumTracks);
	}
	return GetYearRecordCollectionBySQL(col, SQL.c_str());
}

//==============================================================
//		MONTH ADDED
//==============================================================

const TCHAR* const CountMonthAddedSQL = _T("SELECT ")
MONTH_ADDED_SQL(_T("Tracks.DateAdded"))
_T(", count(*) ")
_T("FROM Tracks ");

BOOL SQLManager::GetMonthAddedRecordCollectionByTracksFilter(MonthAddedRecordCollection& col, TracksFilter& Filter, BOOL bCountTracks)
{
	ASSERT(bCountTracks);
	std::tstring SQL(CountMonthAddedSQL);
	CString Where(GetWhereSQL(Filter));
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}
	if (bCountTracks)
	{
		SQL += _T(" GROUP BY strftime('%Y-%m', Tracks.DateAdded) ");
		SQL += GetHavingSQL(Filter.MinNumTracks);
	}
	return GetMonthAddedRecordCollectionBySQL(col, SQL.c_str());
}


//==============================================================
//		PRIVATE UTILITIES
//==============================================================

//Returns " AND cond1 AND cond2 ..." (callers skip the first 4 characters).
CString SQLManager::GetWhereSQL(TracksFilter& Filter)
{
	CString ret;
	AddSQLNumParameter(ret, Filter.TrackID,			_T("Tracks.ID"));
	AddSQLNumParameter(ret, Filter.ArtistID,		_T("Tracks.artID"));
	AddSQLNumParameter(ret, Filter.AlbumID,			_T("Tracks.albID"));

	AddSQLNumParameter(ret, Filter.GenreID,			_T("Tracks.GenreID"));
	AddSQLNumParameter(ret, Filter.CollectionID,	_T("Tracks.CollectionID"));

	AddSQLStrParameter(ret, Filter.Title,			_T("Tracks.Name"));
	AddSQLStrParameter(ret, Filter.Album,			_T("Albums.Name"));
	AddSQLStrParameter(ret, Filter.Artist,			_T("Artists.Name"));
	AddSQLStrParameter(ret, Filter.Location,		_T("Tracks.Path"));
	AddSQLStrParameter(ret, Filter.Genre,			_T("Genres.Name"));

	AddSQLNumParameter(ret, Filter.Year,			_T("Tracks.Year"));
	if (Filter.Rating.match != NUMM_Disabled)
	{
		AddSQLNumParameter(ret, Filter.Rating,		_T("Tracks.Rating"));
		NumericMatch rat2;
		rat2.match = NUMM_Over;
		rat2.val = 1;
		AddSQLNumParameter(ret, rat2,_T("Tracks.Rating"));
	}
	AddSQLNumParameter(ret, Filter.TrackNo,			_T("Tracks.TrackNo"));
	AddSQLNumParameter(ret, Filter.Size,			_T("Tracks.Size"));
	AddSQLNumParameter(ret, Filter.Duration,		_T("Tracks.Duration"));
	AddSQLNumParameter(ret, Filter.Bitrate,			_T("Tracks.Bitrate"));
	AddSQLNumParameter(ret, Filter.TrackType,		_T("Tracks.Type"));

	AddSQLNumParameter(ret, Filter.MonthAdded,		MONTH_ADDED_SQL(_T("Tracks.DateAdded")));

	AddSQLDateParameter(ret, Filter.DateAdded,		_T("Tracks.DateAdded"));
	return ret;
}

CString SQLManager::GetHavingSQL(NumericMatch nm)
{
	CString ret;
	if (nm.match != NUMM_Disabled)
	{
		ASSERT(nm.val > 0);
		ret.Format(_T(" HAVING count(*) %s %d"),
			nm.match == NUMM_Over ? _T(">") : (nm.match == NUMM_Under ? _T("<=") : _T("=")),
			nm.val);
	}
	return ret;
}

void SQLManager::AddSQLCountParameter(CString& ret, NumericMatch nm, LPCTSTR fieldName)
{
	if (nm.match == NUMM_Disabled || nm.val == 0)
		return;
	LPCTSTR oper = nm.match == NUMM_Over ? _T(">") : _T("<=");
	CString cond;
	cond.Format(_T(" AND %s IN (SELECT %s FROM Tracks GROUP BY %s HAVING count(%s) %s %d)"),
		fieldName, fieldName, fieldName, fieldName, oper, nm.val);
	ret += cond;
}

void SQLManager::AddSQLNumParameter(CString& ret, NumericMatch nm, LPCTSTR str)
{
	if (nm.match == NUMM_Disabled)
		return;
	LPCTSTR op;
	switch (nm.match)
	{
	case NUMM_Exact:	op = _T("=");	break;
	case NUMM_Over:		op = _T(">=");	break;
	case NUMM_Under:	op = _T("<");	break;
	case NUMM_Diff:		op = _T("<>");	break;
	default:
		TRACE(_T("@1 SQLManager::AddSQLNumParameter Unknown operator\r\n"));
		ASSERT(FALSE);
		return;
	}
	CString cond;
	cond.Format(_T(" AND %s%s%d"), str, op, nm.val);
	ret += cond;
}

void SQLManager::AddSQLStrParameter(CString& ret, TextMatch tm, LPCTSTR str)
{
	if (tm.match == TXTM_Disabled)
		return;
	ASSERT(!tm.val.empty());
	if (tm.val.empty())
		return;
	ret += _T(" AND ");
	ret += str;
	switch (tm.match)
	{
	case TXTM_Exact:
		ret += _T(" = ");
		ret += SQLQuote(tm.val.c_str()).c_str();
		break;
	case TXTM_Like:
		{
			std::tstring pattern(_T("%"));
			pattern += tm.val;
			pattern += _T("%");
			ret += _T(" LIKE ");
			ret += SQLQuote(pattern.c_str()).c_str();
		}
		break;
	default:
		ASSERT(FALSE);
		ret += _T(" IS NOT NULL");
		return;
	}
}

void SQLManager::AddSQLDateParameter(CString& ret, DateMatch dm, LPCTSTR str)
{
	if (dm.match == DATM_Disabled)
		return;
	ret += _T(" AND ");
	ret += str;
	ret += dm.match == DATM_After ? _T(">") : _T("<");
	ret += SQLDate(dm.val).c_str();
}


BOOL SQLManager::DeleteTrackRecord(UINT ID)
{
	ASSERT(ID > 0);
	DeleteAllTrackInfo(ID);
	CString sql;
	sql.Format(_T("DELETE FROM Tracks WHERE ID=%u"), ID);
	m_changes.tracks.removals++;
	return m_pDB->Execute(sql) != -1;
}

BOOL SQLManager::GetVirtualTrackCollectionRecord(CollectionRecord& cr)
{
	ASSERT(0);return FALSE;//TODO
}

BOOL SQLManager::GetFirstVal(LPCTSTR sql, INT& val)
{
	SQLiteStmt rs(m_pDB);
	if (rs.Open(sql) && !rs.IsEOF())
	{
		val = rs.GetINTFieldValue(0);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::DeleteAlbumRecord(UINT ID)
{
	ASSERT(ID > 0);
	DeleteAllAlbumInfo(ID);
	CString sql;
	sql.Format(_T("DELETE FROM Albums WHERE ID=%u"), ID);
	m_changes.albums.removals++;
	m_albumCache.ID = 0;
	return m_pDB->Execute(sql) != -1;
}

BOOL SQLManager::DeleteArtistRecord(UINT ID)
{
	ASSERT(ID > 0);
	DeleteAllArtistInfo(ID);
	CString sql;
	sql.Format(_T("DELETE FROM Artists WHERE ID=%u"), ID);
	m_changes.artists.removals++;
	m_artistCache.ID = 0;
	return m_pDB->Execute(sql) != -1;
}

BOOL SQLManager::DeleteGenreRecord(UINT ID)
{
	ASSERT(ID > 0);
	CString sql;
	sql.Format(_T("DELETE FROM Genres WHERE ID=%u"), ID);
	m_changes.genres.removals++;
	m_genreCache.ID = 0;
	return m_pDB->Execute(sql) != -1;
}

BOOL SQLManager::DeleteCollectionRecord(UINT ID, BOOL bForce)
{
	ASSERT(ID > 0);
	CString sql;
	if (bForce)
	{
		sql.Format(_T("DELETE FROM Tracks WHERE CollectionID=%u"), ID);
		if (m_pDB->Execute(sql) == -1)
			return FALSE;
	}
#ifdef USE_INTEGRITY_CHECKS
	else
	{
		return FALSE;
	}
#endif // USE_INTEGRITY_CHECKS
	sql.Format(_T("DELETE FROM Collections WHERE ID=%u"), ID);
	if (m_pDB->Execute(sql) == -1)
		return FALSE;
	m_changes.collections.removals++;
	return TRUE;
}

BOOL SQLManager::DeleteHistLogRecord(UINT ID)
{
	//CAUTION The track Record must be handled properly
	ASSERT(ID > 0);
	CString sql;
	sql.Format(_T("DELETE FROM HistLog WHERE ID=%u"), ID);
	return m_pDB->Execute(sql) != -1;
}


UINT SQLManager::GetTrackCount(TracksFilter& tf)
{
	CString Where = GetWhereSQL(tf);
	CString SQL = _T("SELECT count(*) FROM Tracks");
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE");
		SQL += &((LPCTSTR)Where)[4];
	}
	SQLiteStmt rs(m_pDB);
	if (rs.Open(SQL) && !rs.IsEOF())
		return rs.GetUINTFieldValue(0);
	return 0;
}

BOOL SQLManager::GetTrackIDList(TracksFilter& tf, std::vector<UINT>& idList)
{
	CString Where = GetWhereSQL(tf);
	CString SQL;
	if (tf.Album.match == TXTM_Disabled)
		SQL = _T("SELECT ID FROM Tracks");
	else
	{
		//This have been Changed because of AlbumByName case...
		SQL = _T("SELECT Tracks.ID FROM Tracks INNER JOIN Albums ON Tracks.albID = Albums.ID");
	}
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE");
		SQL +=  &((LPCTSTR)Where)[4];
	}
	SQLiteStmt rs(m_pDB);
	if (rs.Open(SQL))
	{
		while (!rs.IsEOF())
		{
			idList.push_back(rs.GetUINTFieldValue(0));
			rs.MoveNext();
		}
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::LogTrackInHistory(LPCTSTR artist, LPCTSTR track, HistoryLogActionsEnum actID, SYSTEMTIME& localTime)
{
	ASSERT(actID > HLA_Unknown && actID < HLA_Last);
	ASSERT(artist!= NULL && track != NULL);
	if (!(artist!= NULL && track != NULL && actID > HLA_Unknown && actID < HLA_Last))
		return FALSE;
	if (artist[0] == 0)
		artist = TS_UnknownString;
	if (track[0] == 0)
		track = TS_UnknownString;
	HistArtistRecord art;
	if (!GetHistArtistRecordUnique(artist, art))
	{
		art.name = artist;
		if (!AddNewHistArtistRecord(art))
			return FALSE;
	}
	ASSERT(art.IsValid());
	HistTrackRecord tra;
	if (!GetHistTrackRecordUnique(art.ID, track, tra))
	{
		tra.artID = art.ID;
		tra.name = track;
		if (!AddNewHistTrackRecord(tra))
			return FALSE;
	}
	ASSERT(tra.IsValid());

	HistLogRecord log;
	log.actID = actID;
	log.dateAdded = localTime;
	log.trackID = tra.ID;
	return AddNewHistLogRecord(log);
}


BOOL SQLManager::GetHistoryLogStats(SQLManager::HistoryLogStats& stats)
{
	SQLiteStmt rs(m_pDB);
	if (rs.Open(_T("SELECT count(*),actID,MIN(DateAdded),MAX(DateAdded) FROM HistLog GROUP BY actID")))
	{
		while (!rs.IsEOF())
		{
			HistoryLogActionsEnum actID = (HistoryLogActionsEnum) rs.GetUINTFieldValue(1);
			if (actID == HLA_Played)
			{
				stats.plays = rs.GetUINTFieldValue(0);
				stats.firstTime = rs.GetSYSTEMTIMEFieldValue(2);
				stats.lastTime = rs.GetSYSTEMTIMEFieldValue(3);
			}
			else if (actID == HLA_Clicked)
				stats.actions = rs.GetUINTFieldValue(0);
			else
				ASSERT(0);
			rs.MoveNext();
		}
		return TRUE;
	}
	return FALSE;
}


BOOL SQLManager::GetHistoryTrackStats(LPCTSTR Artist, LPCTSTR Track, SYSTEMTIME* startDate, SYSTEMTIME* endDate,//IN
								 SQLManager::HistoryTrackStats& stats)//OUT
{
	ASSERT(Artist!=NULL);
	ASSERT(Track!=NULL);
	if (Artist == NULL || Track == NULL)
		return FALSE;
#ifdef _DEBUG
	if (startDate && endDate)
	{
		FILETIME ftStart;
		FILETIME ftEnd;
		SystemTimeToFileTime(startDate, &ftStart);
		SystemTimeToFileTime(endDate, &ftEnd);
		ASSERT(CompareFileTime(&ftStart, &ftEnd) == -1);
	}
#endif
	std::tstring dateWhere;
	if (startDate != NULL)
	{
		dateWhere += _T(" AND HistLog.DateAdded >= ");
		dateWhere += SQLDate(*startDate);
	}
	if (endDate != NULL)
	{
		dateWhere += _T(" AND HistLog.DateAdded < ");
		dateWhere += SQLDate(*endDate);
	}

	std::tstring SQL(_T("SELECT count(*), MIN(HistLog.DateAdded), MAX(HistLog.DateAdded) ")
		_T("FROM HistLog INNER JOIN HistTracks ON HistLog.trackID = HistTracks.ID ")
		_T("INNER JOIN HistArtists ON HistTracks.artID = HistArtists.ID ")
		_T("WHERE HistArtists.Name = ? AND HistTracks.Name = ? AND HistLog.actID = 1"));
	SQL += dateWhere;
	SQLiteStmt rs(m_pDB);
	if (rs.Prepare(SQL.c_str()) && rs.BindText(1, Artist) && rs.BindText(2, Track) &&
		rs.Step() && !rs.IsEOF())
	{
		stats.hits = rs.GetUINTFieldValue(0);
		if (stats.hits > 0)
		{
			stats.firstTime = rs.GetSYSTEMTIMEFieldValue(1);
			stats.lastTime = rs.GetSYSTEMTIMEFieldValue(2);
			std::tstring rankSQL(_T("SELECT count(*) AS Hits FROM HistLog WHERE actID=1 "));
			rankSQL += dateWhere;
			rankSQL += _T(" GROUP BY trackID ORDER BY count(*) DESC");
			SQLiteStmt rsRank(m_pDB);
			if (rsRank.Open(rankSQL.c_str()))
			{
				while (!rsRank.IsEOF())
				{
					stats.rank++;
					if (stats.hits == rsRank.GetUINTFieldValue(0))
						break;
					rsRank.MoveNext();
				}
			}
		}
	}
	return TRUE;
}


//WHERE conditions shared by the three history collection queries.
void SQLManager::AddHistoryWhere(CString& Where, SYSTEMTIME* startDate, SYSTEMTIME* endDate,
	TCHAR* ArtistFilter, TCHAR* TrackFilter, HistoryLogActionsEnum actID)
{
	DateMatch dm;
	if (startDate != NULL)
	{
		dm.match = DATM_After;
		dm.val = *startDate;
		AddSQLDateParameter(Where, dm, _T("HistLog.DateAdded"));
	}
	if (endDate != NULL)
	{
		dm.match = DATM_Before;
		dm.val = *endDate;
		AddSQLDateParameter(Where, dm, _T("HistLog.DateAdded"));
	}
	TextMatch tm;
	if (ArtistFilter != NULL)
	{
		tm.match = TXTM_Like;
		tm.val = ArtistFilter;
		AddSQLStrParameter(Where, tm, _T("HistArtists.Name"));
	}
	if (TrackFilter != NULL)
	{
		tm.match = TXTM_Like;
		tm.val = TrackFilter;
		AddSQLStrParameter(Where, tm, _T("HistTracks.Name"));
	}
	if (actID > HLA_Unknown && actID < HLA_Last)
	{
		NumericMatch nm;
		nm.match = NUMM_Exact;
		nm.val = actID;
		AddSQLNumParameter(Where, nm, _T("HistLog.actID"));
	}
}

const TCHAR* const HistoryJoinSQL = _T("FROM HistArtists INNER JOIN HistTracks ON HistTracks.artID = HistArtists.ID ")
	_T("INNER JOIN HistLog ON HistLog.trackID = HistTracks.ID ");

BOOL SQLManager::GetFullLogRecordCollection(FullHistLogRecordCollection& col,
											SYSTEMTIME* startDate/* = NULL*/,
											SYSTEMTIME* endDate /*= NULL*/,
											TCHAR* ArtistFilter /*= NULL*/,
											TCHAR* TrackFilter /*= NULL*/,
											HistoryLogActionsEnum actID /*= -1*/,
											UINT limitResults /*= 0*/)
{
	std::tstring SQL(_T("SELECT ")
		_T("HistLog.ID, HistLog.actID, HistLog.DateAdded, HistLog.trackID, ")
		_T("HistTracks.ID, HistTracks.Name, HistTracks.artID, ")
		_T("HistArtists.ID, HistArtists.Name "));
	SQL += HistoryJoinSQL;
	CString Where;
	AddHistoryWhere(Where, startDate, endDate, ArtistFilter, TrackFilter, actID);
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}

	SQLiteStmt rs(m_pDB);
	if (!rs.Open(SQL.c_str()))
		return FALSE;
	UINT countResults = 0;
	while (!rs.IsEOF())
	{
		FullHistLogRecordSP rec(new FullHistLogRecord);
		short startIdx = 0;
		GetHistLogRecordFields(rec->log, rs, startIdx);
		GetHistTrackRecordFields(rec->track, rs, startIdx);
		GetHistArtistRecordFields(rec->artist, rs, startIdx);
		col.push_back(rec);
		countResults++;
		if (countResults == limitResults)
			return TRUE;
		rs.MoveNext();
	}
	return TRUE;
}

BOOL SQLManager::GetFullHistTrackRecordCollection(FullHistTrackRecordCollection& col,
											SYSTEMTIME* startDate/* = NULL*/,
											SYSTEMTIME* endDate /*= NULL*/,
											TCHAR* ArtistFilter /*= NULL*/,
											TCHAR* TrackFilter /*= NULL*/,
											HistoryLogActionsEnum actID /*= -1*/,
											UINT limitResults /*= 0*/)
{
	std::tstring SQL(_T("SELECT ")
		_T("max(HistTracks.ID), max(HistTracks.Name), max(HistTracks.artID), ")
		_T("max(HistArtists.ID), max(HistArtists.Name), ")
		_T("count(*) AS Hits, min(HistLog.DateAdded) AS FirstTime, max(HistLog.DateAdded) AS LastTime "));
	SQL += HistoryJoinSQL;
	CString Where;
	AddHistoryWhere(Where, startDate, endDate, ArtistFilter, TrackFilter, actID);
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}
	SQL += _T(" GROUP BY HistLog.trackID");

	SQLiteStmt rs(m_pDB);
	if (!rs.Open(SQL.c_str()))
		return FALSE;
	UINT countResults = 0;
	while (!rs.IsEOF())
	{
		FullHistTrackRecordSP rec(new FullHistTrackRecord);
		short startIdx = 0;
		GetHistTrackRecordFields(rec->track, rs, startIdx);
		GetHistArtistRecordFields(rec->artist, rs, startIdx);
		rec->hits = rs.GetUINTFieldValue(startIdx++);
		rec->firstTime = rs.GetSYSTEMTIMEFieldValue(startIdx++);
		rec->lastTime = rs.GetSYSTEMTIMEFieldValue(startIdx++);
		col.push_back(rec);
		countResults++;
		if (countResults == limitResults)
			return TRUE;
		rs.MoveNext();
	}
	return TRUE;
}

BOOL SQLManager::GetFullHistArtistRecordCollection(FullHistArtistRecordCollection& col,
									   SYSTEMTIME* startDate/* = NULL*/,
									   SYSTEMTIME* endDate /*= NULL*/,
									   TCHAR* ArtistFilter /*= NULL*/,
									   TCHAR* TrackFilter /*= NULL*/,
									   HistoryLogActionsEnum actID /*= 0*/,
									   UINT limitResults /*= 0*/)
{
	std::tstring SQL(_T("SELECT ")
		_T("max(HistArtists.ID), max(HistArtists.Name), ")
		_T("count(*) AS Hits, min(HistLog.DateAdded) AS FirstTime, max(HistLog.DateAdded) AS LastTime "));
	SQL += HistoryJoinSQL;
	CString Where;
	AddHistoryWhere(Where, startDate, endDate, ArtistFilter, TrackFilter, actID);
	if (!Where.IsEmpty())
	{
		SQL += _T(" WHERE ");
		SQL += &((LPCTSTR)Where)[4];
	}
	SQL += _T(" GROUP BY HistTracks.artID");

	SQLiteStmt rs(m_pDB);
	if (!rs.Open(SQL.c_str()))
		return FALSE;
	UINT countResults = 0;
	while (!rs.IsEOF())
	{
		FullHistArtistRecordSP rec(new FullHistArtistRecord);
		short startIdx = 0;
		GetHistArtistRecordFields(rec->artist, rs, startIdx);
		rec->hits = rs.GetUINTFieldValue(startIdx++);
		rec->firstTime = rs.GetSYSTEMTIMEFieldValue(startIdx++);
		rec->lastTime = rs.GetSYSTEMTIMEFieldValue(startIdx++);
		col.push_back(rec);
		countResults++;
		if (countResults == limitResults)
			return TRUE;
		rs.MoveNext();
	}
	return TRUE;
}


BOOL SQLManager::GetHistTrackRankCollection(std::vector<Rank>& col)
{
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(_T("SELECT Hits, count(Hits) ")
		_T("FROM (SELECT count(*) AS Hits FROM HistLog WHERE actID = 1 GROUP BY trackID) ")
		_T("GROUP BY Hits ORDER BY Hits DESC")))
		return FALSE;
	while (!rs.IsEOF())
	{
		Rank r;
		r.Hits = rs.GetUINTFieldValue(0);
		r.HitsCount = rs.GetUINTFieldValue(1);
		col.push_back(r);
		rs.MoveNext();
	}
	return TRUE;
}

BOOL SQLManager::GetHistArtistRankCollection(std::vector<Rank>& col)
{
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(_T("SELECT Hits, count(Hits) ")
		_T("FROM (SELECT count(*) AS Hits FROM HistLog INNER JOIN HistTracks ON HistLog.trackID=HistTracks.ID ")
		_T("WHERE HistLog.actID = 1 GROUP BY HistTracks.artID) ")
		_T("GROUP BY Hits ORDER BY Hits DESC")))
		return FALSE;
	while (!rs.IsEOF())
	{
		Rank r;
		r.Hits = rs.GetUINTFieldValue(0);
		r.HitsCount = rs.GetUINTFieldValue(1);
		col.push_back(r);
		rs.MoveNext();
	}
	return TRUE;
}


BOOL SQLManager::EvaluateTrackInfo(InfoItemTypeEnum iit)
{
	switch (iit)
	{
	case IIT_TrackLyrics:
	case IIT_TrackComment:
	case IIT_TrackPersonal:
	case IIT_TrackTablatures:
		return TRUE;
	}
	ASSERT(0);
	return FALSE;
}
BOOL SQLManager::EvaluateAlbumInfo(InfoItemTypeEnum iit)
{
	switch (iit)
	{
	case IIT_AlbumReview:
		return TRUE;
	}
	ASSERT(0);
	return FALSE;
}
BOOL SQLManager::EvaluateArtistInfo(InfoItemTypeEnum iit)
{
	switch (iit)
	{
	case IIT_ArtistBio:
		return TRUE;
	}
	ASSERT(0);
	return FALSE;
}

//-----------------------------------------
//-----------Info (TracksInfo / AlbumsInfo / ArtistsInfo)
//tblName and fldName are compile-time constants (never user input).
//-----------------------------------------

BOOL SQLManager::AddNewInfo(LPCTSTR tblName, LPCTSTR fldName, UINT ID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(tblName != NULL && fldName != NULL && ID > 0 && info != NULL);
	if (info[0] == 0)
		return FALSE;
	CString sql;
	sql.Format(_T("INSERT INTO %s (%s, Type, Name, DateAdded) VALUES (?,?,?,?)"), tblName, fldName);
	SYSTEMTIME st;
	GetLocalTime(&st);
	SQLiteStmt stmt(m_pDB);
	if (stmt.Prepare(sql) && stmt.BindUInt(1, ID) && stmt.BindInt(2, iit) &&
		stmt.BindText(3, info) && stmt.BindDate(4, st) && stmt.Exec() > 0)
	{
		m_changes.info.additions++;
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::UpdateInfo(LPCTSTR tblName, LPCTSTR fldName, UINT ID, InfoItemTypeEnum iit, LPCTSTR info)
{
	CString sql;
	sql.Format(_T("UPDATE %s SET Name=? WHERE %s=? AND Type=?"), tblName, fldName);
	SQLiteStmt stmt(m_pDB);
	if (stmt.Prepare(sql) && stmt.BindText(1, info) && stmt.BindUInt(2, ID) &&
		stmt.BindInt(3, iit) && stmt.Exec() > 0)
	{
		m_changes.info.updates++;
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::DeleteInfo(LPCTSTR tblName, LPCTSTR fldName, UINT ID, InfoItemTypeEnum iit)
{
	CString sql;
	sql.Format(_T("DELETE FROM %s WHERE %s=%u AND Type=%d"), tblName, fldName, ID, iit);
	return m_pDB->Execute(sql) != -1;
}

BOOL SQLManager::GetInfoRecord(LPCTSTR tblName, LPCTSTR fldName, UINT ID, InfoItemTypeEnum iit, InfoRecord& ir)
{
	CString sql;
	sql.Format(_T("SELECT ID, Name, DateAdded FROM %s WHERE %s=%u AND Type=%d"), tblName, fldName, ID, iit);
	SQLiteStmt rs(m_pDB);
	if (rs.Open(sql) && !rs.IsEOF())
	{
		ir.ID = rs.GetUINTFieldValue(0);
		rs.GetStringFieldValue(1, ir.name);
		ir.dateAdded = rs.GetSYSTEMTIMEFieldValue(2);
		return TRUE;
	}
	return FALSE;
}

BOOL SQLManager::AddNewTrackInfo(UINT trackID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(trackID != 0 && info != NULL);
	if (!EvaluateTrackInfo(iit))
		return FALSE;
	return AddNewInfo(_T("TracksInfo"), _T("trackID"), trackID, iit, info);
}
BOOL SQLManager::AddNewAlbumInfo(UINT albID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(albID != 0 && info != NULL);
	if (!EvaluateAlbumInfo(iit))
		return FALSE;
	return AddNewInfo(_T("AlbumsInfo"), _T("albID"), albID, iit, info);
}
BOOL SQLManager::AddNewArtistInfo(UINT artID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(artID != 0 && info != NULL);
	if (!EvaluateArtistInfo(iit))
		return FALSE;
	return AddNewInfo(_T("ArtistsInfo"), _T("artID"), artID, iit, info);
}

BOOL SQLManager::UpdateTrackInfo(UINT trackID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(trackID != 0 && info != NULL);
	if (!EvaluateTrackInfo(iit))
		return FALSE;
	return UpdateInfo(_T("TracksInfo"), _T("trackID"), trackID, iit, info);
}
BOOL SQLManager::UpdateAlbumInfo(UINT albID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(albID != 0 && info != NULL);
	if (!EvaluateAlbumInfo(iit))
		return FALSE;
	return UpdateInfo(_T("AlbumsInfo"), _T("albID"), albID, iit, info);
}
BOOL SQLManager::UpdateArtistInfo(UINT artID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(artID != 0 && info != NULL);
	if (!EvaluateArtistInfo(iit))
		return FALSE;
	return UpdateInfo(_T("ArtistsInfo"), _T("artID"), artID, iit, info);
}

BOOL SQLManager::DeleteTrackInfo(UINT trackID, InfoItemTypeEnum iit)
{
	ASSERT(trackID != 0);
	if (!EvaluateTrackInfo(iit))
		return FALSE;
	return DeleteInfo(_T("TracksInfo"), _T("trackID"), trackID, iit);
}
BOOL SQLManager::DeleteAlbumInfo(UINT albID, InfoItemTypeEnum iit)
{
	ASSERT(albID != 0);
	if (!EvaluateAlbumInfo(iit))
		return FALSE;
	return DeleteInfo(_T("AlbumsInfo"), _T("albID"), albID, iit);
}
BOOL SQLManager::DeleteArtistInfo(UINT artID, InfoItemTypeEnum iit)
{
	ASSERT(artID != 0);
	if (!EvaluateArtistInfo(iit))
		return FALSE;
	return DeleteInfo(_T("ArtistsInfo"), _T("artID"), artID, iit);
}

BOOL SQLManager::DeleteAllTrackInfo(UINT trackID)
{
	CString sql;
	sql.Format(_T("DELETE FROM TracksInfo WHERE trackID=%u"), trackID);
	return m_pDB->Execute(sql) != -1;
}
BOOL SQLManager::DeleteAllAlbumInfo(UINT albID)
{
	return DeleteAlbumInfo(albID, IIT_AlbumReview);
}
BOOL SQLManager::DeleteAllArtistInfo(UINT artID)
{
	return DeleteArtistInfo(artID, IIT_ArtistBio);
}

BOOL SQLManager::AdjustTrackInfo(UINT trackID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(trackID != 0 && info != NULL);
	if (!EvaluateTrackInfo(iit))
		return FALSE;
	InfoRecord ir;
	if (GetTrackInfoRecord(trackID, iit, ir))
	{
		if (info[0] == 0)
			return DeleteTrackInfo(trackID, iit);
		return UpdateTrackInfo(trackID, iit, info);
	}
	if (info[0] == 0)
		return TRUE;//Nothing to add, nothing to delete
	return AddNewTrackInfo(trackID, iit, info);
}
BOOL SQLManager::AdjustAlbumInfo(UINT albID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(albID != 0 && info != NULL);
	if (!EvaluateAlbumInfo(iit))
		return FALSE;
	InfoRecord ir;
	if (GetAlbumInfoRecord(albID, iit, ir))
	{
		if (info[0] == 0)
			return DeleteAlbumInfo(albID, iit);
		return UpdateAlbumInfo(albID, iit, info);
	}
	if (info[0] == 0)
		return TRUE;
	return AddNewAlbumInfo(albID, iit, info);
}
BOOL SQLManager::AdjustArtistInfo(UINT artID, InfoItemTypeEnum iit, LPCTSTR info)
{
	ASSERT(artID != 0 && info != NULL);
	if (!EvaluateArtistInfo(iit))
		return FALSE;
	InfoRecord ir;
	if (GetArtistInfoRecord(artID, iit, ir))
	{
		if (info[0] == 0)
			return DeleteArtistInfo(artID, iit);
		return UpdateArtistInfo(artID, iit, info);
	}
	if (info[0] == 0)
		return TRUE;
	return AddNewArtistInfo(artID, iit, info);
}

BOOL SQLManager::GetTrackInfoRecord(UINT trackID, InfoItemTypeEnum iit, InfoRecord& ir)
{
	ASSERT(trackID != 0);
	if (!EvaluateTrackInfo(iit))
		return FALSE;
	return GetInfoRecord(_T("TracksInfo"), _T("trackID"), trackID, iit, ir);
}
BOOL SQLManager::GetAlbumInfoRecord(UINT albID, InfoItemTypeEnum iit, InfoRecord& ir)
{
	ASSERT(albID != 0);
	if (!EvaluateAlbumInfo(iit))
		return FALSE;
	return GetInfoRecord(_T("AlbumsInfo"), _T("albID"), albID, iit, ir);
}
BOOL SQLManager::GetArtistInfoRecord(UINT artID, InfoItemTypeEnum iit, InfoRecord& ir)
{
	ASSERT(artID != 0);
	if (!EvaluateArtistInfo(iit))
		return FALSE;
	return GetInfoRecord(_T("ArtistsInfo"), _T("artID"), artID, iit, ir);
}

UINT SQLManager::GetTrackInfoFlags(UINT trackID)
{
	CString sql;
	sql.Format(_T("SELECT Type FROM TracksInfo WHERE trackID=%u"), trackID);
	UINT flags = TRF_NoFlags;
	SQLiteStmt rs(m_pDB);
	if (!rs.Open(sql))
		return flags;
	while (!rs.IsEOF())
	{
		switch (rs.GetUINTFieldValue(0))
		{
		case IIT_TrackComment:
			flags |= TRF_HasComments;
			break;
		case IIT_TrackLyrics:
			flags |= TRF_HasLyrics;
			break;
		case IIT_TrackPersonal:
			flags |= TRF_HasPersonal;
			break;
		case IIT_TrackTablatures:
			flags |= TRF_HasTablatures;
			break;
		}
		rs.MoveNext();
	}
	return flags;
}


static bool UDgreater(const SQLManager::ValuedID& elem1, const SQLManager::ValuedID& elem2)
{
	return elem1.value > elem2.value;
}

BOOL SQLManager::GetSimilarArtists(std::vector<ValuedID>& col, UINT artistID, INT maxTagsToExamine, INT maxArtistsPerGenreToExamine)
{
	ASSERT(artistID != 0);
	SQLiteStmt rs(m_pDB);
	if (!rs.Prepare(_T("SELECT genreID,count(*) FROM Tracks ")
			_T("WHERE artID=? AND genreID>1 GROUP BY genreID ORDER BY count(*) DESC")) ||
		!rs.BindUInt(1, artistID) || !rs.Step())
		return FALSE;
	SQLiteStmt rsArt(m_pDB);
	SQLiteStmt rsArtTrackCount(m_pDB);
	if (!rsArt.Prepare(_T("SELECT artID,count(*) FROM Tracks ")
			_T("WHERE genreID=? AND artID<>? GROUP BY artID ORDER BY count(*) DESC")) ||
		!rsArtTrackCount.Prepare(_T("SELECT count(*) FROM Tracks WHERE artID=?")))
		return FALSE;
	INT tagSearch = 0;
	while (!rs.IsEOF())
	{
		tagSearch++;
		if (tagSearch > maxTagsToExamine)//===LIMIT THE TAG SEARCHING
			break;
		UINT genID = rs.GetUINTFieldValue(0);
		UINT traCount = rs.GetUINTFieldValue(1);
		rsArt.Reset();
		if (rsArt.BindUInt(1, genID) && rsArt.BindUInt(2, artistID) && rsArt.Step())
		{
			INT artSearch = 0;
			while (!rsArt.IsEOF())
			{
				artSearch++;
				if (artSearch > maxArtistsPerGenreToExamine)//===LIMIT THE ARTIST SEARCHING
					break;
				UINT artID = rsArt.GetUINTFieldValue(0);
				UINT count = rsArt.GetUINTFieldValue(1);

				INT artistTrackCount = count;
				rsArtTrackCount.Reset();
				if (rsArtTrackCount.BindUInt(1, artID) && rsArtTrackCount.Step() && !rsArtTrackCount.IsEOF())
					artistTrackCount = rsArtTrackCount.GetUINTFieldValue(0);
				if (artistTrackCount == 0)
					artistTrackCount = 1;

				DOUBLE value = (traCount * count) / (DOUBLE)artistTrackCount;
				BOOL bFound = FALSE;
				for (std::vector<ValuedID>::iterator it = col.begin(); it != col.end(); it++)
				{
					if (it->ID == artID)
					{
						it->value += value;
						bFound = TRUE;
						it->appearanceCount++;
						break;
					}
				}
				if (bFound == FALSE)
					col.push_back(ValuedID(artID, value));

				rsArt.MoveNext();
			}
		}
		rs.MoveNext();
	}
	std::sort(col.begin(), col.end(), UDgreater);
	return TRUE;
}
