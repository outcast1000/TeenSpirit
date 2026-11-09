// PPDetailedSearch.cpp : implementation file
//

#include "stdafx.h"
#include "TeenSpirit.h"
#include "DetailedSearchCHdlg.h"
#include "PrgApi.h"


// CPPDetailedSearch dialog

IMPLEMENT_DYNAMIC(CDetailedSearchCHdlg, CPropertyPage)

CDetailedSearchCHdlg::CDetailedSearchCHdlg()
	: CSearchPropertyPages(CDetailedSearchCHdlg::IDD)
{

}

CDetailedSearchCHdlg::~CDetailedSearchCHdlg()
{
}

void CDetailedSearchCHdlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}



BEGIN_MESSAGE_MAP(CDetailedSearchCHdlg, CSearchPropertyPages)
	ON_BN_CLICKED(IDC_SEARCH, &CDetailedSearchCHdlg::OnChange)
END_MESSAGE_MAP()

void CDetailedSearchCHdlg::ApplyTranslation(ITranslation& translation)
{
	if (m_hWnd)
	{
		PrgAPI* pLang = PRGAPI();
		SetDlgItemText(IDC_ST_ARTIST, pLang->GetString(IDS_ARTIST));
		SetDlgItemText(IDC_ST_ALBUM, pLang->GetString(IDS_ALBUM));
		SetDlgItemText(IDC_ST_TITLE, pLang->GetString(IDS_TITLE));
		SetDlgItemText(IDC_ST_LOCATION, pLang->GetString(IDS_LOCATION));
		SetDlgItemText(IDC_SEARCH, pLang->GetString(IDS_SEARCH));
	}
}

BOOL CDetailedSearchCHdlg::OnInitDialog()
{
	CSearchPropertyPages::OnInitDialog();
	//ApplyTranslation();
	return TRUE;  // return TRUE unless you set the focus to a control
}

LPCTSTR CDetailedSearchCHdlg::GetArtist()
{
	if (m_hWnd)
		GetDlgItemText(IDC_EDT_ARTIST, m_Artist, CDetailedSearchCHdlgLEN);
	else
		m_Artist[0] = 0;
	return m_Artist;
}
LPCTSTR CDetailedSearchCHdlg::GetAlbum()
{
	if (m_hWnd)
		GetDlgItemText(IDC_EDT_ALBUM, m_Album, CDetailedSearchCHdlgLEN);
	else
		m_Album[0] = 0;
	return m_Album;
}
LPCTSTR CDetailedSearchCHdlg::GetTitle()
{
	if (m_hWnd)
		GetDlgItemText(IDC_EDT_TITLE, m_Title, CDetailedSearchCHdlgLEN);
	else
		m_Title[0] = 0;
	return m_Title;
}
LPCTSTR CDetailedSearchCHdlg::GetLocation()
{
	if (m_hWnd)
		GetDlgItemText(IDC_EDT_LOCATION, m_Location, CDetailedSearchCHdlgLEN);
	else
		m_Location[0] = 0;
	return m_Location;
}