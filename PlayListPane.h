#pragma once
#include "IPane.h"
#include "TSTranslatable.h"
#include "IStateChangeListener.h"

#include "PlayListListCtrl.h"

static PaneInfo PlayListPaneInfo = {_T("PlayListPane"), _T("PlayList Pane"), _T("Internal"), TRUE};


class PlayListPane :	public IPane,
	public TSTranslatable,			//=== It supports translations
	public IStateChangeListener		//=== It requires state change events
{
public:
	PlayListPane();   // standard constructor
	virtual ~PlayListPane();

	virtual BOOL Init(HWND hwndParent);
	virtual void UnInit();

	virtual BOOL Show();
	virtual void Hide();

	virtual void MovePane(INT x, INT y, INT cx, INT cy);

	virtual LPCTSTR GetTitle(UINT captionNum);
	virtual BOOL GetButtonInfo(PaneButtonInfo& bInfo, UINT idx);
	virtual ITSMenu* GetMenu(UINT idx);

	virtual BOOL OnButton(UINT idx);

	virtual const PaneInfo& GetPaneInfo()						{return PlayListPaneInfo;}

	virtual BOOL LoadState(IPaneState& paneState);
	virtual BOOL SaveState(IPaneState& paneState);

	//=== public ITranslatable
	virtual void ApplyTranslation(ITranslation& translation)	{ResetAllMenus();}

	//=== public IStateChangeListener
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()						{return _T("PlayListPane");}

private:
	enum MenuItemsEnum
	{
		MI_First = 0x2000,
		MI_ShowNumbers,
		MI_Last
	};
	enum MenusEnum
	{
		M_Main,
		M_Last
	};
	ITSMenu* m_pMenu[M_Last];

private:
	TCHAR m_stringBuffer[500];
	CPlayListListCtrl m_PlayList;
	HWND m_hwndParent;

private:
	ITSMenu& GetInternalMenu(MenusEnum mnu);
	void ResetAllMenus();


};
