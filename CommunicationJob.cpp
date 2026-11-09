
#include "stdafx.h"
#include "CommunicationJob.h"
#include "WebPageUtilities.h"

CommunicationJob::~CommunicationJob()
{
}

void CommunicationJob::Execute()
{
	m_state = ST_Running;
	std::string page;
	//m_url = _T("http://services.artificialspirit.com/tsservice.php");
	//TRACE(_T("@3 CommunicationJob::Execute\r\n"));
	TRACEST(_T("CommunicationJob::Execute"), m_url.c_str());
	if (m_hNet)
	{
		if (m_data.empty())
		{
			if (DownloadWebPage(page, m_hNet, m_url.c_str()))
			{
				if (page != "OK")
					TRACE(_T("@1 CommunicationJob::Execute RESP: '%s'"), CA2CT(page.c_str()));
			}
		}
		else
		{
			if (DownloadWebPagePost(page, m_hNet, m_url.c_str(), m_data.c_str()))
			{
				if (page != "OK")
					TRACE(_T("@1 CommunicationJob::Execute RESP: '%s'"), CA2CT(page.c_str()));
			}

		}
	}
	else
		TRACE(_T("@1 CommunicationJob::Execute hNet == NULL\r\n"));

	m_state = ST_Finished;
}


