#ifndef _CInfoPage_h_
#define _CInfoPage_h_

#include "TSPropertyPage.h"
#include "XHTMLStatic.h"
#include "GuiControls/ColorEdit.h"

class CInfoPage : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CInfoPage)

public:
	CInfoPage(UINT nIDCaption);
	virtual ~CInfoPage();

// Dialog Data
	enum { IDD = IDD_PPINFO };

	BOOL IsSomethingChanged()	{return m_bInitialized && (m_InfoAtStart != info);}
	CString info;

	void EnableFixedFont(BOOL bFixedFont)	{m_bFixedFont = bFixedFont;}
	void EnableReadOnlyMode(BOOL bReadOnly)	{m_bReadOnly = bReadOnly;}

	virtual BOOL OnSetActive();
protected:
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	LRESULT NotifyHyperlinkClicked(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	CXHTMLStatic m_Static;
	CString m_InfoAtStart;
	BOOL m_bInitialized;
	BOOL m_bFixedFont;
	BOOL m_bReadOnly;
	CColorEdit m_edit;

};

#endif