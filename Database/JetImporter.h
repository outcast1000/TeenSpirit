//	One-time import of the legacy Jet/Access database (music.mdb, schema
//	version 6, used up to 0.98f) into the SQLite database.
//
//	Requires the Microsoft Jet 4.0 OLE DB provider, which exists only for
//	32-bit processes; in 64-bit builds Import() always fails with a message.
#ifndef _JetImporter_h_
#define _JetImporter_h_

#include <string>

class SQLiteDB;

namespace JetImporter
{
	//Copies all tables (preserving IDs) into db, which must contain the current
	//schema and no rows. Runs inside the caller's transaction; the .mdb is
	//opened read-only and never modified.
	//On failure returns FALSE and a description in error.
	BOOL Import(LPCTSTR mdbPath, SQLiteDB& db, std::wstring& error);
}

#endif
