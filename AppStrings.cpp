#include "stdafx.h"
#include "AppStrings.h"
#include "ITranslation/ITranslation.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	LPCTSTR const cErrStr = _T("ErrorString");

	//Looks up table[idx] when 0 <= idx < count, otherwise returns outOfRange.
	template <size_t N>
	INT Lookup(const INT (&table)[N], INT idx, INT outOfRange)
	{
		return (idx >= 0 && idx < (INT)N) ? table[idx] : outOfRange;
	}
}

LPCTSTR AppStrings::Get(ITranslation& tr, INT resID)
{
	ASSERT(resID != 0);
	LPCTSTR ret = NULL;
	if (resID > 0)
		ret = tr.GetString(resID);
	else
		TRACE(_T("@0 AppStrings::Get Unknown resID:%d\r\n"), resID);
	return ret != NULL ? ret : _T("");
}

LPCTSTR AppStrings::ForItemType(ITranslation& tr, ItemTypeEnum it)
{
	ASSERT(it >= IT_Unknown && it < IT_Last);
	static const INT res[] =
	{
		IDS_UNKNOWN,		//IT_Unknown
		IDS_VARIOUS,		//IT_Various
		IDS_ARTIST,			//IT_Artist
		IDS_ALBUM,			//IT_Album
		IDS_GENRE,			//IT_Genre
		IDS_YEAR,			//IT_Year
		IDS_UNKNOWN,		//IT_AlbumCollection
		IDS_COLLECTION,		//IT_Collection
		IDS_TITLE,			//IT_Track
		IDS_UNKNOWN,		//IT_MultipleTracks
		IDS_UNKNOWN			//IT_QuickSearch
	};
	static_assert(sizeof(res) / sizeof(res[0]) == IT_Last, "one entry per ItemTypeEnum value");
	return Get(tr, Lookup(res, it, IDS_UNKNOWN));
}

LPCTSTR AppStrings::ForInfoItemType(ITranslation& tr, InfoItemTypeEnum iit)
{
	ASSERT(iit >= IIT_Unknown && iit < IIT_Last);
	static const INT res[] =
	{
		IDS_UNKNOWN,		//IIT_Unknown
		IDS_ARTISTBIO,
		IDS_ALBUMREVIEW,
		IDS_COMMENTS,
		IDS_LYRICS,
		IDS_PERSONAL,
		IDS_TABLATURES,
		IDS_ARTISTPICTURE,
		IDS_ALBUMPICTURE
	};
	//The remaining picture types (genre, year, ...) have no name of their own.
	return Get(tr, Lookup(res, iit, IDS_UNKNOWN));
}

LPCTSTR AppStrings::ForCollectionType(ITranslation& tr, CollectionTypesEnum ct)
{
	ASSERT(ct >= CTYPE_Unknown && ct < CTYPE_Last);
	static const INT res[] =
	{
		IDS_UNKNOWN,		//CTYPE_Unknown
		IDS_LOCALFOLDER,
		IDS_MEDIA,
		IDS_NETFOLDER,
		IDS_VIRTUALTRACKS,	//CTYPE_Virtual
		IDS_REMOVABLE,
		IDS_AUDIOCD,
		IDS_UNKNOWN
	};
	static_assert(sizeof(res) / sizeof(res[0]) == CTYPE_Last, "one entry per CollectionTypesEnum value");
	return Get(tr, Lookup(res, ct, IDS_UNKNOWN));
}

LPCTSTR AppStrings::ForRandomMode(ITranslation& tr, RandomTrackModeEnum rtm)
{
	ASSERT(rtm >= RTM_All && rtm < RTM_Last);
	static const INT res[] =
	{
		IDS_ALL,
		IDS_ARTIST,
		IDS_ALBUM,
		IDS_GENRE,
		IDS_YEAR,
		IDS_COLLECTION,
		IDS_NEW
	};
	static_assert(sizeof(res) / sizeof(res[0]) == RTM_Last, "one entry per RandomTrackModeEnum value");
	return Get(tr, Lookup(res, rtm, IDS_UNKNOWN));
}

LPCTSTR AppStrings::ForTrackTextType(ITranslation& tr, TrackTextType ttt)
{
	ASSERT(ttt >= 0 && ttt < TTT_Last);
	static const INT res[] = {IDS_TITLE, IDS_ALBUM, IDS_ARTIST};
	static_assert(sizeof(res) / sizeof(res[0]) == TTT_Last, "one entry per TrackTextType value");
	return Get(tr, Lookup(res, ttt, IDS_UNKNOWN));
}

