#include "stdafx.h"
#include "SQLiteDB.h"
#include <sqlite3.h>
#include <vector>

namespace
{
	std::string ToUtf8(LPCWSTR s, INT len = -1)
	{
		if (s == NULL)
			return std::string();
		INT n = WideCharToMultiByte(CP_UTF8, 0, s, len, NULL, 0, NULL, NULL);
		if (n <= 0)
			return std::string();
		std::string out(n, '\0');
		WideCharToMultiByte(CP_UTF8, 0, s, len, &out[0], n, NULL, NULL);
		if (len == -1)
			out.resize(n - 1);	//drop the terminator
		return out;
	}

	//Upper-cases with the invariant casing table: the same folding that
	//CompareStringOrdinal(bIgnoreCase=TRUE) uses, so LIKE and = agree.
	std::wstring ToUpperInvariant(LPCWSTR s)
	{
		INT len = (INT)wcslen(s);
		if (len == 0)
			return std::wstring();
		INT n = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, s, len, NULL, 0, NULL, NULL, 0);
		if (n <= 0)
			return std::wstring(s, len);
		std::wstring out(n, L'\0');
		LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, s, len, &out[0], n, NULL, NULL, 0);
		return out;
	}

	int CollateNoCaseU(void*, int nBytes1, const void* p1, int nBytes2, const void* p2)
	{
		//CSTR_LESS_THAN=1, CSTR_EQUAL=2, CSTR_GREATER_THAN=3
		int r = CompareStringOrdinal((LPCWCH)p1, nBytes1 / 2, (LPCWCH)p2, nBytes2 / 2, TRUE);
		return r == 0 ? 0 : r - 2;
	}

	//like(pattern, value [, escape]) -- "value LIKE pattern [ESCAPE escape]"
	void LikeFunc(sqlite3_context* ctx, int argc, sqlite3_value** argv)
	{
		const wchar_t* pattern = (const wchar_t*)sqlite3_value_text16(argv[0]);
		const wchar_t* value = (const wchar_t*)sqlite3_value_text16(argv[1]);
		if (pattern == NULL || value == NULL)
		{
			sqlite3_result_null(ctx);
			return;
		}
		unsigned int esc = 0;
		if (argc == 3)
		{
			const wchar_t* e = (const wchar_t*)sqlite3_value_text16(argv[2]);
			if (e == NULL || wcslen(e) != 1)
			{
				sqlite3_result_error(ctx, "ESCAPE expression must be a single character", -1);
				return;
			}
			esc = ToUpperInvariant(e)[0];
		}
		std::string p8 = ToUtf8(ToUpperInvariant(pattern).c_str());
		std::string v8 = ToUtf8(ToUpperInvariant(value).c_str());
		sqlite3_result_int(ctx, sqlite3_strlike(p8.c_str(), v8.c_str(), esc) == 0 ? 1 : 0);
	}
}

//==============================================================
//		SQLiteDB
//==============================================================

SQLiteDB::SQLiteDB():m_db(NULL)
{
}

SQLiteDB::~SQLiteDB()
{
	Close();
}

void SQLiteDB::SetError(LPCTSTR context)
{
	m_lastError = context;
	if (m_db != NULL)
	{
		m_lastError += L": ";
		m_lastError += (LPCWSTR)sqlite3_errmsg16(m_db);
	}
	TRACE(_T("@1 SQLiteDB %s\r\n"), m_lastError.c_str());
}

