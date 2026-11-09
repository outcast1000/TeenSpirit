#include "StdAfx.h"
#include "TSPropertyPage.h"
#include "PrgAPI.h"


IMPLEMENT_DYNAMIC(CTSPropertyPage, CPropertyPage)

static const COLORREF bkColor = RGB(255,220,150);

CTSPropertyPage::CTSPropertyPage(UINT nIDTemplate, UINT nIDCaption):
CPropertyPage(nIDTemplate),
m_pPPParent(NULL),
m_resourceID(nIDCaption)
{
	m_resourceID = nIDCaption;
	if (m_resourceID != 0)
		m_Caption = PRGAPI()->GetString(m_resourceID);
	m_psp.pszTitle = m_Caption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_EditBrush = CreateSolidBrush(bkColor);
}

HBRUSH CTSPropertyPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = NULL;//CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX)
	{
		hbr = m_EditBrush;//(HBRUSH)GetStockObject(GRAY_BRUSH);
		//pDC->SetBkMode(TRANSPARENT);
		pDC->SetBkColor(bkColor);
		pDC->SetTextColor(RGB(0,0,50));
		//if (!m_bLocationEnabled)
		//{
		//	if (::GetWindowLong(pWnd->m_hWnd, GWL_STYLE) & ES_READONLY)
		//	{
		//		pDC->SetTextColor(RGB(200,0,0));
		//		pDC->SetBkColor(RGB(255,200,200));
		//		hbr = (HBRUSH)GetStockObject(GRAY_BRUSH);
		//	}
		//}
	}
	else
		hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);

	return hbr;
}


CTSPropertyPage::~CTSPropertyPage()
{
	DeleteObject(m_EditBrush);
}

LPCTSTR CTSPropertyPage::GetTitle()
{
	ASSERT(m_resourceID != NULL);
	if (m_resourceID != 0)
		return PRGAPI()->GetString(m_resourceID);
	return _T("");
}

BOOL CTSPropertyPage::PreTranslateMessage(MSG* pMsg)
{
	//TRIPLE CLICK SUPPORT FOR ALL EDITS IN ALL PROPERTY PAGES
	if (IsDoubleClick(pMsg->message)) 
	{
		m_uWhichButton = IsDoubleClick(pMsg->message);	// save which button
		m_uTimeLastDblClk = clock();				// ..and current time
		// update double-click time. This is inefficient, but expedient because
		// don't have to worry about WM_SETTINGCHANGE.
		m_uClocksPerDblClk = GetDoubleClickTime() * CLOCKS_PER_SEC / 1000;

	} 
	else if (IsButtonDown(pMsg->message) && IsButtonDown(pMsg->message)==m_uWhichButton) 
	{
		CPropertyPage::WindowProc(pMsg->message, pMsg->wParam, pMsg->lParam);
		if ((clock() - m_uTimeLastDblClk) < m_uClocksPerDblClk) 
		{
			TCHAR bf[5];
			GetClassName(pMsg->hwnd, bf, 5);
			if (_tcsncmp(bf, _T("Edit"), 4)==0)
				::PostMessage(pMsg->hwnd, EM_SETSEL, 0, -1);
		}
	}

	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CTSPropertyPage::OnHelp()
{
	LPCTSTR ret = GetHelpID();
	if (ret != NULL)
		PRGAPI()->GoToSiteURL(PrgAPI::SA_Help, _T("propertypage"), ret);
}

