#pragma once

#include "DataRecords.h"
#include "afxwin.h"
#include "GuiControls\FlexDialogHeader.h"

// CRenamerDlg dialog

class CRenamerDlg : public CDialog
{
	DECLARE_DYNAMIC(CRenamerDlg)

public:
	CRenamerDlg(FullTrackRecordCollection& col, CWnd* pParent = NULL);   // standard constructor
	virtual ~CRenamerDlg();

// Dialog Data
	enum { IDD = IDD_RENAMER };

	void SetFullTrackRecordCollection(FullTrackRecordCollection& col);
	BOOL IsSomethingChanged()	{return m_bSomethingChanged;}

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedRename();
	afx_msg void OnBnClickedSkip();
	afx_msg void OnBnClickedAll();
	afx_msg void OnBnClickedCancel();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	BOOL PrepareNextTrack();

private:
	BOOL m_bSomethingChanged;
	CListCtrl m_lstTags;
	CComboBox m_cmbCasing;
	CComboBox m_cmbPattern;
	FullTrackRecordCollection& m_col;
	FlexDialogHeader m_header;
public:
	afx_msg void UpdatePreview();
	afx_msg void UpdatePreviewDelayed();
	BOOL GetNewFileName(LPTSTR bf, UINT bfLen);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	BOOL RenameTrack(FullTrackRecordSP& rec, LPCTSTR pat);
	void InsertItem(INT rowID, LPCTSTR id, INT descID, LPCTSTR curVal);


};
