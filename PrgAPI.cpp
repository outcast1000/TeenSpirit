#include "stdafx.h"
#include "PrgAPI.h"

#include "TeenSpirit.h"
#include "ObjFactory.h"
#include "AppSettings/AppSettings.h"
#include "LocalPictureManager.h"
#include "InfoDownloadManager.h"
#include "ActionManager.h"
#include "ServicesRegistrationSrv.h"
#include "TeenSpiritPlayer.h"
#include "FileTagger.h"
#include "ResourceUtils.h"
#include "SkinManager.h"
#include "MenuManager.h"
#include "SQLManager.h"
#include "CollectionManager.h"
#include "FileVersion.h"
#include "FlexButton.h"
#include "InformationProviders/InfoProviderFactory.h"
#include "MessageDlg.h"
#include "InfoCtrl.h"
#include "HotKeyManager.h"
#include "ShortcutManager.h"
#include "ConfigurationManager.h"
#include "LastFMServices.h"
#include "Trackers/MultiTracker.h"
#include "UserProgressDlg.h"
#include "VersionNo.h"
#include "MediaPlayerUtilities.h"
#include "ApplicationMonitor.h"
#include "AppMonitor.h"
#include "Language/LanguageManager.h"
#include "ExceptionHandler.h"
#include "BassInstance.h"//Is needed for initial bass configuration (Init)
#include "TranslationManager.h"
#include "StateManager.h"
#include "MainTreeCtrl.h"
#include "TSMenu.h"
#include "TrayToolTipDlg.h"
#include "MiniPlayerDlg.h"
#include "QuickSearchDlg.h"
#include "SearchDlg.h"
#include "AppOptions.h"
#include "AppStrings.h"
#include "AppResources.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



//	DatabasePath=L:\TS-storage\db\
//	StoragePath=L:\TS-storage\pix\


UINT cDefaultBaseFontSize = 14;
LPCTSTR cDefaultBaseFontName = _T("Tahoma");

LPCTSTR const cGeneral =			_T("General");
LPCTSTR const cPlayerSettings =		_T("Player");
LPCTSTR const cInfoControlSettings =_T("InfoControl");
LPCTSTR const cPaneSettings	=		_T("Panes");
LPCTSTR const cViewerPane =			_T("ViewerPane");
LPCTSTR const cTree	=				_T("Tree");
LPCTSTR const cTrackList =			_T("TrackList");
LPCTSTR const cPlayList =			_T("PlayList");


LPCTSTR const cLastFM =				_T("LastFM");

//LPCTSTR const SettingsCategory = _T("Main");
LPCTSTR const cOptionLanguage =		_T("Language");
LPCTSTR const cOptionSkin =			_T("Skin");

LPCTSTR const cOptionUser =			_T("User");
LPCTSTR const cOptionPass =			_T("Pass");


PrgAPI* PrgAPI::m_pInst = NULL;

PrgAPI* PrgAPI::Instance()
{
	if (m_pInst == NULL)
	{
		m_pInst = new PrgAPI;
		m_pInst->Init();
	}
	return m_pInst;
}
void PrgAPI::Shutdown()
{
	if (m_pInst == NULL)
		return;
	if (!m_pInst->StopBackgroundWork())
	{
		//=== A job thread is still running and uses the services (and PRGAPI()).
		//=== Destroying them would be a use-after-free; the process is exiting,
		//=== so save the settings and leave everything alive instead.
		TRACE(_T("@0 PrgAPI::Shutdown. Background work still running; skipping teardown\r\n"));
		m_pInst->GetAppMonitor()->StopMonitoring();
		m_pInst->SaveSettings();
		if (m_pInst->m_pAppSettings != NULL)
			m_pInst->m_pAppSettings->Save();
		return;
	}
	//=== m_pInst stays valid while the destructor runs: the services call PRGAPI()
	//=== while being destroyed (e.g. TSStateChangeListener unregistering).
	delete m_pInst;
	m_pInst = NULL;
}

BOOL PrgAPI::StopBackgroundWork()
{
	//=== Closing the shared handle makes in-flight WinInet calls return at once.
	if (m_hInternet != 0)
	{
		InternetCloseHandle(m_hInternet);
		m_hInternet = NULL;
	}
	BOOL bStopped = TRUE;
	if (m_pCollectionManager != NULL && !m_pCollectionManager->StopBackgroundWork())
		bStopped = FALSE;
	if (m_pInfoDownloadManager != NULL && !m_pInfoDownloadManager->StopBackgroundWork())
		bStopped = FALSE;
	if (m_pLastFMServices != NULL && !m_pLastFMServices->StopWorker(30000))
		bStopped = FALSE;
	return bStopped;
}

void PrgAPI::SaveSettings()
{
	TRACEST(_T("PrgAPI::SaveSettings."));
	AppSettings* pAppSettings = GetAppSettings();

	TCHAR monitor[500];
	GetAppMonitor()->Save(monitor, 500);
	SetStringOption(SOPT_GEN_Monitoring, monitor);

	TranslationManager* pLM = GetTranslationManager();
	pAppSettings->Write(cGeneral, cOptionLanguage, pLM->GetTranslationPath());
	SkinManager* pSM = GetSkinManager();
	pAppSettings->Write(cGeneral, cOptionSkin, pSM->GetSkin().GetSkinPath());

	SetOption(OPT_GEN_RelativeFontSize, pSM->GetSkin().GetRelativeFontSize());

	if (m_pInfoProviderFactory != NULL)
		m_pInfoProviderFactory->SaveSettings();
	if (m_pHotKeyManager != NULL)
		m_pHotKeyManager->SaveSettings();
	if (m_pTrayToolTipDlg != NULL)
	{
		//=== Save Settings
	}
	if (m_pMiniPlayerDlg != NULL)
	{
		//=== Save Settings
	}
	if (m_pQuickSearchDlg != NULL)
	{
		//=== Save Settings
	}
	if (m_pSearchDlg != NULL)
	{
		//=== Save Settings
	}

	if (m_pLastFMServices != NULL)
	{
		pAppSettings->Write(cLastFM, cOptionUser, m_pLastFMServices->GetUserName());
		pAppSettings->Write(cLastFM, cOptionPass, m_pLastFMServices->GetMD5Password());
	}
	if (m_pLocalPictureManager)
		m_pLocalPictureManager->SaveState(GetPath(PATH_UserRoot));
	WriteOptions();
	pAppSettings->Save();
}

#ifdef _UNITTESTING
BOOL PrgAPI::UnitTest()
{
	for (int i = 0; i < 250; i++)
	{
		FLOAT stars = Rating2Stars(i);
		UNITTEST(stars <= 6.0);
		if (i < 251)
			UNITTEST(Stars2Rating(stars) == i);
	}
	return TRUE;
}
#endif


