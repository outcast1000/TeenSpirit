#ifndef _CMainArtistPage_h_
#define _CMainArtistPage_h_

#include "TSPropertyPage.h"

class CMainArtistPage : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CMainArtistPage)

public:
	CMainArtistPage(UINT nIDCaption);
	virtual ~CMainArtistPage();

// Dialog Data
	enum { IDD = IDD_PPARTISTS };
	BOOL IsSomethingChanged()	{return m_SomethingChanged;}
	BOOL SaveChanges();
	CString Artist;
	CString Genre;
	INT Rating;

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnChange();
private:
	BOOL m_SomethingChanged;
	CComboBox m_cmbRating;
	//static const int cMaxPath = 100;
	//TCHAR ArtistName[cMaxPath];
	//TCHAR GenreName[cMaxPath];
	//int Rating;
};

#endif