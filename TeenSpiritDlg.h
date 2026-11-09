#ifndef _CTeenSpiritDlg_h_
#define _CTeenSpiritDlg_h_

#include "TSSkinnable.h"

#include "ToolbarWnd.h"

#include "PaneController.h"

#include "PlayerBarWnd.h"
#include "StatusBarWnd.h"

#include "SystemTray.h"

#define WM_THREADED_NOTIFICATION WM_APP + 100
#define ID_TRAY_SHOWMAINWINDOW 300

// CTeenSpiritDlg dialog
class CTeenSpiritDlg : public CDialog, public TSSkinnable, public TSStateChangeListener
{
// Construction
public:
	CTeenSpiritDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CTeenSpiritDlg();	// standard constructor


// Dialog Data
	enum { IDD = IDD_TEENSPIRIT_DIALOG };


	void OnUpdateFlush(CMenu *pMenu);

	virtual void ApplySkin(ISkin& skin);


	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()								{return _T("CTeenSpiritDlg");}


protected:// by CDialog 
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK()									{}
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnFinalShowWindow(WPARAM wParam, LPARAM lParam);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	DECLARE_MESSAGE_MAP()

private:
	void ShowTrayMenu(CPoint& pt);
	void ShowTray();
	void HideTray();
	LRESULT OnTrayNotify(WPARAM wParam, LPARAM lParam);

	void LoadSettings();
	void SaveSettings();
	void PositionControls(int cx, int cy);

	void MaximizeWindow(RECT& rcNormal);

private:

	CToolbarWnd m_toolBar;
	PaneController m_PaneController;
	CStatusBarWnd m_statusBar;
	CPlayerBarWnd m_playerBar;

	HICON m_hBigIcon;
	HICON m_hSmallIcon;
	
	CSystemTray m_TrayIcon;
	static LPCTSTR SettingsCategory;

	INT m_lastUpdateActionsPerformed;
	UINT m_monitorPlayerTick;

	UINT m_paneUpLine;
	UINT m_paneDownLine;

	enum Timers
	{
		TMR_InitialDelay = 500,
		TMR_CheckNewVersion,
		TMR_AutoUpdateDB,
		TMR_HeartBeat,
		TMR_AppMonitor,
		TMR_PlayerMonitor,
		TMR_ShowTrayTooltipOnMouseMove,
		TMR_Last
	};
	CPoint m_ptTrayCursorPos;

};

#endif