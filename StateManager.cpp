////*********************************************************************
//// INCLUDES
////*********************************************************************
#include "StdAfx.h"
#include "StateManager.h"
#include "SQLManager.h"
#include "AppSettings/AppSettings.h"
#include <vector>

//===========================================================================
//=== TSStateChangeListener
//===========================================================================

LPCTSTR SM2Str(UINT stateMessage)
{
	switch (stateMessage)
	{
	case SM_MediaChanged:				return _T("MediaChanged");
	case SM_PlayListChanged:			return _T("PlayListChanged");
	case SM_PlayerVolumeChanged:		return _T("PlayerVolumeChanged");
	case SM_PlayerSettingsChanged:		return _T("PlayerSettingsChanged");
	case SM_InfoChanged:				return _T("InfoChanged");
	case SM_PictureChanged:				return _T("PictureChanged");
	case SM_DatabaseUpdated:			return _T("DatabaseUpdated");
	case SM_CollectionManagerEvent:		return _T("CollectionManagerEvent");
	case SM_CurrentSectionChanged:		return _T("SectionChanged");
	case SM_CurrentTrackChanged:		return _T("TrackChanged");
	case SM_LocateTrackRequest:			return _T("SectionChangeRequest");
	}
	return NULL;
}



//========================================================================
//===TSState
//========================================================================

FullTrackRecordCollection& TSState::GetTrackList()
{
	if (bIsTrackListDirty)
	{
		//=== This function has been crashed several times in a certain user (290558176 v866)
		//=== Other user (919176317 v866) said "in vista,when searching in window,the application will quit" multiple times
		TRACEST(_T("TSState::GetTrackList"));
		m_trackList.clear();
		if (activeTracksFilter.IsEmpty() == FALSE)
		{
			SQLManager* pSM = m_getSQLManager ? m_getSQLManager() : NULL;
			ASSERT(pSM != NULL);
			if (pSM != NULL)
				pSM->GetFullTrackRecordCollectionByTracksFilter(m_trackList, activeTracksFilter);
		}
		//pSM->GetFullTrackRecordCollectionBySimpleSearch(m_trackList, quickSearchString.c_str());
		bIsTrackListDirty = FALSE;
	}
	return m_trackList;

}

//========================================================================
//===StateManager
//========================================================================

StateManager::StateManager(std::function<SQLManager*()> getSQLManager)
{
	m_MainThread = GetCurrentThreadId();
	m_state.SetSQLManagerProvider(getSQLManager);
}

void StateManager::RegisterStateChangeListener(IStateChangeListener& listener)
{
	m_pListeners.insert(&listener);
}
void StateManager::UnRegisterStateChangeListener(IStateChangeListener& listener)
{
	//=== Unregistering a listener that isn't registered is allowed: the panes
	//=== register while shown, and unregister on hide and again on destruction.
	m_pListeners.erase(&listener);
}

void StateManager::SendMessage(UINT stateMessage)
{
	if (m_MainThread != GetCurrentThreadId())
	{
		TRACE(_T("@2BaseDirector::SendMessage. Message from another thread...Posted"));
		//If the SendMessage have came from another thread then we will "Post" the
		//message for the following "Heartbeat" which will run from the Main Thread
		//PostMessage Also is "Thread Safe" for the messages vector
		PostMessage(stateMessage);
	}
	else
	{
		switch (stateMessage)
		{
		case SM_CurrentTrackChanged:
		case SM_CurrentSectionChanged:
		case SM_DatabaseUpdated:
		case SM_CollectionManagerEvent:
			m_state.bIsTrackListDirty = TRUE;
			break;
		}

		//=== Broadcast over a snapshot: a listener may register or unregister listeners
		//=== (create/destroy windows) while handling the message, which would invalidate
		//=== an iterator into m_pListeners. Skip listeners removed during the broadcast.
		std::vector<IStateChangeListener*> listeners(m_pListeners.begin(), m_pListeners.end());
#ifdef _DEBUG
		TRACE(_T("StateManager::SendMessage Broadcasting '%s' * * * * * * *\r\n"), SM2Str(stateMessage));
#endif
		for (size_t i = 0; i < listeners.size(); i++)
		{
			IStateChangeListener* pListener = listeners[i];
			if (m_pListeners.find(pListener) == m_pListeners.end())
				continue;
#ifdef _DEBUG
			if (pListener->OnStateChanged(stateMessage))
				TRACE(_T("\t+ %s\r\n"), pListener->GetStateListenerName());
			else
				TRACE(_T("\t- %s\r\n"), pListener->GetStateListenerName());
#else
			pListener->OnStateChanged(stateMessage);
#endif
		}
#ifdef _DEBUG
		TRACE(_T("StateManager::SendMessage Broadcasting '%s' - - - - - - -\r\n"), SM2Str(stateMessage));
#endif
	}


}

void StateManager::PostMessage(UINT stateMessages)
{
	CSingleLock lock(&m_cs, TRUE);
	m_messagesInQueue.insert(stateMessages);
	//m_messageInQueue = stateMessages;
}

void StateManager::HeartBeat()
{
	//=== Take the queued messages and dispatch them without holding the lock.
	//=== Messages posted while dispatching (by a listener or a worker thread) stay
	//=== queued for the next heartbeat, and a re-entrant HeartBeat (from a modal
	//=== loop inside a listener) only sees an empty or new queue.
	std::set<UINT> messages;
	{
		CSingleLock lock(&m_cs, TRUE);
		messages.swap(m_messagesInQueue);
	}
	for (std::set<UINT>::const_iterator it = messages.begin(); it != messages.end(); ++it)
		SendMessage(*it);
}

//BOOL StateManager::LoadState(AppSettings& params)
//{
//	std::tstring sortingStr;
//	params.Read(_T("StateManager"), _T("Sorting"), sortingStr, _T(""));
//	m_state.sortOptionCollection.LoadFromString(sortingStr.c_str());
//	return TRUE;
//}
//
//BOOL StateManager::SaveState(AppSettings& params)
//{
//	TCHAR bf[300];
//	m_state.sortOptionCollection.SaveToString(bf, 300);
//	params.Write(_T("StateManager"), _T("Sorting"), bf);
//	return TRUE;
//}






