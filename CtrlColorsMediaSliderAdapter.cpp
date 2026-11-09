#include "stdafx.h"
#include "CtrlColorsMediaSliderAdapter.h"
#include "MediaSlider.h"

CtrlColorsMediaSliderAdapter::CtrlColorsMediaSliderAdapter():
m_pMediaSlider(NULL)
{

}

void CtrlColorsMediaSliderAdapter::ConfigAdapter(CMediaSlider* pMediaSlider)
{
	m_pMediaSlider = pMediaSlider;
}

void CtrlColorsMediaSliderAdapter::SetColor(UINT idx, COLORREF value)
{
	if (m_pMediaSlider != NULL && idx < CMediaSlider::COL_Last)
		m_pMediaSlider->SetColor((CMediaSlider::COLORS)idx, value);
	else
		ASSERT(0);
}
COLORREF CtrlColorsMediaSliderAdapter::GetColor(UINT idx)
{
	if (m_pMediaSlider != NULL && idx < CMediaSlider::COL_Last)
		return m_pMediaSlider->GetColor((CMediaSlider::COLORS)idx);
	return -1;
}

LPCTSTR CtrlColorsMediaSliderAdapterColNames[] = {
	_T("Bk"),
	_T("Border"),
	_T("FirstPart"),
	_T("SecondPart"),
	_T("CursorBorder")
};

LPCTSTR CtrlColorsMediaSliderAdapter::GetColorName(UINT idx)
{
	if (idx < CMediaSlider::COL_Last)
		return CtrlColorsMediaSliderAdapterColNames[idx];
	return NULL;
}

