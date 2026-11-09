// DriveDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TeenSpirit.h"
#include "DriveDlg.h"
#include "PrgAPI.h"
//#include "Language.h"

// CDriveDlg dialog

IMPLEMENT_DYNAMIC(CDriveDlg, CDialog)

CDriveDlg::CDriveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDriveDlg::IDD, pParent)
{
	
}

CDriveDlg::~CDriveDlg()
{
}

void CDriveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDriveDlg, CDialog)
END_MESSAGE_MAP()


// CDriveDlg message handlers

BOOL CDriveDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	PrgAPI* pAPI = PRGAPI();
	m_Header.SetIconHandle(pAPI->GetIcon(ICO_Media16));
	m_Header.SetTitleText(pAPI->GetString(IDS_SELECTDRIVE));
	m_Header.SetDescText(pAPI->GetString(IDS_SELECTDRIVEDET));
	m_Header.SetTitleFont(pAPI->GetFont(FONT_DialogsBigBold));
	m_Header.SetDescFont(pAPI->GetFont(FONT_Dialogs));
	m_Header.Init(this);
	m_Header.MoveCtrls(this);
	SetWindowText(pAPI->GetString(IDS_SELECTDRIVE));

	return TRUE;  // return TRUE unless you set the focus to a control
}
