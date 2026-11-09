#ifndef _MiniPlayerPP_h_
#define _MiniPlayerPP_h_
#pragma once
#include "TSPropertyPage.h"
#include "afxwin.h"

class CMiniPlayerPP : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CMiniPlayerPP)

public:
	CMiniPlayerPP(UINT nIDCaption);
	virtual ~CMiniPlayerPP();

// Dialog Data
	enum { IDD = IDD_PPMINIPLAYER };

	BOOL IsSomethingChanged()							{return m_bSomethingChanged;}
	virtual void ApplyTranslation(ITranslation& skin);//
	virtual LPCTSTR GetHelpID()						{return _T("mini_player");}

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnChange();
	virtual void OnOK();
	void SaveChanges();

private:
	BOOL m_bSomethingChanged;
	BOOL m_bNeedsUpdating;
	CSliderCtrl m_sldTransparency;
	CSliderCtrl m_sldMaxSize;
	//CComboBox m_cmbVisibility;
//Main Dlg
	void LoadOptions();
	void SaveOptions();
public:
};

#endif
