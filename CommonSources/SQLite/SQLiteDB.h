//	Thin SQLite wrapper used by the database layer (replaces the ADO/Jet wrapper).
//
//	- SQLiteDB:   a connection. Opened in serialized mode (safe to share between
//	              threads at the API level), WAL journal, busy timeout.
//	              Registers Jet-compatible text semantics:
//	                * collation NOCASE_U: Unicode ordinal, case-insensitive
//	                  (CompareStringOrdinal). Used by every text column.
//	                * like(): case-insensitive for all scripts, not only ASCII.
//	- SQLiteStmt: a prepared statement. Column getters mirror the old
//	              ADORecordset names so the record readers stay unchanged.
//
//	Dates are stored as ISO-8601 text "YYYY-MM-DD HH:MM:SS" (local time, as Jet did).
//	A zeroed SYSTEMTIME is stored as NULL and NULL reads back as a zeroed SYSTEMTIME.
#ifndef _SQLiteDB_h_
#define _SQLiteDB_h_

#include <string>

struct sqlite3;
struct sqlite3_stmt;

class SQLiteDB
{
public:
	SQLiteDB();
	~SQLiteDB();

	BOOL Open(LPCTSTR path);
	void Close();
	BOOL IsOpen() const							{return m_db != NULL;}

	//Executes one or more statements without results.
	//Returns the number of rows changed by the last statement, or -1 on error.
	INT Execute(LPCTSTR sql);

	BOOL BeginTransaction();
	BOOL CommitTransaction();
	BOOL RollbackTransaction();

	INT GetUserVersion();
	BOOL SetUserVersion(INT version);

	LPCTSTR GetLastError() const				{return m_lastError.c_str();}
	sqlite3* GetHandle()						{return m_db;}

	//Formatting helpers for the date representation used in the database.
	static std::wstring FormatDate(const SYSTEMTIME& st);
	static BOOL ParseDate(const unsigned char* text, SYSTEMTIME& st);

private:
	friend class SQLiteStmt;
	void SetError(LPCTSTR context);
	sqlite3* m_db;
	std::wstring m_lastError;

	SQLiteDB(const SQLiteDB&);
	SQLiteDB& operator=(const SQLiteDB&);
};

class SQLiteStmt
{
public:
	explicit SQLiteStmt(SQLiteDB* pDB);
	~SQLiteStmt();

	//Compiles sql. Any previous statement is finalized.
	BOOL Prepare(LPCTSTR sql);
	//Prepare + Step: convenience for queries without parameters.
	BOOL Open(LPCTSTR sql)						{return Prepare(sql) && Step();}

	//Parameters are 1-based.
	BOOL BindInt(INT idx, INT value);
	BOOL BindUInt(INT idx, UINT value);
	BOOL BindInt64(INT idx, __int64 value);
	BOOL BindDouble(INT idx, DOUBLE value);
	BOOL BindText(INT idx, LPCTSTR value);		//NULL binds SQL NULL
	BOOL BindDate(INT idx, const SYSTEMTIME& value);
	BOOL BindNull(INT idx);

	//Executes (or advances) the statement. FALSE on error.
	//After a successful Step, IsEOF() tells whether a row is available.
	BOOL Step();
	//Executes a statement that returns no rows. Returns rows changed or -1 on error.
	INT Exec();
	//Clears bindings and rewinds so the statement can be executed again.
	void Reset();

	BOOL IsEOF() const							{return m_bEOF;}
	void MoveNext()								{Step();}

	//Columns are 0-based. NULL reads as 0 / empty / zeroed SYSTEMTIME.
	INT GetINTFieldValue(INT col);
	UINT GetUINTFieldValue(INT col);
	SHORT GetSHORTFieldValue(INT col);
	__int64 GetINT64FieldValue(INT col);
	DOUBLE GetDOUBLEFieldValue(INT col);
	BOOL GetStringFieldValue(INT col, std::wstring& value);
	SYSTEMTIME GetSYSTEMTIMEFieldValue(INT col);
	BOOL IsFieldNull(INT col);

private:
	SQLiteDB* m_pDB;
	sqlite3_stmt* m_stmt;
	BOOL m_bEOF;

	SQLiteStmt(const SQLiteStmt&);
	SQLiteStmt& operator=(const SQLiteStmt&);
};

#endif
