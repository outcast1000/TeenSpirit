#pragma once

#include "IPane.h"
#include "ITranslation/ITranslation.h"
#include "TSSkin.h"
#include "IStateChangeListener.h"

#include "GuiControls/BCMenu.h"
#include "VisualizationContainer.h"

class MediaPlayer;

static PaneInfo VisualizationPaneInfo = {_T("VisualizationPane"), _T("Visualization Pane"), _T(""), FALSE};

class VisualizationPane : public IPane,
	public ISkinnable,				//=== It supports skins
	public IStateChangeListener		//=== It requires state change events
{
public:
	VisualizationPane();
	virtual ~VisualizationPane();

	virtual BOOL Init(HWND hwndParent);
	virtual void UnInit();

	virtual BOOL Show();
	virtual void Hide();
	virtual void MovePane(INT x, INT y, INT cx, INT cy);

	virtual LPCTSTR GetTitle(UINT captionNum);
	virtual BOOL GetButtonInfo(PaneButtonInfo& bInfo, UINT idx)	{return FALSE;}
	virtual ITSMenu* GetMenu(UINT idx)							{return NULL;}

	virtual BOOL OnButton(UINT idx)								{return FALSE;}

	virtual const PaneInfo& GetPaneInfo()						{return VisualizationPaneInfo;}

	virtual BOOL LoadState(IPaneState& paneState);
	virtual BOOL SaveState(IPaneState& paneState);

	//=== public IStateChangeListener
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()						{return VisualizationPaneInfo.friendlyName;}

	//=== public ISkinnable
	virtual void ApplySkin(ISkin& skin)							{}


	static INT GetInstances()									{return m_instances;}

private:
	static INT m_instances;
	CVisualizationContainer m_visCtrl;


};
