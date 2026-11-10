#include "stdafx.h"
#include "JetImporter.h"
#include "SQLite/SQLiteDB.h"

#ifdef _WIN64

BOOL JetImporter::Import(LPCTSTR mdbPath, SQLiteDB& db, std::wstring& error)
{
	error = L"Importing the old database (music.mdb) needs the 32-bit version of the program. "
		L"Run the 32-bit version once to convert it.";
	return FALSE;
}

#else

//ADO type library (32-bit ADO dir is on the include path, see Directory.Build.props).
#import <msado15.dll> rename("EOF", "adoEOF")

namespace
{
	//The legacy format's fixed database password (not a secret: it only made
	//the .mdb opaque to Access users).
	LPCWSTR const cJetPassword = L"DontMessWithIt";
	const INT cSupportedJetVersion = 6;

	struct TableSpec
	{
		LPCWSTR table;
		LPCWSTR jetColumns;		//Jet SQL (reserved words bracketed)
		LPCWSTR sqliteColumns;
		INT columnCount;
		BOOL bIgnoreDuplicates;	//Tables with unique indexes that old databases did not always enforce
	};

	const TableSpec cTables[] =
	{
		{L"Genres",			L"ID, Name",									L"ID, Name",									2, FALSE},
		{L"Artists",		L"ID, Name, GenreID, Rating",					L"ID, Name, GenreID, Rating",					4, FALSE},
		{L"Albums",			L"ID, Name, artID, [Year], Rating",				L"ID, Name, artID, Year, Rating",				5, FALSE},
		{L"Collections",	L"ID, Name, PathName, Serial, Type, DateAdded, DateUpdated",
							L"ID, Name, PathName, Serial, Type, DateAdded, DateUpdated",					7, FALSE},
		{L"Tracks",			L"ID, Name, Type, DateAdded, albID, artID, CollectionID, genreID, Path, Bitrate, Duration, [Size], TrackNo, [Year], Rating",
							L"ID, Name, Type, DateAdded, albID, artID, CollectionID, genreID, Path, Bitrate, Duration, Size, TrackNo, Year, Rating",
																															15, FALSE},
		{L"HistArtists",	L"ID, Name",									L"ID, Name",									2, FALSE},
		{L"HistTracks",		L"ID, Name, artID",								L"ID, Name, artID",								3, FALSE},
		{L"HistLog",		L"ID, actID, DateAdded, trackID",				L"ID, actID, DateAdded, trackID",				4, FALSE},
		{L"TracksInfo",		L"ID, trackID, Name, DateAdded, Type",			L"ID, trackID, Name, DateAdded, Type",			5, FALSE},
		{L"AlbumsInfo",		L"ID, albID, Name, DateAdded, Type",			L"ID, albID, Name, DateAdded, Type",			5, FALSE},
		{L"ArtistsInfo",	L"ID, artID, Name, DateAdded, Type",			L"ID, artID, Name, DateAdded, Type",			5, FALSE},
		{L"LocalPic",		L"parID, infoType, Path, Type",					L"parID, infoType, Path, Type",					4, TRUE},
		{L"InfoRequests",	L"requestTimeStamp, hash",						L"requestTimeStamp, hash",						2, TRUE},
	};

	std::wstring ComErrorText(_com_error& e)
	{
		std::wstring s;
		_bstr_t desc(e.Description());
		if (desc.length() > 0)
			s = (LPCWSTR)desc;
		else
			s = e.ErrorMessage();
		WCHAR code[32];
		_snwprintf_s(code, _TRUNCATE, L" (0x%08lX)", (unsigned long)e.Error());
		code[31] = 0;
		return s + code;
	}

	BOOL BindVariant(SQLiteStmt& st, INT idx, const _variant_t& value)
	{
		switch (value.vt)
		{
		case VT_NULL:
		case VT_EMPTY:
			return st.BindNull(idx);
		case VT_BOOL:
			return st.BindInt(idx, value.boolVal != VARIANT_FALSE ? 1 : 0);
		case VT_I1: case VT_UI1: case VT_I2: case VT_UI2:
		case VT_I4: case VT_UI4: case VT_INT: case VT_UINT:
		case VT_I8: case VT_UI8:
			{
				_variant_t v;
				if (FAILED(VariantChangeType(&v, &value, 0, VT_I8)))
					return FALSE;
				return st.BindInt64(idx, v.llVal);
			}
		case VT_R4: case VT_R8: case VT_CY: case VT_DECIMAL:
			{
				_variant_t v;
				if (FAILED(VariantChangeType(&v, &value, 0, VT_R8)))
					return FALSE;
				return st.BindDouble(idx, v.dblVal);
			}
		case VT_DATE:
			{
				SYSTEMTIME sysTime;
				if (!VariantTimeToSystemTime(value.date, &sysTime))
					return FALSE;
				return st.BindDate(idx, sysTime);
			}
		case VT_BSTR:
			return st.BindText(idx, value.bstrVal != NULL ? value.bstrVal : L"");
		default:
			{
				_variant_t v;
				if (FAILED(VariantChangeType(&v, &value, 0, VT_BSTR)))
					return FALSE;
				return st.BindText(idx, v.bstrVal != NULL ? v.bstrVal : L"");
			}
		}
	}

	INT CountRows(SQLiteDB& db, LPCWSTR table)
	{
		std::wstring sql(L"SELECT count(*) FROM ");
		sql += table;
		SQLiteStmt st(&db);
		if (st.Open(sql.c_str()) && !st.IsEOF())
			return st.GetINTFieldValue(0);
		return -1;
	}

