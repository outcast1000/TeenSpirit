#include "stdafx.h"
#include "MultiThreadJobQueue.h"
#include <mutex>
#include <condition_variable>
#include <deque>
#include <queue>
#include <algorithm>

//State shared by the queue and its worker threads. Each worker holds a
//reference, so it outlives the queue if a job is still running at shutdown.
struct MultiThreadJobQueue::Shared
{
	Shared():maxThreads(3),assignJobDelay(0),lastAssignTick(0),
		bPaused(false),bExit(false),threads(0),total(0)	{}

	std::mutex mx;
	std::condition_variable cv;			//Signals: new work, finished job, setting change, exit
	std::deque<JobSP> waiting;
	std::queue<JobSP> finished;
	std::vector<JobSP> running;
	INT maxThreads;
	UINT assignJobDelay;
	ULONGLONG lastAssignTick;
	bool bPaused;
	bool bExit;
	INT threads;						//Live worker threads
	INT total;							//Jobs ever added

	void AbortLocked()
	{
		while (!waiting.empty())
		{
			finished.push(waiting.front());
			waiting.pop_front();
		}
		for (size_t i = 0; i < running.size(); i++)
			running[i]->Abort();
	}
};

MultiThreadJobQueue::MultiThreadJobQueue():
m_shared(new Shared),
m_bShutdown(FALSE)
{
}

MultiThreadJobQueue::~MultiThreadJobQueue()
{
	if (!m_bShutdown)
		Shutdown(cDefaultShutdownTimeout);
}

BOOL MultiThreadJobQueue::Shutdown(DWORD timeoutMs)
{
	TRACE(_T("@2 MultiThreadJobQueue::Shutdown\r\n"));
	std::vector<CWinThread*> threads;
	{
		std::lock_guard<std::mutex> lk(m_shared->mx);
		m_bShutdown = TRUE;
		m_shared->bExit = true;
		m_shared->AbortLocked();
		threads.swap(m_threads);
	}
	m_shared->cv.notify_all();

	BOOL bAllExited = TRUE;
	const ULONGLONG start = GetTickCount64();
	for (size_t i = 0; i < threads.size(); i++)
	{
		DWORD wait = timeoutMs;
		if (timeoutMs != INFINITE)
		{
			ULONGLONG elapsed = GetTickCount64() - start;
			wait = elapsed >= timeoutMs ? 0 : (DWORD)(timeoutMs - elapsed);
		}
		if (WaitForSingleObject(threads[i]->m_hThread, wait) == WAIT_OBJECT_0)
			delete threads[i];
		else
		{
			//Not terminated (that could leave locks/heap/COM state corrupt):
			//the thread finishes its job and exits by itself; its CWinThread is leaked.
			TRACE(_T("@0 MultiThreadJobQueue::Shutdown. A job did not finish in time; leaving its thread running\r\n"));
			bAllExited = FALSE;
		}
	}
	return bAllExited;
}

