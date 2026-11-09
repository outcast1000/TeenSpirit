
#include "stdafx.h"
#include "TeenSpirit.h"
#include "MultipleTracksPage.h"
#include "PrgAPI.h"
//#include "Language.h"

IMPLEMENT_DYNAMIC(CMultipleTracksPage, CPropertyPage)


CMultipleTracksPage::CMultipleTracksPage(UINT nIDCaption)
: CTSPropertyPage(CMultipleTracksPage::IDD, nIDCaption)
{

}
CMultipleTracksPage::~CMultipleTracksPage()
{

}

void CMultipleTracksPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PPMTINFO, m_Info);
}

BEGIN_MESSAGE_MAP(CMultipleTracksPage, CPropertyPage)
	ON_BN_CLICKED(IDC_CHK_ALBUM, &CMultipleTracksPage::OnChange)
	ON_BN_CLICKED(IDC_CHK_ARTIST, &CMultipleTracksPage::OnChange)
	ON_BN_CLICKED(IDC_CHK_GENRE, &CMultipleTracksPage::OnChange)
	ON_BN_CLICKED(IDC_CHK_YEAR, &CMultipleTracksPage::OnChange)
	ON_BN_CLICKED(IDC_CHK_COMMENT, &CMultipleTracksPage::OnChange)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


BOOL CMultipleTracksPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	PrgAPI* pLang = PRGAPI();
	SetDlgItemText(IDC_GR_MAINTAGS, pLang->GetString(IDS_MAINTAGS));
	SetDlgItemText(IDC_CHK_ALBUM, pLang->GetString(IDS_ALBUM));
	SetDlgItemText(IDC_CHK_ARTIST, pLang->GetString(IDS_ARTIST));
	SetDlgItemText(IDC_GR_EXTRATAGS, pLang->GetString(IDS_EXTRATAGS));
	SetDlgItemText(IDC_CHK_GENRE, pLang->GetString(IDS_GENRE));
	SetDlgItemText(IDC_CHK_YEAR, pLang->GetString(IDS_YEAR));
	SetDlgItemText(IDC_CHK_COMMENT, pLang->GetString(IDS_COMMENTS));

	SetDlgItemText(IDC_ALBUMNAME, Album);
	SetDlgItemText(IDC_ARTISTNAME, Artist);
	SetDlgItemText(IDC_GENRENAME, Genre);
	SetDlgItemText(IDC_YEARNAME, Year);
	SetDlgItemText(IDC_COMMENT, Comment);

	m_Info.SetWindowText(Info);
	//m_Info.SetBkMode(TRANSPARENT);
	OnChange();
	EnumChildWindows(m_hWnd, SetFontToChilds, (LPARAM)pLang->GetFont(FONT_Dialogs)->m_hObject);
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CMultipleTracksPage::OnChange()
{
	GetDlgItem(IDC_ALBUMNAME)->EnableWindow(IsDlgButtonChecked(IDC_CHK_ALBUM));
	GetDlgItem(IDC_ARTISTNAME)->EnableWindow(IsDlgButtonChecked(IDC_CHK_ARTIST));
	GetDlgItem(IDC_GENRENAME)->EnableWindow(IsDlgButtonChecked(IDC_CHK_GENRE));
	GetDlgItem(IDC_YEARNAME)->EnableWindow(IsDlgButtonChecked(IDC_CHK_YEAR));
	GetDlgItem(IDC_COMMENT)->EnableWindow(IsDlgButtonChecked(IDC_CHK_COMMENT));
}

void CMultipleTracksPage::OnOK()
{
	bAlbum = IsDlgButtonChecked(IDC_CHK_ALBUM);
	bArtist = IsDlgButtonChecked(IDC_CHK_ARTIST);
	bGenre = IsDlgButtonChecked(IDC_CHK_GENRE);
	bYear = IsDlgButtonChecked(IDC_CHK_YEAR);
	bComment = IsDlgButtonChecked(IDC_CHK_COMMENT);
	GetDlgItemText(IDC_ALBUMNAME, Album);
	GetDlgItemText(IDC_ARTISTNAME, Artist);
	GetDlgItemText(IDC_GENRENAME, Genre);
	GetDlgItemText(IDC_YEARNAME, Year);
	GetDlgItemText(IDC_COMMENT, Comment);
	CPropertyPage::OnOK();
}

