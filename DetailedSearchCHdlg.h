#pragma once

#include "SearchPropertyPages.h"
#define CDetailedSearchCHdlgLEN 100

class CDetailedSearchCHdlg : public CSearchPropertyPages
{
	DECLARE_DYNAMIC(CDetailedSearchCHdlg)

public:
	CDetailedSearchCHdlg();
	virtual ~CDetailedSearchCHdlg();

// Dialog Data
	enum { IDD = IDD_PPDETAILEDSEARCH };

	LPCTSTR GetArtist();
	LPCTSTR GetAlbum();
	LPCTSTR GetTitle();
	LPCTSTR GetLocation();
	virtual void ApplyTranslation(ITranslation& skin); //ICtrlStrings
protected:
	virtual BOOL CDetailedSearchCHdlg::OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	TCHAR m_Artist[CDetailedSearchCHdlgLEN];
	TCHAR m_Album[CDetailedSearchCHdlgLEN];
	TCHAR m_Title[CDetailedSearchCHdlgLEN];
	TCHAR m_Location[CDetailedSearchCHdlgLEN];

};
