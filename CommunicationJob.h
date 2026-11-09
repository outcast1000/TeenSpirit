#ifndef _CommunicationJobJob_h_
#define _CommunicationJobJob_h_

#include "MultiThreadJobQueue.h"
#include <atomic>
#include <WinInet.h>

class CommunicationJob: public Job
{
public:
	virtual ~CommunicationJob();

	virtual State GetState()									{return m_state;}
	virtual void Execute();
	virtual void Abort()										{}


	void SetHINTERNET(HINTERNET hNet)							{m_hNet = hNet;}
	static CommunicationJob* CreateJob(LPCTSTR url, LPCSTR data, HINTERNET hNet)
	{
		CommunicationJob* pJob = new CommunicationJob(url, data);
		pJob->SetHINTERNET(hNet);
		return pJob;
	}

private:
	CommunicationJob(LPCTSTR url, LPCSTR data):m_url(url),m_data(data),m_state(ST_Idle),m_hNet(NULL)
	{}
	std::basic_string<TCHAR> m_url;
	std::basic_string<CHAR> m_data;
	std::atomic<State> m_state;
	HINTERNET m_hNet;


};


#endif