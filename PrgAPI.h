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



enum APP_FONTS
{
	FONT_Dialogs,
	FONT_DialogsBold,
	FONT_DialogsUnderline,
	FONT_DialogsItalics,
	FONT_DialogsBig,
	FONT_DialogsBigBold,
	FONT_Fixed,
	FONT_Last
};

enum APP_TASKS
{
	TSK_First,
	TSK_InformationDownload,
	TSK_CollectionUpdate,
	TSK_Last
};

enum APP_ICONS
{
	ICO_Main,

	ICO_Info,
	ICO_Warning,
	ICO_Error,
	ICO_Prev24,
	ICO_Play24,
	ICO_Pause24,
	ICO_Stop24,
	ICO_Next24,
	ICO_Crossfade16,
	ICO_CrossfadeAc16,
	ICO_Equalizer16,
	ICO_EqualizerAc16,
	ICO_Back16,
	ICO_Forward16,
	ICO_Home16,
	ICO_Refresh16,
	ICO_Internet16,
	ICO_Add16,
	ICO_Artist16,
	ICO_2Stars16,
	ICO_3Stars16,
	ICO_Top1016,
	ICO_Top10016,
	ICO_TopRest16,
	ICO_Played16,
	ICO_MouseClick16,
	ICO_Audio16,
	ICO_Video16,
	ICO_Virtual16,
	ICO_Comment16,
	ICO_Lyrics16,
	ICO_Personal16,
	ICO_Tablature16,

	ICO_StarEmpty16,
	ICO_StarGold16,
	ICO_StarGoldHalf16,
	ICO_StarEmptyBad16,
	ICO_StarBad16,
	ICO_Minus16,
	ICO_New16,
	ICO_Collapsed16,
	ICO_Expanded16,
	ICO_LocalFolder16,
	ICO_NetFolder16,
	ICO_Media16,
	ICO_AudioCD16,
	ICO_Removable16,

	ICO_Play16,
	ICO_Copy16,
	ICO_Delete16,
	ICO_Find16,
	ICO_Properties16,
	ICO_Providers16,
	ICO_Buy16,
	ICO_Options16,
	ICO_Window16,
	ICO_Clear16,

	ICO_Text16,
	ICO_Picture16,

	ICO_Up16,
	ICO_Down16,

	ICO_Collections16,
	ICO_History16,
	ICO_Games16,
	ICO_Main16,

	ICO_SwitchPane16,
	
	ICO_Minimize16,
	ICO_Restore16,
	ICO_Maximize16,
	ICO_Close16,

	ICO_IncreaseFont16,
	ICO_DecreaseFont16,
	ICO_ResetFont16,

	ICO_Album16,
	ICO_Insert16,
	ICO_Remove16,

	ICO_Mute16,
	ICO_Nomute16,

	ICO_Last
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



	//---Fonts
	CFont* GetFont(APP_FONTS fon);
	//---Colors
	//void RegisterColorControl(ICtrlColors* pCtrl, LPCTSTR ctrlName);
	//void UnRegisterColorControl(ICtrlColors* pCtrl);
	//void RefreshColorControls();
	//BOOL SaveSkin(LPCTSTR dirPath);
	//BOOL GetSkinColor(LPCTSTR section, LPCTSTR clrName, COLORREF& clr);
	//---Icons
	HICON GetIcon(APP_ICONS icon);
	HICON GetIconForTrackType(TrackTypesEnum tt);
	HICON GetIconForRank(INT rank);
	APP_ICONS GetAppIconForRank(INT rank);
	HICON GetIconForStars(INT stars);
	HICON GetIconForCollectionType(CollectionTypesEnum colType);
	APP_ICONS GetAppIconForCollectionType(CollectionTypesEnum colType);
	HICON GetIconForInfoItemType(InfoItemTypeEnum iit);
	//---ImageLists
	CImageList& GetImageList16();
	//---Options
	void SetGlobalButtonStyle(FlexButton& btn, APP_ICONS ico = ICO_Last);

private:
	CImageList m_il16;
	//struct ColorCtrlRegistration
	//{
	//	ColorCtrlRegistration(LPCTSTR confname, ICtrlColors* pCtrl):
	//			ConfigName(confname),
	//			pControl(pCtrl){}
	//	LPCTSTR ConfigName;
	//	ICtrlColors* pControl;
	//};
	//void LoadColors(ColorCtrlRegistration* pCtrlReg);
	CFont* m_pAppFonts[FONT_Last];
	HICON m_AppIcons[ICO_Last];
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
	void ClearFonts();
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