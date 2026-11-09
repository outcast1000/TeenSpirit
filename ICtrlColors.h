#pragma once

#define COL_NOT_IMPLEMENTED	0xFE0100

class ICtrlColors
{
public:
	ICtrlColors()			{}
	virtual ~ICtrlColors()	{}

	virtual void SetColor(UINT idx, COLORREF value)	= 0;
	virtual COLORREF GetColor(UINT idx)				= 0;
	virtual LPCTSTR GetColorName(UINT idx)			= 0;
};

