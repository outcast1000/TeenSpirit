// LatestAdditionsPP.cpp : implementation file
//

#include "stdafx.h"
#include "TeenSpirit.h"
#include "LatestAdditionsPP.h"
#include "PrgApi.h"


// CLatestAdditionsPP dialog

IMPLEMENT_DYNAMIC(CLatestAdditionsPP, CPropertyPage)

CLatestAdditionsPP::CLatestAdditionsPP()
	: CSearchPropertyPages(CLatestAdditionsPP::IDD)
{
	m_st.wYear = 0;
}

CLatestAdditionsPP::~CLatestAdditionsPP()
{
}

void CLatestAdditionsPP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DATEPICKER, m_Date);
}


BEGIN_MESSAGE_MAP(CLatestAdditionsPP, CPropertyPage)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATEPICKER, &CLatestAdditionsPP::OnDtnDatetimechangeDatepicker)
END_MESSAGE_MAP()

void CLatestAdditionsPP::ApplyTranslation(ITranslation& translation)
{
	if(m_hWnd)
	{
		PrgAPI* pLang = PRGAPI();
		SetDlgItemText(IDC_INFO, pLang->GetString(IDS_LATESTADDITIONS));
		SetDlgItemText(IDC_SINCE, pLang->GetString(IDS_SINCE));
	}
}

// CLatestAdditionsPP message handlers
BOOL CLatestAdditionsPP::OnInitDialog()
{
	CSearchPropertyPages::OnInitDialog();
	//m_Info.SetWindowText(_T("Returns the <b>latest</b> files that have been added to you collection."));
	if (m_st.wYear > 1500)
		m_Date.SetTime(&m_st);
	//ApplyTranslation();
	return TRUE;  // return TRUE unless you set the focus to a control
}


void CLatestAdditionsPP::OnDtnDatetimechangeDatepicker(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMDATETIMECHANGE pDTChange = reinterpret_cast<LPNMDATETIMECHANGE>(pNMHDR);
	OnChange();
	*pResult = 0;
}

void CLatestAdditionsPP::GetFirstDate(SYSTEMTIME* fDate)
{
	ASSERT(fDate != NULL);
	UpdateData();
	m_Date.GetTime(fDate);
}
void CLatestAdditionsPP::SetFirstDate(SYSTEMTIME* fDate)
{
	ASSERT(fDate != NULL);
	if (m_Date.m_hWnd)
	{
		m_Date.SetTime(fDate);
		UpdateData(FALSE);
	}
	else
		m_st = *fDate;
}
