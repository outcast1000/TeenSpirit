#pragma once

#include "IPane.h"
#include "ITranslation/ITranslation.h"
#include "TSSkin.h"
#include "IStateChangeListener.h"

#include "InfoCtrl.h"
#include "VideoContainer.h"

static PaneInfo InfoPaneInfo = {_T("InfoPane"), _T("Information Pane"), _T(""), TRUE};


class InfoPane : public IPane,
	public ITranslatable,			//=== It supports translations
	public IStateChangeListener		//=== It requires state change events
{
public:
	InfoPane();
	virtual ~InfoPane();

	virtual BOOL Init(HWND hwndParent);
	virtual void UnInit();

	virtual BOOL Show();
	virtual void Hide();
	virtual void MovePane(INT x, INT y, INT cx, INT cy);

	virtual LPCTSTR GetTitle(UINT captionNum);
	virtual BOOL GetButtonInfo(PaneButtonInfo& bInfo, UINT idx);
	virtual ITSMenu* GetMenu(UINT idx);

	virtual BOOL OnButton(UINT idx);

	virtual const PaneInfo& GetPaneInfo()						{return InfoPaneInfo;}

	virtual BOOL LoadState(IPaneState& paneState);
	virtual BOOL SaveState(IPaneState& paneState);

	//=== public IStateChangeListener
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()						{return InfoPaneInfo.friendlyName;}

	//=== public ITranslatable
	virtual void ApplyTranslation(ITranslation& translation);

protected:
	//virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//afx_msg void OnSize(UINT nType, int cx, int cy);
	//DECLARE_MESSAGE_MAP()
private:
	CInfoCtrl m_infoCtrl;
	BOOL m_bSyncWithPlayer;
	BOOL m_bSyncWithSections;
	BOOL m_bSyncWithTracks;
	ITSMenu* m_pOptionsMenu;
	ITSMenu* m_pShowItemsMenu;
	BOOL m_bAutomaticallyShowVideo;
	CVideoContainer* m_pVideoContainer;
	HWND m_hwndParent;

	enum Buttons
	{
		BT_Options,
		BT_Last
	};
private:
	void GetBoolValue(IPaneState& paneState, LPCTSTR value, BOOL& bValue);
	TCHAR m_stringBuffer[500];

	enum MenuItems
	{
		MI_SyncWithPlayer = 0x500,
		MI_SyncWithSectionChanger,
		MI_SyncWithTracksChanger,
		MI_AutomaticallyShowVideo,

		MI_ShowItemsStart,
		MI_ShowItemsEnd = MI_ShowItemsStart + IIT_Last,

		MI_Last
	};
};