BOOL SQLiteDB::Open(LPCTSTR path)
{
	ASSERT(m_db == NULL);
	if (m_db != NULL || path == NULL)
		return FALSE;
	std::string path8 = ToUtf8(path);
	INT rc = sqlite3_open_v2(path8.c_str(), &m_db,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
	if (rc != SQLITE_OK)
	{
		SetError(_T("Open"));
		Close();
		return FALSE;
	}
	sqlite3_extended_result_codes(m_db, 1);
	sqlite3_busy_timeout(m_db, 10000);
	if (sqlite3_create_collation(m_db, "NOCASE_U", SQLITE_UTF16, NULL, CollateNoCaseU) != SQLITE_OK ||
		sqlite3_create_function(m_db, "like", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, NULL, LikeFunc, NULL, NULL) != SQLITE_OK ||
		sqlite3_create_function(m_db, "like", 3, SQLITE_UTF16 | SQLITE_DETERMINISTIC, NULL, LikeFunc, NULL, NULL) != SQLITE_OK)
	{
		SetError(_T("Register functions"));
		Close();
		return FALSE;
	}
	//WAL: readers do not block the writer (UI reads while a collection scan writes).
	if (Execute(_T("PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL; PRAGMA foreign_keys=OFF;")) == -1)
	{
		Close();
		return FALSE;
	}
	return TRUE;
}

void SQLiteDB::Close()
{
	if (m_db != NULL)
	{
		sqlite3_exec(m_db, "PRAGMA optimize;", NULL, NULL, NULL);
		sqlite3_close_v2(m_db);
		m_db = NULL;
	}
}

INT SQLiteDB::Execute(LPCTSTR sql)
{
	ASSERT(m_db != NULL && sql != NULL);
	if (m_db == NULL || sql == NULL)
		return -1;
	std::string sql8 = ToUtf8(sql);
	char* err = NULL;
	if (sqlite3_exec(m_db, sql8.c_str(), NULL, NULL, &err) != SQLITE_OK)
	{
		sqlite3_free(err);
		std::wstring ctx(_T("Execute '"));
		ctx += sql;
		ctx += _T("'");
		SetError(ctx.c_str());
		return -1;
	}
	return sqlite3_changes(m_db);
}

BOOL SQLiteDB::BeginTransaction()		{return Execute(_T("BEGIN IMMEDIATE")) != -1;}
BOOL SQLiteDB::CommitTransaction()		{return Execute(_T("COMMIT")) != -1;}
BOOL SQLiteDB::RollbackTransaction()	{return Execute(_T("ROLLBACK")) != -1;}

INT SQLiteDB::GetUserVersion()
{
	SQLiteStmt st(this);
	if (st.Open(_T("PRAGMA user_version")) && !st.IsEOF())
		return st.GetINTFieldValue(0);
	return -1;
}

BOOL SQLiteDB::SetUserVersion(INT version)
{
	TCHAR sql[64];
	_sntprintf_s(sql, _TRUNCATE, _T("PRAGMA user_version=%d"), version);
	sql[63] = 0;
	return Execute(sql) != -1;
}

std::wstring SQLiteDB::FormatDate(const SYSTEMTIME& st)
{
	WCHAR bf[32];
	_snwprintf_s(bf, _TRUNCATE, L"%04u-%02u-%02u %02u:%02u:%02u",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	bf[31] = 0;
	return bf;
}

BOOL SQLiteDB::ParseDate(const unsigned char* text, SYSTEMTIME& st)
{
	memset(&st, 0, sizeof(st));
	if (text == NULL)
		return FALSE;
	UINT y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
	INT n = sscanf_s((const char*)text, "%u-%u-%u %u:%u:%u", &y, &mo, &d, &h, &mi, &s);
	if (n < 3)
		return FALSE;
	st.wYear = (WORD)y;
	st.wMonth = (WORD)mo;
	st.wDay = (WORD)d;
	st.wHour = (WORD)h;
	st.wMinute = (WORD)mi;
	st.wSecond = (WORD)s;
	//Round-trip through FILETIME to fill in wDayOfWeek (as VariantTimeToSystemTime did).
	FILETIME ft;
	if (SystemTimeToFileTime(&st, &ft))
		FileTimeToSystemTime(&ft, &st);
	return TRUE;
}

//==============================================================
//		SQLiteStmt
//==============================================================

SQLiteStmt::SQLiteStmt(SQLiteDB* pDB):m_pDB(pDB),m_stmt(NULL),m_bEOF(TRUE)
{
	ASSERT(pDB != NULL);
}

SQLiteStmt::~SQLiteStmt()
{
	if (m_stmt != NULL)
		sqlite3_finalize(m_stmt);
}

BOOL SQLiteStmt::Prepare(LPCTSTR sql)
{
	ASSERT(sql != NULL);
	if (m_stmt != NULL)
	{
		sqlite3_finalize(m_stmt);
		m_stmt = NULL;
	}
	m_bEOF = TRUE;
	if (m_pDB == NULL || m_pDB->m_db == NULL || sql == NULL)
		return FALSE;
	if (sqlite3_prepare16_v2(m_pDB->m_db, sql, -1, &m_stmt, NULL) != SQLITE_OK)
	{
		std::wstring ctx(_T("Prepare '"));
		ctx += sql;
		ctx += _T("'");
		m_pDB->SetError(ctx.c_str());
		m_stmt = NULL;
		return FALSE;
	}
	return TRUE;
}

BOOL SQLiteStmt::BindInt(INT idx, INT value)
{
	return m_stmt != NULL && sqlite3_bind_int(m_stmt, idx, value) == SQLITE_OK;
}
BOOL SQLiteStmt::BindUInt(INT idx, UINT value)
{
	return m_stmt != NULL && sqlite3_bind_int64(m_stmt, idx, (sqlite3_int64)value) == SQLITE_OK;
}
BOOL SQLiteStmt::BindInt64(INT idx, __int64 value)
{
	return m_stmt != NULL && sqlite3_bind_int64(m_stmt, idx, value) == SQLITE_OK;
}
BOOL SQLiteStmt::BindDouble(INT idx, DOUBLE value)
{
	return m_stmt != NULL && sqlite3_bind_double(m_stmt, idx, value) == SQLITE_OK;
}
BOOL SQLiteStmt::BindText(INT idx, LPCTSTR value)
{
	if (m_stmt == NULL)
		return FALSE;
	if (value == NULL)
		return sqlite3_bind_null(m_stmt, idx) == SQLITE_OK;
	return sqlite3_bind_text16(m_stmt, idx, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
}
BOOL SQLiteStmt::BindDate(INT idx, const SYSTEMTIME& value)
{
	if (value.wYear == 0)
		return BindNull(idx);
	return BindText(idx, SQLiteDB::FormatDate(value).c_str());
}
BOOL SQLiteStmt::BindNull(INT idx)
{
	return m_stmt != NULL && sqlite3_bind_null(m_stmt, idx) == SQLITE_OK;
}

BOOL SQLiteStmt::Step()
{
	m_bEOF = TRUE;
	if (m_stmt == NULL)
		return FALSE;
	INT rc = sqlite3_step(m_stmt);
	if (rc == SQLITE_ROW)
	{
		m_bEOF = FALSE;
		return TRUE;
	}
	if (rc == SQLITE_DONE)
		return TRUE;
	std::wstring ctx(_T("Step '"));
	const char* sql8 = sqlite3_sql(m_stmt);
	if (sql8 != NULL)
	{
		INT n = MultiByteToWideChar(CP_UTF8, 0, sql8, -1, NULL, 0);
		std::vector<WCHAR> w(n > 0 ? n : 1);
		MultiByteToWideChar(CP_UTF8, 0, sql8, -1, &w[0], n);
		ctx += &w[0];
	}
	ctx += _T("'");
	m_pDB->SetError(ctx.c_str());
	return FALSE;
}

INT SQLiteStmt::Exec()
{
	if (!Step())
		return -1;
	return sqlite3_changes(m_pDB->m_db);
}

void SQLiteStmt::Reset()
{
	if (m_stmt != NULL)
	{
		sqlite3_reset(m_stmt);
		sqlite3_clear_bindings(m_stmt);
	}
	m_bEOF = TRUE;
}

INT SQLiteStmt::GetINTFieldValue(INT col)
{
	return m_stmt != NULL ? sqlite3_column_int(m_stmt, col) : 0;
}
UINT SQLiteStmt::GetUINTFieldValue(INT col)
{
	return m_stmt != NULL ? (UINT)sqlite3_column_int64(m_stmt, col) : 0;
}
SHORT SQLiteStmt::GetSHORTFieldValue(INT col)
{
	return (SHORT)GetINTFieldValue(col);
}
__int64 SQLiteStmt::GetINT64FieldValue(INT col)
{
	return m_stmt != NULL ? sqlite3_column_int64(m_stmt, col) : 0;
}
DOUBLE SQLiteStmt::GetDOUBLEFieldValue(INT col)
{
	return m_stmt != NULL ? sqlite3_column_double(m_stmt, col) : 0.0;
}
BOOL SQLiteStmt::GetStringFieldValue(INT col, std::wstring& value)
{
	value.clear();
	if (m_stmt == NULL)
		return FALSE;
	const void* p = sqlite3_column_text16(m_stmt, col);
	if (p != NULL)
		value.assign((const wchar_t*)p, sqlite3_column_bytes16(m_stmt, col) / sizeof(wchar_t));
	return TRUE;
}
SYSTEMTIME SQLiteStmt::GetSYSTEMTIMEFieldValue(INT col)
{
	SYSTEMTIME st;
	memset(&st, 0, sizeof(st));
	if (m_stmt != NULL && sqlite3_column_type(m_stmt, col) != SQLITE_NULL)
		SQLiteDB::ParseDate(sqlite3_column_text(m_stmt, col), st);
	return st;
}
BOOL SQLiteStmt::IsFieldNull(INT col)
{
	return m_stmt == NULL || sqlite3_column_type(m_stmt, col) == SQLITE_NULL;
}
