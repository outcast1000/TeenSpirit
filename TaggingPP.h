#pragma once

#include "TSPropertyPage.h"
#include "afxcmn.h"
#include "afxwin.h"

class CTaggingPP : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CTaggingPP)

public:
	CTaggingPP(UINT nIDCaption);
	virtual ~CTaggingPP();

// Dialog Data
	enum { IDD = IDD_PPTAGGING };
	BOOL IsSomethingChanged()	{return m_bSomethingChanged;}
	virtual void ApplyTranslation(ITranslation& skin); //ICtrlStrings
	virtual LPCTSTR GetHelpID()						{return _T("tagging");}
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnChange();
	virtual void OnOK();
	void SaveChanges();

private:
	BOOL m_bSomethingChanged;
	BOOL m_bNeedsUpdating;

	void LoadTagManagerOptions();
	void SaveTagManagerOptions();
	//void SetControlState();
	void UpdateScrollers();
	void AddString(LPCTSTR text, INT option);

	CSliderCtrl m_rdManual;
	CSliderCtrl m_rdAuto;
	CCheckListBox m_lstRead;
	CCheckListBox m_lstWrite;
};