void MultiThreadJobQueue::SpawnWorkersLocked()
{
	Shared& s = *m_shared;
	//Start threads while there is waiting work that the existing threads cannot pick up.
	while (!s.bExit && s.threads < s.maxThreads &&
		s.threads - (INT)s.running.size() < (INT)s.waiting.size())
	{
		std::shared_ptr<Shared>* pRef = new std::shared_ptr<Shared>(m_shared);
		CWinThread* pThread = AfxBeginThread(MultiThreadJobQueue::WorkerST, pRef,
			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		if (pThread == NULL)
		{
			delete pRef;
			TRACE(_T("@0 MultiThreadJobQueue. Could not create a worker thread\r\n"));
			break;
		}
		pThread->m_bAutoDelete = FALSE;
		m_threads.push_back(pThread);
		s.threads++;
		pThread->ResumeThread();
	}
}

BOOL MultiThreadJobQueue::AddJob(JobSP job)
{
	ASSERT(job.get() != NULL);
	if (job.get() == NULL)
		return FALSE;
	{
		std::lock_guard<std::mutex> lk(m_shared->mx);
		if (m_shared->bExit)
			return FALSE;
		m_shared->waiting.push_back(job);
		m_shared->total++;
		SpawnWorkersLocked();
	}
	m_shared->cv.notify_all();
	return TRUE;
}

void MultiThreadJobQueue::AbortJobs()
{
	{
		std::lock_guard<std::mutex> lk(m_shared->mx);
		m_shared->AbortLocked();
	}
	m_shared->cv.notify_all();
}

void MultiThreadJobQueue::Pause()
{
	std::lock_guard<std::mutex> lk(m_shared->mx);
	m_shared->bPaused = true;
}

void MultiThreadJobQueue::Resume()
{
	{
		std::lock_guard<std::mutex> lk(m_shared->mx);
		m_shared->bPaused = false;
	}
	m_shared->cv.notify_all();
}

BOOL MultiThreadJobQueue::IsJobAssignerActive()
{
	std::lock_guard<std::mutex> lk(m_shared->mx);
	return !m_shared->bPaused;
}

INT MultiThreadJobQueue::GetMaxThreads()
{
	std::lock_guard<std::mutex> lk(m_shared->mx);
	return m_shared->maxThreads;
}

void MultiThreadJobQueue::SetMaxThreads(INT threads)
{
	ASSERT(threads > 0);
	{
		std::lock_guard<std::mutex> lk(m_shared->mx);
		m_shared->maxThreads = threads > 0 ? threads : 1;
		SpawnWorkersLocked();
	}
	m_shared->cv.notify_all();
}

UINT MultiThreadJobQueue::GetAssignJobDelay()
{
	std::lock_guard<std::mutex> lk(m_shared->mx);
	return m_shared->assignJobDelay;
}

void MultiThreadJobQueue::SetAssignJobDelay(UINT ms)
{
	{
		std::lock_guard<std::mutex> lk(m_shared->mx);
		m_shared->assignJobDelay = ms;
	}
	m_shared->cv.notify_all();
}

MultiThreadJobQueue::JobStatisics MultiThreadJobQueue::GetStatistics()
{
	std::lock_guard<std::mutex> lk(m_shared->mx);
	JobStatisics stats;
	stats.waiting = (INT)m_shared->waiting.size();
	stats.working = (INT)m_shared->running.size();
	stats.finished = (INT)m_shared->finished.size();
	stats.total = m_shared->total;
	return stats;
}

BOOL MultiThreadJobQueue::RetrieveFinishedJob(JobSP& job)
{
	std::lock_guard<std::mutex> lk(m_shared->mx);
	if (m_shared->finished.empty())
		return FALSE;
	job = m_shared->finished.front();
	m_shared->finished.pop();
	return TRUE;
}

UINT MultiThreadJobQueue::WorkerST(LPVOID pParam)
{
	ASSERT(pParam != NULL);
	std::shared_ptr<Shared> pShared(*(std::shared_ptr<Shared>*)pParam);
	delete (std::shared_ptr<Shared>*)pParam;
	Shared& s = *pShared;

	//Jobs may use COM (e.g. the DirectShow tagger).
	HRESULT hrCom = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	std::unique_lock<std::mutex> lk(s.mx);
	while (!s.bExit)
	{
		if (s.bPaused || s.waiting.empty() || (INT)s.running.size() >= s.maxThreads)
		{
			s.cv.wait(lk);
			continue;
		}
		if (s.assignJobDelay > 0 && s.lastAssignTick != 0)
		{
			ULONGLONG elapsed = GetTickCount64() - s.lastAssignTick;
			if (elapsed < s.assignJobDelay)
			{
				s.cv.wait_for(lk, std::chrono::milliseconds(s.assignJobDelay - elapsed));
				continue;
			}
		}
		JobSP job = s.waiting.front();
		s.waiting.pop_front();
		s.running.push_back(job);
		s.lastAssignTick = GetTickCount64();

		lk.unlock();
		job->Execute();
		lk.lock();

		s.running.erase(std::find(s.running.begin(), s.running.end(), job));
		s.finished.push(job);
		s.cv.notify_all();		//A slot is free (maxThreads) / delay may have passed
	}
	s.threads--;
	lk.unlock();

	if (SUCCEEDED(hrCom))
		CoUninitialize();
	return 0;
}
