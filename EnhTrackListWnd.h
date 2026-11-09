#ifndef _EnhTrackListWnd_h_
#define _EnhTrackListWnd_h_

#include "TSTranslatable.h"
#include "TSSkinnable.h"
#include "LabelEx.h"
#include "GuiControls/IconEditEx.h"
#include "TracksFilter.h"
#include "TeenSpiritEnums.h"
#include "MainListCtrl.h"


class CMainListCtrl;
class CIconEditEx;

class CEnhTrackListWnd :	public CWnd,
							public TSSkinnable, 
							public TSTranslatable
{
	DECLARE_DYNAMIC(CEnhTrackListWnd)

public:
	CEnhTrackListWnd();
	virtual ~CEnhTrackListWnd();

public:
	void SetTitleHeight(INT height);
	//void SetMainControl(CWnd* pWnd);
	void SetText(LPCTSTR text);

	BOOL Create(CWnd* pParent, UINT nID);

	virtual void ApplyTranslation(ITranslation& skin);
	virtual void ApplySkin(ISkin& skin);

	BOOL OnStateChanged(UINT stateMessage);
	void PlayTracks();
	void EnqueueTracks();

	CMainListCtrl& GetMainTrackList()	{return m_trackList;}

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
	afx_msg void OnEnChangeSearchedt();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	DECLARE_MESSAGE_MAP()
private:
	INT m_TitleHeight;
	CIconEditEx m_searchEdit;
	CLabelEx m_caption;
	void Resize(int cx, int cy);
	CMainListCtrl m_trackList;
	FullTrackRecordCollection col;
	UINT m_locateTrackID;


public:
	afx_msg void OnDestroy();
};

#endif
