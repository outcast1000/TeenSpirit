#ifndef _CMainTrackPage_h_
#define _CMainTrackPage_h_

// CMainTrackPage dialog
#include "TSPropertyPage.h"

class CMainTrackPage : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CMainTrackPage)

public:
	CMainTrackPage(UINT nIDCaption);
	virtual ~CMainTrackPage();

// Dialog Data
	enum { IDD = IDD_PPTRACKS };

	BOOL IsSomethingChanged()	{return m_bSomethingChanged;}
	CString Location;
	BOOL m_bLocationEnabled;
	CString CollectionName;

	CString Title;
	UINT TrackNo;
	CString Album;
	CString Artist;
	CString Genre;
	UINT Year;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnChange();
	virtual void OnOK();
	void SaveChanges();
private:
	BOOL m_bSomethingChanged;
};

#endif