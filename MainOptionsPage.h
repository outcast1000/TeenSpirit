#pragma once
#include "TSPropertyPage.h"
#include "afxwin.h"
#include "FlexButton.h"

class CMainOptionsPage : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CMainOptionsPage)

public:
	CMainOptionsPage(UINT nIDCaption);
	virtual ~CMainOptionsPage();

// Dialog Data
	enum { IDD = IDD_PPOPTIONSMAIN };

	BOOL IsSomethingChanged()							{return m_bSomethingChanged;}
	virtual void ApplyTranslation(ITranslation& skin);//

protected:
	virtual LPCTSTR GetHelpID()						{return _T("main_options");}

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnBnClickedCmdLastfm();
	afx_msg void OnChange();
	virtual void OnOK();
	void SaveChanges();
	void LoadMainDlgOptions();
	void SaveMainDlgOptions();

private:
	BOOL m_bSomethingChanged;
	BOOL m_bNeedsUpdating;
	FlexButton m_cmdLastFM;
};
