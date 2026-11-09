#ifndef _LocalPictureManager_h_
#define _LocalPictureManager_h_

class IInfoProvider;
class DirImageInfoProvider;
class CachedImageInfoProvider;
class SQLManager;
class InfoProviderFactory;

#include "DataRecords.h"
#include "GuiControls/GdiPlusBitmapCache.h"
#include "GuiControls/GdiPlusPicDrawer.h"
#include <memory>

//	Finds the pictures of artists and albums: the stored main picture (in the
//	database), the pictures downloaded or added by the user (in the storage
//	folder), and for albums the images in the album's folder.
class LocalPictureManager
{
public:
	//What the picture manager needs from the application.
	class Host
	{
	public:
		virtual ~Host()														{}
		//Asked when a thumbnail is drawn for an item without a picture.
		virtual void RequestArtistPicture(ArtistRecord& rec)				= 0;
		virtual void RequestAlbumPicture(AlbumRecord& rec, LPCTSTR artist)	= 0;
	};

	//The two picture providers are registered with (and owned by) providers.
	LocalPictureManager(SQLManager& sqlManager, InfoProviderFactory& providers, std::unique_ptr<Host> host);
	~LocalPictureManager();

public:
	BOOL Init(LPCTSTR StoragePath);

	void LoadState(LPCTSTR stateRoot);
	void SaveState(LPCTSTR stateRoot);

	BOOL AddArtistPicture(const ArtistRecord& rec, LPCTSTR imagePath);
	BOOL GetArtistPictures(const ArtistRecord& rec, std::tstringvector& col);
	LPCTSTR GetMainArtistPicture(const ArtistRecord& rec);
	void RemoveArtistPicture(const ArtistRecord& rec, LPCTSTR imagePath);
	void ResetArtistCache(const ArtistRecord& rec);
	void SetMainArtistPicture(const ArtistRecord& rec, LPCTSTR path);


	BOOL AddAlbumPicture(const FullAlbumRecord& rec, LPCTSTR imagePath);
	BOOL GetAlbumPictures(const FullAlbumRecord& rec, std::tstringvector& col);
	LPCTSTR GetMainAlbumPicture(const FullAlbumRecord& rec);
	void RemoveAlbumPicture(const FullAlbumRecord& rec, LPCTSTR imagePath);
	void ResetAlbumCache(const FullAlbumRecord& rec);
	void SetMainAlbumPicture(const FullAlbumRecord& rec, LPCTSTR path);

	//=== Returns TRUE if a vaild picture have been found and correctly drawn
	BOOL DrawThumbnail(InfoItemTypeEnum iit, UINT itemID, Gdiplus::Graphics& g, Gdiplus::Rect& rcDest);
	BOOL DrawThumbnail(InfoItemTypeEnum iit, UINT itemID, HDC hdc, RECT& rcDest);

	BOOL DrawDefaultThumbnail(InfoItemTypeEnum iit, Gdiplus::Graphics& g, Gdiplus::Rect& rcDest);
	BOOL DrawDefaultThumbnail(InfoItemTypeEnum iit, HDC hdc, RECT& rcDest);


	void EnableFolderImages(BOOL bEnable)		{m_bEnableFolderImages = bEnable;}
	BOOL IsFolderImagesEnabled()				{return m_bEnableFolderImages;}

	void SetDefaultThumbnail(InfoItemTypeEnum iit, Gdiplus::Image* pImage);
	Gdiplus::Image* GetDefaultThumbnail(InfoItemTypeEnum iit);

	//void ProcessFailedRequests();

private:
	LPCTSTR GetFirstArtistPicture(const ArtistRecord& rec);
	LPCTSTR GetFirstAlbumPicture(const FullAlbumRecord& rec);
	DWORD CalculateCacheUID(InfoItemTypeEnum iit, UINT itemID);
	//void RequestArtistPicDownload(const FullArtistRecordSP& rec);
	//void RequestAlbumPicDownload(const FullAlbumRecordSP& rec);
private:
	LocalPictureManager(const LocalPictureManager&);
	LocalPictureManager& operator=(const LocalPictureManager&);

	SQLManager& m_sqlManager;
	InfoProviderFactory& m_providers;
	std::unique_ptr<Host> m_host;
	BOOL m_bEnableFolderImages;

	GdiPlusBitmapCache* m_pThumbnailCache; 



	CachedImageInfoProvider* m_pCIIP;
	DirImageInfoProvider* m_pDIIP;
	typedef std::map<UINT, std::tstring> CacheContainer;
	CacheContainer m_artists;
	CacheContainer m_albums;

	GdiPlusPicDrawer m_defGlobal;
	GdiPlusPicDrawer m_defAlbum;
	GdiPlusPicDrawer* m_defDrawer[IIT_Last];


	//std::list<FullArtistRecordSP> m_failedArtistRequests;
	//std::list<FullAlbumRecordSP> m_failedAlbumRequests;
};

#endif