PrgAPI::PrgAPI():
m_pNotificationTarget(NULL),
m_pAppSettings(NULL),
m_pLocalPictureManager(NULL),
m_pInfoDownloadManager(NULL),
m_pMediaPlayer(NULL),
m_pInfoProviderFactory(NULL),
m_hInternet(0),
m_pActionManager(NULL),
m_pServicesRegistrationSrv(NULL),
m_pFileTagger(NULL),
m_pSkinManager(NULL),
m_bDownloading(FALSE),
m_pMenuManager(NULL),
m_pTranslationManager(NULL),
m_pSQLManager(NULL),
//m_pTrackViewer(NULL),
//m_pSectionViewer(NULL),
m_pCollectionManager(NULL),
m_pHotKeyManager(NULL),
m_pTrayToolTipDlg(NULL),
m_pMiniPlayerDlg(NULL),
m_pQuickSearchDlg(NULL),
m_pSearchDlg(NULL),
m_pLastFMServices(NULL),
m_pTracker(NULL),
m_pShortcutManager(NULL),
m_pConfigurationManager(NULL),
m_pStateManager(NULL),
m_pAppMonitor(NULL),
m_pAppOptions(NULL),
m_pAppResources(NULL)
{
}

PrgAPI::~PrgAPI()
{
	//=== YOU SHOULD NOT DESTROY A WINDOW HERE. 
	//		AN ERROR MAY OCCUR AS THE MFC DOES NOT LIKE TO DESTROY WINDOWS AFTER IT HAS DESTROYED 
	//		HIS MANIN WND. For example... if you try to destroy TrayToolTipDlg here
	//		a premature exit may occur before reaching this point
	TRACEST(_T("PrgAPI::~PrgAPI"));
	GetAppMonitor()->StopMonitoring();
	SaveSettings();
	if (m_hInternet != 0)
	{
		InternetCloseHandle(m_hInternet);
		m_hInternet = NULL;
	}
	delete m_pSkinManager;
	m_pSkinManager = NULL;
	delete m_pHotKeyManager;
	m_pHotKeyManager = NULL;
	DestroyTrayToolTipDlg();
	DestroyMiniPlayerDlg();
	DestroyQuickSearchDlg();
	DestroyAdvancedSearchDlg();

	//TRACE(_T("@2*delete* m_pShortcutManager\r\n"));
	delete m_pShortcutManager;
	m_pShortcutManager = NULL;
	delete m_pConfigurationManager;
	m_pConfigurationManager = NULL;
	//TRACE(_T("@2*delete* m_pLastFMServices\r\n"));
	delete m_pLastFMServices;
	m_pLastFMServices = NULL;
	//TRACE(_T("@2*delete* m_pTracker\r\n"));
	delete m_pTracker;
	m_pTracker = NULL;
	//TRACE(_T("@2*delete* m_pCollectionManager\r\n"));
	delete m_pCollectionManager;
	m_pCollectionManager = NULL;
	//TRACE(_T("@2*delete* m_pServicesRegistrationSrv\r\n"));
	delete m_pServicesRegistrationSrv;
	m_pServicesRegistrationSrv = NULL;
	//TRACE(_T("@2*delete* m_pMediaPlayer\r\n"));
	delete m_pMediaPlayer;
	m_pMediaPlayer = NULL;
	//TRACE(_T("@2*delete* m_pLocalPictureManager\r\n"));
	delete m_pLocalPictureManager;
	m_pLocalPictureManager = NULL;
	//TRACE(_T("@2*delete* m_pInfoDownloadManager\r\n"));
	delete m_pInfoDownloadManager;
	m_pInfoDownloadManager = NULL;
	//TRACE(_T("@2*delete* m_pActionManager\r\n"));
	delete m_pActionManager;
	m_pActionManager = NULL;
	//TRACE(_T("@2*delete* m_pInfoProviderFactory\r\n"));
	delete m_pInfoProviderFactory;
	m_pInfoProviderFactory = NULL;
	//TRACE(_T("@2*delete* m_pFileTagger\r\n"));
	delete m_pFileTagger;
	m_pFileTagger = NULL;
	//TRACE(_T("@2*delete* m_pMenuManager\r\n"));
	delete m_pMenuManager;
	m_pMenuManager = NULL;
	//TRACE(_T("@2*delete* m_pLanguageManager\r\n"));
	delete m_pTranslationManager;
	m_pTranslationManager = NULL;
	//=== Deleted after the services above: they may send state messages or
	//=== unregister listeners while being destroyed, which would otherwise
	//=== lazily re-create a StateManager that is never freed.
	delete m_pStateManager;
	m_pStateManager = NULL;
	//TRACE(_T("@2*delete* ObjFactory\r\n"));
	delete ObjFactory::Instance();
	//TRACE(_T("@2*delete* m_pSQLManager\r\n"));
	delete m_pSQLManager;
	m_pSQLManager = NULL;
	delete m_pAppResources;
	m_pAppResources = NULL;
	//TRACE(_T("@2*delete* m_pAppSettings\r\n"));
	delete m_pAppOptions;
	m_pAppOptions = NULL;
	if (m_pAppSettings != NULL)
	{
		m_pAppSettings->Save();
		delete m_pAppSettings;
		m_pAppSettings = NULL;
	}
	delete m_pAppMonitor;
	m_pAppMonitor = NULL;
}


AppMonitor* PrgAPI::GetAppMonitor()					
{
	if (m_pAppMonitor == NULL)
	{
		m_pAppMonitor = new AppMonitor;
		TCHAR monitor[500];
		if (GetStringOption(SOPT_GEN_Monitoring, monitor, 500))
		{
			VERIFY(m_pAppMonitor->Load(monitor));
		}
		AppMonitor::State& st = m_pAppMonitor->GetState();
		if (st.TotalRuns == 0)
		{
			//=== This is the first time that this appmonitor runs [version >= 96b]
			//=== Check if ver 96 has ever been run
			ApplicationMonitor amJaangle(TRUE);
			amJaangle.InitByName(_T("Jaangle"));
			if (amJaangle.GetCount(ApplicationMonitor::CT_TotalRuns) > 0)
			{
				//=== Version 96 Found. Get the values
				st.TotalRuns = amJaangle.GetCount(ApplicationMonitor::CT_TotalRuns);
				st.TotalCrashes = amJaangle.GetCount(ApplicationMonitor::CT_TotalCrashes);
				amJaangle.GetInstallationDate(&st.InstallationDate);
				st.RunningTimeInMinutes = amJaangle.GetRunningTimeInMinutes();
				st.AppUniqueID = amJaangle.GetAppUniqueID();
			}
			//=== Check if ver < 96 (Teen Spirit) has ever been run
			ApplicationMonitor apTS(TRUE);
			apTS.InitByName(_T("Teen Spirit"));
			if (apTS.GetCount(ApplicationMonitor::CT_TotalRuns) > 0)
			{
				//=== Version Teen Spirit (<96) Found.
				if (st.TotalRuns == 0)
				{
					//=== Jaangle has never been run
					st.TotalRuns = apTS.GetCount(ApplicationMonitor::CT_TotalRuns);
					st.TotalCrashes = apTS.GetCount(ApplicationMonitor::CT_TotalCrashes);
				}
				//===The previous code has a bug and it couldn't take 
				//		the UniqueID & real installation Date. So we will re-grab it here
				apTS.GetInstallationDate(&st.InstallationDate);
				st.RunningTimeInMinutes = apTS.GetRunningTimeInMinutes();
				st.AppUniqueID = apTS.GetAppUniqueID();
			}

		}
		m_pAppMonitor->StartMonitoring();
		m_pAppMonitor->IncreaseAppCount();
		if (m_pAppMonitor->IsAfterCrash())
			m_pAppMonitor->IncreaseCrashCount();
		m_pAppMonitor->Save(monitor, 500);
		SetStringOption(SOPT_GEN_Monitoring, monitor);
	}
	return m_pAppMonitor;
}


