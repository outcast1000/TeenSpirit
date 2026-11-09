//*********************************************************************
// StateManager
//*********************************************************************
#ifndef _STATEMANAGER_H
#define _STATEMANAGER_H

#include "IStateChangeListener.h"
#include <set>
#include "TeenSpiritEnums.h"
#include "TracksFilter.h"
#include "DataRecords.h"
#include "RecordCollectionSorter.h"
#include <functional>
class AppSettings;
class SQLManager;


enum StateMessageEnum
{
	SM_None,

	SM_MediaChanged,
	SM_PlayListChanged,
	SM_PlayerVolumeChanged,
	SM_PlayerSettingsChanged,

	SM_InfoChanged,
	SM_PictureChanged,

	SM_DatabaseUpdated,
	SM_TrackUpdated,

	SM_CollectionManagerEvent,

	SM_CurrentSectionChanged,
	SM_CurrentTrackChanged,

	SM_LocateTrackRequest,

	SM_SetQuickSearch
};

//=== The Interface implementation that must inherit all the controls that listen to state changes.
//=== Registers for its lifetime with a state manager: the default constructor uses the
//=== application's, new code should pass it. The manager must outlive the listener.
class TSStateChangeListener: public IStateChangeListener
{
public:
	TSStateChangeListener();
	explicit TSStateChangeListener(IStateManager& stateManager): m_pStateManager(&stateManager)
	{
		m_pStateManager->RegisterStateChangeListener(*this);
	}
	TSStateChangeListener(const TSStateChangeListener& other): IStateChangeListener(other), m_pStateManager(other.m_pStateManager)
	{
		m_pStateManager->RegisterStateChangeListener(*this);
	}
	TSStateChangeListener& operator=(const TSStateChangeListener&)	{return *this;}	//stays registered where it was
	virtual ~TSStateChangeListener()
	{
		m_pStateManager->UnRegisterStateChangeListener(*this);
	}
private:
	IStateManager* m_pStateManager;
};

class TSState
{
public:
	//enum SectionSelectionStyleEnum
	//{
	//	SSS_Normal,
	//	SSS_QuickSearch,
	//	SSS_Last
	//};
public:
	TSState():
		trackListTrackID(0),
//		sectionSelectionStyle(SSS_Normal),
		bIsTrackListDirty(TRUE),
		locateTrackID(0)
		//m_pTrackList(NULL)
	{}
	~TSState()				{}


	//=== TS State is the common object that all GUI controls must see to modify their view.
	//=== The GUI concepts are:
	//		Controls that modify the Tracks Filter (Producers).
	//			- Music Browser (modifies activeTracksFilter, activeItemType)
	//			- Quick Search Box (modifies activeTracksFilter)
	//		Controls that display information based on that filter (Consumers)
	//			- Track List (listens activeTracksFilter) (modifies activeItemType)
	//			- Info Control (listens activeTracksFilter, activeItemType)
	//		Controls that modify the current play state
	//			- Player(s)	(Producer)
	//		Controls that display the current play state
	//			- PlayerBar
	//			- PlayList
	//			- InfoControl

	//SectionSelectionStyleEnum sectionSelectionStyle;
	TracksFilter activeTracksFilter;
	//std::tstring quickSearchString;
	ItemTypeEnum activeItemType;
	SortOptionCollection sortOptionCollection;
	UINT trackListTrackID;
	BOOL bIsTrackListDirty;

	//The tracks matching activeTracksFilter (queried again when bIsTrackListDirty).
	FullTrackRecordCollection& GetTrackList();

	std::basic_string<TCHAR> quickSearchString;
	UINT locateTrackID;

	void SetSQLManagerProvider(std::function<SQLManager*()> getSQLManager)	{m_getSQLManager = getSQLManager;}
private:
	FullTrackRecordCollection m_trackList;
	std::function<SQLManager*()> m_getSQLManager;

};

class StateManager: public IStateManager
{
public:
	//getSQLManager: the database the track list is read from (asked when first needed).
	//The thread creating the StateManager is the main (UI) thread.
	explicit StateManager(std::function<SQLManager*()> getSQLManager);
	virtual ~StateManager()														{ASSERT(m_pListeners.empty());}	//=== A listener outlives its manager

public:
	virtual void RegisterStateChangeListener(IStateChangeListener& listener);
	virtual void UnRegisterStateChangeListener(IStateChangeListener& listener);

	virtual void SendMessage(UINT stateMessage);
	virtual void PostMessage(UINT stateMessage);

	virtual void HeartBeat();

	TSState& GetState()				{return m_state;}

	//BOOL LoadState(AppSettings& params);
	//BOOL SaveState(AppSettings& params);
	//void ClearState();

private:
	std::set<IStateChangeListener*> m_pListeners;
	TSState m_state;

	//UINT m_messageInQueue;
	std::set<UINT> m_messagesInQueue;
	DWORD m_MainThread;
	CCriticalSection m_cs;


};
#endif