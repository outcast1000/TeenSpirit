#ifndef _TSSKINABLE_H
#define _TSSKINABLE_H

#include "TSSkin.h"

//	Registers the object with a skin manager for its lifetime. The default
//	constructor uses the application's SkinManager; new code should pass the
//	manager. The object unregisters from the manager it registered with, so the
//	manager must outlive it.
class TSSkinnable : public ISkinnable
{
public:
	TSSkinnable();
	explicit TSSkinnable(ISkinManager& skinManager): m_pSkinManager(&skinManager)
	{
		m_pSkinManager->RegisterSkinnable(*this);
	}
	TSSkinnable(const TSSkinnable& other): ISkinnable(other), m_pSkinManager(other.m_pSkinManager)
	{
		m_pSkinManager->RegisterSkinnable(*this);
	}
	TSSkinnable& operator=(const TSSkinnable&)	{return *this;}	//stays registered where it was
	virtual ~TSSkinnable()
	{
		m_pSkinManager->UnRegisterSkinnable(*this);
	}
private:
	ISkinManager* m_pSkinManager;
};
#endif
