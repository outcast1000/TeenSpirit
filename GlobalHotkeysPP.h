#pragma once

#include "HotKeyCtrlAPI.h"
#include "afxwin.h"
#include "TSPropertyPage.h"

// CMediaPlayerPP dialog
class MediaPlayer;

class CGlobalHotkeysPP : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CGlobalHotkeysPP)

public:
	CGlobalHotkeysPP(UINT nIDCaption);
	virtual ~CGlobalHotkeysPP();

// Dialog Data
	enum { IDD = IDD_PPGLOBALHOTKEYS };
	virtual LPCTSTR GetHelpID()						{return _T("global_hotkeys");}

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);


	DECLARE_MESSAGE_MAP()


public:
	BOOL IsSomethingChanged()	{return m_bSomethingChanged;}
	virtual void ApplyTranslation(ITranslation& skin);

private:
	afx_msg void OnChange();
	virtual void OnOK();
	void SaveChanges();

private:
	BOOL m_bSomethingChanged;
	BOOL m_bNeedsUpdating;

	void LoadGlobalHotkeysOptions();
	void SaveGlobalHotkeysOptions();
	CHotKeyCtrlAPI m_HKPlay;
	CHotKeyCtrlAPI m_HKStop;
	CHotKeyCtrlAPI m_HKPrevious;
	CHotKeyCtrlAPI m_HKNext;
	CHotKeyCtrlAPI m_HKMiniPlayer;
	CHotKeyCtrlAPI m_HKTooltip;
	CHotKeyCtrlAPI m_HKQuickSearch;

};
