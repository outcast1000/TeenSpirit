#ifndef _CSimpleSearchCHdlg_h_
#define _CSimpleSearchCHdlg_h_

// CSimpleSearchCHdlg dialog
#include "SearchPropertyPages.h"

class CSimpleSearchCHdlg : public CSearchPropertyPages
{
	DECLARE_DYNAMIC(CSimpleSearchCHdlg)

public:
	CSimpleSearchCHdlg();
	virtual ~CSimpleSearchCHdlg();

	enum FILETYPE_OPTIONS
	{
		FTO_First,
		FTO_All,
		FTO_Audio,
		FTO_Video,
		FTO_Last
	};
// Dialog Data
	enum { IDD = IDD_PPSIMPLESEARCH };

	void GetSimpleSearchText(LPTSTR bf, UINT len);
	void SetSimpleSearchText(LPCTSTR bf);
	FILETYPE_OPTIONS GetFileTypeOption();
	void SetFileTypeOption(FILETYPE_OPTIONS fto);

	//void SetRealParent(CWnd* pParent){m_pParent = pParent;}
	virtual void ApplyTranslation(ITranslation& skin); //ICtrlStrings
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

private:
	//void ShowAllCheckChanged();
	FILETYPE_OPTIONS m_fto;
public:
	afx_msg void OnBnClickedSearch();
};

#endif