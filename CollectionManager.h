#ifndef _CollectionManager_h_
#define _CollectionManager_h_

#include "TeenSpiritEnums.h"
#include "DataRecords.h"
#include "CollectionUpdateJob.h"
#include <wininet.h>
#include <memory>
#include <mutex>
#include <deque>

class MultiThreadJobQueue;
class SQLManager;
class AppOptions;
class ITranslation;
class FileTagger;

//	Adds, scans and removes collections. Scans run on one background thread with
//	their own database connection; the rest runs on the UI thread, which must call
//	ConsumeResult regularly (it finishes scans and delivers their messages).
class CollectionManager
{
public:
	//What the collection manager needs from the application.
	class Host
	{
	public:
		virtual ~Host()										{}
		virtual void SendStateMessage(UINT stateMessage)	= 0;	//SM_CollectionManagerEvent, SM_DatabaseUpdated
		virtual void NotifyUser(LPCTSTR msg)				= 0;	//progress messages (UI thread)
		virtual HINTERNET GetInternetHandle()				= 0;	//FreeDB lookups for audio CDs
		virtual FileTagger* CreateFileTagger()				= 0;	//owned by the caller
	};

	//sqlManager: the UI thread's connection; scans open their own on the same file.
	CollectionManager(SQLManager& sqlManager, AppOptions& options, ITranslation& translation, std::unique_ptr<Host> host);
	~CollectionManager();

	//Stops the background scan job (asks it to abort and waits). Returns FALSE if
	//it did not exit in time; it then keeps running and still uses its objects.
	BOOL StopBackgroundWork();

public:
	//The caller should call "CollectionExists" first so it can inform the user.
	//The caller may call the "GetCollectionType".
	//		If it is a CD must Inform the user that it will Add the full CD
	//The caller may call the "GetVolumeLabel" to suggest a name for the collection
	BOOL AddCollection(LPCTSTR path, LPCTSTR name);
	BOOL UpdateCollection(CollectionRecordSP rec, BOOL bAllowAddNew, BOOL bAllowUpdate, BOOL bAllowRemove);
	BOOL CollectionExists(LPCTSTR pathName);

	BOOL RemoveCollection(UINT ID);
	BOOL RenameCollection(UINT ID, LPCTSTR newName);


	BOOL ImportFullTrackRecordCollection(CollectionRecord& cr, FullTrackRecordCollection& col);

	BOOL RefreshLocalCollections(BOOL bAllowAddNew, BOOL bAllowUpdate, BOOL bAllowRemove);
	BOOL IsCollectionPending(const CollectionRecord& rec) const;

	const CollectionUpdateJob::Progress& GetProgress() const					{return m_progress;}

	INT GetPendingJobs();

	//===Async Result Producer (UI thread)
	void ConsumeResult();
	enum State
	{
		S_Idle,			//There are no more jobs to execute
		S_Working,		//Working on some jobs
		S_Paused,		//Paused.
		S_Last
	};
	State GetState();
	//===Async Result Producer END!

	//At most this many scan messages wait for ConsumeResult; older ones are dropped.
	static const size_t cMaxPendingMessages = 50;

	static DWORD GetVolumeSerial(LPCTSTR path);
	static BOOL GetVolumeLabel(LPCTSTR path, LPTSTR bfMaxPath);
	static CollectionTypesEnum GetCollectionType(LPCTSTR pathName);
private:
	CollectionManager(const CollectionManager&);
	CollectionManager& operator=(const CollectionManager&);

	BOOL ImportAudioCDCollection(CollectionRecordSP rec);
	BOOL ReleaseCollectionWorkingStatus(const CollectionRecord& rec);
	FileTagger& GetFileTagger();
	//Database connection used by the (single) background job thread.
	SQLManager* GetJobSQLManager();
	BOOL StartJob(CollectionRecordSP rec, const CollectionUpdateJob::Instructions& instr);
	//Called by the scan thread.
	void QueueMessage(const std::tstring& msg);
private:
	SQLManager& m_sqlManager;
	AppOptions& m_options;
	ITranslation& m_translation;
	std::unique_ptr<Host> m_host;
	MultiThreadJobQueue* m_pMTQueue;
	BOOL m_bWorkStopped;
	BOOL m_bWorkerStillRunning;
	FileTagger* m_pFileTagger;
	SQLManager* m_pJobSQLManager;
	State m_state;
	std::vector<UINT> m_workingCollections;
	CollectionUpdateJob::Progress m_progress;
	std::mutex m_messagesLock;
	std::deque<std::tstring> m_messages;
};

#endif