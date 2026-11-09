#ifndef _CVisualizationContainer_h_
#define _CVisualizationContainer_h_

#pragma once
//#include "IVisualizationContainer.h"
class MediaPlayer;

class CVisualizationContainer:public CWnd
{
public:
	CVisualizationContainer():
	m_pMediaPlayer(NULL),
		m_WaitToStopVis(0),
		m_windowStyle(0),
		m_windowExStyle(0),
		m_windowParent(0),
		m_bFullScreen(FALSE)
	{}
	virtual ~CVisualizationContainer();

	virtual void SetMediaPlayer(MediaPlayer* pPlayer);	

private:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()

	void UpdateVisualization();

private:
	BOOL SetFullScreen(BOOL bFullScreen);
	MediaPlayer* m_pMediaPlayer;
	INT m_WaitToStopVis;
	BOOL m_bFullScreen;
	DWORD m_windowStyle;
	DWORD m_windowExStyle;
	CWnd* m_windowParent;
	CRect m_rcWindow;

protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnDestroy();
};

#endif