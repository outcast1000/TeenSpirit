#include "stdafx.h"

#include "TeenSpirit.h"
#include "TeenSpiritDlg.h"
#include <locale.h>
#include "PrgAPI.h"
#include "ShortcutManager.h"
#include "HotKeyManager.h"
#include "ActionManager.h"
#include "TrayToolTipDlg.h"
#include "MiniPlayerDlg.h"
#include "QuickSearchDlg.h"
//#include "ExceptionHandler.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

LPCTSTR const CTeenSpiritApp::sAppTitle = _T("Jaangle");
LPCTSTR const CTeenSpiritApp::sWndClass= _T("JaangleWndClass");

BEGIN_MESSAGE_MAP(CTeenSpiritApp, CWinApp)
END_MESSAGE_MAP()

CTeenSpiritApp::CTeenSpiritApp():
m_AppMutex(FALSE, CTeenSpiritApp::sAppTitle),
m_bDontUseNextRButtonUp(FALSE)
{
}


//#include "CollectionManager.h"
//#include "SQLManager.h"
CTeenSpiritApp theApp;

BOOL CTeenSpiritApp::InitInstance()
{
	//Set locale is needed for "Proper Case"
	setlocale( LC_CTYPE, "" );
	//Only One Instance are allowed
#ifndef _DEBUG
	CSingleLock lock(&m_AppMutex, FALSE);
	if (!lock.Lock(0))
	{
		if ( CWnd* pWnd = CWnd::FindWindow(sWndClass, NULL))
		{
			if (pWnd->IsWindowVisible())
				pWnd->SetForegroundWindow();
			else
				pWnd->SendMessage( WM_SYSCOMMAND, SC_RESTORE );
			for (int i = 1; i < __argc; i++)
			{
				COPYDATASTRUCT cds;
				cds.dwData = COMMANDLINE_ARG;
				cds.cbData = (_tcslen(__targv[i])+1) * sizeof(TCHAR);
				cds.lpData = __targv[i];
				pWnd->SendMessage(WM_COPYDATA, 0, (LPARAM) (LPVOID) &cds );
				if (i==__argc-1)
				{
					cds.cbData = 0;
					cds.lpData = 0;
					pWnd->SendMessage(WM_COPYDATA, 0, (LPARAM) (LPVOID) &cds );
				}
			}
		}
		exit(0);
	}
#endif
	TCHAR path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	TCHAR* pos = _tcsrchr(path, '\\');
	pos++;
	*pos = 0;
#ifndef _DEBUG
#ifdef ALOG_ACTIVE
	_tcsncat_s(path, _T("log.me"), _TRUNCATE);
	ALog::Inst()->LoadConfiguration(path);
#endif
#else
	_tcsncat_s(path, _T("debug.me"), _TRUNCATE);
	if (!ALog::Inst()->LoadConfiguration(path))//!ALog::Inst()->LoadConfiguration(path))
	{
#ifdef USE_DEBUGGER_EITHER_WAY
		//ALog::Inst()->SetViewer(ALog::GetAutoViewer());
		ALog::Inst()->AddViewer(new DebugViewer());
		//ALog::Inst()->SetViewer(new AutoWindowViewer(AfxGetInstanceHandle()));
		//ALog::Inst()->SetViewer(new FileViewer(_T("c:\\debug.log")));
		ALog::Inst()->SetOption(ALog::LO_MaxAllowedDepth, 5);
		ALog::Inst()->SetOption(ALog::LO_DefaultDepth, 3);
		ALog::Inst()->SetOption(ALog::LO_ShowDepth, 1);
		ALog::Inst()->SetOption(ALog::LO_ShowTimeFromStartup, 1);
		ALog::Inst()->SetOption(ALog::LO_ShowThreadID, 1);
		ALog::Inst()->SetOption(ALog::LO_AlwaysCheckForLastError, 1);

		//ALog::Inst()->Log(_T("@2@T Measuring %s\r\n"), _T("Formating"));
		//ALog::Inst()->Log(_T("@2@T Measuring %s\r\n"), _T("Formating"));
		//ALog::Inst()->Log(_T("@M"));
		//ALog::Inst()->Log(_T("@N clean\r\n"));
		//ALog::Inst()->Log(_T("@0@W Desktop Window\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@1@W Desktop Window 2\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@W Desktop Window 4\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@0@E LastError %s\r\n"), _T("With Formating"));
		//ALog::Inst()->Log(_T("@E LastError 2\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@1 www %d\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@2 www %d\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@3 www %d\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@4 www %d\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@5 www %d\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@D www %d\r\n"), ::GetDesktopWindow());
		//ALog::Inst()->Log(_T("@D@I Process Info\r\n"));
#endif
	}

#endif
	TRACE(_T("@3@I Starting\r\n"));
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);


	CWinApp::InitInstance();
	AfxEnableControlContainer();
	OleInitialize(NULL);	//DragDrop Files
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	//=== UNITTESTING ======================================================
	//if (!RunTests())
	//	return TRUE;

	//=== Create SQL MANAGER and Initiate GUI
	PrgAPI* pAPI = PRGAPI();
	pAPI->GetHotKeyManager()->Init();//m_hWnd);
	if (pAPI->GetSQLManager() != NULL)
	{
		WNDCLASS wc;
		::GetClassInfo(AfxGetInstanceHandle(), _T("#32770"), &wc);
		wc.lpszClassName = sWndClass;
		AfxRegisterClass(&wc);
		CTeenSpiritDlg dlg;
		m_pMainWnd = &dlg;
		dlg.DoModal();
	}
	else
		MessageBox(NULL, _T("Cannot Open / Create Database. Aborting..."), _T("Database Error"), MB_OK | MB_ICONERROR);
	return TRUE;
}

