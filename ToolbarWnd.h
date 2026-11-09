#ifndef _CMainToolbarDlg_h_
#define _CMainToolbarDlg_h_

#include "afxwin.h"

#include "FlexButton.h"
#include "TSTranslatable.h"
#include "TSSkinnable.h"

#include "GuiControls/BCMenu.h"
#include "PrgAPI.h"

class CToolbarWnd : public CWnd, public TSSkinnable, public TSTranslatable
{
public:
	CToolbarWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CToolbarWnd();

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);



	enum Buttons
	{
		CMD_Search = 1400,
		CMD_History,
		CMD_Options,
		CMD_Collections,
		CMD_Games,
		CMD_About,

		//CMD_DecreaseFontSize,
		//CMD_ResetFontSize,
		//CMD_IncreaseFontSize,
		CMD_SetFontSize,

		CMD_Min,
		CMD_Restore,
		CMD_Close,

		CMD_Last
	};

	//virtual void SetFont(CFont* m_ToolFont);

	//---------------------------------------------------
	//IControl ---------------------------------------
	//---------------------------------------------------
public:
	virtual void ApplyTranslation(ITranslation& translation);
	virtual void ApplySkin(ISkin& skin);

	//---------------------------------------------------
	//ICtrlColors ---------------------------------------
	//---------------------------------------------------
public:
	enum COLORS
	{
		COL_Bk,
		COL_Text,
		COL_ButtonBk,
		COL_ButtonText,
		COL_ButtonOverBk,
		COL_ButtonOverText,
		COL_ButtonDownBk,
		COL_ButtonDownText,
		COL_Last
	};
	virtual void SetColor(UINT idx, COLORREF value);
	virtual COLORREF GetColor(UINT idx);
	//virtual LPCTSTR GetColorName(UINT idx);
private:
	COLORREF m_colors[COL_Last];
	//---------------------------------------------------
	//---------------------------------------------------
	//---------------------------------------------------


protected:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void OnOK(){}
	virtual void OnCancel(){}

	DECLARE_MESSAGE_MAP()

private:
	void ClearMenu();

	FlexButton m_About;

	FlexButton m_Search;
	FlexButton m_History;
	FlexButton m_Options;
	FlexButton m_Collections;
	FlexButton m_Games;

	//FlexButton m_decreaseFontSize;
	//FlexButton m_resetFontSize;
	//FlexButton m_increaseFontSize;
	FlexButton m_setFontSize;

	FlexButton m_min;
	FlexButton m_restore;
	FlexButton m_close;

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	void PositionControls(INT cx, INT cy);
	INT PositionButtonLeft(FlexButton& btn, INT curLeft, INT Height);
	void ConfigureButton(FlexButton& btn, APP_ICONS ico);
	void ConfigureRightButtons(FlexButton& btn, APP_ICONS ico);

	INT m_ToolbarHeight;
	BCMenu m_collectionsMenu;
	BCMenu m_aboutMenu;
	BCMenu m_optionsMenu;
	BCMenu m_fontSizeMenu;
	BOOL m_bPassedOnEraseBk;

};

#endif