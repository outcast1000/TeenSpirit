#include "stdafx.h"
#include "StateManager.h"
#include "PrgAPI.h"

TSStateChangeListener::TSStateChangeListener():
m_pStateManager(PRGAPI()->GetStateManager())
{
	m_pStateManager->RegisterStateChangeListener(*this);
}
