#ifndef _CFullTextSearchCHdlg_h_
#define _CFullTextSearchCHdlg_h_

#include "SearchPropertyPages.h"

class CFullTextSearchCHdlg : public CSearchPropertyPages
{
	DECLARE_DYNAMIC(CFullTextSearchCHdlg)

public:
	CFullTextSearchCHdlg();
	virtual ~CFullTextSearchCHdlg();

	enum TEXT_TYPES
	{
		TT_First,
		TT_Lyrics,
		TT_Comments,
		TT_Personal,
		TT_AlbumReview,
		TT_ArtistBio,
		TT_Path,
		TT_Last
	};
// Dialog Data
	enum { IDD = IDD_PPFULLTEXT };

	void GetSearchText(LPTSTR bf, UINT len);
	void SetSearchText(LPCTSTR bf);
	TEXT_TYPES GetTextTypeOption();
	void SetTextTypeOption(TEXT_TYPES fto);
	virtual void ApplyTranslation(ITranslation& skin); //ICtrlStrings
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()

private:
	TEXT_TYPES m_tto;
};

#endif