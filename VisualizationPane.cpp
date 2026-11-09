// PlayListPane.cpp : implementation file
//

#include "stdafx.h"
#include "TeenSpirit.h"
#include "VisualizationPane.h"
#include "MediaPlayer.h"
#include "PrgAPI.h"

INT VisualizationPane::m_instances = 0;

VisualizationPane::VisualizationPane()
{
	m_instances++;
}

VisualizationPane::~VisualizationPane()
{
	m_visCtrl.DestroyWindow();
	m_instances--;
}

BOOL VisualizationPane::Init(HWND hwndParent)
{
	if (m_visCtrl.GetSafeHwnd())
		return TRUE;
	if (m_visCtrl.Create(_T("STATIC"), NULL, WS_VISIBLE | WS_CHILD, CRect(0,0,0,0), CWnd::FromHandle(hwndParent), 910, NULL))
	{
		m_visCtrl.SetMediaPlayer(PRGAPI()->GetMediaPlayer());
		return TRUE;
	}
	return FALSE;
}

void VisualizationPane::UnInit()
{
	if (m_visCtrl.GetSafeHwnd())
		m_visCtrl.DestroyWindow();
}

BOOL VisualizationPane::Show()
{
	if (m_visCtrl.GetSafeHwnd())
	{
		m_visCtrl.ShowWindow(SW_SHOW);
		return TRUE;
	}
	return FALSE;
}

void VisualizationPane::Hide()
{
	if (m_visCtrl.GetSafeHwnd())
		m_visCtrl.ShowWindow(SW_HIDE);
}

void VisualizationPane::MovePane(INT x, INT y, INT cx, INT cy)
{
	m_visCtrl.MoveWindow(x, y, cx, cy);
}

LPCTSTR VisualizationPane::GetTitle(UINT idx)
{
	switch (idx % 1)
	{
	case 0:
		return PRGAPI()->GetString(IDS_VISUALIZATIONS);
	}
	return NULL;
}

BOOL VisualizationPane::LoadState(IPaneState& paneState)
{
	return TRUE;
}

BOOL VisualizationPane::SaveState(IPaneState& paneState)
{
	return TRUE;
}

BOOL VisualizationPane::OnStateChanged(UINT stateMessages)
{
	BOOL bHandled = FALSE;
	return bHandled;
}

