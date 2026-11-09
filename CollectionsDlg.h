#ifndef _CCollectionsDlg_h_
#define _CCollectionsDlg_h_
#include "CollectionsList.h"
//#include "DirectorPat/Director.h"
//#include "TransmitterPattern/Transmitter.h"
//#include "XHTMLStatic.h"
#include "LabelEx.h"
#include "GuiControls\FlexDialogHeader.h"
#include "StateManager.h"


class CollectionManager;

class CCollectionsDlg : public CDialog, public TSStateChangeListener
{
	DECLARE_DYNAMIC(CCollectionsDlg)

public:
	CCollectionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCollectionsDlg();
	enum { IDD = IDD_COLLECTIONS };

public:
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()				{return _T("CCollectionsDlg");}

	void UpdateInfo();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnBnClickedExit();
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedUpdate();
	afx_msg LRESULT ListNotification(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	void Refresh();
	void ResizeControls(int cx, int cy);


private:
	HICON m_hIcon;
	CCollectionsList m_List;
	CLabelEx m_info;
	FlexDialogHeader m_header;

};
#endif