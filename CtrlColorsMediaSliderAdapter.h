#pragma once

#include "ICtrlColors.h"

class CMediaSlider;

class CtrlColorsMediaSliderAdapter:public ICtrlColors
{
public:
	CtrlColorsMediaSliderAdapter();
	virtual ~CtrlColorsMediaSliderAdapter()				{}

	void ConfigAdapter(CMediaSlider* pMediaSlider);

	virtual void SetColor(UINT idx, COLORREF value);
	virtual COLORREF GetColor(UINT idx);
	virtual LPCTSTR GetColorName(UINT idx);
private:
	CMediaSlider* m_pMediaSlider;
};

