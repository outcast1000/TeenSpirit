#pragma once

#include "IPane.h"
#include "ITranslation/ITranslation.h"
#include "TSSkin.h"
#include "StateManager.h"
#include "VideoContainer.h"

class MediaPlayer;

static PaneInfo VideoPaneInfo = {_T("VideoPane"), _T("Video Pane"), _T(""), FALSE};

class VideoPane : public IPane,
	public ISkinnable,				//=== It supports skins
	public TSStateChangeListener		//=== It requires state change events
{
public:
	VideoPane();
	virtual ~VideoPane();

	virtual BOOL Init(HWND hwndParent);
	virtual void UnInit();

	virtual BOOL Show();
	virtual void Hide();
	virtual void MovePane(INT x, INT y, INT cx, INT cy);

	virtual LPCTSTR GetTitle(UINT captionNum);
	virtual BOOL GetButtonInfo(PaneButtonInfo& bInfo, UINT idx)	{return FALSE;}
	virtual ITSMenu* GetMenu(UINT idx)							{return NULL;}

	virtual BOOL OnButton(UINT idx)								{return FALSE;}

	virtual const PaneInfo& GetPaneInfo()						{return VideoPaneInfo;}

	virtual BOOL LoadState(IPaneState& paneState);
	virtual BOOL SaveState(IPaneState& paneState);

	//=== public IStateChangeListener
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()						{return VideoPaneInfo.friendlyName;}

	//=== public ISkinnable
	virtual void ApplySkin(ISkin& skin)							{}


	static BOOL IsCreatable();

private:
	static INT m_instances;
	CVideoContainer m_visCtrl;


};
