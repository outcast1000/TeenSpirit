#ifndef _InfoDownloadManager_h_
#define _InfoDownloadManager_h_

#include "InformationProviders/InfoProvider.h"
#include "DataRecords.h"
#include "AppOptions.h"		//AppOptionsEnum
#include "MessageViewer.h"	//SEVERITY
#include "TagInfo.h"
#include <set>
#include <memory>
#include <deque>
#include "MultiThreadJobQueue.h"	//JobSP
class InfoProviderFactory;
class SQLManager;
class ITranslation;

//	Downloads artist/album/track information and pictures on background threads
//	and stores the results. Requests and ConsumeResult (which handles the finished
//	downloads) run on the UI thread.
class InfoDownloadManager
{
public:
	//What the download manager needs from the application.
	class Host
	{
	public:
		virtual ~Host()														{}
		virtual void SendStateMessage(UINT stateMessage)					= 0;	//SM_InfoChanged, SM_PictureChanged
		virtual void NotifyUser(LPCTSTR msg, SEVERITY sev)					= 0;
		//Store a downloaded picture. FALSE when it is already there.
		virtual BOOL AddArtistPicture(const ArtistRecord& rec, LPCTSTR path)	= 0;
		virtual BOOL AddAlbumPicture(const FullAlbumRecord& rec, LPCTSTR path)	= 0;
		//Write accepted lyrics into the file's tags.
		virtual void TagTrack(LPCTSTR path, TagInfo& ti)					= 0;

		//Asks the user about downloaded lyrics/tablatures; text may be edited.
		enum Answer
		{
			ANS_Accept,
			ANS_AcceptAll,		//accept the rest of this batch too
			ANS_Retry,			//try the provider's next result
			ANS_Cancel,
			ANS_CancelAll,		//cancel the rest of this batch
		};
		virtual Answer ConfirmTrackInfo(InfoItemTypeEnum iit, LPCTSTR typeName, LPCTSTR details,
			LPCTSTR provider, std::tstring& text)							= 0;
	};

	InfoDownloadManager(InfoProviderFactory& ipf, SQLManager& sqlManager, AppOptions& options,
		ITranslation& translation, std::unique_ptr<Host> host);
	~InfoDownloadManager();

	//Stops the download jobs (asks them to abort and waits). Returns FALSE if one
	//did not exit in time; it then keeps running and still uses its objects.
	BOOL StopBackgroundWork();

public:
	//providerName: an IPI_UniqueID; NULL for the default provider of the service.
	//bForce: ignore the auto-download option and the "already tried" check.
	//bSync: run the download now, on this thread; its result is handled by the next ConsumeResult.
	BOOL RequestArtistInfo(ArtistRecord& rec, LPCTSTR providerName = NULL, BOOL bForce = FALSE, BOOL bSync = FALSE);
	BOOL RequestArtistPic(ArtistRecord& rec, LPCTSTR providerName = NULL, BOOL bForce = FALSE, BOOL bSync = FALSE);
	BOOL RequestAlbumInfo(AlbumRecord& rec, LPCTSTR artist, LPCTSTR providerName = NULL, BOOL bForce = FALSE, BOOL bSync = FALSE);
	BOOL RequestAlbumPic(AlbumRecord& rec, LPCTSTR artist, LPCTSTR providerName = NULL, BOOL bForce = FALSE, BOOL bSync = FALSE);
	BOOL RequestLyrics(TrackRecord& rec, LPCTSTR artist, LPCTSTR providerName = NULL, BOOL bForce = FALSE, BOOL bSync = FALSE);
	BOOL RequestTablatures(TrackRecord& rec, LPCTSTR artist, LPCTSTR providerName = NULL, BOOL bForce = FALSE, BOOL bSync = FALSE);


	void ResetArtistCache(ArtistRecord& rec);
	void ResetAlbumCache(AlbumRecord& rec, LPCTSTR artist);
	void ResetTrackCache(TrackRecord& rec, LPCTSTR artist);


	BOOL IsWorking();
	INT GetPendingJobs();

	void ConsumeResult();

	//The provider used when a request names none (an IPI_UniqueID, or NULL).
	static LPCTSTR GetDefaultProvider(IInfoProvider::ServiceEnum service);

private:
	InfoDownloadManager(const InfoDownloadManager&);
	InfoDownloadManager& operator=(const InfoDownloadManager&);

	BOOL IsFirstRequest(const IInfoProvider::Request& request);
	MultiThreadJobQueue& GetQueue();
	BOOL NextFinishedJob(JobSP& job);


	BOOL RequestInfo(IInfoProvider::Request& request,
		UINT helpID, LPCTSTR providerName, BOOL bForce, BOOL bSync,
		AppOptionsEnum optionToCheck, InfoItemTypeEnum iit);
	BOOL HandleArtistInfo(UINT artID, const IInfoProvider::Result& result);
	BOOL HandleArtistPic(UINT artID, const IInfoProvider::Result& result);
	BOOL HandleAlbumInfo(UINT albID, const IInfoProvider::Result& result);
	BOOL HandleAlbumPic(UINT albID, const IInfoProvider::Result& result);
	BOOL HandleTrackInfo(UINT traID, const IInfoProvider::Result& result, InfoItemTypeEnum iit);

	void NotifyUserForSuccess(LPCTSTR infoType, LPCTSTR reqInfo, LPCTSTR addInfo);
	LPCTSTR Str(INT resID);
	static UINT CalcStringHash(LPCTSTR str);
	static UINT CalcRequestHash(const IInfoProvider::Request& request);


private:
	InfoProviderFactory& m_IPFactory;
	SQLManager& m_sqlManager;
	AppOptions& m_options;
	ITranslation& m_translation;
	std::unique_ptr<Host> m_host;
	MultiThreadJobQueue* m_pMTQueue;
	BOOL m_bWorkStopped;
	BOOL m_bWorkerStillRunning;
	BOOL m_bAcceptAll;
	BOOL m_bCancelAll;
	BOOL m_bReEntranceProtection;
	std::set<UINT> m_RequestHashes;//Used by IsFirstRequest
	BOOL m_bOldInfoRequestsRead;
	std::deque<JobSP> m_syncFinished;	//bSync requests, handled by ConsumeResult


};

#endif