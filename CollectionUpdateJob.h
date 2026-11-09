#ifndef _CollectionUpdateJob_h_
#define _CollectionUpdateJob_h_

#include "MultiThreadJobQueue.h"
#include <atomic>
#include "DataRecords.h"

class SQLManager;
class FileTagger;

class CollectionUpdateJob: public Job
{
public:
	enum Action
	{
		AC_Idle,
		AC_RemovingInexistentFiles,
		AC_CountingDirectoryFiles,
		AC_AddingUpdatingFiles,
		AC_Last
	};
	struct Progress
	{
		Progress():curAction(AC_Idle),curPos(0),maxPos(0),updateActionsPerformed(0){}
		INT curPos;
		INT maxPos;
		INT updateActionsPerformed;
		Action curAction;
	};
	struct Statistics
	{
		Statistics()				{memset(this, 0, sizeof(Statistics));}
		UINT DirsChecked;
		UINT FilesTotal;			//AllFiles
		UINT FilesSupported;		//Supported = FilesTotal - "Unsupported"
		UINT FilesAdded;			//Added
		UINT FilesUpdated;			//Updated
		UINT FilesDeleted;			//Deleted
		UINT FilesFailed;			//Failed
	};
	struct Instructions
	{
		BOOL bProcessFiles;
		BOOL bAllowAddNew;
		BOOL bAllowUpdate;
		BOOL bAllowRemove;
	};


	virtual ~CollectionUpdateJob();

	virtual State GetState()									{return m_state;}
	virtual void Execute();
	virtual void Abort()										{m_bAbort = TRUE;}

	BOOL GetCollectionRecord(CollectionRecordSP& col)		
	{
		ASSERT(m_collection.get() != NULL);
		ASSERT(GetState() == ST_Idle || GetState() == ST_Finished || GetState() == ST_Aborted);
		if (m_collection.get() != NULL && 
			(GetState() == ST_Idle || GetState() == ST_Finished || GetState() == ST_Aborted))
		{
			col = m_collection;
			return TRUE;
		}
		return FALSE;
	}
	const Progress GetProgress() const						{return m_progress;}
	const Statistics& GetStatistics() const					{return m_statistics;}

	static CollectionUpdateJob* CreateJob(
		CollectionRecordSP rec, Instructions instr, 
		SQLManager& sqlManager, FileTagger& fileTagger,
		Progress& progress)
	{
		CollectionUpdateJob* pJob = new CollectionUpdateJob(sqlManager, fileTagger, progress);
		pJob->m_collection = rec;
		pJob->m_instructions = instr;
		return pJob;
	}

private:
	CollectionUpdateJob(SQLManager& sqlManager, FileTagger& fileTagger, Progress& progress):
	//m_curJobAction(AC_Idle),
	m_state(Job::ST_Idle),
	m_bAbort(FALSE),
	//m_currentProgress(-1),
	//m_totalProgress(0),
	m_sqlManager(sqlManager),
	m_fileTagger(fileTagger),
	m_progress(progress)
	{}
	std::atomic<State> m_state;
	std::atomic<BOOL> m_bAbort;
	//INT m_currentProgress, m_totalProgress;
	//INT m_partCurrentProgress, m_partTotalProgress;
	Statistics m_statistics;
	Progress& m_progress;
	//Action m_curJobAction;
	//Job Params
	CollectionRecordSP m_collection;
	Instructions m_instructions;
	SQLManager& m_sqlManager;
	FileTagger& m_fileTagger;
private:
	INT ClearDeletedTracks(UINT collectionID, LPCTSTR startPath);
	void RecurseDirectories(UINT collectionID, LPCTSTR startPath, const Instructions& instr, Statistics& stats);
	//void CombinePath(LPTSTR destPath, LPCTSTR path, LPCTSTR fileName);
	BOOL ImportAudioCDCollection(LPCTSTR path);
	BOOL CombinePath(std::basic_string<TCHAR>& destPath, LPCTSTR path, LPCTSTR fileName);


};


#endif