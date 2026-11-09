

#include "stdafx.h"
#include "TSSkinnable.h"
#include "PrgAPI.h"

TSSkinnable::TSSkinnable()
{
	PRGAPI()->GetSkinManager()->RegisterSkinnable(*this);
}

TSSkinnable::~TSSkinnable()
{
	PRGAPI()->GetSkinManager()->UnRegisterSkinnable(*this);
}

