#ifndef _AppStrings_h_
#define _AppStrings_h_

//	Translated display names for the application's enums. Each function takes the
//	translation to use, so it can be called (and tested) without PrgAPI.
//	Missing strings come back as "" and out-of-range values as "ErrorString";
//	the returned pointers are owned by the translation.

#include "TeenSpiritEnums.h"
#include "RecordCollectionSorter.h"		//RecordSortOptionsEnum
#include "MediaPlayers/MediaPlayer.h"	//MediaPlayerNextMode

class ITranslation;

namespace AppStrings
{
	//The string for a resource ID ("" when the translation has none).
	LPCTSTR Get(ITranslation& tr, INT resID);

	LPCTSTR ForItemType(ITranslation& tr, ItemTypeEnum it);
	LPCTSTR ForInfoItemType(ITranslation& tr, InfoItemTypeEnum iit);
	LPCTSTR ForCollectionType(ITranslation& tr, CollectionTypesEnum ct);
	LPCTSTR ForRandomMode(ITranslation& tr, RandomTrackModeEnum rtm);
	LPCTSTR ForTrackTextType(ITranslation& tr, TrackTextType ttt);
	LPCTSTR ForPlayListMode(ITranslation& tr, MediaPlayerNextMode mpm);
	LPCTSTR ForTrackType(ITranslation& tr, TrackTypesEnum tt);
	LPCTSTR ForStars(ITranslation& tr, UINT stars);	//0 (clear) - 5
	LPCTSTR ForCaseStyle(ITranslation& tr, CaseStyleEnum cStyle);
	LPCTSTR ForRecordSortOption(ITranslation& tr, RecordSortOptionsEnum rso);
	LPCTSTR ForSectionFilter(ITranslation& tr, SectionFiltersEnum sf);
	LPCTSTR ForSectionType(ITranslation& tr, SectionTypeEnum st);
}

#endif