BOOL PrgAPI::Init()
{
	TRACEST(_T("PrgAPI::Init"));


	ASSERT(AppOptions::IsTableConsistent());
	GetAppOptions();	//=== Reads the options
	//===Check for Crash
	LPCTSTR files[] = CRASHREPORT_FILES;
	TCHAR crashLogPath[MAX_PATH];
	_sntprintf(crashLogPath, MAX_PATH, _T("%s%s"), GetPath(PATH_AppRoot), files[0]);
	BOOL bCrashLogExists = FALSE;
	if (_taccess(crashLogPath, 0) == 0)
	{
		bCrashLogExists = TRUE;
		TRACE(_T("@1 PrgAPI::Init Crash Log exists meaning that the app has crashed\r\n"));
		//=== Crash Log exists meaning that the app has crashed.
		//=== 1. Delete the file
		DeleteFile(crashLogPath);
		//=== 2. Increase the crash count
		//GetAppMonitor()->IncreaseCrashCount();
		//=== 3. Change the debugging option
		if (GetOption(OPT_GEN_Debugging) == 0)
		{
			if (MessageBox(NULL, _T("Jaangle seems to have crashed. Do you want to enable crash logging?"), NULL, MB_YESNO) == IDYES)
			{
				SetOption(OPT_GEN_Debugging, 1);
				WriteOptions();//=== Save this info (in a case of a second crash)
			}
		}
	}
	if (GetAppMonitor()->IsAfterCrash())
	{
		if (bCrashLogExists)
			TRACE(_T("@1 PrgAPI::Init Abnormal Ending Detected. CrashLog OK\r\n"));
		else
		{
			//ASSERT(0);
			TRACE(_T("@1 PrgAPI::Init Abnormal Ending Detected. CrashLog DOES NOT EXIST\r\n"));
		}
	}
	else
	{
		if (bCrashLogExists)
		{
			ASSERT(0);
			TRACE(_T("@1 PrgAPI::Init CrashLog Found but there was no crash detection\r\n"));
		}
	}
#ifdef ALOG_ACTIVE
	if (GetOption(OPT_GEN_Debugging) == 1)
	{
		TCHAR debugLogPath[MAX_PATH];
		_sntprintf(debugLogPath, MAX_PATH, _T("%sdebug.log"), GetPath(PATH_AppRoot));
		ALog* pLog = ALog::Inst();
		pLog->AddViewer(new PrunnedFileViewer(debugLogPath, 15));
		pLog->SetOption(ALog::LO_MaxAllowedDepth, 5);
		pLog->SetOption(ALog::LO_DefaultDepth, 3);
		pLog->SetOption(ALog::LO_ShowDepth, 1);
		pLog->SetOption(ALog::LO_ShowTimeFromStartup, 1);
		pLog->SetOption(ALog::LO_ShowThreadID, 1);
		pLog->SetOption(ALog::LO_AlwaysCheckForLastError, 1);
	}
#endif

	//=== Initial bass instance initialization
	BassInstance::bUseFloatDSP = GetOption(OPT_PL_UseFloatDSP) != 0;
	BassInstance::bUseSampleFloat = GetOption(OPT_PL_UseSampleFloat) != 0;

	std::tstring value;
	GetAppSettings()->Read(cGeneral, cOptionLanguage, value, _T(""));
	BOOL bLoaded = FALSE;
	TranslationManager* pLM = GetTranslationManager();
	if (value.empty())
	{
		//=== This value does not exist. Load lang ny langID
		pLM->LoadTranslationByLANGID(GetSystemDefaultLangID());
	}
	else if (value[0] == '@')
	{
		//=== Default Language. Do not load anything
	}
	else
	{
		//=== Load the selected translation
		pLM->LoadTranslation(value.c_str());
	}


	FileVersion fv;
	if (GetFileVersion(fv))
	{
		if (fv.Build != GetOption(OPT_GEN_LastVersion))
		{
			TRACE(_T("@2 New Version detected %d->%d\r\n"), GetOption(OPT_GEN_LastVersion), fv.Build);
			AppMonitor::State& state = GetAppMonitor()->GetState();
			state.Runs = 1;
			state.Crashes = 0;
			SetOption(OPT_GEN_LastVersion, fv.Build);
			//Write it here. Otherwise if the program crash It ll reset the creashes for the current version
			GetAppOptions().Write(OPT_GEN_LastVersion);
		}
		else
			TRACE(_T("@2Version: %d\r\n"), fv.Build);

	}

	return TRUE;
}

//to BE removed
AppSettings* PrgAPI::GetAppSettings()
{
	if (m_pAppSettings == NULL)
	{
		TRACEST(_T("PrgAPI::GetAppSettings [CREATE]"));
		m_pAppSettings = ObjFactory::Instance()->CreateAppSettings();
	}
	return m_pAppSettings;
}

AppOptions& PrgAPI::GetAppOptions()
{
	if (m_pAppOptions == NULL)
	{
		TRACEST(_T("PrgAPI::GetAppOptions [CREATE]"));
		m_pAppOptions = new AppOptions(*GetAppSettings());
		m_pAppOptions->Read();
	}
	return *m_pAppOptions;
}

