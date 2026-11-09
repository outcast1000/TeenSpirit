#pragma once


#include "TSPropertyPage.h"
class CInfoRetrievalPP: public CTSPropertyPage
{
	DECLARE_DYNAMIC(CInfoRetrievalPP)

public:
	CInfoRetrievalPP(UINT nIDCaption);
	virtual ~CInfoRetrievalPP();

// Dialog Data
	enum { IDD = IDD_PPINTERNETOPTIONS };

public:
	BOOL IsSomethingChanged()	{return m_bSomethingChanged;}

public://
	virtual void ApplyTranslation(ITranslation& skin);
	virtual LPCTSTR GetHelpID()						{return _T("information_retrieval");}


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnChange();
	virtual void OnOK();
	void SaveChanges();

private:
	BOOL m_bSomethingChanged;
	BOOL m_bNeedsUpdating;

	void LoadNetSessionOptions();
	void SaveNetSessionOptions();

	void LoadInfoMgrOptions();
	void SaveInfoMgrOptions();
};
