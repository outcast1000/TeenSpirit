//	MultiThreadJobQueue tests: scheduling, limits, COM on workers, abort and
//	shutdown (threads are never terminated), and no busy-waiting when idle.
#include "stdafx.h"
#include "TestFramework.h"
#include "MultiThreadJobQueue.h"
#include <atomic>
#include <vector>
#include <algorithm>

namespace
{
	std::atomic<INT> gRunningNow(0);
	std::atomic<INT> gMaxRunning(0);

	class TestJob : public Job
	{
	public:
		//workMs: how long Execute takes. cooperative: whether Abort() shortens it.
		TestJob(INT id, DWORD workMs, BOOL cooperative = TRUE):
			m_id(id), m_workMs(workMs), m_cooperative(cooperative),
			m_state(ST_Idle), m_bAbort(FALSE), m_comHr(E_FAIL), m_bSTA(FALSE),
			m_startTick(0), m_bExecuted(FALSE), m_bCompleted(FALSE)	{}
		virtual State GetState()	{return m_state;}
		virtual void Abort()		{m_bAbort = TRUE;}
		virtual void Execute()
		{
			m_state = ST_Running;
			m_bExecuted = TRUE;
			m_startTick = GetTickCount64();
			INT now = ++gRunningNow;
			INT prev = gMaxRunning;
			while (now > prev && !gMaxRunning.compare_exchange_weak(prev, now)) {}

			//COM must already be initialized as STA on worker threads.
			m_comHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			if (SUCCEEDED(m_comHr))
				CoUninitialize();
			APTTYPE apt;
			APTTYPEQUALIFIER q;
			m_bSTA = SUCCEEDED(CoGetApartmentType(&apt, &q)) && (apt == APTTYPE_STA || apt == APTTYPE_MAINSTA);

			ULONGLONG start = GetTickCount64();
			while (GetTickCount64() - start < m_workMs)
			{
				if (m_cooperative && m_bAbort)
					break;
				Sleep(5);
			}
			--gRunningNow;
			m_bCompleted = TRUE;
			m_state = m_bAbort ? ST_Aborted : ST_Finished;
		}
		INT m_id;
		DWORD m_workMs;
		BOOL m_cooperative;
		std::atomic<State> m_state;
		std::atomic<BOOL> m_bAbort;
		HRESULT m_comHr;
		BOOL m_bSTA;
		ULONGLONG m_startTick;
		std::atomic<BOOL> m_bExecuted;
		std::atomic<BOOL> m_bCompleted;
	};
	typedef std::shared_ptr<TestJob> TestJobSP;

	BOOL WaitFor(MultiThreadJobQueue& q, INT finished, DWORD timeoutMs)
	{
		ULONGLONG start = GetTickCount64();
		while (GetTickCount64() - start < timeoutMs)
		{
			MultiThreadJobQueue::JobStatisics s = q.GetStatistics();
			if (s.finished >= finished && s.working == 0 && s.waiting == 0)
				return TRUE;
			Sleep(10);
		}
		return FALSE;
	}

	ULONGLONG ProcessCpuMs()
	{
		FILETIME c, e, k, u;
		GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u);
		ULARGE_INTEGER kk, uu;
		kk.LowPart = k.dwLowDateTime; kk.HighPart = k.dwHighDateTime;
		uu.LowPart = u.dwLowDateTime; uu.HighPart = u.dwHighDateTime;
		return (kk.QuadPart + uu.QuadPart) / 10000;
	}
}

static void TestRunsAllJobsWithinLimit()
{
	wprintf(L"TestRunsAllJobsWithinLimit\n");
	gRunningNow = 0;
	gMaxRunning = 0;
	MultiThreadJobQueue q;
	q.SetMaxThreads(3);
	std::vector<TestJobSP> jobs;
	for (INT i = 0; i < 20; i++)
	{
		jobs.push_back(TestJobSP(new TestJob(i, 30)));
		CHECK(q.AddJob(jobs.back()));
	}
	CHECK(q.GetStatistics().total == 20);
	CHECK(WaitFor(q, 20, 10000));
	CHECK(gMaxRunning == 3);
	INT retrieved = 0;
	JobSP job;
	while (q.RetrieveFinishedJob(job))
	{
		CHECK(job->GetState() == Job::ST_Finished);
		retrieved++;
	}
	CHECK(retrieved == 20);
	CHECK(!q.RetrieveFinishedJob(job));
	BOOL comOK = TRUE;
	for (size_t i = 0; i < jobs.size(); i++)
		if (jobs[i]->m_comHr != S_FALSE || !jobs[i]->m_bSTA)	//S_FALSE: already initialized
			comOK = FALSE;
	CHECK(comOK);
	CHECK(q.Shutdown(5000));
	CHECK(!q.AddJob(JobSP(new TestJob(99, 0))));
}

static void TestMaxThreadsChange()
{
	wprintf(L"TestMaxThreadsChange\n");
	gRunningNow = 0;
	gMaxRunning = 0;
	MultiThreadJobQueue q;
	q.SetMaxThreads(1);
	for (INT i = 0; i < 6; i++)
		q.AddJob(JobSP(new TestJob(i, 100)));
	Sleep(50);
	CHECK(q.GetStatistics().working == 1);
	q.SetMaxThreads(4);		//must start more workers for the waiting jobs
	Sleep(60);
	CHECK(q.GetStatistics().working == 4);
	CHECK(WaitFor(q, 6, 5000));
	CHECK(gMaxRunning == 4);
}

