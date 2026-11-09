#ifndef _TickerWnd_h_
#define _TickerWnd_h_

#include "ICtrlColors.h"


//It is a ticker window.
//It displays 3 kinds of messages
//1. Permanent Messages (Rotate them when the control is idle)
//2. Normal. When there is one it will display it instantly. If there is already another it ll queue the message
//3. Optional. If there are Normal messages curently displaying they will get discarded. Otherwise it ll be shown instantly (No storage for this messages)
#include <queue>
#include <vector>

#define THREAD_SAFE_TICKERWND

class CTickerWnd : public CWnd, public ICtrlColors
{
	DECLARE_DYNAMIC(CTickerWnd)

public:
	enum MESSAGE_TYPE
	{
		MT_Permanent,
		MT_Normal,
		MT_Optional,
		MT_Last
	};
	//enum STYLE
	//{
	//	S_NoBorder,
	//	S_Rect,
	//	S_RoundRect,	//needs styleParam1, styleParam2
	//	B_Last
	//};
	CTickerWnd();
	virtual ~CTickerWnd();

	//void SetStyle(STYLE st, INT styleParam1, INT styleParam2);
	void SetIconSize(INT cxIcon, INT cyIcon);
	void SetMessageDelay(MESSAGE_TYPE mt, UINT delay);
	void SetFont(CFont& m_font);

	//Thread Safe
	void AddMessage(MESSAGE_TYPE mt, LPCTSTR msg, HICON hIcon);
	//Thread Safe
	void ClearMessages(MESSAGE_TYPE mt);

	//---------------------------------------------------
	//ICtrlColors ---------------------------------------
	//#include "ICtrlColors.h" // ,public ICtrlColors
	//---------------------------------------------------
public:
	enum COLORS
	{
		COL_BkPerm,
		COL_TextPerm,
		COL_BkNorm,
		COL_TextNorm,
		COL_BkOpti,
		COL_TextOpti,
		COL_Last
	};
	virtual void SetColor(UINT idx, COLORREF value);
	virtual COLORREF GetColor(UINT idx);
	virtual LPCTSTR GetColorName(UINT idx);

private:
	COLORREF m_colors[COL_Last];
	//---------------------------------------------------
	//---------------------------------------------------
	//---------------------------------------------------
protected:

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
private:
	void DrawSurface(CDC& dc, HICON hIcon, LPCTSTR msg, COLORREF textColor, COLORREF bkColor);

private:
	UINT m_delays[MT_Last];
	INT m_cxIcon,m_cyIcon;
	CFont* m_pFont;
	//STYLE m_style;
	//INT m_styleParam1,m_styleParam2;
	struct MsgStruct
	{
		MsgStruct(LPCTSTR Msg, HICON HIcon):
	msg(Msg),hIcon(HIcon){} 
		std::tstring msg;
		HICON hIcon;
	};
	std::vector<MsgStruct> m_permanantMessages;
	std::queue<MsgStruct> m_normalMessages;
	INT m_curPermamentMsg;
#ifdef THREAD_SAFE_TICKERWND
	CCriticalSection m_cs;
#endif
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};


#endif // _DEBUG
