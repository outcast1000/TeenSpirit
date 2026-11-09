#ifndef PrgAPI_h_
#define PrgAPI_h_

#include "MessageViewer.h"
#include "TeenSpiritEnums.h"
#include "InfoItemTypesEnum.h"
#include "MediaPlayer.h"
#include "RecordCollectionSorter.h"
#include "TranslationManager.h"
#include "SkinManager.h"
#include "AppOptions.h"
#include "AppResources.h"

class AppSettings;
class TeenSpiritPlayer;
class ActionManager;
class FileTagger;
class ServicesRegistrationSrv;
class FileTagger;
class InfoDownloadManager;
class MenuManager;
class SQLManager;
class CollectionManager;
class InfoProviderFactory;
class FlexButton;
class LocalPictureManager;
class HotKeyManager;
class LastFMServices;
class ITracker;
class ShortcutManager;
class AppMonitor;
class ConfigurationManager;
class StateManager;
class ITSMenu;
class TrayToolTipDlg;
class CMiniPlayerDlg;
class QuickSearchDlg;
class CSearchDlg;



enum APP_TASKS
{
	TSK_First,
	TSK_InformationDownload,
	TSK_CollectionUpdate,
	TSK_Last
};



#define PRGAPI PrgAPI::Instance

class PrgAPI
{
#ifdef _UNITTESTING
public:
	static BOOL UnitTest();
#endif
public:
	static PrgAPI* Instance();
	static void Shutdown();
	~PrgAPI();

	BOOL Init();

	void SaveSettings();
public:
	static void FormatDuration(LPTSTR bf, UINT bfLen, INT duration);
	static void FormatDate(TCHAR* bf, size_t bfLen, SYSTEMTIME& st, BOOL bSmart = TRUE);

	//Monitors the following async parameters of the application
	//1. Tracks The History
	void Monitor();
	BOOL m_bDownloading;

private:
	PrgAPI();
	//Cancels network calls and stops the background job queues. FALSE if a job
	//thread is still running (it still uses the services owned by PrgAPI).
	BOOL StopBackgroundWork();
	static PrgAPI* m_pInst;
	MessageViewer* m_pNotificationTarget;

	//--------------------------------------------------------
	//--------------------------------------------APP NOTIFICATIONS
	//--------------------------------------------------------
public:
	void SetNotificationTarget(MessageViewer* pNotificationTarget)	{m_pNotificationTarget = pNotificationTarget;}

	void NotifyUser(LPCTSTR msg, LPCTSTR title = NULL, SEVERITY sev = SEV_InfoL1);
	void NotifyUser(INT msgResID, INT titleResID = NULL, SEVERITY sev = SEV_InfoL1);

	int MessageBox(HWND parent, LPCTSTR message, LPCTSTR caption = NULL, DWORD style = MB_OK | MB_ICONEXCLAMATION);
	int MessageBox(HWND parent, INT messageID, INT captionID = NULL, DWORD style = MB_OK | MB_ICONEXCLAMATION);

private:
	//--------------------------------------------------------
	//--------------------------------------------------------
	//--------------------------------------------------------

	//--------------------------------------------------------
	//-------------------------------RESOURCE HANDLING/SHARING - Strings/Fonts
	//--------------------------------------------------------
public:
	//---Strings
	LPCTSTR GetString(INT resID);
	//void GetString(INT resID, LPTSTR bf, UINT bfLen);
	LPCTSTR GetStringForItemType(ItemTypeEnum it);
	LPCTSTR GetStringForInfoItemType(InfoItemTypeEnum iit);
	LPCTSTR GetStringForCollectionType(CollectionTypesEnum ct);
	LPCTSTR GetStringForRandomMode(RandomTrackModeEnum rtm);
	LPCTSTR GetStringForTrackTextType(TrackTextType ttt);
	LPCTSTR GetStringForPlayListMode(MediaPlayerNextMode mpm);
	LPCTSTR GetStringForTrackType(TrackTypesEnum tt);
	LPCTSTR GetStringForStars(UINT Stars);
	LPCTSTR GetStringForCaseStyle(CaseStyleEnum cStyle);
	LPCTSTR GetStringForRecordSortOption(RecordSortOptionsEnum rso);
	LPCTSTR GetStringForSectionFilter(SectionFiltersEnum sf);
	LPCTSTR GetStringForSectionType(SectionTypeEnum st);

	void GetStringForSectionMode(SectionModesEnum sm, LPTSTR bf, UINT bfLen);