	BOOL ImportTable(ADODB::_ConnectionPtr& con, SQLiteDB& db, const TableSpec& spec, std::wstring& error)
	{
		std::wstring insert(spec.bIgnoreDuplicates ? L"INSERT OR IGNORE INTO " : L"INSERT INTO ");
		insert += spec.table;
		insert += L" (";
		insert += spec.sqliteColumns;
		insert += L") VALUES (";
		for (INT i = 0; i < spec.columnCount; i++)
			insert += i == 0 ? L"?" : L",?";
		insert += L")";
		SQLiteStmt st(&db);
		if (!st.Prepare(insert.c_str()))
		{
			error = db.GetLastError();
			return FALSE;
		}

		INT sourceRows = 0;
		try
		{
			std::wstring select(L"SELECT ");
			select += spec.jetColumns;
			select += L" FROM ";
			select += spec.table;
			ADODB::_RecordsetPtr rs;
			rs.CreateInstance(__uuidof(ADODB::Recordset));
			rs->Open(_bstr_t(select.c_str()), _variant_t((IDispatch*)con, true),
				ADODB::adOpenForwardOnly, ADODB::adLockReadOnly, ADODB::adCmdText);
			while (rs->adoEOF == VARIANT_FALSE)
			{
				st.Reset();
				//Read in column order: Jet memo fields must be read sequentially.
				for (INT i = 0; i < spec.columnCount; i++)
				{
					_variant_t value = rs->Fields->GetItem(_variant_t((long)i))->GetValue();
					if (!BindVariant(st, i + 1, value))
					{
						error = std::wstring(L"Unsupported value in table ") + spec.table;
						return FALSE;
					}
				}
				if (st.Exec() == -1)
				{
					error = db.GetLastError();
					return FALSE;
				}
				sourceRows++;
				rs->MoveNext();
			}
			rs->Close();
		}
		catch (_com_error& e)
		{
			error = std::wstring(L"Reading table ") + spec.table + L": " + ComErrorText(e);
			return FALSE;
		}

		INT destRows = CountRows(db, spec.table);
		TRACE(_T("@2 JetImporter: %s %d rows\r\n"), spec.table, destRows);
		if (destRows < 0 || destRows > sourceRows || (!spec.bIgnoreDuplicates && destRows != sourceRows))
		{
			WCHAR bf[200];
			_snwprintf_s(bf, _TRUNCATE, L"Table %s: read %d rows but wrote %d", spec.table, sourceRows, destRows);
			bf[199] = 0;
			error = bf;
			return FALSE;
		}
		return TRUE;
	}

	BOOL ImportAll(LPCTSTR mdbPath, SQLiteDB& db, std::wstring& error)
	{
		ADODB::_ConnectionPtr con;
		try
		{
			if (FAILED(con.CreateInstance(__uuidof(ADODB::Connection))))
			{
				error = L"ADO is not available on this system.";
				return FALSE;
			}
			std::wstring cs(L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=");
			cs += mdbPath;
			cs += L";Jet OLEDB:Database Password=";
			cs += cJetPassword;
			cs += L";Mode=Read;";
			con->Open(_bstr_t(cs.c_str()), _bstr_t(L""), _bstr_t(L""), ADODB::adConnectUnspecified);

			ADODB::_RecordsetPtr rs = con->Execute(_bstr_t(L"SELECT version FROM DBInfo"), NULL, ADODB::adCmdText);
			INT version = 0;
			if (rs->adoEOF == VARIANT_FALSE)
			{
				_variant_t v = rs->Fields->GetItem(_variant_t((long)0))->GetValue();
				_variant_t vi;
				if (SUCCEEDED(VariantChangeType(&vi, &v, 0, VT_I4)))
					version = vi.lVal;
			}
			rs->Close();
			if (version != cSupportedJetVersion)
			{
				WCHAR bf[200];
				_snwprintf_s(bf, _TRUNCATE, L"Unsupported database version %d (expected %d). "
					L"Open it once with version 0.98 of the program to upgrade it first.",
					version, cSupportedJetVersion);
				bf[199] = 0;
				error = bf;
				con->Close();
				return FALSE;
			}
		}
		catch (_com_error& e)
		{
			error = L"Opening the database: " + ComErrorText(e);
			return FALSE;
		}

		for (size_t i = 0; i < sizeof(cTables) / sizeof(cTables[0]); i++)
		{
			if (!ImportTable(con, db, cTables[i], error))
			{
				try {con->Close();} catch (_com_error&) {}
				return FALSE;
			}
		}
		try {con->Close();} catch (_com_error&) {}

		//Jet stored the (unsigned) request hashes in a signed column.
		if (db.Execute(L"UPDATE InfoRequests SET hash = hash + 4294967296 WHERE hash < 0") == -1)
		{
			error = db.GetLastError();
			return FALSE;
		}
		return TRUE;
	}
}

BOOL JetImporter::Import(LPCTSTR mdbPath, SQLiteDB& db, std::wstring& error)
{
	ASSERT(mdbPath != NULL);
	//ADO needs COM on this thread (normally the UI thread, already initialized).
	HRESULT hrCom = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	BOOL bOK = ImportAll(mdbPath, db, error);
	if (SUCCEEDED(hrCom))
		CoUninitialize();
	return bOK;
}

#endif
