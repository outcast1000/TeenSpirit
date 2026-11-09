#ifndef _ObjFactory_h_
#define _ObjFactory_h_

class TeenSpiritPlayer;
class FileTagger;
class AppSettings;

class ObjFactory
{
public:
	static ObjFactory* Instance();
	~ObjFactory();

	//TeenSpiritPlayer* ObjFactory::CreateMediaPlayer(HWND hWndParent, LPCTSTR visInfo =NULL);
	//TeenSpiritPlayer* CreateMediaPlayer(LPCTSTR visInfo = NULL);
	AppSettings* CreateAppSettings(LPCTSTR conString = NULL);
	FileTagger* CreateFileTagger();

private:
	ObjFactory(void);
private:
	static ObjFactory* g_Inst;
};

#endif