int CTeenSpiritApp::ExitInstance()
{
	PrgAPI::Shutdown();
	::CoUninitialize();
	OleUninitialize();
	return CWinApp::ExitInstance();
}

BOOL CTeenSpiritApp::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYUP:
	case WM_KEYDOWN:
	case WM_CHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		{
			switch (PRGAPI()->GetShortcutManager()->HandleKeyMessage(pMsg->hwnd,
				pMsg->message, pMsg->wParam, pMsg->lParam))
			{
			case ShortcutManager::AR_ReturnFalse:
				return FALSE;
			case ShortcutManager::AR_ReturnTrue:
				return TRUE;
			}
		}
		break;
	case WM_HOTKEY:
		{
			PrgAPI* pAPI = PRGAPI();
			MediaPlayer* pPlayer = pAPI->GetMediaPlayer();
			if (pPlayer == NULL)
				return 0;
			switch (pMsg->wParam)
			{
			case VK_MEDIA_PLAY_PAUSE:
			case HotKeyManager::HK_Play:
				switch (pPlayer->GetPlayState())
				{
				case PS_Playing:
					pPlayer->Pause();
					break;
				default:
					pPlayer->Start();
					break;
				}
				break;
			case VK_MEDIA_STOP:
			case HotKeyManager::HK_Stop:
				pPlayer->Stop();
				break;
			case VK_MEDIA_PREV_TRACK:
			case HotKeyManager::HK_Prev:
				pPlayer->Previous();
				break;
			case VK_MEDIA_NEXT_TRACK:
			case HotKeyManager::HK_Next:
				pPlayer->Next();
				break;
			case HotKeyManager::HK_ToggleMiniPlayer:
				{
					CMiniPlayerDlg* pMP = pAPI->GetMiniPlayerDlg(TRUE);
					pMP->ShowWindow(pMP->IsWindowVisible() ? SW_HIDE : SW_SHOW);
				}
				break;
			case HotKeyManager::HK_Tooltip:
				pAPI->GetTrayToolTipDlg()->Show(5000);
				break;
			case HotKeyManager::HK_QuickSearch:
				{
					QuickSearchDlg* pQSDlg = pAPI->GetQuickSearchDlg();
					pQSDlg->SetSearchText(_T(""));
					pQSDlg->ShowRelative(NULL, QuickSearchDlg::RP_Centered);
				}
				break;
			default:
				ASSERT(0);
			}
			return 0;
		}

		break;
	case WM_MOUSEWHEEL:
		if (LOWORD(pMsg->wParam) == MK_RBUTTON)
		{
			const INT cVolChange = 3;
			SHORT zDelta = HIWORD(pMsg->wParam);
			MediaPlayer* pPlayer = PRGAPI()->GetMediaPlayer();
			INT newVol = pPlayer->GetVolume() + (zDelta > 0 ? cVolChange : -cVolChange);
			if (newVol < 0)
				newVol = 0;
			else if (newVol > 100)
				newVol = 100;
			pPlayer->SetVolume(newVol);
			PRGAPI()->SendMessage(SM_PlayerVolumeChanged);
			m_bDontUseNextRButtonUp = TRUE;
			return TRUE;
		}
		break;
	case WM_RBUTTONUP:
		if (m_bDontUseNextRButtonUp)
		{
			m_bDontUseNextRButtonUp = FALSE;
			return TRUE;
		}
		break;
	}
	return CWinApp::PreTranslateMessage(pMsg);
}
