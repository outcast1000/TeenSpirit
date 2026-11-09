#include "stdafx.h"
#include "TSTranslatable.h"
#include "PrgAPI.h"

TSTranslatable::TSTranslatable():
m_pTranslationManager(PRGAPI()->GetTranslationManager())
{
	m_pTranslationManager->RegisterTranslatable(*this);
}
