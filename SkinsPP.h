#ifndef _SkinsPP_h_
#define _SkinsPP_h_
#pragma once
#include "TSPropertyPage.h"
#include "XHTMLStatic.h"
#include "FlexButton.h"
#include "GuiControls/DualTextComboBox.h"
#include "SkinManager.h"
#include "afxwin.h"
#include "GuiControls/PicWnd.h"

class CSkinsPP : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CSkinsPP)

public:
	CSkinsPP(UINT nIDCaption);
	virtual ~CSkinsPP();

// Dialog Data
	enum { IDD = IDD_PPSKINS};

	BOOL IsSomethingChanged()							{return m_bSomethingChanged;}
	virtual void ApplyTranslation(ITranslation& skin);//
	virtual LPCTSTR GetHelpID()						{return _T("skin");}

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
private:
	void GetNameFromComboSelection(LPTSTR lngName, UINT bfLen);
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
	afx_msg void OnBnClickedCreateskin();

	afx_msg void OnBnClickedUpdateSkin();
	afx_msg void OnBnClickedOpenfolder();

	BOOL GetScreenshot(LPCTSTR path);

private:
	BOOL m_bSomethingChanged;
	BOOL m_bNeedsUpdating;
	CDualTextComboBox m_cmbSkins;
	CXHTMLStatic m_info;
	//CButton /*FlexButton*/ m_download;
	//CButton/*FlexButton*/ m_help;
	//CButton /*FlexButton*/ m_updateSkin;
	//CButton/*FlexButton*/ m_openFolder;
	CPicWnd m_screenShot;
public:
	//afx_msg void OnStnClickedScreenshot();
	afx_msg void OnBnClickedCreatepreview();
};

#endif