static void TestPauseResume()
{
	wprintf(L"TestPauseResume\n");
	MultiThreadJobQueue q;
	q.SetMaxThreads(2);
	q.Pause();
	CHECK(!q.IsJobAssignerActive());
	for (INT i = 0; i < 4; i++)
		q.AddJob(JobSP(new TestJob(i, 10)));
	Sleep(150);
	MultiThreadJobQueue::JobStatisics s = q.GetStatistics();
	CHECK(s.waiting == 4 && s.working == 0 && s.finished == 0);
	q.Resume();
	CHECK(q.IsJobAssignerActive());
	CHECK(WaitFor(q, 4, 5000));
}

static void TestAbortJobs()
{
	wprintf(L"TestAbortJobs\n");
	MultiThreadJobQueue q;
	q.SetMaxThreads(1);
	TestJobSP running(new TestJob(0, 60000));
	TestJobSP waiting(new TestJob(1, 10));
	q.AddJob(running);
	q.AddJob(waiting);
	Sleep(100);
	CHECK(running->m_bExecuted);
	ULONGLONG t0 = GetTickCount64();
	q.AbortJobs();
	CHECK(WaitFor(q, 2, 5000));
	CHECK(GetTickCount64() - t0 < 2000);
	CHECK(running->GetState() == Job::ST_Aborted);
	CHECK(!waiting->m_bExecuted && waiting->GetState() == Job::ST_Idle);	//never started
	//The queue keeps working after an abort
	TestJobSP after(new TestJob(2, 10));
	q.AddJob(after);
	CHECK(WaitFor(q, 3, 5000));
	CHECK(after->GetState() == Job::ST_Finished);
}

static void TestAssignDelay()
{
	wprintf(L"TestAssignDelay\n");
	MultiThreadJobQueue q;
	q.SetMaxThreads(3);
	q.SetAssignJobDelay(100);
	CHECK(q.GetAssignJobDelay() == 100);
	std::vector<TestJobSP> jobs;
	for (INT i = 0; i < 3; i++)
	{
		jobs.push_back(TestJobSP(new TestJob(i, 10)));
		q.AddJob(jobs.back());
	}
	CHECK(WaitFor(q, 3, 5000));
	std::vector<ULONGLONG> starts;
	for (size_t i = 0; i < jobs.size(); i++)
		starts.push_back(jobs[i]->m_startTick);
	std::sort(starts.begin(), starts.end());
	CHECK(starts[1] - starts[0] >= 90 && starts[2] - starts[1] >= 90);
}

static void TestIdleDoesNotSpin()
{
	wprintf(L"TestIdleDoesNotSpin\n");
	MultiThreadJobQueue q;
	q.SetMaxThreads(4);
	for (INT i = 0; i < 4; i++)
		q.AddJob(JobSP(new TestJob(i, 1)));
	CHECK(WaitFor(q, 4, 5000));
	//4 idle workers + the old design's polling would burn CPU here.
	ULONGLONG cpu0 = ProcessCpuMs();
	Sleep(1000);
	ULONGLONG cpu = ProcessCpuMs() - cpu0;
	wprintf(L"  CPU while idle for 1 s: %I64u ms\n", cpu);
	CHECK(cpu < 50);
}

static void TestShutdownNeverTerminates()
{
	wprintf(L"TestShutdownNeverTerminates\n");
	//A job that ignores Abort: Shutdown times out but does not kill its thread.
	TestJobSP stubborn(new TestJob(0, 800, FALSE));
	{
		MultiThreadJobQueue q;
		q.AddJob(stubborn);
		Sleep(50);
		ULONGLONG t0 = GetTickCount64();
		CHECK(!q.Shutdown(100));
		CHECK(GetTickCount64() - t0 < 500);
		CHECK(!stubborn->m_bCompleted);
	}	//queue destroyed while the job is still running
	Sleep(1200);
	CHECK(stubborn->m_bCompleted);	//the thread was left alive and finished its work

	//A cooperative job: Shutdown aborts it and succeeds promptly.
	TestJobSP polite(new TestJob(1, 60000));
	MultiThreadJobQueue q2;
	q2.AddJob(polite);
	Sleep(50);
	ULONGLONG t0 = GetTickCount64();
	CHECK(q2.Shutdown(5000));
	CHECK(GetTickCount64() - t0 < 1000);
	CHECK(polite->GetState() == Job::ST_Aborted);
}

static void TestDestructorWithPendingJobs()
{
	wprintf(L"TestDestructorWithPendingJobs\n");
	ULONGLONG t0 = GetTickCount64();
	{
		MultiThreadJobQueue q;
		q.SetMaxThreads(2);
		for (INT i = 0; i < 50; i++)
			q.AddJob(JobSP(new TestJob(i, 60000)));
		Sleep(50);
	}
	CHECK(GetTickCount64() - t0 < 2000);
}

void RunJobQueueTests()
{
	TestRunsAllJobsWithinLimit();
	TestMaxThreadsChange();
	TestPauseResume();
	TestAbortJobs();
	TestAssignDelay();
	TestIdleDoesNotSpin();
	TestShutdownNeverTerminates();
	TestDestructorWithPendingJobs();
}