LPCTSTR AppStrings::ForPlayListMode(ITranslation& tr, MediaPlayerNextMode mpm)
{
	ASSERT(mpm >= MPNM_Normal && mpm < MPNM_Last);
	static const INT res[] = {IDS_NORMAL, IDS_SHUFFLE, IDS_LOOP, IDS_AUTOCONTINUE};
	static_assert(sizeof(res) / sizeof(res[0]) == MPNM_Last, "one entry per MediaPlayerNextMode value");
	return Get(tr, Lookup(res, mpm, IDS_NORMAL));
}

LPCTSTR AppStrings::ForTrackType(ITranslation& tr, TrackTypesEnum tt)
{
	if (tt < TTYPE_FIRSTVIDEO)
		return Get(tr, IDS_AUDIO);
	else if (tt < TTYPE_FIRSTVIRTUAL)
		return Get(tr, IDS_VIDEO);
	return Get(tr, IDS_VIRTUALTRACKS);
}

LPCTSTR AppStrings::ForStars(ITranslation& tr, UINT stars)
{
	ASSERT(stars < 6);
	static const INT res[] = {IDS_CLEAR, IDS_POOR, IDS_OK, IDS_GOOD, IDS_VERYGOOD, IDS_OUTSTANDING};
	if (stars >= sizeof(res) / sizeof(res[0]))
		return cErrStr;
	return Get(tr, res[stars]);
}

LPCTSTR AppStrings::ForCaseStyle(ITranslation& tr, CaseStyleEnum cStyle)
{
	ASSERT(cStyle >= 0 && cStyle < CASE_Last);
	static const INT res[] = {IDS_ALLCAPS, IDS_NOCAPS, IDS_FIRSTCAP, IDS_PROPERCASE};
	static_assert(sizeof(res) / sizeof(res[0]) == CASE_Last, "one entry per CaseStyleEnum value");
	if (!(cStyle >= 0 && cStyle < CASE_Last))
		return cErrStr;
	return Get(tr, res[cStyle]);
}

LPCTSTR AppStrings::ForRecordSortOption(ITranslation& tr, RecordSortOptionsEnum rso)
{
	ASSERT(rso > RSO_None && rso < RSO_Last);
	static const INT res[] = {0, IDS_NAME, IDS_TRACKNO, IDS_ALBUM, IDS_ARTIST,
		IDS_RATING, IDS_LOCATION, IDS_SIZE, IDS_DURATION, IDS_BITRATE, IDS_YEAR,
		IDS_GENRE, IDS_COLLECTION, IDS_DATEADDED, IDS_NUMBEROFTRACKS,
		IDS_HITS, IDS_FIRSTAPPEARANCE, IDS_LASTAPPEARANCE};
	static_assert(sizeof(res) / sizeof(res[0]) == RSO_Last, "one entry per RecordSortOptionsEnum value");
	if (!(rso > RSO_None && rso < RSO_Last))
		return cErrStr;
	return Get(tr, res[rso]);
}

LPCTSTR AppStrings::ForSectionFilter(ITranslation& tr, SectionFiltersEnum sf)
{
	ASSERT(sf > SFILT_First && sf < SFILT_Last);
	static const INT res[] = {0, IDS_ALL, IDS_AUDIO, IDS_VIDEO, IDS_NEW};
	static_assert(sizeof(res) / sizeof(res[0]) == SFILT_Last, "one entry per SectionFiltersEnum value");
	if (!(sf > SFILT_First && sf < SFILT_Last))
		return cErrStr;
	return Get(tr, res[sf]);
}

LPCTSTR AppStrings::ForSectionType(ITranslation& tr, SectionTypeEnum st)
{
	ASSERT(st > ST_Unknown && st < ST_Last);
	static const INT res[] = {0, 0, IDS_ARTIST, IDS_ALBUM, IDS_GENRE, IDS_YEAR, IDS_COLLECTION,
		IDS_ALBUM, IDS_MONTH, 0};
	static_assert(sizeof(res) / sizeof(res[0]) == ST_Last, "one entry per SectionTypeEnum value");
	if (!(st > ST_Unknown && st < ST_Last))
		return cErrStr;
	if (res[st] == 0)
		return _T("");	//ST_Directory has no name
	return Get(tr, res[st]);
}
