#pragma once

#include "afxcmn.h"
#include "FlexButton.h"
#include "ICtrlColors.h"
#include "Label.h"
#include "LabelEx.h"
#include "TSTranslatable.h"
#include "TSSkinnable.h"
#include "CollectionUpdateJob.h"
#include "LevelMeter.h"
#include "MediaSlider.h"
#include "CtrlColorsMediaSliderAdapter.h"
#include "HTMLLite/PPTooltip.h"
#include "GuiControls/BCMenu.h"
#include "StateManager.h"



class MediaPlayer;
// CStatusBarChDlg dialog
//class IVisualizationContainer;

class CPlayerBarWnd : public CWnd, 
	public TSSkinnable, public TSTranslatable, public TSStateChangeListener,
	public MediaSliderListener
{
public:
	CPlayerBarWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPlayerBarWnd();

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	
	void Update();
	void SetMediaPlayer(MediaPlayer* pPlayer);
	virtual void ApplySkin(ISkin& skin); //TSSkinnable
	virtual void ApplyTranslation(ITranslation& skin); //TSTranslatable
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()				{return _T("CPlayerBarWnd");}
	virtual void OnMediaSliderClick(CMediaSlider* pMediaSlider);



	//---------------------------------------------------
	//ICtrlColors ---------------------------------------
	//#include "ICtrlColors.h" // ,public ICtrlColors
	//---------------------------------------------------
public:
	enum COLORS
	{
		COL_Bk,
		COL_Text,
		COL_Slider1,
		COL_Slider2,
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
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnSliderMessage( WPARAM wParam, LPARAM lParam );
	//afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()

private:
	//void Resize(int cx = 0, int cy = 0);
	void ConfigureButton(FlexButton& btn, HICON hIcon);
	void ResizePlayerControls();
	void SetTimeInfo(DOUBLE pos, DOUBLE total);
	void ResizeWindow(int cx, int cy);
	void UpdateWindowText(CWnd& wnd, LPCTSTR text);



	enum ControlsEnum
	{
		CTRL_Previous = 667,
		CTRL_Play,
		CTRL_Pause,
		CTRL_Stop,
		CTRL_Next,
		CTRL_CrossFade,
		CTRL_Equalizer,
		CTRL_Slider,
		CTRL_SldVolume,
		CTRL_VolMeterL,
		CTRL_VolMeterR,
		CTRL_TimeInfo,
		CTRL_FullTimeInfo,
		CTRL_PlayerInfo,
		CTRL_Mute,
		CTRL_Last
	};

private:
	CFont m_MainFont;
//From Player
	//[Subclassed Controls]
	//FlexButton m_start;


	FlexButton m_previous;
	FlexButton m_play;
	FlexButton m_pause;
	FlexButton m_stop;
	FlexButton m_next;

	FlexButton m_mute;
	FlexButton m_crossfade;
	FlexButton m_equalizer;

	static const INT cSldEqCount = 3;
	CMediaSlider m_sldEq[cSldEqCount];

	CLevelMeter m_volMeterL;
	CLevelMeter m_volMeterR;
	
	CMediaSlider m_volumeSlider;
	CMediaSlider m_positionSlider;
	//CtrlColorsMediaSliderAdapter m_volumeSliderColorsAdapter;
	//CtrlColorsMediaSliderAdapter m_positionSliderColorsAdapter;
	CLabel m_timeInfo;
	CLabel m_fullTimeInfo;
	CLabelEx m_playerInfo;


	MediaPlayer* m_pPlayer;
	//CRect m_rcPlayer;//, m_rcInfo;

	//INT m_netPendingJobs;
	//CollectionUpdateJob::Progress m_oldColProgress;
	CToolTipCtrl m_tooltip;

};
