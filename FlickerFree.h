#ifndef _FlickerFree_h_
#define _FlickerFree_h_

//Class Objective
//Make an efficient memory DC for double buffering graphics

//Class Usage

//	1. Make a Member Variable in the CWnd declaration
//	CFlickerFree m_ff; 

//	2. Write OnPaint like this. [Use dc for drawing. m_ff is a CFlickerFree member var]
//	CPaintDC _dc(this); // device context for painting
//	CFlickerFreeDC dc(m_ff, _dc);

//	---Optional---
//	3. Override OnEraseBkgnd like this. [Draw everything in OnPaint. Can cause flickering]
//	BOOL CWnd::OnEraseBkgnd(CDC* pDC)
//	{
//		return TRUE;
//	}


class CFlickerFree
{
public:
	CFlickerFree();
	~CFlickerFree();

	CDC& GetMemDC();
	void SetTargetDC(CDC& targetDC);
	void Draw();
	void SetSize(INT cx, INT cy);
	INT GetWidth()		{return m_bitmapRect.Width();}
	INT GetHeight()		{return m_bitmapRect.Height();}
private:
	CDC m_dc;
	CBitmap m_bitmap;
	CRect m_bitmapRect;
	CBitmap* m_pOldBitmap;
	CDC* m_targetDC;
	//CRect m_rect;
	CRect m_targetRC;
};

class CFlickerFreeDC:public CDC
{
public:
	explicit CFlickerFreeDC(CFlickerFree& ff, CDC& dc);
	virtual ~CFlickerFreeDC();

private:
	CFlickerFree* m_ff;
};
#endif