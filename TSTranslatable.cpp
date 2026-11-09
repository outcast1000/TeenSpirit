#include "stdafx.h"
#include "TSTranslatable.h"
#include "PrgAPI.h"

TSTranslatable::TSTranslatable()
{
	PRGAPI()->GetTranslationManager()->RegisterTranslatable(*this);
}
TSTranslatable::~TSTranslatable()
{
	PRGAPI()->GetTranslationManager()->UnRegisterTranslatable(*this);
}

