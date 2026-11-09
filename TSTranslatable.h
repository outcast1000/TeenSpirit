#ifndef _TSTRANSLATABLE_H_
#define _TSTRANSLATABLE_H_

#include "ITranslation/ITranslation.h"

//	Registers the object with a translation manager for its lifetime. The default
//	constructor uses the application's TranslationManager; new code should pass
//	the manager. The object unregisters from the manager it registered with, so
//	the manager must outlive it.
class TSTranslatable: public ITranslatable
{
public:
	TSTranslatable();
	explicit TSTranslatable(ITranslationManager& translationManager): m_pTranslationManager(&translationManager)
	{
		m_pTranslationManager->RegisterTranslatable(*this);
	}
	TSTranslatable(const TSTranslatable& other): ITranslatable(other), m_pTranslationManager(other.m_pTranslationManager)
	{
		m_pTranslationManager->RegisterTranslatable(*this);
	}
	TSTranslatable& operator=(const TSTranslatable&)	{return *this;}	//stays registered where it was
	virtual ~TSTranslatable()
	{
		m_pTranslationManager->UnRegisterTranslatable(*this);
	}
private:
	ITranslationManager* m_pTranslationManager;
};


#endif
