//	StateManager (the app's event bus): broadcasts, messages from other threads,
//	listeners coming and going during a broadcast, and the shared track list.
#include "stdafx.h"
#include "TestFramework.h"
#include "StateManager.h"
#include "SQLManager.h"
#include <thread>
#include <vector>

namespace
{
	class Listener: public TSStateChangeListener
	{
	public:
		explicit Listener(IStateManager& sm): TSStateChangeListener(sm), m_thread(0)	{}
		virtual BOOL OnStateChanged(UINT msg)
		{
			m_received.push_back(msg);
			m_thread = GetCurrentThreadId();
			return TRUE;
		}
		virtual LPCTSTR GetStateListenerName()	{return L"Listener";}
		std::vector<UINT> m_received;
		DWORD m_thread;
	};

	//Unregisters itself and another listener when it gets a message.
	class Leaver: public IStateChangeListener
	{
	public:
		Leaver(StateManager& sm, IStateChangeListener& other): m_sm(sm), m_other(other), m_count(0)	{}
		virtual BOOL OnStateChanged(UINT)
		{
			m_count++;
			m_sm.UnRegisterStateChangeListener(*this);
			m_sm.UnRegisterStateChangeListener(m_other);
			return TRUE;
		}
		virtual LPCTSTR GetStateListenerName()	{return L"Leaver";}
		StateManager& m_sm;
		IStateChangeListener& m_other;
		INT m_count;
	};

	//Counts messages without the TSStateChangeListener registration.
	class Plain: public IStateChangeListener
	{
	public:
		Plain(): m_count(0)							{}
		virtual BOOL OnStateChanged(UINT)			{m_count++; return TRUE;}
		virtual LPCTSTR GetStateListenerName()		{return L"Plain";}
		INT m_count;
	};

	void TestBroadcast()
	{
		StateManager sm(nullptr);
		{
			Listener a(sm), b(sm);
			sm.SendMessage(SM_MediaChanged);
			CHECK(a.m_received.size() == 1 && a.m_received[0] == SM_MediaChanged);
			CHECK(b.m_received.size() == 1);
			CHECK(a.m_thread == GetCurrentThreadId());
			{
				Listener copy(a);			//a copy registers too
				sm.SendMessage(SM_InfoChanged);
				CHECK(copy.m_received.size() == 2 && a.m_received.size() == 2);
			}
			sm.SendMessage(SM_InfoChanged);	//the copy is gone
			CHECK(a.m_received.size() == 3);
		}
		sm.SendMessage(SM_InfoChanged);		//nobody left: nothing touched

		//Listeners that leave during a broadcast
		Plain p1, p2;
		Leaver leaver(sm, p2);
		sm.RegisterStateChangeListener(p1);
		sm.RegisterStateChangeListener(leaver);
		sm.RegisterStateChangeListener(p2);
		sm.SendMessage(SM_PictureChanged);
		CHECK(p1.m_count == 1 && leaver.m_count == 1 && p2.m_count <= 1);
		INT p2Count = p2.m_count;
		sm.SendMessage(SM_PictureChanged);
		CHECK(p1.m_count == 2 && leaver.m_count == 1 && p2.m_count == p2Count);
		sm.UnRegisterStateChangeListener(p1);

		//Unregistering twice, or a listener that never registered, is allowed
		//(the panes unregister on hide and again when destroyed)
		Plain p3, never;
		sm.RegisterStateChangeListener(p3);
		sm.UnRegisterStateChangeListener(p3);
		sm.UnRegisterStateChangeListener(p3);
		sm.UnRegisterStateChangeListener(never);
		sm.SendMessage(SM_InfoChanged);
		CHECK(p3.m_count == 0 && never.m_count == 0);
	}

	void TestPostAndThreads()
	{
		StateManager sm(nullptr);
		Listener l(sm);

		//Posted messages wait for the heartbeat; duplicates are merged
		sm.PostMessage(SM_DatabaseUpdated);
		sm.PostMessage(SM_DatabaseUpdated);
		sm.PostMessage(SM_PlayListChanged);
		CHECK(l.m_received.empty());
		sm.HeartBeat();
		CHECK(l.m_received.size() == 2);
		sm.HeartBeat();
		CHECK(l.m_received.size() == 2);

		//From another thread SendMessage is posted, then delivered on the main thread
		l.m_received.clear();
		std::thread worker([&sm]()
		{
			sm.SendMessage(SM_InfoChanged);
			sm.PostMessage(SM_PictureChanged);
		});
		worker.join();
		CHECK(l.m_received.empty());
		sm.HeartBeat();
		CHECK(l.m_received.size() == 2 && l.m_thread == GetCurrentThreadId());
	}

	void TestTrackList()
	{
		std::wstring db = TempPath(L"state.db");
		DeleteFileW(db.c_str());
		SQLManager sql;
		CHECK(sql.Init(db.c_str()));
		CollectionRecord col;
		col.location = L"C:\\StateTests";
		col.type = CTYPE_LocalFolder;
		CHECK(sql.AddNewCollectionRecord(col));
		TagInfo ti;
		ti.Artist = L"State Artist";
		ti.Title = L"One";
		ti.validFields = TagInfo_Artist | TagInfo_Title;
		CHECK(sql.AddNewTrackFromTagInfo(col.ID, L"C:\\StateTests\\One.mp3", ti));

		INT asked = 0;
		StateManager sm([&sql, &asked]() -> SQLManager* {asked++; return &sql;});
		CHECK(asked == 0);								//not needed until the list is read
		TSState& state = sm.GetState();
		CHECK(state.GetTrackList().empty() && asked == 0);	//empty filter: no query

		state.activeTracksFilter.Artist.val = L"State Artist";
		state.activeTracksFilter.Artist.match = TXTM_Exact;
		state.bIsTrackListDirty = TRUE;
		CHECK(state.GetTrackList().size() == 1 && asked == 1);

		//Cached until a message marks it dirty
		ti.Title = L"Two";
		CHECK(sql.AddNewTrackFromTagInfo(col.ID, L"C:\\StateTests\\Two.mp3", ti));
		CHECK(state.GetTrackList().size() == 1 && asked == 1);
		sm.SendMessage(SM_PlayListChanged);				//doesn't touch the track list
		CHECK(state.GetTrackList().size() == 1);
		sm.SendMessage(SM_DatabaseUpdated);
		CHECK(state.GetTrackList().size() == 2 && asked == 2);
		const UINT dirtying[] = {SM_CurrentTrackChanged, SM_CurrentSectionChanged, SM_CollectionManagerEvent};
		for (size_t i = 0; i < sizeof(dirtying) / sizeof(dirtying[0]); i++)
		{
			state.bIsTrackListDirty = FALSE;
			sm.SendMessage(dirtying[i]);
			CHECK(state.bIsTrackListDirty);
		}
	}
}

void RunStateTests()
{
	wprintf(L"State tests\n");
	TestBroadcast();
	TestPostAndThreads();
	TestTrackList();
}
