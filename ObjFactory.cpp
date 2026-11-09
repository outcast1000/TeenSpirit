#include "StdAfx.h"
#include "objfactory.h"
#include "AppSettings/IniAppSettings.h"
#include "ActionManager.h"
#include "ServicesRegistrationSrv.h"

//Media Players Include



ObjFactory* ObjFactory::g_Inst = 0;

ObjFactory::ObjFactory(void)
{
}

LPCTSTR const cPlayerSettings = _T("PLAYER");

ObjFactory::~ObjFactory(void)
{
	TRACEST(_T("ObjFactory::~ObjFactory"));
}

ObjFactory* ObjFactory::Instance()
{
	if (g_Inst == 0)
		g_Inst = new ObjFactory;
	return g_Inst;
}

#include "AppSettings/IniAppSettings.h"
AppSettings* ObjFactory::CreateAppSettings(LPCTSTR conString)
{
	if (conString != NULL && !PathFileExists(conString))
	{
		TCHAR bf[MAX_PATH];
		_tcsncpy(bf, conString, MAX_PATH);
		TCHAR* ch = _tcsrchr(bf, '\\');
		if (ch != NULL)
			*ch = 0;
		SHCreateDirectoryEx(NULL, bf, NULL);
	}
	AppSettings* pSet = new IniAppSettings();
	pSet->Load(conString);
	return pSet;
}


#include "EnhTagLibTagger.h"

FileTagger* ObjFactory::CreateFileTagger()
{
	return new EnhTagLibTagger();
}
