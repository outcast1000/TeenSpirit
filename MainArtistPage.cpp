// MainArtistsPage.cpp : implementation file
//

#include "stdafx.h"
#include "TeenSpirit.h"
#include "MainArtistPage.h"
#include "PrgAPI.h"


IMPLEMENT_DYNAMIC(CMainArtistPage, CPropertyPage)

CMainArtistPage::CMainArtistPage(UINT nIDCaption)
	: CTSPropertyPage(CMainArtistPage::IDD, nIDCaption),
	m_SomethingChanged(FALSE)
{
}

CMainArtistPage::~CMainArtistPage()
{
}

void CMainArtistPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMBRATING, m_cmbRating);
}

BEGIN_MESSAGE_MAP(CMainArtistPage, CPropertyPage)
	ON_EN_CHANGE(IDC_ARTISTNAME, &CMainArtistPage::OnChange)
	ON_EN_CHANGE(IDC_GENRENAME, &CMainArtistPage::OnChange)
	ON_CBN_SELCHANGE(IDC_CMBRATING, &CMainArtistPage::OnChange)
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CMainArtistPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	//Construct Dialog
	PrgAPI* pAPI = PRGAPI();
	SetDlgItemText(IDC_GR_MAINTAGS, pAPI->GetString(IDS_MAINTAGS));
	SetDlgItemText(IDC_ST_ARTIST, pAPI->GetString(IDS_ARTIST));
	SetDlgItemText(IDC_ST_GENRE, pAPI->GetString(IDS_GENRE));
	SetDlgItemText(IDC_GR_EXTRATAGS, pAPI->GetString(IDS_EXTRATAGS));
	SetDlgItemText(IDC_ST_RATING, pAPI->GetString(IDS_RATING));
	for (int i = 0; i < 6; i++)
		m_cmbRating.AddString(pAPI->GetStringForStars(i));
	EnumChildWindows(m_hWnd, SetFontToChilds, (LPARAM)pAPI->GetFont(FONT_Dialogs)->m_hObject);

	SetDlgItemText(IDC_ARTISTNAME, Artist);
	SetDlgItemText(IDC_GENRENAME, Genre);
	m_cmbRating.SetCurSel(Rating == 0 ? 0 : (INT)Rating2Stars(Rating));
	//m_Info.SetBkMode(TRANSPARENT);
	m_SomethingChanged = FALSE;

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CMainArtistPage::OnChange()
{
	m_SomethingChanged = TRUE;
}


void CMainArtistPage::OnDestroy()
{
	CTSPropertyPage::OnDestroy();
	GetDlgItemText(IDC_ARTISTNAME, Artist);
	GetDlgItemText(IDC_GENRENAME, Genre);
	INT curSel = m_cmbRating.GetCurSel();
	Rating = (curSel == 0) ? 0 : Stars2Rating((FLOAT)curSel);
}
