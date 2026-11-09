// AddPaneDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TeenSpirit.h"
#include "AddPaneDlg.h"


// CAddPaneDlg dialog

IMPLEMENT_DYNAMIC(CAddPaneDlg, CDialog)

CAddPaneDlg::CAddPaneDlg(PaneController* pPaneController, CWnd* pParent /*=NULL*/)
	: CDialog(CAddPaneDlg::IDD, pParent),
	m_pPaneControler(pPaneController)
{
	ASSERT(m_pPaneControler != NULL);
}

CAddPaneDlg::~CAddPaneDlg()
{
}

void CAddPaneDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LST_PANES, m_lstPanes);
}


BEGIN_MESSAGE_MAP(CAddPaneDlg, CDialog)
	ON_WM_DESTROY()
	ON_LBN_DBLCLK(IDC_LST_PANES, &CAddPaneDlg::OnLbnDblclkLstPanes)
	ON_BN_CLICKED(IDC_RESET, &CAddPaneDlg::OnBnClickedReset)
END_MESSAGE_MAP()


// CAddPaneDlg message handlers

BOOL CAddPaneDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	PrgAPI* pAPI = PRGAPI();
	//m_Header.SetIconSize(16, 16);
	m_Header.SetIconHandle(pAPI->GetIcon(ICO_Add16));
	m_Header.SetTitleText(pAPI->GetString(IDS_ADDPANE));
	//m_Header.SetDescText(pAPI->GetString(IDS_SELECTDRIVEDET));
	m_Header.SetTitleFont(pAPI->GetFont(FONT_DialogsBigBold));
	m_Header.SetDescFont(pAPI->GetFont(FONT_Dialogs));
	m_Header.Init(this);
	m_Header.MoveCtrls(this);


	//=== Translate Dialog
	SetWindowText(pAPI->GetString(IDS_ADDPANE));
	SetDlgItemText(IDC_RD_UP, pAPI->GetString(IDS_UP));
	SetDlgItemText(IDC_RD_DOWN, pAPI->GetString(IDS_DOWN));
	SetDlgItemText(IDC_RD_LEFT, pAPI->GetString(IDS_LEFT));
	SetDlgItemText(IDC_RD_RIGHT, pAPI->GetString(IDS_RIGHT));
	SetDlgItemText(IDC_GRP_POSITION, pAPI->GetString(IDS_POSITION));

	EnumChildWindows(m_hWnd, SetFontToChilds, (LPARAM)pAPI->GetFont(FONT_Dialogs)->m_hObject);
	m_lstPanes.SetFont(pAPI->GetFont(FONT_Dialogs));


	//=== Fill Item Data
	for (INT i = 0; i < (INT)m_pPaneControler->GetPaneCreatorCount(); i++)
	{
		IPaneCreator* pPC = m_pPaneControler->GetPaneCreator(i);
		if (pPC->IsCreatable())
		{
			LPCTSTR fName = pPC->GetPaneInfo().friendlyName;
			if (fName == NULL)
				fName = pAPI->GetString(IDS_UNKNOWN); 
			INT nItem = m_lstPanes.AddString(fName);
			m_lstPanes.SetItemData(nItem, i);
		}
	}
	CheckRadioButton(IDC_RD_LEFT, IDC_RD_DOWN, IDC_RD_RIGHT);
	m_lstPanes.SetCurSel(0);

	SetDlgItemText(IDC_RESET, pAPI->GetString(IDS_RESETWINDOWLAYOUT));

	return TRUE;
}

void CAddPaneDlg::OnDestroy()
{
	m_paneIndex = m_lstPanes.GetItemData(m_lstPanes.GetCurSel());
	m_position = PaneController::POS_Left;
	if (IsDlgButtonChecked(IDC_RD_RIGHT))
		m_position = PaneController::POS_Right;
	else if (IsDlgButtonChecked(IDC_RD_UP))
		m_position = PaneController::POS_Up;
	else if (IsDlgButtonChecked(IDC_RD_DOWN))
		m_position = PaneController::POS_Down;
	CDialog::OnDestroy();
}

void CAddPaneDlg::OnLbnDblclkLstPanes()
{
	if (m_lstPanes.GetCurSel() != -1)
		OnOK();
}

void CAddPaneDlg::OnBnClickedReset()
{
	m_pPaneControler->LoadDefaultState();
	OnCancel();
}

IPaneCreator* CAddPaneDlg::GetPaneCreator()			
{
	if (m_paneIndex < 0)
		return NULL;
	return m_pPaneControler->GetPaneCreator(m_paneIndex);
}

