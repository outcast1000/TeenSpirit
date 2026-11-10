// MainTrackPage.cpp : implementation file
//

#include "stdafx.h"
#include "TeenSpirit.h"
#include "MainTrackPage.h"
#include "PrgAPI.h"
//#include "Language.h"
// CMainTrackPage dialog

IMPLEMENT_DYNAMIC(CMainTrackPage, CTSPropertyPage)

CMainTrackPage::CMainTrackPage(UINT nIDCaption)
	: CTSPropertyPage(CMainTrackPage::IDD, nIDCaption),
	m_bSomethingChanged(FALSE),
	TrackNo(0),
	Year(0),
	m_bLocationEnabled(FALSE)
{
	
}

CMainTrackPage::~CMainTrackPage()
{
}

void CMainTrackPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMainTrackPage, CPropertyPage)
	ON_EN_CHANGE(IDC_ARTISTNAME, &CMainTrackPage::OnChange)
	ON_EN_CHANGE(IDC_ALBUMNAME, &CMainTrackPage::OnChange)
	ON_EN_CHANGE(IDC_GENRENAME, &CMainTrackPage::OnChange)
	ON_EN_CHANGE(IDC_YEARNAME, &CMainTrackPage::OnChange)
	ON_EN_CHANGE(IDC_TRACKNAME, &CMainTrackPage::OnChange)
	ON_EN_CHANGE(IDC_TRACKNO, &CMainTrackPage::OnChange)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()



BOOL CMainTrackPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	PrgAPI* pLang = PRGAPI();
	SetDlgItemText(IDC_ST_COLLECTION, pLang->GetString(IDS_COLLECTION));
	SetDlgItemText(IDC_ST_TITLE, pLang->GetString(IDS_TITLE));
	SetDlgItemText(IDC_ST_TRACKNO, pLang->GetString(IDS_TRACKNO));
	SetDlgItemText(IDC_ST_ALBUM, pLang->GetString(IDS_ALBUM));
	SetDlgItemText(IDC_ST_ARTIST, pLang->GetString(IDS_ARTIST));
	SetDlgItemText(IDC_ST_GENRE, pLang->GetString(IDS_GENRE));
	SetDlgItemText(IDC_ST_YEAR, pLang->GetString(IDS_YEAR));
	SetDlgItemText(IDC_GR_LOCATION, pLang->GetString(IDS_LOCATION));
	SetDlgItemText(IDC_GR_MAINTAGS, pLang->GetString(IDS_MAINTAGS));
	SetDlgItemText(IDC_GR_EXTRATAGS, pLang->GetString(IDS_EXTRATAGS));
	//m_Info.SetBkMode(TRANSPARENT);
	SetDlgItemText(IDC_ARTISTNAME, Artist);
	SetDlgItemText(IDC_ALBUMNAME, Album);
	SetDlgItemText(IDC_GENRENAME, Genre);
	SetDlgItemText(IDC_TRACKNAME, Title);
	if (TrackNo > 0)
	{
		TCHAR bf[100];
		_itot_s(TrackNo, bf, 10);
		SetDlgItemText(IDC_TRACKNO, bf);
	}
	if (Year > 0)
	{
		TCHAR bf[100];
		_itot_s(Year, bf, 10);
		SetDlgItemText(IDC_YEARNAME, bf);
	}
	SetDlgItemText(IDC_LOCATION, Location);
	SetDlgItemText(IDC_COLLECTION, CollectionName);
	if (!m_bLocationEnabled)
	{
		GetDlgItem(IDC_LOCATION)->SendMessage(EM_SETREADONLY, 1, 0);
		GetDlgItem(IDC_LOCATION)->ModifyStyle(WS_TABSTOP, 0, 0);
	}
	EnumChildWindows(m_hWnd, SetFontToChilds, (LPARAM)PRGAPI()->GetFont(FONT_Dialogs)->m_hObject);

	m_bSomethingChanged = FALSE;
	return FALSE;  // return TRUE unless you set the focus to a control
}

void CMainTrackPage::OnChange()
{
	m_bSomethingChanged = TRUE;
}

void CMainTrackPage::OnOK()
{
	GetDlgItemText(IDC_ARTISTNAME, Artist);
	GetDlgItemText(IDC_ALBUMNAME, Album);
	GetDlgItemText(IDC_GENRENAME, Genre);
	GetDlgItemText(IDC_TRACKNAME, Title);
	TCHAR bf[100];
	GetDlgItemText(IDC_TRACKNO, bf, 100);
	TrackNo = _ttoi(bf);
	GetDlgItemText(IDC_YEARNAME, bf, 100);
	Year = _ttoi(bf);
	GetDlgItemText(IDC_LOCATION, Location);
	CPropertyPage::OnOK();
}