LPCTSTR PrgAPI::GetString(INT resID)
{
	return AppStrings::Get(*GetTranslationManager(), resID);
}
LPCTSTR PrgAPI::GetStringForItemType(ItemTypeEnum it)
{
	return AppStrings::ForItemType(*GetTranslationManager(), it);
}
LPCTSTR PrgAPI::GetStringForInfoItemType(InfoItemTypeEnum iit)
{
	return AppStrings::ForInfoItemType(*GetTranslationManager(), iit);
}
LPCTSTR PrgAPI::GetStringForCollectionType(CollectionTypesEnum ct)
{
	return AppStrings::ForCollectionType(*GetTranslationManager(), ct);
}
LPCTSTR PrgAPI::GetStringForRandomMode(RandomTrackModeEnum rtm)
{
	return AppStrings::ForRandomMode(*GetTranslationManager(), rtm);
}
LPCTSTR PrgAPI::GetStringForTrackTextType(TrackTextType ttt)
{
	return AppStrings::ForTrackTextType(*GetTranslationManager(), ttt);
}
LPCTSTR PrgAPI::GetStringForPlayListMode(MediaPlayerNextMode mpm)
{
	return AppStrings::ForPlayListMode(*GetTranslationManager(), mpm);
}
LPCTSTR PrgAPI::GetStringForTrackType(TrackTypesEnum tt)
{
	return AppStrings::ForTrackType(*GetTranslationManager(), tt);
}
LPCTSTR PrgAPI::GetStringForStars(UINT Stars)
{
	return AppStrings::ForStars(*GetTranslationManager(), Stars);
}
LPCTSTR PrgAPI::GetStringForCaseStyle(CaseStyleEnum cStyle)
{
	return AppStrings::ForCaseStyle(*GetTranslationManager(), cStyle);
}
LPCTSTR PrgAPI::GetStringForRecordSortOption(RecordSortOptionsEnum rso)
{
	return AppStrings::ForRecordSortOption(*GetTranslationManager(), rso);
}
LPCTSTR PrgAPI::GetStringForSectionFilter(SectionFiltersEnum sf)
{
	return AppStrings::ForSectionFilter(*GetTranslationManager(), sf);
}
LPCTSTR PrgAPI::GetStringForSectionType(SectionTypeEnum st)
{
	return AppStrings::ForSectionType(*GetTranslationManager(), st);
}

void PrgAPI::GetStringForSectionMode(SectionModesEnum sm, LPTSTR bf, UINT bfLen)
{
	INT curLen = 0;
	SectionTypeEnum st = CMainTreeCtrl::GetChildForSectionType(sm, ST_Root);
	bf[0] = 0;
	while (st != ST_Unknown)
	{
		LPCTSTR prefix = _T("");
		if (bf[0] != 0)
			prefix = _T(" - ");
		//TRACE(_T("bf: %u(%s)\r\n"), (INT)&bf[0], bf);
		//TRACE(_T("bfLen:%u (%d)\r\n"), (INT)&bfLen, (INT) bfLen);
		//TRACE(_T("curLen:%u(%d)\r\n"), (INT)&curLen, (INT)curLen);
		INT len = _sntprintf(&bf[curLen], bfLen - curLen, _T("%s%s"), prefix, GetStringForSectionType(st));
		curLen += len;
		st = CMainTreeCtrl::GetChildForSectionType(sm, st);
	}
	bf[bfLen - 1] = 0;
}




void PrgAPI::NotifyUser(LPCTSTR msg, 
				LPCTSTR title/* = NULL*/, 
				SEVERITY sev /*= SEV_InfoL1*/)
{
	ASSERT(msg != NULL);
	PRGAPI()->GetActionManager()->LogUserMessage(msg, title, sev);
	if (m_pNotificationTarget != NULL)
		m_pNotificationTarget->Show(msg, title, sev);
}
void PrgAPI::NotifyUser(INT msgResID, 
						INT titleResID/* = NULL*/, 
						SEVERITY sev /*= SEV_InfoL1*/)
{
	NotifyUser(GetString(msgResID), (titleResID > 0) ? GetString(titleResID) : 0, sev);
}


int PrgAPI::MessageBox(HWND parentHWND, LPCTSTR message, LPCTSTR caption, DWORD style)
{
	CMessageDlg dlg;
	dlg.SetIcon(GetIcon(ICO_Main));
	dlg.SetText(CMessageDlg::TT_Title, caption);
	dlg.SetText(CMessageDlg::TT_SubTitle, message);
	DWORD type = style & 0xF;

	switch (type)
	{
	case MB_OK:
		dlg.SetText(CMessageDlg::TT_ButtonRight, GetString(IDS_OK));
		dlg.DoModal();
		return IDOK;
		break;
	case MB_YESNO:
		dlg.SetText(CMessageDlg::TT_ButtonRight, GetString(IDS_NO_S));
		dlg.SetText(CMessageDlg::TT_ButtonMid, GetString(IDS_YES_S));
		switch (dlg.DoModal())
		{
		case CMessageDlg::RES_Close:
		case CMessageDlg::RES_Right:
			return IDNO;
		case CMessageDlg::RES_Mid:
			return IDYES;
		}
		break;
	default:
		if (caption == NULL || caption[0] == 0)
			caption = CTeenSpiritApp::sAppTitle;
		return ::MessageBox(parentHWND, message, caption, style);
	}
	return IDCANCEL;
}

int PrgAPI::MessageBox(HWND parentHWND, INT messageID, INT captionID, DWORD style)
{
	return MessageBox(parentHWND, GetString(messageID), (captionID > 0) ? GetString(captionID) : 0, style);
}

void PrgAPI::WriteOptions()
{
	TRACEST(_T("PrgAPI::WriteOptions."));
	AppSettings* pAppSettings = GetAppSettings();
	if (m_pMediaPlayer != NULL)//GetOption will fail to return Real Player Options
	{
		TCHAR playListFileName[MAX_PATH];
		GetPath(PATH_LastM3UFile, playListFileName);
		DeleteFile(playListFileName);
		MediaPlayerUtilities::SaveM3U(*m_pMediaPlayer, playListFileName);
		INT ACMode = 0;
		SetOption(OPT_PL_PlayMode, m_pMediaPlayer->GetNextMode(&ACMode));//OPT_PL_PlayMode,
		SetOption(OPT_PL_AutoContinueMode, ACMode);
		SetOption(OPT_PL_Volume, m_pMediaPlayer->GetVolume());
		SetOption(OPT_PL_Crossfade, m_pMediaPlayer->GetEngineConfig(MECO_Crossfade));
		SetOption(OPT_PL_CrossfadeAtStart, m_pMediaPlayer->GetEngineConfig(MECO_CrossfadeAtStart));
		SetOption(OPT_PL_CrossfadeAtEnd, m_pMediaPlayer->GetEngineConfig(MECO_CrossfadeAtEnd));
		SetOption(OPT_PL_EqualizerMode, m_pMediaPlayer->GetEngineConfig(MECO_EqualizerMode));
		SetOption(OPT_PL_Mute, m_pMediaPlayer->GetMute());

		if (m_pMediaPlayer->EngineSupports(MECO_EqualizerMode))
		{
			for (INT i = MECO_EqualizerBandStart; i < MECO_EqualizerBandEnd; i++)
			{
				if (m_pMediaPlayer->EngineSupports(MediaEngineConfigOption(i)))
				{
					TCHAR bf[20];
					_sntprintf(bf, 20, _T("EQ%d"), i - MECO_EqualizerBandStart);
					pAppSettings->Write(cPlayerSettings, bf, m_pMediaPlayer->GetEngineConfig(MediaEngineConfigOption(i)));
				}

			}
		}
		SetOption(OPT_PL_LastTrack, m_pMediaPlayer->GetPlayListPos());
		SetOption(OPT_PL_LastTime, (INT)m_pMediaPlayer->GetMediaPos());
	}
	GetAppOptions().Write();
}

