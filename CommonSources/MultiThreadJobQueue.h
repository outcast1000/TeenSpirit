#ifndef _MultiThreadJobQueue_h_
#define _MultiThreadJobQueue_h_

#include <vector>
#include <memory>

class Job
{
public:
	virtual ~Job()				{}
	enum State
	{
		ST_Idle,			//Not started yet (also: aborted before it started)
		ST_Running,			//While Running
		ST_Aborted,			//Execute returned after an Abort request
		ST_Finished,		//Execute returned normally
		ST_Last
	};
	//Called from any thread.
	virtual State GetState()	= 0;
	//Called once, on a worker thread (COM is initialized there as STA).
	virtual void Execute()		= 0;
	//Called from any thread while the job runs: Execute should return soon.
	virtual void Abort()		= 0;
};

typedef std::shared_ptr<Job> JobSP;

class CWinThread;

//Runs jobs on up to GetMaxThreads() worker threads.
//
//Workers sleep while there is nothing to do (no polling). Threads are never
//terminated: Shutdown/destruction asks the running jobs to abort and waits.
//If a job does not return in time its thread is left to finish on its own;
//the queue's internal state is reference-counted, so such a thread never
//touches freed memory.
class MultiThreadJobQueue
{
public:
	MultiThreadJobQueue();
	//Calls Shutdown(cDefaultShutdownTimeout) if Shutdown was not called.
	virtual ~MultiThreadJobQueue();

	enum {cDefaultShutdownTimeout = 30000};

	//FALSE after Shutdown.
	BOOL AddJob(JobSP job);
	//Aborts the running jobs and moves the waiting ones (not executed,
	//state ST_Idle) to the finished queue.
	void AbortJobs();
	//While paused no new job is started (running jobs continue).
	void Pause();
	void Resume();
	//Retrieves (and removes) the next finished job.
	BOOL RetrieveFinishedJob(JobSP& job);

	BOOL IsJobAssignerActive();

	//===Maximum number of jobs running at the same time.
	INT GetMaxThreads();
	void SetMaxThreads(INT threads);
	//===Minimum delay between starting two jobs. Default 0 (no delay).
	//		Useful for example to avoid flooding a web server with simultaneous requests.
	UINT GetAssignJobDelay();
	void SetAssignJobDelay(UINT ms);

	struct JobStatisics
	{
		JobStatisics():waiting(0),working(0),finished(0),total(0)	{}
		INT waiting;
		INT working;
		INT finished;
		INT total;
	};
	JobStatisics GetStatistics();

	//Stops accepting jobs, aborts them (see AbortJobs) and waits up to
	//timeoutMs for the worker threads to exit.
	//Returns TRUE when all of them exited. On FALSE, objects that the still
	//running jobs use must be kept alive.
	BOOL Shutdown(DWORD timeoutMs);

private:
	struct Shared;
	void SpawnWorkersLocked();
	static UINT WorkerST(LPVOID pParam);

	std::shared_ptr<Shared> m_shared;
	std::vector<CWinThread*> m_threads;
	BOOL m_bShutdown;

	MultiThreadJobQueue(const MultiThreadJobQueue&);
	MultiThreadJobQueue& operator=(const MultiThreadJobQueue&);
};

#endif
