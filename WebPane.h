#pragma once
#include "IPane.h"
#include "TSTranslatable.h"
#include "IStateChangeListener.h"
#include "TSBrowserEnh.h"
#include "ServicesRegistrationSrv.h"

static PaneInfo WebPaneInfo = {_T("WebPane"), _T("Web Pane"), _T("Internal"), TRUE};


class WebPane :	public IPane,
	public TSTranslatable,							//=== It supports translations
	public ServicesRegistrationSrv::IWebViewer,
	public IStateChangeListener						//=== It requires state change events
{
public:
	WebPane();   // standard constructor
	virtual ~WebPane();

	virtual BOOL Init(HWND hwndParent);
	virtual void UnInit();

	virtual BOOL Show();
	virtual void Hide();

	virtual void MovePane(INT x, INT y, INT cx, INT cy);

	virtual LPCTSTR GetTitle(UINT captionNum);
	virtual BOOL GetButtonInfo(PaneButtonInfo& bInfo, UINT idx);
	virtual ITSMenu* GetMenu(UINT idx);

	virtual BOOL OnButton(UINT idx);

	virtual const PaneInfo& GetPaneInfo()						{return WebPaneInfo;}

	virtual BOOL LoadState(IPaneState& paneState);
	virtual BOOL SaveState(IPaneState& paneState);

	//=== public ITranslatable
	virtual void ApplyTranslation(ITranslation& translation)	{ResetAllMenus();}

	//=== public IStateChangeListener
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()						{return WebPaneInfo.Name;}

	virtual BOOL OpenURL(LPCTSTR url);

private:
	TSBrowserEnh m_web;
	enum MenuItemsEnum
	{
		MI_First = 0x2000,
		MI_SyncWithPlayer,
		MI_SyncWithSectionChanger,
		MI_SyncWithTracksChanger,
		MI_UseAsWebViewer,
		MI_Last
	};
	enum MenusEnum
	{
		M_Main,
		M_Last
	};
	ITSMenu* m_pMenu[M_Last];
	ITSMenu& GetInternalMenu(MenusEnum mnu);

	INT m_SyncWithArtistService;
	INT m_SyncWithAlbumService;
	INT m_SyncWithTrackService;

	BOOL m_bSyncWithPlayer;
	BOOL m_bSyncWithSectionsChanger;
	BOOL m_bSyncWithTracksChanger;

	BOOL m_bUseAsWebViewer;

private:
	//TCHAR m_stringBuffer[500];
	//CPlayListListCtrl m_PlayList;
	HWND m_hwndParent;

private:
	void ResetAllMenus();
	//ITSMenu& GetInternalMenu(MenusEnum mnu);
	//void ResetAllMenus();


};