	//---Fonts & Icons (owned by AppResources)
	AppResources& GetAppResources();
	CFont* GetFont(APP_FONTS fon)					{return GetAppResources().GetFont(fon);}
	HICON GetIcon(APP_ICONS icon)					{return GetAppResources().GetIcon(icon);}
	HICON GetIconForTrackType(TrackTypesEnum tt)	{return GetIcon(AppResources::IconForTrackType(tt));}
	HICON GetIconForRank(INT rank)					{return GetIcon(AppResources::IconForRank(rank));}
	APP_ICONS GetAppIconForRank(INT rank)			{return AppResources::IconForRank(rank);}
	HICON GetIconForStars(INT stars)				{return GetIcon(AppResources::IconForStars(stars));}
	HICON GetIconForCollectionType(CollectionTypesEnum colType)		{return GetIcon(AppResources::IconForCollectionType(colType));}
	APP_ICONS GetAppIconForCollectionType(CollectionTypesEnum colType)	{return AppResources::IconForCollectionType(colType);}
	HICON GetIconForInfoItemType(InfoItemTypeEnum iit)	{return GetIcon(AppResources::IconForInfoItemType(iit));}
	//---ImageLists
	CImageList& GetImageList16()					{return GetAppResources().GetImageList16();}
	//---Options
	void SetGlobalButtonStyle(FlexButton& btn, APP_ICONS ico = ICO_Last);

private:
	AppResources* m_pAppResources;
	//struct ColorCtrlRegistration
	//{
	//	ColorCtrlRegistration(LPCTSTR confname, ICtrlColors* pCtrl):
	//			ConfigName(confname),
	//			pControl(pCtrl){}
	//	LPCTSTR ConfigName;
	//	ICtrlColors* pControl;
	//};
	//void LoadColors(ColorCtrlRegistration* pCtrlReg);
	//std::vector<IControl*> m_pControl;
	//std::vector<ColorCtrlRegistration> m_pColorsCtrls;

	//--------------------------------------------------------
	//--------------------------------------------------------
	//--------------------------------------------------------

	//--------------------------------------------------------
	//------------------------------------------APPLICATION OBJECTS
	//--------------------------------------------------------
public:
	HINTERNET GetInternetHandle();
	AppSettings* GetAppSettings();	//TO BE REMOVED
	SkinManager* GetSkinManager();
	ActionManager* GetActionManager();
	LocalPictureManager* GetLocalPictureManager();
	SQLManager* GetSQLManager();
	BOOL InfoDownloadManagerExists()						{return m_pInfoDownloadManager != NULL;}
	InfoDownloadManager* GetInfoDownloadManager();
	BOOL CollectionManagerExists()							{return m_pCollectionManager != NULL;}
	CollectionManager* GetCollectionManager();
	ServicesRegistrationSrv* GetServicesRegistrationSrv();
	MediaPlayer* GetMediaPlayer();
	FileTagger* GetFileTagger();
	MenuManager* GetMenuManager();
	TranslationManager* GetTranslationManager();
	InfoProviderFactory* GetInfoProviderFactory();
	HotKeyManager* GetHotKeyManager();
	LastFMServices* GetLastFMServices();
	ITracker* GetTracker();
	ShortcutManager* GetShortcutManager();
	ConfigurationManager* GetConfigurationManager();
	StateManager* GetStateManager();
	
	TrayToolTipDlg* GetTrayToolTipDlg(); 
	void DestroyTrayToolTipDlg();
	CMiniPlayerDlg* GetMiniPlayerDlg(BOOL bForceCreate); 
	void DestroyMiniPlayerDlg();
	QuickSearchDlg* GetQuickSearchDlg(); 
	void DestroyQuickSearchDlg();
	CSearchDlg* GetAdvancedSearchDlg();
	void DestroyAdvancedSearchDlg();






	//ITrackViewer* GetTrackViewer()								{ASSERT(m_pTrackViewer); return m_pTrackViewer;}
	//void RegisterTrackViewer(ITrackViewer* pTrackViewer)		{m_pTrackViewer = pTrackViewer;}
	//ISectionViewer* GetSectionViewer()							{ASSERT(m_pSectionViewer); return m_pSectionViewer;}
	//void RegisterSectionViewer(ISectionViewer* pSectionViewer)	{m_pSectionViewer = pSectionViewer;}


private:
	TranslationManager* m_pTranslationManager;
	SkinManager* m_pSkinManager;
	AppSettings* m_pAppSettings;
	LocalPictureManager* m_pLocalPictureManager;
	InfoDownloadManager* m_pInfoDownloadManager;
	TeenSpiritPlayer* m_pMediaPlayer;
	ActionManager* m_pActionManager;
	ServicesRegistrationSrv* m_pServicesRegistrationSrv;
	FileTagger* m_pFileTagger;
	AppMonitor* m_pAppMonitor;
	MenuManager* m_pMenuManager;
	SkinManager* m_pSkinManger;
	SQLManager* m_pSQLManager;
	CollectionManager* m_pCollectionManager;
	InfoProviderFactory* m_pInfoProviderFactory;

