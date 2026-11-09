//	/*
// 	*
// 	* Copyright (C) 2003-2010 Alexandros Economou
//	*
//	* This file is part of Jaangle (http://www.jaangle.com)
// 	*
// 	* This Program is free software; you can redistribute it and/or modify
// 	* it under the terms of the GNU General Public License as published by
// 	* the Free Software Foundation; either version 2, or (at your option)
// 	* any later version.
// 	*
// 	* This Program is distributed in the hope that it will be useful,
// 	* but WITHOUT ANY WARRANTY; without even the implied warranty of
// 	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// 	* GNU General Public License for more details.
// 	*
// 	* You should have received a copy of the GNU General Public License
// 	* along with GNU Make; see the file COPYING. If not, write to
// 	* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
// 	* http://www.gnu.org/copyleft/gpl.html
// 	*
//	*/ 
#include "stdafx.h"
#include "MultiTracker.h"

#include "LocalHistoryTracker.h"
#include "LastFMAdapterTracker.h"
#include "AppOptions.h"


MultiTracker::MultiTracker(AppOptions& options, SQLManager& sqlManager, std::function<LastFMServices*()> getLastFM):
m_options(options),
m_sqlManager(sqlManager),
m_getLastFM(getLastFM),
m_pLH(NULL),
m_pLFM(NULL)
{
}

MultiTracker::~MultiTracker(void)
{
	delete m_pLFM;
	delete m_pLH;
}

void MultiTracker::SubmitTrack(Action action, const TrackerTrack& track, UINT actionTime)
{
	TRACEST(_T("MultiTracker::SubmitTrack"));
	if (m_options.Get(OPT_GEN_LastFMScrobbler))
	{
		if (m_pLFM == NULL)
		{
			LastFMServices* pLastFM = m_getLastFM ? m_getLastFM() : NULL;
			if (pLastFM != NULL)
			{
				m_pLFM = new LastFMAdapterTracker;
				m_pLFM->SetLastFM(pLastFM);
			}
		}
		if (m_pLFM != NULL)
			m_pLFM->SubmitTrack(action, track, actionTime);
	}
	if (m_options.Get(OPT_GEN_NotifyHistory))
	{
		if (m_pLH == NULL)
		{
			m_pLH = new LocalHistoryTracker;
			m_pLH->SetSQLManager(&m_sqlManager);
		}
		m_pLH->SubmitTrack(action, track, actionTime);
	}
}
