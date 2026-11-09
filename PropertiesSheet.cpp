

#include "stdafx.h"
#include "TeenSpirit.h"
#include "PropertiesSheet.h"
#include "PrgAPI.h"

// CPropertiesSheet

IMPLEMENT_DYNAMIC(CPropertiesSheet, CTreePropSheet)

CPropertiesSheet::CPropertiesSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CTreePropSheet(nIDCaption, pParentWnd, iSelectPage)
{

}

CPropertiesSheet::CPropertiesSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CTreePropSheet(pszCaption, pParentWnd, iSelectPage)
{
}


CPropertiesSheet::~CPropertiesSheet()
{
}


BEGIN_MESSAGE_MAP(CPropertiesSheet, CTreePropSheet)
END_MESSAGE_MAP()

BOOL CPropertiesSheet::OnInitDialog ()
{
	BOOL bResult = CTreePropSheet::OnInitDialog ();

	CTreeCtrl* pTree = GetPageTreeControl();
	pTree->SetBkColor(RGB(248,248,248));
	pTree->SetTextColor(RGB(0,0,0));
	pTree->SetIndent(0);


	PrgAPI* pAPI = PRGAPI();
	EnumChildWindows(m_hWnd, SetFontToChilds, (LPARAM)pAPI->GetFont(FONT_Dialogs)->m_hObject);
	m_header.SetDraggable(TRUE);
	m_header.SetIconSize(48, 48);
	m_header.SetTitleFont(pAPI->GetFont(FONT_DialogsBigBold));
	m_header.SetDescFont(pAPI->GetFont(FONT_Dialogs));

	if (m_header.GetIconHandle() == NULL)
		m_header.SetIconHandle(pAPI->GetIcon(ICO_Main));
	m_header.Init(this);
	m_header.MoveCtrls(this);


	return bResult;
}