	//ITrackViewer* m_pTrackViewer;
	//ISectionViewer* m_pSectionViewer;
	HINTERNET m_hInternet;

	HotKeyManager* m_pHotKeyManager;
	LastFMServices* m_pLastFMServices;
	ITracker* m_pTracker;
	ShortcutManager* m_pShortcutManager;
	ConfigurationManager* m_pConfigurationManager;
	StateManager* m_pStateManager;
	TrayToolTipDlg* m_pTrayToolTipDlg; 
	CMiniPlayerDlg* m_pMiniPlayerDlg;
	QuickSearchDlg* m_pQuickSearchDlg;
	CSearchDlg* m_pSearchDlg;





	//--------------------------------------------------------
	//--------------------------------------------------------
	//--------------------------------------------------------

	//--------------------------------------------------------
	//------------------------------------------------OPTIONS
	//--------------------------------------------------------
public:
	AppOptions& GetAppOptions();
	INT GetOption(AppOptionsEnum opt)							{return GetAppOptions().Get(opt);}
	void SetOption(AppOptionsEnum opt, INT val)					{GetAppOptions().Set(opt, val);}
	BOOL GetStringOption(AppStringOptionsEnum opt, LPTSTR bf, UINT bfLen)	{return GetAppOptions().GetString(opt, bf, bfLen);}
	BOOL SetStringOption(AppStringOptionsEnum opt, LPCTSTR value)			{return GetAppOptions().SetString(opt, value);}

private:
	AppOptions* m_pAppOptions;
	//Copies the player state into the options and writes them to the settings.
	void WriteOptions();

	//Returns the directory where the exe resides. (Including the last '\')
	//DWORD GetMacAddressHash();
	//--------------------------------------------------------
	//--------------------------------------------------------
	//--------------------------------------------------------

public:
	BOOL IsTrackNew(SYSTEMTIME& dateAdded, UINT DaysMeansNew = 0);

	enum SiteActionEnum
	{
		SA_Help,
		SA_Navigate,
		SA_Last
	};
	BOOL GetSiteURL(LPTSTR url, UINT urlLen, SiteActionEnum actID, LPCTSTR p1 = NULL, LPCTSTR p2 = NULL);
	BOOL GoToSiteURL(SiteActionEnum actID, LPCTSTR p1 = NULL, LPCTSTR p2 = NULL);
	AppMonitor* GetAppMonitor();

	void SendMessage(UINT stateMessage);
	void PostMessage(UINT stateMessage);


	ITSMenu* CreatePopupMenu();



//public:
//	void SetBaseFontSize(UINT size)			{m_baseFontSize = size;}
//	void SetBaseFontName(LPCTSTR fontName)	{ASSERT(fontName != NULL); m_baseFontName = fontName;}
//	LPCTSTR GetBaseFontName()	{return m_baseFontName.c_str();}
//	UINT GetBaseFontSize()		{return m_baseFontSize;}
//
	//void IncreaseRelativeFontSize();
	//void DecreaseRelativeFontSize();
	//void ResetRelativeFontSize();
//
//private:
//	UINT m_baseFontSize;
//	std::tstring m_baseFontName;

private:
	enum PathsEnum
	{
		PATH_First,
		
		PATH_AppRoot,
		PATH_StorageRoot,
		PATH_SkinRoot,
		PATH_TranslationsRoot,
		PATH_WebServicesRoot,
		PATH_UserRoot,
		PATH_DatabaseRoot,

		PATH_LastM3UFile,
		PATH_ServicesXMLFile,
		PATH_ServicesUserXMLFile,
		
		PATH_Last
	};
	LPCTSTR GetPath(PathsEnum pathType, LPTSTR buffer = NULL);
	TCHAR m_tempPath[MAX_PATH];
};


//App Functions
UINT Stars2Rating(FLOAT Stars);
FLOAT Rating2Stars(UINT Rating);
BOOL CALLBACK SetFontToChilds(HWND hwnd, LPARAM lParam);


#endif