void PrgAPI::FormatDuration(LPTSTR bf, UINT bfLen, INT duration)
{
	ASSERT(bf != NULL && bfLen > 1);
	INT h = duration / 3600;
	duration = duration - h * 3600;
	INT m = duration / 60;
	if (h == 0)
		_sntprintf(bf, bfLen, _T("%d:%02d"), m, duration - m * 60);
	else
		_sntprintf(bf, bfLen, _T("%d:%02d:%02d"), h, m, duration - m * 60);
	bf[bfLen - 1] = 0;
}



void PrgAPI::FormatDate(TCHAR* bf, size_t bfLen, SYSTEMTIME& st, BOOL bSmart)
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	DOUBLE dt, dst;
	SystemTimeToVariantTime(&t, &dt);
	SystemTimeToVariantTime(&st, &dst);
	if (dt - dst <= 4.0 && bSmart)
	{
		if (dt - dst > 1.0)
			GetDateFormat(LOCALE_SYSTEM_DEFAULT, NULL, &st, _T("dddd "), bf, bfLen);
		else
			bf[0] = 0;
		INT curLen = _tcslen(bf);
		GetTimeFormat(LOCALE_SYSTEM_DEFAULT, NULL, &st, _T("HH:mm:ss"), &bf[curLen], bfLen - curLen);
	}
	else
	{
		GetDateFormat(LOCALE_SYSTEM_DEFAULT, NULL, &st, _T("d MMM yyyy"), bf, bfLen);
	}
}

