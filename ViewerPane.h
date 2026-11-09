#pragma once

#include "IPane.h"
#include "ITranslation/ITranslation.h"
#include "TSSkin.h"
#include "IStateChangeListener.h"

//#include "DirectorPat/BaseDirector.h"
//#include "TransmitterPattern/Transmitter.h"
#include "GuiControls/BCMenu.h"
//#include "CaptionedPane.h"

class CInfoCtrl;
class CVisualizationContainer;
class MediaPlayer;
class TSBrowserEnh;

class CViewerPane :	public IPane,
	//public CWnd,					//=== It is an MFC Window
	//public ITranslatable,			//=== It supports translations
	//public ISkinnable,				//=== It supports skins
	public IStateChangeListener		//=== It requires state change events
{
public:
	CViewerPane();   // standard constructor
	virtual ~CViewerPane();

	//=== IPane
	virtual BOOL Init(IPaneController* pPaneController, HWND hwndParent, UINT nID);
	virtual void UnInit();
	virtual BOOL Show();
	virtual void Hide();
	virtual void MovePane(INT x, INT y, INT cx, INT cy);
	virtual void SaveSettings();
	virtual LPCTSTR GetTitle(UINT captionNum);

	//=== public ITranslatable
	virtual void ApplyTranslation(ITranslation& translation);

	//=== public ISkinnable
	virtual void ApplySkin(ISkin& skin);

	//=== public IStateChangeListener
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()				{return _T("CViewerPane");}

protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
private:
	CInfoCtrl* m_pInfoCtrl;
	CVisualizationContainer* m_pVisCtrl;
	TSBrowserEnh* m_pBrowser;
	MODES m_mod;
	BOOL m_bInitialized;
	BOOL m_bSyncWithPlayer;
	BCMenu m_OptionsMenu;
	CInfoCtrl* GetInfoControl();
	TSBrowserEnh* GetBrowserCtrl();
	CVisualizationContainer* GetVisualizationCtrl();
	//BOOL m_bAutoViewerMode;
	//std::tstring m_caption;
private:
	void OnInit();
	//TCHAR m_stringBuffer[500];

private:
	TCHAR m_stringBuffer[500];


};
