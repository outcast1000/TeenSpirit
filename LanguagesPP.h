#ifndef _LanguagesPP_h_
#define _LanguagesPP_h_

#pragma once

#include "TSPropertyPage.h"
#include "XHTMLStatic.h"
#include "FlexButton.h"
#include "GuiControls/DualTextComboBox.h"

class CLanguagesPP : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CLanguagesPP)

public:
	CLanguagesPP(UINT nIDCaption);
	virtual ~CLanguagesPP();

// Dialog Data
	enum { IDD = IDD_PPLANGUAGES };

	BOOL IsSomethingChanged()							{return m_bSomethingChanged;}
	virtual void ApplyTranslation(ITranslation& skin);//
	virtual LPCTSTR GetHelpID()						{return _T("language");}

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
private:
//	void GetNameFromComboSelection(LPTSTR lngName, UINT bfLen);
	afx_msg void OnChange();
	void SaveChanges();
	void CreateLangFile(BOOL bUpdateCurrent);
	void LoadOptions();
	void SaveOptions();
	void RefreshInfo();
	afx_msg void OnBnClickedCreate();
	//afx_msg void OnBnClickedDownload();
	//afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedUpdate();
	afx_msg void OnBnClickedOpenfolder();
	afx_msg void OnBnClickedDebug();

private:
	BOOL m_bSomethingChanged;
	BOOL m_bNeedsUpdating;
	CDualTextComboBox m_cmbTranslations;
	CXHTMLStatic m_info;
	//CButton m_download;
	//CButton m_create;
	//CButton m_update;
	//CButton m_help;
};

#endif
