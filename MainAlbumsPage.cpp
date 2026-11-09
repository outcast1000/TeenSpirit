// MainAlbumsPage.cpp : implementation file
//

#include "stdafx.h"
#include "TeenSpirit.h"
#include "MainAlbumsPage.h"
#include "PrgAPI.h"

// MainAlbumsPage dialog

IMPLEMENT_DYNAMIC(CMainAlbumsPage, CPropertyPage)

CMainAlbumsPage::CMainAlbumsPage(UINT nIDCaption)
: CTSPropertyPage(CMainAlbumsPage::IDD, nIDCaption),
m_SomethingChanged(FALSE)
{

}

CMainAlbumsPage::~CMainAlbumsPage()
{
}

void CMainAlbumsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMBRATING, m_cmbRating);
}


BEGIN_MESSAGE_MAP(CMainAlbumsPage, CPropertyPage)
	ON_EN_CHANGE(IDC_ALBUMNAME,		&CMainAlbumsPage::OnChange)
	ON_EN_CHANGE(IDC_ARTISTNAME,	&CMainAlbumsPage::OnChange)
	ON_EN_CHANGE(IDC_YEAR,			&CMainAlbumsPage::OnChange)
	ON_CBN_SELCHANGE(IDC_CMBRATING, &CMainAlbumsPage::OnChange)
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CMainAlbumsPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	PrgAPI* pAPI = PRGAPI();
	SetDlgItemText(IDC_GR_MAINTAGS, pAPI->GetString(IDS_MAINTAGS));
	SetDlgItemText(IDC_ST_ALBUM, pAPI->GetString(IDS_ALBUM));
	SetDlgItemText(IDC_ST_ARTIST, pAPI->GetString(IDS_ARTIST));
	SetDlgItemText(IDC_ST_YEAR, pAPI->GetString(IDS_YEAR));
	SetDlgItemText(IDC_GR_EXTRATAGS, pAPI->GetString(IDS_EXTRATAGS));
	SetDlgItemText(IDC_ST_RATING, pAPI->GetString(IDS_RATING));
	for (int i = 0; i < 6; i++)
		m_cmbRating.AddString(pAPI->GetStringForStars(i));
	m_cmbRating.SetCurSel(Rating == 0 ? 0 : (INT)Rating2Stars(Rating));
	SetDlgItemText(IDC_ALBUMNAME, Album);
	SetDlgItemText(IDC_ARTISTNAME, Artist);
	if (Year > 0)
	{
		TCHAR bf[100];
		_itot(Year, bf, 10);
		SetDlgItemText(IDC_YEAR, bf);
	}
	m_SomethingChanged = FALSE;
	EnumChildWindows(m_hWnd, SetFontToChilds, (LPARAM)pAPI->GetFont(FONT_Dialogs)->m_hObject);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CMainAlbumsPage::OnChange()
{
	m_SomethingChanged = TRUE;
}

void CMainAlbumsPage::OnDestroy()
{
	CTSPropertyPage::OnDestroy();
	GetDlgItemText(IDC_ALBUMNAME, Album);
	GetDlgItemText(IDC_ARTISTNAME, Artist);
	TCHAR YearName[100];
	GetDlgItemText(IDC_YEAR, YearName, 100);
	Year = _ttoi(YearName); 
	INT curSel = m_cmbRating.GetCurSel();
	Rating = (curSel == 0) ? 0 : Stars2Rating((FLOAT)curSel);
}
