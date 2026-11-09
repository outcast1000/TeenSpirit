#ifndef _CMultiPicturePage_h_
#define _CMultiPicturePage_h_

#include "GuiControls/PicWndEx.h"
#include "GuiControls/BCMenu.h"
#include "TSPropertyPage.h"
#include "FlexButton.h"
class CMultiPicturePage : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CMultiPicturePage)

public:
	enum PicStatus
	{
		PS_Unknown,
		PS_Normal,
		PS_Removed,
		PS_AddedByFile,
		PS_AddedByClipboard,
		PS_Last
	};

	struct PicInfo
	{
		LPCTSTR path;
		PicStatus status;
		BOOL bDefault;
	};

	CMultiPicturePage(UINT nIDCaption);
	virtual ~CMultiPicturePage();

	void AddPicture(const PicInfo& picInfo);
	BOOL GetPicture(UINT idx, PicInfo& picInfo);
	UINT GetPictureCount()	{return m_pics.size();}

	//void SetPicInfo(InformationManager::PICTURE_ITEM_TYPE pit, UINT ItemID);
	//void SetTitles(LPCTSTR Title, LPCTSTR SubTitle);
	//void EnableButtons(BOOL bOpenButton, BOOL bDownloadButton, BOOL bPasteButton);

// Dialog Data
	enum { IDD = IDD_PPPICTURE };
	virtual BOOL IsSomethingChanged()	{return m_bInitialized && m_bSomethingChanged;}

protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
//	LRESULT NotifyHyperlinkClicked(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
private:
	void Refresh();
private:
	CPicWndEx m_Pic;
	enum Buttons
	{
		B_Add,
		B_Prev,
		B_Next,
		B_Actions,
		B_Last
	};
	FlexButton m_buttons[B_Last];
	BOOL m_bInitialized;
	BOOL m_bSomethingChanged;


	struct PicInfoStore
	{
		std::tstring path;
		PicStatus status;
		BOOL bDefault;
	};
	std::vector<PicInfoStore> m_pics;
	INT m_curPic;
	BCMenu m_actionMenu;
	BCMenu m_addMenu;
public:
	afx_msg void OnBnClickedRdDefault();
};

#endif