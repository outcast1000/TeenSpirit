#ifndef _CMultipleTracksPage_h_
#define _CMultipleTracksPage_h_

#include "TSPropertyPage.h"
#include "XHTMLStatic.h"

class CMultipleTracksPage : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CMultipleTracksPage)

public:
	CMultipleTracksPage(UINT nIDCaption);
	virtual ~CMultipleTracksPage();

// Dialog Data
	enum { IDD = IDD_PPMULTIPLETRACKS };

	CString Album;
	CString Artist;
	CString Genre;
	CString Year;
	CString Comment;
	BOOL bAlbum;
	BOOL bArtist;
	BOOL bGenre;
	BOOL bYear;
	BOOL bComment;

	CString Info;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnChange();
	virtual void OnOK();
private:
	CXHTMLStatic m_Info;
};
#endif