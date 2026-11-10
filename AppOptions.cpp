#include "stdafx.h"
#include "AppOptions.h"
#include "AppSettings/AppSettings.h"
#include "TeenSpiritEnums.h"		//RTM_All, MPA_Play

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	LPCTSTR const cGeneral =			_T("General");
	LPCTSTR const cPlayerSettings =		_T("Player");
	LPCTSTR const cInfoMgrSettings =	_T("InfoManager");
	LPCTSTR const cTagManager =			_T("TagManager");
	LPCTSTR const cMiniPlayer =			_T("MiniPlayer");

	struct OptInfo
	{
		AppOptionsEnum optID;
		LPCTSTR valueName;
		LPCTSTR category;
		INT defVal;
	};

	//Indexed by AppOptionsEnum: keep the order of the enum.
	const OptInfo Option2Info[] =
	{
		{OPT_GEN_Debugging, _T("Debugging"), cGeneral, 0},
		{OPT_GEN_LastFMScrobbler, _T("LastFMScrobbler"), cGeneral, 0},
		{OPT_GEN_NotifyHistory, _T("NotifyHistory"), cGeneral, 1},

		{OPT_GEN_DaysMeansNew, _T("DaysMeansNew"), cGeneral, 30},
		{OPT_GEN_LastVersion, _T("LastVersion"), cGeneral, 0},

		{OPT_GEN_AutoUpdateDB, _T("AutoUpdateDB"), cGeneral, 0},
		{OPT_GEN_AutoUpdateDBDelay, _T("AutoUpdateDBDelay"), cGeneral, 300},	//(sec)
		{OPT_GEN_MinimizeToTray, _T("MinimizeToTray"), cGeneral, 1},
		{OPT_GEN_MultimediaKeysActive, _T("MultimediaKeysActive"), cGeneral, 1},

		{OPT_GEN_RelativeFontSize, _T("RelativeFontSize"), cGeneral, 0},
		{OPT_GEN_ForceClearTypeFonts, _T("ForceClearTypeFonts"), cGeneral, 1},
		{OPT_GEN_ShowToolTipOnMediaChanged, _T("ShowToolTipOnMediaChanged"), cGeneral, 1},

		{OPT_TAG_WriteComments, _T("WriteComments"), cTagManager, 1},
		{OPT_TAG_WriteLyrics, _T("WriteLyrics"), cTagManager, 1},
		{OPT_TAG_WriteRating, _T("WriteRating"), cTagManager, 0},
		{OPT_TAG_SafetyOption, _T("SafetyOption"), cTagManager, 50},
		{OPT_TAG_SafetyOptionManual, _T("SafetyOptionManual"), cTagManager, 100},

		{OPT_TAG_MP3, _T("mp3"), cTagManager, 3},
		{OPT_TAG_OGG, _T("ogg"), cTagManager, 3},
		{OPT_TAG_OGA, _T("oga"), cTagManager, 3},
		{OPT_TAG_FLAC, _T("flac"), cTagManager, 3},
		{OPT_TAG_MPC, _T("mpc"), cTagManager, 3},
		{OPT_TAG_WV, _T("wv"), cTagManager, 3},
		{OPT_TAG_SPX, _T("spx"), cTagManager, 3},
		{OPT_TAG_TTA, _T("tta"), cTagManager, 3},
		{OPT_TAG_M4A, _T("m4a"), cTagManager, 1},		//read only (TagLib 1.6 crashed on some files; TagLib 2 reads them)
		{OPT_TAG_WMA, _T("wma"), cTagManager, 1},
		{OPT_TAG_AIF, _T("aif"), cTagManager, 3},
		{OPT_TAG_WAV, _T("wav"), cTagManager, 3},

		{OPT_PL_PlayMode, _T("PlayMode"), cPlayerSettings, 3},
		{OPT_PL_AutoContinueMode, _T("AutoContinueMode"), cPlayerSettings, RTM_All},
		{OPT_PL_Volume, _T("Volume"), cPlayerSettings, 90},
		{OPT_PL_RememberLastTrack, _T("RememberLastTrack"), cPlayerSettings, 1},
		{OPT_PL_LastTrack, _T("LastTrack"), cPlayerSettings, 0},
		{OPT_PL_RememberLastTime, _T("RememberLastTime"), cPlayerSettings, 0},
		{OPT_PL_LastTime, _T("LastTime"), cPlayerSettings, 0},
		{OPT_PL_AutoPlay, _T("AutoPlay"), cPlayerSettings, 0},
		{OPT_PL_Crossfade, _T("Crossfade"), cPlayerSettings, 1},
		{OPT_PL_CrossfadeAtStart, _T("CrossfadeAtStart"), cPlayerSettings, 0},
		{OPT_PL_Mute, _T("Mute"), cPlayerSettings, 0},
		{OPT_PL_DefaultAction, _T("DefaultAction"), cPlayerSettings, MPA_Play},

		{OPT_PL_CrossfadeAtEnd, _T("CrossfadeAtEnd"), cPlayerSettings, 3000},
		{OPT_PL_EqualizerMode, _T("EqualizerMode"), cPlayerSettings, 0},
		{OPT_PL_UseSampleFloat, _T("UseSampleFloat"), cPlayerSettings, 1},
		{OPT_PL_UseFloatDSP, _T("UseFloatDSP"), cPlayerSettings, 1},

		{OPT_IM_AutoDlArtistBio, _T("AutoDlArtistBio"), cInfoMgrSettings, 1},
		{OPT_IM_AutoDlAlbumReview, _T("AutoDlAlbumReview"), cInfoMgrSettings, 1},
		{OPT_IM_AutoDlLyrics, _T("AutoDlLyrics"), cInfoMgrSettings, 0},
		{OPT_IM_AutoDlAlbumPicture, _T("AutoDlAlbumPicture"), cInfoMgrSettings, 1},
		{OPT_IM_AutoDlArtistPicture, _T("AutoDlArtistPicture"), cInfoMgrSettings, 1},
		{OPT_IM_AutoDlActivationDelay, _T("AutoDlActivationDelay"), cInfoMgrSettings, 3},
		{OPT_IM_UseFolderImages, _T("UseFolderImages"), cInfoMgrSettings, 1},

		{OPT_MINIPLR_ShowAtStartup, _T("ShowAtStartup"), cMiniPlayer, 0},
		{OPT_MINIPLR_Automatic, _T("Automatic"), cMiniPlayer, 1},
		{OPT_MINIPLR_ForceOnTop, _T("ForceOntTop"), cMiniPlayer, 0},			//(sic) existing ini key
		{OPT_MINIPLR_ForceOnTopDelay, _T("ForceOntTopDelay"), cMiniPlayer, 15},	//(sic) existing ini key
		{OPT_MINIPLR_Transparency, _T("Transparency"), cMiniPlayer, 140},
		{OPT_MINIPLR_ShowTooltips, _T("ShowTooltips"), cMiniPlayer, 1},
		{OPT_MINIPLR_ShowExtraInfo, _T("ShowExtraInfo"), cMiniPlayer, 1},
		{OPT_MINIPLR_MaxSize, _T("MaxSize"), cMiniPlayer, 400},
		{OPT_MINIPLR_DisableTransparencyOnMouseOver, _T("DisableTransparencyOnMouseOver"), cMiniPlayer, 1},
	};
	static_assert(sizeof(Option2Info) / sizeof(Option2Info[0]) == OPT_Last, "Option2Info must have one entry per AppOptionsEnum value");

	struct StringOptInfo
	{
		LPCTSTR valueName;
		LPCTSTR category;
		LPCTSTR defVal;
	};

	//Indexed by AppStringOptionsEnum.
	const StringOptInfo StringOption2Info[] =
	{
		{_T("AppMonitoring"), cGeneral, NULL},	//SOPT_GEN_Monitoring
	};
	static_assert(sizeof(StringOption2Info) / sizeof(StringOption2Info[0]) == SOPT_Last, "StringOption2Info must have one entry per AppStringOptionsEnum value");

	BOOL IsValid(AppOptionsEnum opt)		{return opt > OPT_Unknown && opt < OPT_Last;}
	BOOL IsValid(AppStringOptionsEnum opt)	{return opt > SOPT_Unknown && opt < SOPT_Last;}
}

