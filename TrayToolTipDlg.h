#ifndef _TrayToolTipDlg_h_
#define _TrayToolTipDlg_h_

#include "FlexButton.h"
#include "DataRecords.h"
#include "FlickerFree.h"
#include "MediaSlider.h"
#include "StateManager.h"

class TrayToolTipDlg : public CWnd, public MediaSliderListener, TSStateChangeListener
{
	static const INT s_marginWidth;
	static const INT s_normalButtonWidth;
	static const INT s_systemButtonWidth;
	static const INT s_distanceBetweenButtonsWidth;
	static const INT s_volumeWidth;

	static const INT s_marginHeight;
	static const INT s_internalMarginHeight;

	static const INT s_buttonHeight;
	static const INT s_sliderHeight;

	//=== Deign s_marginWidth - PictureWidth - s_marginWidth -	titleWidth			- s_marginWidth
	//														 -	space - buttons 
	//	s_marginHeight - title height - s_buttonHeight - s_sliderHeight - s_marginHeight

public:
	TrayToolTipDlg();
	virtual ~TrayToolTipDlg();

public:

	//=== nAutoHideTimeOut: secs for autohide
	//=== bCloseOnMouse: Autohide when the cursor moves or when it moves out of the window
	//BOOL Show(DWORD nAutoHideTimeOut, BOOL bInvalidateData);
	BOOL Show(DWORD nAutoHideTimeOut);
	void OnMediaChanged(BOOL bShowWindow);
	void OnMouseOverTray(CPoint& p);

	void SetFadeInDuration(INT duration);
	INT GetFadeInDuration();
	void SetFadeOutDuration(INT duration);
	INT GetFadeOutDuration();
	void SetTransparency(INT trans);
	INT GetTransparency();
	void SetMaxWidth(INT maxWidth)		{m_maxWidth = maxWidth;}
	INT GetMaxWidth()					{return m_maxWidth;}
	void SetMinWidth(INT minWidth)		{m_minWidth = minWidth;}
	INT GetMinWidth()					{return m_minWidth;}
	//void SetTitleHeight(INT height)		{m_titleHeight = height;}
	//INT GetTitleHeight()				{return m_titleHeight;}
	INT GetTotalHeight()				{return 
		s_marginHeight + m_fontHeight[FNT_AppTitle] +
		/*s_marginHeight + */m_fontHeight[FNT_Title] +
		/*s_marginHeight + */m_fontHeight[FNT_Subtitle] +
		/*s_marginHeight + */s_buttonHeight + 
		/*s_marginHeight + */s_sliderHeight +
		s_marginHeight +
		s_internalMarginHeight * 4;
	}


	virtual void OnMediaSliderClick(CMediaSlider* pMediaSlider);

	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()	{return _T("TrayToolTipDlg");}




public:
	enum COLORS
	{
		COL_Transparency,
		COL_Bk,
		COL_Bk2,
		COL_Border,
		COL_TitleText,
		COL_SubTitleText,
		COL_PictureBorder,
		COL_Last
	};

	virtual void SetColor(UINT idx, COLORREF value);
	virtual COLORREF GetColor(UINT idx);
private:
	void UpdateSliders();
	void ApplyTransparencyAndBorder(CDC& dc);
	void Hide();
	void Draw(CDC& dc);
	BOOL Create(CWnd* pParent);
	COLORREF m_colors[COL_Last];
	//---------------------------------------------------
	//---------------------------------------------------
	//---------------------------------------------------

protected:
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()

private:
	enum TIMERS
	{
		TMR_HideOnMouse = 500,
		TMR_Transparency,
		TMR_MediaChanged,
		TMR_UpdateSliders,
		TMR_Last
	};
	enum FONTS
	{
		FNT_AppTitle,
		FNT_Title,
		FNT_Subtitle,
		FNT_Text,
		FNT_Last
	};
	CFont m_font[FNT_Last];
	INT m_fontHeight[FNT_Last];
	CPoint m_cursorPos;
	INT m_transparency;
	INT m_transparencyStep;
	INT m_transparencyStepDelay;
	INT m_realTransparency;
	INT m_maxWidth;
	INT m_minWidth;
	INT m_windowWidth;
	//INT m_titleHeight;
	INT m_pictureWidth;


	enum State
	{
		S_Showed,
		S_Hidden,
		S_Showing,
		S_Hiding,
		S_Last
	} m_state;

	DWORD m_tickCountToClose;
	BOOL m_bDataAreInvalid;

	enum PlayButtons
	{
		BTN_Prev,
		BTN_Play,
		BTN_Pause,
		BTN_Stop,
		BTN_Next,
		BTN_Last
	};
	FlexButton m_buttons[BTN_Last];

	enum SystemButtons
	{
		SBTN_QuickSearch,
		SBTN_Last
	};
	FlexButton m_systemButtons[SBTN_Last];
	FlexButton m_closeButton;

	void SetRealTransparency(INT transparency);
	BOOL CalculateNewWindowLeftTop(INT& x, INT& y);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	void ConfigureButton(FlexButton& button);
	CMediaSlider m_positionSlider;
	CMediaSlider m_volumeSlider;


	CFlickerFree m_ff;
};

#endif
