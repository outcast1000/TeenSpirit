#ifndef _CMainAlbumsPage_h_
#define _CMainAlbumsPage_h_

#include "TSPropertyPage.h"

class CMainAlbumsPage : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CMainAlbumsPage)

public:
	CMainAlbumsPage(UINT nIDCaption);
	virtual ~CMainAlbumsPage();
// Dialog Data
	enum { IDD = IDD_PPALBUMS };

	BOOL IsSomethingChanged()	{return m_SomethingChanged;}
	CString Album;
	CString Artist;
	UINT Year;
	INT Rating;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnChange();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()

private:
	//LPCTSTR m_TextInfo;
	BOOL m_SomethingChanged;
	CComboBox m_cmbRating;

};

#endif