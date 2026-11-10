#ifndef _QuickSearchDlg_h_
#define _QuickSearchDlg_h_

#include "TSTranslatable.h"
#include "TSSkinnable.h"
//#include "LabelEx.h"
#include "GuiControls/IconEditEx.h"
#include "TracksFilter.h"
#include "TeenSpiritEnums.h"
#include "TrackListCtrl.h"
#include "FlexButton.h"

class CIconEditEx;

class QuickSearchDlg :	public CWnd,
							public ISkinnable, 
							public ITranslatable
{
public:
	QuickSearchDlg();
	virtual ~QuickSearchDlg();

public:
	enum RelativePosition
	{
		RP_Centered,
		RP_Outside
	};

	void ShowRelative(CWnd* wndRelative, RelativePosition rp);
	void SetSearchText(LPCTSTR searchText);
	//INT GetDesiredHeight();

	virtual void ApplyTranslation(ITranslation& skin);
	virtual void ApplySkin(ISkin& skin);

public:
	enum COLORS
	{
		COL_Bk,
		COL_Text,
		COL_Last
	};

	virtual void SetColor(UINT idx, COLORREF value);
	virtual COLORREF GetColor(UINT idx);
private:
	COLORREF m_colors[COL_Last];
	//---------------------------------------------------
	//---------------------------------------------------
	//---------------------------------------------------

protected:
	BOOL Create(CWnd* pParent);
	void Resize(int cx, int cy);
	void SetTitleHeight(INT height);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEnChangeSearchedt();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	DECLARE_MESSAGE_MAP()
private:
	void HideWindow();
	CIconEditEx m_searchEdit;
	TrackListCtrl m_trackList;
	FullTrackRecordCollection m_tracks;
	INT m_titleHeight;
	//INT m_records;
	std::tstring m_initialSearchText;
	FlexButton m_closeButton;

	BOOL m_bCloseOnAction;
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};

#endif
