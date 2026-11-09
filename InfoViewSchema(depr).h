#ifndef _InfoViewSchema_h_
#define _InfoViewSchema_h_

#include "ViewSchema.h"
#include "InfoTableSchema.h"


class InfoViewSchema:public ViewSchema
{
public:

	InfoViewSchema()								{}
	static void GetSQL(TCHAR* sql, int len, 
		UINT InfoID);
	static void GetSQLJoined(TCHAR* sql, int len, 
		ITEM_INFO_TYPE iit, UINT itemID);

	//Artists Main Table
	virtual void GetInfo(CString& info);
	virtual INT GetType();
	virtual void GetDateAdded(SYSTEMTIME& t);
	virtual INT GetInfoID();
};

#endif