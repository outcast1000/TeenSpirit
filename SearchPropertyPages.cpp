// RatingPP.cpp : implementation file
//

#include "stdafx.h"
#include "TeenSpirit.h"
#include "SearchDlg.h"
#include "SearchPropertyPages.h"
#include "PrgAPI.h"


CSearchPropertyPages::CSearchPropertyPages(UINT nID):
CPropertyPage(nID),
m_pParent(NULL)
{
}

CSearchPropertyPages::~CSearchPropertyPages()
{
}

BOOL CSearchPropertyPages::OnInitDialog()
{
	CDialog::OnInitDialog();
	EnumChildWindows(m_hWnd, SetFontToChilds, (LPARAM)PRGAPI()->GetFont(FONT_Dialogs)->m_hObject);
	ApplyTranslation(*PRGAPI()->GetTranslationManager());

	//m_Info.SetBkMode(TRANSPARENT);
	return TRUE;  // return TRUE unless you set the focus to a control
}


void CSearchPropertyPages::OnChange()
{
	m_pParent->SendMessage(WM_PAGEUPDATED);
}
BOOL CSearchPropertyPages::OnSetActive()
{
	OnChange();
	return CPropertyPage::OnSetActive();
}

BOOL CSearchPropertyPages::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		if (pMsg->wParam == VK_RETURN)
		{
			OnChange();
			return TRUE;
		}
		else if (pMsg->wParam == VK_ESCAPE)
		{
			m_pParent->PostMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
			return TRUE;
		}
		break;
	}
	return CPropertyPage::PreTranslateMessage(pMsg);
}
