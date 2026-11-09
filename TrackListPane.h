#pragma once

#include "IPane.h"
#include "ITranslation/ITranslation.h"
#include "TSSkin.h"
#include "IStateChangeListener.h"

#include "EnhTrackListWnd.h"

static PaneInfo TrackListPaneInfo = {_T("TrackListPane"), _T("Track List Pane"), _T(""), TRUE};


class TrackListPane :	public IPane,
	public TSTranslatable,			//=== It supports translations
	public IStateChangeListener		//=== It requires state change events
{
public:
	TrackListPane();
	virtual ~TrackListPane();

	virtual BOOL Init(HWND hwndParent);
	virtual void UnInit();

	virtual BOOL Show();
	virtual void Hide();
	virtual void MovePane(INT x, INT y, INT cx, INT cy);

	virtual LPCTSTR GetTitle(UINT captionNum);
	virtual BOOL GetButtonInfo(PaneButtonInfo& bInfo, UINT idx);
	virtual ITSMenu* GetMenu(UINT idx);

	virtual BOOL OnButton(UINT idx);

	virtual const PaneInfo& GetPaneInfo()						{return TrackListPaneInfo;}

	virtual BOOL LoadState(IPaneState& paneState);
	virtual BOOL SaveState(IPaneState& paneState);

	//=== public IStateChangeListener
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()						{return TrackListPaneInfo.friendlyName;}

	//=== public ITranslatable
	virtual void ApplyTranslation(ITranslation& translation);



private:
	TCHAR m_stringBuffer[500];
	CEnhTrackListWnd m_enhTrackList;

	enum Buttons
	{
		CMD_Options = 0,
		CMD_Play,
		CMD_Enqueue,
		CMD_ShowInfoIcons,
		CMD_SyncWithPlayer,
		CMD_SyncWithSectionChanger,
		CMD_Last
	};

	//IPaneController* m_pPaneController;
	HWND m_hwndParent;
	CRect m_clientRC;
	ITSMenu* m_poptionsMenu;
	BOOL m_bSyncWithPlayer;
	BOOL m_bSyncWithSectionChanger;
};
