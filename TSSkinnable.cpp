#include "stdafx.h"
#include "TSSkinnable.h"
#include "PrgAPI.h"

TSSkinnable::TSSkinnable():
m_pSkinManager(PRGAPI()->GetSkinManager())
{
	m_pSkinManager->RegisterSkinnable(*this);
}
