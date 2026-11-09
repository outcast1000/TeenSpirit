#pragma once
#include "TSTranslatable.h"

class CSearchPropertyPages : public CPropertyPage, public TSTranslatable
{
public:
	explicit CSearchPropertyPages(UINT nID);
	virtual ~CSearchPropertyPages();

	void SetRealParent(CWnd* pParent){m_pParent = pParent;}

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnChange();

private:
	CWnd* m_pParent;

};
