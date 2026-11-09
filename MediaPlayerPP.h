#pragma once

#include "HotKeyCtrlAPI.h"
#include "afxwin.h"
#include "TSPropertyPage.h"

// CMediaPlayerPP dialog
class MediaPlayer;
class CTeenSpiritDlg;

class CMediaPlayerPP : public CTSPropertyPage
{
	DECLARE_DYNAMIC(CMediaPlayerPP)

public:
	CMediaPlayerPP(UINT nIDCaption);
	virtual ~CMediaPlayerPP();

// Dialog Data
	enum { IDD = IDD_PPPLAYEROPTIONS };
	virtual LPCTSTR GetHelpID()						{return _T("media_player");}

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);


	DECLARE_MESSAGE_MAP()


public:
	BOOL IsSomethingChanged()	{return m_bSomethingChanged;}
	void SetObjectsToConfigure(MediaPlayer* pPlayer)
	{
		m_pPlayer = pPlayer;
	}

	virtual void ApplyTranslation(ITranslation& skin);

private:
	afx_msg void OnChange();
	virtual void OnOK();
	void SaveChanges();

private:
	BOOL m_bSomethingChanged;
	BOOL m_bNeedsUpdating;

	//CTeenSpiritDlg* m_pTSDlg;
	MediaPlayer* m_pPlayer;
	void LoadMPCtrlOptions();
	void SaveMPCtrlOptions();
	CHotKeyCtrlAPI m_HKPlay;
	CHotKeyCtrlAPI m_HKStop;
	CHotKeyCtrlAPI m_HKPrevious;
	CHotKeyCtrlAPI m_HKNext;
	CComboBox m_cmbPlayControl;
	CComboBox m_cmbDefaultPlayerAction;
	//CSliderCtrl m_SldStart;
	CSliderCtrl m_SldEnd;

};
