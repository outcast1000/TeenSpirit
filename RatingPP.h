#pragma once

#include "SearchPropertyPages.h"

class CRatingPP : public CSearchPropertyPages
{
	DECLARE_DYNAMIC(CRatingPP)

public:
	CRatingPP();
	virtual ~CRatingPP();

// Dialog Data
	enum { IDD = IDD_PPBESTWORSE };


	void SetRatingOption(INT rating, BOOL bOver);
	void GetRatingOption(INT& rating, BOOL& bOver);
	//void SetRealParent(CWnd* pParent){m_pParent = pParent;}
	virtual void ApplyTranslation(ITranslation& skin); //ICtrlStrings
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//afx_msg void OnChange();
protected:
	INT m_Rating;
	BOOL m_bOver;

	DECLARE_MESSAGE_MAP()
public:
	CSliderCtrl m_sldRating;
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
