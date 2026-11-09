#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#define COMMANDLINE_ARG	100

#include "resource.h"		// main symbols



class CTeenSpiritApp : public CWinApp
{
public:
	CTeenSpiritApp();
	virtual ~CTeenSpiritApp()	{}

	static LPCTSTR const sAppTitle;
	static LPCTSTR const sWndClass;
protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	DECLARE_MESSAGE_MAP()
private:
	CMutex m_AppMutex;
	BOOL m_bDontUseNextRButtonUp;
};

extern CTeenSpiritApp theApp;
