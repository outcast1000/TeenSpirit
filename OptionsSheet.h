#pragma once


#include "TSTranslatable.h"
#include "TreePopSheet/TreePropSheet.h"
#include "GuiControls/FlexDialogHeader.h"


class CButtonX: public CButton
{
public:
	CButtonX()					{}
	virtual ~CButtonX()	{}
	DECLARE_MESSAGE_MAP()
protected:
	afx_msg void OnPaint();
};

class CTSPropertyPage;
using namespace TreePropSheet;
class COptionsSheet : public CTreePropSheet,public TSTranslatable
{

public:
	COptionsSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~COptionsSheet();
	virtual void ApplyTranslation(ITranslation& skin);

	void AddTSPropertyPage(CTSPropertyPage* page);
	void SetStartUpPage(UINT m_nIDCaption);

	FlexDialogHeader& GetHeaderControl()	{return m_header;}

protected:
	DECLARE_MESSAGE_MAP()
	virtual void AddPage(CPropertyPage* pPage)	{}//Overriden. Use AddTSPropertyPage
	virtual BOOL OnInitDialog();
	virtual afx_msg void OnHelp();

private:
	std::vector<CTSPropertyPage*> m_pages;
	UINT m_nIDCaption;
	FlexDialogHeader m_header;

	CButtonX m_helpButton;

};