void PrgAPI::Monitor()
{
	if (m_pMediaPlayer && m_pMediaPlayer->GetPlayState() == PS_Playing)
		GetActionManager()->TrackHistory();
	if (InfoDownloadManagerExists())
		m_pInfoDownloadManager->ConsumeResult();
	if (CollectionManagerExists())
		m_pCollectionManager->ConsumeResult();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//------------Global Objects acquisition
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------



#include "TeenSpiritPlayer.h"
#include "DShowEngine.h"
#include "MultiEngine.h"
#include "HttpLinkEngine.h"
//#include "FullTracksPlaylist.h"
//#include "ImplPlaylist.h"
#include "WinampVisualization.h"
#include "NativeVisualization.h"
#include "FadeBassEngine.h"
#include "VectorPlaylist.h"



//---GetMediaPlayer()
//We Create on demand Using the objfactory and then we configure
MediaPlayer* PrgAPI::GetMediaPlayer()
{
	if (m_pMediaPlayer == NULL)
	{
		TRACEST(_T("PrgAPI::GetMediaPlayer [CREATION]"));
		HWND hwndPlayer = ::CreateWindow(_T("Static"), _T("PlayerWnd"), WS_CHILD, 0, 0, 0, 0, AfxGetMainWnd()->m_hWnd, 0, AfxGetInstanceHandle(), 0);
		
		m_pMediaPlayer = new TeenSpiritPlayer;
		MultiEngine* eng = new MultiEngine;
		eng->AddEngine(new FadeBassEngine);
		eng->AddEngine(new DShowEngine);
		eng->AddEngine(new HttpLinkEngine);
		m_pMediaPlayer->SetMediaPlayerEngine(eng);
		MediaPlayList* pList = new VectorPlayList;
		m_pMediaPlayer->SetMediaPlayList(pList);
		WinampVisualization* pVis = new WinampVisualization;
		pVis->LoadPlugin(_T("vis_avs.dll"));
		//MediaVisualization* pVis = new NativeVisualization;
		m_pMediaPlayer->SetMediaVisualization(pVis);
		//m_pMediaPlayer->Init(eng, lst, pwVis, hwndPlayer);

		m_pMediaPlayer->SetNextMode(MediaPlayerNextMode(GetOption(OPT_PL_PlayMode)), GetOption(OPT_PL_AutoContinueMode));
		eng->SetVolume(GetOption(OPT_PL_Volume));
		eng->SetMute(GetOption(OPT_PL_Mute));
		eng->SetConfig(MECO_Crossfade, GetOption(OPT_PL_Crossfade));
		eng->SetConfig(MECO_CrossfadeAtStart, GetOption(OPT_PL_CrossfadeAtStart));
		eng->SetConfig(MECO_CrossfadeAtEnd, GetOption(OPT_PL_CrossfadeAtEnd));
		eng->SetConfig(MECO_EqualizerMode, GetOption(OPT_PL_EqualizerMode));

		
		for (INT i = MECO_EqualizerBandStart; i < MECO_EqualizerBandEnd; i++)
		{
			TCHAR bf[20];
			_sntprintf(bf, 20, _T("EQ%d"), i - MECO_EqualizerBandStart);
			INT val = 50;
			GetAppSettings()->Read(cPlayerSettings, bf, val, 50);
			eng->SetConfig(MediaEngineConfigOption(i), val);
		}
	
		
		
		//SendMessage(SM_PlayerSettingsChanged);
		//SendMessage(SM_PlayerVolumeChanged);


		//Parse command line
		BOOL bEnqueue = FALSE;
		for (INT i=1; i<__argc;i++)
		{
			if (_tcscmp(_T("-q"), __targv[i]))
				bEnqueue = FALSE;
			if (__targv[i][0] != '-' && __targv[i][0] !='\\')
			{
				if (bEnqueue)
					m_pMediaPlayer->Enqueue(__targv[i]);
				else
					m_pMediaPlayer->Play(__targv[i]);
				bEnqueue = TRUE;
			}
		}
		BOOL bLoadOldPlaylist = FALSE;
		if (m_pMediaPlayer->GetPlayListCount() == 0)
		{
			bLoadOldPlaylist = TRUE;
			MediaPlayerUtilities::InsertM3U(*m_pMediaPlayer, GetPath(PATH_LastM3UFile));
		}
		if (bLoadOldPlaylist)
		{
			if (GetOption(OPT_PL_RememberLastTrack))
			{
				if ((INT)m_pMediaPlayer->GetPlayListCount() > GetOption(OPT_PL_LastTrack))
				{
					m_pMediaPlayer->SetPlayListPos(GetOption(OPT_PL_LastTrack));
					if (GetOption(OPT_PL_RememberLastTime))
					{
						//m_pMediaPlayer->Start();
						//m_pMediaPlayer->Pause();
						m_pMediaPlayer->SetMediaPos(DOUBLE(GetOption(OPT_PL_LastTime)));
					}
				}
			}
		}
		if (GetOption(OPT_PL_AutoPlay) && m_pMediaPlayer->GetPlayListCount() > 0)
		{
			m_pMediaPlayer->Start();
		}
	}
	return m_pMediaPlayer;
}

//---GetInformationManager()
//We Create on demand HERE and then we configure
LocalPictureManager* PrgAPI::GetLocalPictureManager()
{
	if (m_pLocalPictureManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetLocalPictureManager [CREATION]"));
		m_pLocalPictureManager = new LocalPictureManager;
		m_pLocalPictureManager->Init(GetPath(PATH_StorageRoot));
		m_pLocalPictureManager->LoadState(GetPath(PATH_UserRoot));
		m_pLocalPictureManager->EnableFolderImages(GetOption(OPT_IM_UseFolderImages) != 0);
	}
	return m_pLocalPictureManager;
}



InfoDownloadManager* PrgAPI::GetInfoDownloadManager()
{
	if (m_pInfoDownloadManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetInfoDownloadManager [CREATION]"));
		m_pInfoDownloadManager = new InfoDownloadManager(*GetInfoProviderFactory());
	}
	return m_pInfoDownloadManager;

}

HINTERNET PrgAPI::GetInternetHandle()
{
	if (m_hInternet == NULL)
	{
		TRACEST(_T("PrgAPI::GetInternetHandle [CREATION]"));
		INT i = 0;
		GetAppSettings()->Read(_T("General"), _T("ImpersonateAgent"), (INT&) i,  0);
		FileVersion fv;
		GetFileVersion(fv);
		TCHAR defAgent[200];
		_sntprintf(defAgent, 200, _T("Jaangle/%d.%d.%d (+https://github.com/outcast1000/TeenSpirit)"), fv.Major, fv.Minor, fv.Build);
		defAgent[199] = 0;
		std::tstring agent = defAgent;
		if (i != 0)
			GetAppSettings()->Read(_T("General"), _T("FalseAgentName"), agent,  _T(""));
		TRACE(_T("@2{Internet Handle Created}\n"));
		m_hInternet = InternetOpen(agent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	}
	return m_hInternet;
}

//---GetActionManager()
//We Create on demand HERE
ActionManager* PrgAPI::GetActionManager()
{
	if (m_pActionManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetActionManager"));
		m_pActionManager = new ActionManager;
	}
	return m_pActionManager;
}

//---GetCollectionManager()
//We Create on demand HERE
//CollectionManager* PrgAPI::GetCollectionManager(BOOL bForceCreate)
//{
//	if (m_pCollectionManager == NULL && bForceCreate)
//	{
//		m_pCollectionManager = new CollectionManager;
//		TRACE(_T("@2{Collection Manager Created}\n"));
//	}
//	return m_pCollectionManager;
//}

//---GetFileTagger()
//We Create on demand Using the objfactory
FileTagger* PrgAPI::GetFileTagger()
{
	if (m_pFileTagger == NULL)
	{
		TRACEST(_T("PrgAPI::GetFileTagger"));
		m_pFileTagger = ObjFactory::Instance()->CreateFileTagger();
	}
	m_pFileTagger->SetConfig(FileTagger::CONF_ReadDepth, GetOption(OPT_TAG_SafetyOptionManual));
	return m_pFileTagger;
}


//---GetServicesRegistrationSrv()
//We Create on demand HERE and we configure
ServicesRegistrationSrv* PrgAPI::GetServicesRegistrationSrv()
{
	if (m_pServicesRegistrationSrv == NULL)
	{
		TRACEST(_T("PrgAPI::GetServicesRegistrationSrv"));
		m_pServicesRegistrationSrv = new ServicesRegistrationSrv;
		std::tstring appPath;
		m_pServicesRegistrationSrv->LoadServices(GetPath(PATH_WebServicesRoot));
	}
	return m_pServicesRegistrationSrv;
}

AppResources& PrgAPI::GetAppResources()
{
	if (m_pAppResources == NULL)
		m_pAppResources = new AppResources(AfxGetResourceHandle());
	return *m_pAppResources;
}

void PrgAPI::SetGlobalButtonStyle(FlexButton& btn, APP_ICONS ico)
{
	btn.ApplyPredifinedButtonsStyle(FlexButton::PBS_HotButton);
	if (ico < ICO_Last)
		btn.SetIcon(GetIcon(ico), 16, 16);
	btn.SetTextStyle(FlexButton::TS_SingleLineCenter);
}





SkinManager* PrgAPI::GetSkinManager()
{
	if (m_pSkinManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetSkinManager [CREATION]"));
		m_pSkinManager = new SkinManager;
		m_pSkinManager->SetSkinsFolder(GetPath(PATH_SkinRoot));
		std::tstring SkinName;
		GetAppSettings()->Read(cGeneral, cOptionSkin, SkinName, _T(""));
		if (!SkinName.empty())
		{
			if (m_pSkinManager->LoadSkin(SkinName.c_str()) == FALSE)
			{
				TRACE(_T("PrgAPI::GetSkinManager Skin not loaded\r\n"));
			}
		}
		if (m_pSkinManager->GetSkin().GetSkinPath()[0] == 0)
			m_pSkinManager->LoadSkin(_T("Default"));
		m_pSkinManager->GetSkin().SetFontQuality(GetOption(OPT_GEN_ForceClearTypeFonts) == 1 ? CLEARTYPE_QUALITY : DEFAULT_QUALITY);
		m_pSkinManager->GetSkin().SetRelativeFontSize(GetOption(OPT_GEN_RelativeFontSize));
	}
	return m_pSkinManager;
}

LPCTSTR PrgAPI::GetPath(PathsEnum pathType, LPTSTR buffer/* = NULL*/)
{
	if (buffer == NULL)
		buffer = m_tempPath;
	switch (pathType)
	{
	case PATH_AppRoot:
		{
			::GetModuleFileName(0, buffer, MAX_PATH);
			LPTSTR pszFileName = _tcsrchr(buffer, '\\');
			pszFileName[1] = 0;
		}
		break;
	case PATH_SkinRoot:
		{
			TCHAR appRoot[MAX_PATH];
			_sntprintf(buffer, MAX_PATH, _T("%sSkins\\"), GetPath(PATH_AppRoot, appRoot));
			if (!PathFileExists(buffer))
				CreateDirectory(buffer, NULL);
		}
		break;
	case PATH_TranslationsRoot:
		{
			TCHAR appRoot[MAX_PATH];
			_sntprintf(buffer, MAX_PATH, _T("%sLangs\\"), GetPath(PATH_AppRoot, appRoot));
			if (!PathFileExists(buffer))
				CreateDirectory(buffer, NULL);
		}
		break;
	case PATH_WebServicesRoot:
		TCHAR appRoot[MAX_PATH];
		_sntprintf(buffer, MAX_PATH, _T("%sWebServices\\"), GetPath(PATH_AppRoot, appRoot));
		if (!PathFileExists(buffer))
			CreateDirectory(buffer, NULL);
		break;
	case PATH_StorageRoot:
	case PATH_UserRoot:
	case PATH_DatabaseRoot:
		{
			LPCTSTR iniEntry = NULL;
			LPCTSTR defFolder = NULL;
			switch (pathType)
			{
			case PATH_StorageRoot:
				iniEntry = _T("StoragePath");
				defFolder = _T("Storage\\");
				break;
			case PATH_UserRoot:
				iniEntry = _T("UserPath");
				defFolder = _T("User\\");
				break;
			case PATH_DatabaseRoot:
				iniEntry = _T("DatabasePath");
				defFolder = _T("");
				break;
			default:
				ASSERT(0);
				return NULL;
			}
			BOOL bUseDefaultPath = TRUE;
			std::tstring path;
			if (GetAppSettings()->Read(cGeneral, iniEntry, path, _T("")) && path.size() > 0)
			{
				if (path[path.size() - 1] != '\\')
					path.push_back('\\');
				//=== Callers pass MAX_PATH buffers; a longer configured path
				//=== (plus a file name) cannot be used, so fall back to the default.
				if (path.size() >= MAX_PATH - 20)
					TRACE(_T("@0 PrgAPI::GetPath. Configured path too long; using the default\r\n"));
				else
				{
					_tcscpy(buffer, path.c_str());
					if (CreateDirectory(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
						bUseDefaultPath = FALSE;
				}
			}
			if (bUseDefaultPath)
			{
				//=== Default Path
				TCHAR appRoot[MAX_PATH];
				_sntprintf(buffer, MAX_PATH, _T("%s%s"), GetPath(PATH_AppRoot, appRoot), defFolder);
				if (!PathFileExists(buffer))
					CreateDirectory(buffer, NULL);
			}
		}
		break;
	case PATH_LastM3UFile:
		{
			TCHAR userRoot[MAX_PATH];
			_sntprintf(buffer, MAX_PATH, _T("%slasttracks.m3u"), GetPath(PATH_UserRoot, userRoot));
		}
		break;
	case PATH_ServicesXMLFile:
		{
			TCHAR appRoot[MAX_PATH];
			_sntprintf(buffer, MAX_PATH, _T("%sservices.xml"), GetPath(PATH_AppRoot, appRoot));
		}
		break;
	case PATH_ServicesUserXMLFile:
		{
			TCHAR appRoot[MAX_PATH];
			_sntprintf(buffer, MAX_PATH, _T("%sservices.xml"), GetPath(PATH_UserRoot, appRoot));
		}
		break;
	default:
		ASSERT(0);
		return NULL;
	}
	return buffer;
}








BOOL PrgAPI::IsTrackNew(SYSTEMTIME& dateAdded, UINT DaysMeansNew/* = 0*/)
{
	if (DaysMeansNew == 0)
		DaysMeansNew = GetOption(OPT_GEN_DaysMeansNew);
	if (DaysMeansNew == 0)
		return FALSE;
	SYSTEMTIME dateCur;
	GetLocalTime(&dateCur);
	DOUBLE dAdded, dCur;
	SystemTimeToVariantTime(&dateAdded, &dAdded);
	SystemTimeToVariantTime(&dateCur, &dCur);
	return (dCur - dAdded < GetOption(OPT_GEN_DaysMeansNew));
}

BOOL PrgAPI::GetSiteURL(LPTSTR url, UINT urlLen, SiteActionEnum actID, LPCTSTR p1, LPCTSTR p2)
{
	ASSERT(actID < SA_Last && url != NULL && urlLen > 0);
	if (url == NULL || urlLen == 0)
		return FALSE;
	LPCTSTR target = _T("https://github.com/outcast1000/TeenSpirit");
	switch (actID)
	{
	case SA_Help:
		target = _T("https://github.com/outcast1000/TeenSpirit#readme");
		break;
	case SA_Navigate:
		if (p1 != NULL && _tcscmp(p1, _T("forum")) == 0)
			target = _T("https://github.com/outcast1000/TeenSpirit/issues");
		break;
	default:
		return FALSE;
	}
	_tcsncpy(url, target, urlLen);
	url[urlLen - 1] = 0;
	return TRUE;
}

BOOL PrgAPI::GoToSiteURL(SiteActionEnum actID, LPCTSTR p1, LPCTSTR p2)
{
	TRACEST(_T("PrgAPI::GoToSiteURL"));
	TCHAR url[1000];
	if (!GetSiteURL(url, 1000, actID, p1, p2))
		return FALSE;
	ShellExecute(NULL, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);
	return TRUE;
}

MenuManager* PrgAPI::GetMenuManager()
{
	if (m_pMenuManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetMenuManager [CREATION]"));
		m_pMenuManager = new MenuManager;
	}
	return m_pMenuManager;
}

TranslationManager* PrgAPI::GetTranslationManager()
{
	if (m_pTranslationManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetTranslationManager [CREATION]"));
		m_pTranslationManager = new TranslationManager();
		m_pTranslationManager->SetTranslationsFolder(GetPath(PATH_TranslationsRoot));
	}
	return m_pTranslationManager;
}

CollectionManager* PrgAPI::GetCollectionManager()
{
	if (m_pCollectionManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetCollectionManager [CREATION]"));
		m_pCollectionManager = new CollectionManager;
	}
	return m_pCollectionManager;
}

InfoProviderFactory* PrgAPI::GetInfoProviderFactory()
{
	if (m_pInfoProviderFactory == NULL)
	{
		TRACEST(_T("PrgAPI::GetInfoProviderFactory [CREATION]"));
		m_pInfoProviderFactory = new InfoProviderFactory;
		m_pInfoProviderFactory->Init(GetInternetHandle(), GetAppSettings());
		//m_pInfoProviderFactory->LoadPlugins(GetPath(PATH_InfoProvidersRoot));

	}
	return m_pInfoProviderFactory;
}
HotKeyManager* PrgAPI::GetHotKeyManager()
{
	if (m_pHotKeyManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetHotKeyManager [CREATION]"));
		m_pHotKeyManager = new HotKeyManager;
	}
	return m_pHotKeyManager;
}

TrayToolTipDlg* PrgAPI::GetTrayToolTipDlg()
{
	if (m_pTrayToolTipDlg == NULL)
	{
		TRACEST(_T("PrgAPI::TrayToolTipDlg [CREATION]"));
		m_pTrayToolTipDlg = new TrayToolTipDlg;
	}
	return m_pTrayToolTipDlg;
}

void PrgAPI::DestroyTrayToolTipDlg()
{
	if (m_pTrayToolTipDlg != NULL)
	{
		if (m_pTrayToolTipDlg->GetSafeHwnd())
			m_pTrayToolTipDlg->DestroyWindow();
		delete m_pTrayToolTipDlg;
		m_pTrayToolTipDlg = NULL;
	}
}

CMiniPlayerDlg* PrgAPI::GetMiniPlayerDlg(BOOL bForceCreate)
{
	if (m_pMiniPlayerDlg == NULL && bForceCreate)
	{
		TRACEST(_T("PrgAPI::GetMiniPlayerDlg [CREATION]"));
		m_pMiniPlayerDlg = new CMiniPlayerDlg;
		m_pMiniPlayerDlg->Create(CWnd::GetDesktopWindow());
		m_pMiniPlayerDlg->LoadOptions();
	}
	return m_pMiniPlayerDlg;
}

void PrgAPI::DestroyMiniPlayerDlg()
{
	if (m_pMiniPlayerDlg != NULL)
	{
		m_pMiniPlayerDlg->SaveOptions();
		if (m_pMiniPlayerDlg->GetSafeHwnd())
			m_pMiniPlayerDlg->DestroyWindow();
		delete m_pMiniPlayerDlg;
		m_pMiniPlayerDlg = NULL;
	}
}


QuickSearchDlg* PrgAPI::GetQuickSearchDlg()
{
	if (m_pQuickSearchDlg == NULL)
	{
		TRACEST(_T("PrgAPI::GetQuickSearchDlg [CREATION]"));
		m_pQuickSearchDlg = new QuickSearchDlg;
	}
	return m_pQuickSearchDlg;
}

void PrgAPI::DestroyQuickSearchDlg()
{
	if (m_pQuickSearchDlg != NULL)
	{
		if (m_pQuickSearchDlg->GetSafeHwnd())
			m_pQuickSearchDlg->DestroyWindow();
		delete m_pQuickSearchDlg;
		m_pQuickSearchDlg = NULL;
	}
}

CSearchDlg* PrgAPI::GetAdvancedSearchDlg()
{
	if (m_pSearchDlg == NULL)
	{
		TRACEST(_T("PrgAPI::GetAdvancedSearchDlg [CREATION]"));
		m_pSearchDlg = new CSearchDlg;
		m_pSearchDlg->Create(IDD_SEARCH, AfxGetMainWnd());
		m_pSearchDlg->SetWindowText(PRGAPI()->GetString(IDS_SEARCH));
		m_pSearchDlg->CenterWindow( AfxGetMainWnd());
	}				
	return m_pSearchDlg;

}
void PrgAPI::DestroyAdvancedSearchDlg()
{
	if (m_pSearchDlg != NULL)
	{
		if (m_pSearchDlg->GetSafeHwnd())
			m_pSearchDlg->DestroyWindow();
		delete m_pSearchDlg;
		m_pSearchDlg = NULL;
	}

}



ShortcutManager* PrgAPI::GetShortcutManager()
{
	if (m_pShortcutManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetShortcutManager [CREATION]"));
		m_pShortcutManager = new ShortcutManager;
	}
	return m_pShortcutManager;
}

ConfigurationManager* PrgAPI::GetConfigurationManager()
{
	if (m_pConfigurationManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetConfigurationManager [CREATION]"));
		m_pConfigurationManager = new ConfigurationManager;
	}
	return m_pConfigurationManager;
}

StateManager* PrgAPI::GetStateManager()
{
	if (m_pStateManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetStateManager [CREATION]"));
		m_pStateManager = new StateManager;
	}
	return m_pStateManager;
}



SQLManager* PrgAPI::GetSQLManager()
{
	if (m_pSQLManager == NULL)
	{
		TRACEST(_T("PrgAPI::GetSQLManager [CREATION]"));
		m_pSQLManager = new SQLManager;
		std::tstring path(GetPath(PATH_DatabaseRoot));
		path += _T("music.db");	//An existing music.mdb next to it is imported on first run
		if (!m_pSQLManager->Init(path.c_str()))
		{
			delete m_pSQLManager;
			m_pSQLManager = 0;
			TRACE(_T("@0{Database Engine Failed to be created}\n"));
		}

	}

	return m_pSQLManager;
}

LastFMServices* PrgAPI::GetLastFMServices()
{
	if (m_pLastFMServices == NULL)
	{
		TRACEST(_T("PrgAPI::GetLastFMServices"));
		m_pLastFMServices = new LastFMServices();
		m_pLastFMServices->Init(GetInternetHandle());

		AppSettings* pSettings = GetAppSettings();
		std::tstring val;
		pSettings->Read(cLastFM, cOptionUser, val, _T(""));
		m_pLastFMServices->SetUserName(val.c_str());
		pSettings->Read(cLastFM, cOptionPass, val, _T(""));
		m_pLastFMServices->SetMD5Password(val.c_str());
	}
	return m_pLastFMServices;
}

ITracker* PrgAPI::GetTracker()
{
	if (m_pTracker == NULL)
	{
		TRACEST(_T("PrgAPI::GetTracker [CREATION]"));
		m_pTracker= new MultiTracker();
	}
	return m_pTracker;
}


void PrgAPI::SendMessage(UINT stateMessage)
{
	GetStateManager()->SendMessage(stateMessage);
}
void PrgAPI::PostMessage(UINT stateMessage)
{
	GetStateManager()->PostMessage(stateMessage);
}

ITSMenu* PrgAPI::CreatePopupMenu()
{
	return new TSMenu;
}



UINT Stars2Rating(FLOAT Stars)
{
	ASSERT(Stars >= 0.0f && Stars <= 5.0f); //should be 1 - 5
	return UINT(INT(Stars*100+.5) * 50 / 100);

}

//Returns:
//0 for 0 (No Rating)
//1 for (1-50) Bad
//2 for (51-100) Neutral
//3 for (101-150) 1 Star ...
//4 for (151-200) 2 Star ...
//5 for (201-) 3 Star ...
FLOAT Rating2Stars(UINT Rating)
{	
	FLOAT stars = Rating / 50.0f;
	if (stars > 5.0f)
		stars = 5.0f;
	return stars;
}

BOOL CALLBACK SetFontToChilds(HWND hwnd, LPARAM lParam)
{
	ASSERT(hwnd != NULL);
	ASSERT(lParam != NULL);
	if (lParam == NULL)
		return FALSE;
	SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, FALSE);
	return TRUE;
}










