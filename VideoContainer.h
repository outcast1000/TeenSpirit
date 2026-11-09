#ifndef _CVideoContainer_h_
#define _CVideoContainer_h_

#pragma once

#include "MediaPlayer.h"

class CVideoContainer:public CWnd
{
public:
	CVideoContainer():
	m_pMediaPlayer(NULL),
		m_windowStyle(0),
		m_windowExStyle(0),
		m_windowParent(0),
		m_bFullScreen(FALSE)
	{}
	virtual ~CVideoContainer();

	BOOL Init(MediaPlayer* pPlayer);
	BOOL IsFullScreen()								{return m_bFullScreen;}
	void SetFullScreen(BOOL bFullScreen);


protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
private:
	MediaPlayer* m_pMediaPlayer;
	BOOL m_bFullScreen;
	DWORD m_windowStyle;
	DWORD m_windowExStyle;
	CWnd* m_windowParent;
	CRect m_rcWindow;
};

#endif