AppOptions::AppOptions(AppSettings& settings):
m_settings(settings)
{
	for (INT i = 0; i < OPT_Last; i++)
		m_values[i] = Option2Info[i].defVal;
}

void AppOptions::Read()
{
	for (INT i = 0; i < OPT_Last; i++)
		m_settings.Read(Option2Info[i].category, Option2Info[i].valueName, m_values[i], Option2Info[i].defVal);
}

void AppOptions::Write()
{
	for (INT i = 0; i < OPT_Last; i++)
		m_settings.Write(Option2Info[i].category, Option2Info[i].valueName, m_values[i]);
}

void AppOptions::Write(AppOptionsEnum opt)
{
	ASSERT(IsValid(opt));
	if (IsValid(opt))
		m_settings.Write(Option2Info[opt].category, Option2Info[opt].valueName, m_values[opt]);
}

INT AppOptions::Get(AppOptionsEnum opt) const
{
	ASSERT(IsValid(opt));
	return IsValid(opt) ? m_values[opt] : 0;
}

void AppOptions::Set(AppOptionsEnum opt, INT val)
{
	ASSERT(IsValid(opt));
	if (IsValid(opt))
		m_values[opt] = val;
}

BOOL AppOptions::GetString(AppStringOptionsEnum opt, LPTSTR bf, UINT bfLen)
{
	ASSERT(IsValid(opt) && bf != NULL && bfLen > 0);
	if (!IsValid(opt) || bf == NULL || bfLen == 0)
		return FALSE;
	const StringOptInfo& info = StringOption2Info[opt];
	std::tstring tmp;
	if (!m_settings.Read(info.category, info.valueName, tmp, info.defVal))
		return FALSE;
	if (tmp.size() >= bfLen)
		return FALSE;
	_tcsncpy_s(bf, bfLen, tmp.c_str(), _TRUNCATE);
	return TRUE;
}

BOOL AppOptions::SetString(AppStringOptionsEnum opt, LPCTSTR value)
{
	ASSERT(IsValid(opt));
	if (!IsValid(opt))
		return FALSE;
	return m_settings.Write(StringOption2Info[opt].category, StringOption2Info[opt].valueName, value);
}

LPCTSTR AppOptions::GetName(AppOptionsEnum opt)
{
	ASSERT(IsValid(opt));
	return IsValid(opt) ? Option2Info[opt].valueName : NULL;
}

LPCTSTR AppOptions::GetCategory(AppOptionsEnum opt)
{
	ASSERT(IsValid(opt));
	return IsValid(opt) ? Option2Info[opt].category : NULL;
}

INT AppOptions::GetDefault(AppOptionsEnum opt)
{
	ASSERT(IsValid(opt));
	return IsValid(opt) ? Option2Info[opt].defVal : 0;
}

BOOL AppOptions::IsTableConsistent()
{
	for (INT i = 0; i < OPT_Last; i++)
	{
		if (Option2Info[i].optID != i)
			return FALSE;
	}
	return TRUE;
}
