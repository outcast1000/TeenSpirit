#pragma once


#include "afxdtctl.h"
#include "SearchPropertyPages.h"

class CLatestAdditionsPP : public CSearchPropertyPages
{
	DECLARE_DYNAMIC(CLatestAdditionsPP)

public:
	CLatestAdditionsPP();
	virtual ~CLatestAdditionsPP();

// Dialog Data
	enum { IDD = IDD_PPLATESTADDITIONS };

	void GetFirstDate(SYSTEMTIME* fDate);
	void SetFirstDate(SYSTEMTIME* fDate);
	//void SetRealParent(CWnd* pParent){m_pParent = pParent;}
	//BOOL IsActive();
	//void SetActive(BOOL bActive);
	virtual void ApplyTranslation(ITranslation& skin); //ICtrlStrings

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnDtnDatetimechangeDatepicker(NMHDR *pNMHDR, LRESULT *pResult);
	//afx_msg void OnChange();

	DECLARE_MESSAGE_MAP()

private:
	SYSTEMTIME m_st;
	CDateTimeCtrl m_Date;
};
