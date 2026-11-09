#ifndef _AppOptions_h_
#define _AppOptions_h_

//	The application's integer and string options, stored in the settings file
//	(jaangle.ini). Holds the option table (name, section, default) and the
//	current values; it depends only on the AppSettings interface, so it can be
//	tested with an in-memory AppSettings.

class AppSettings;

enum AppOptionsEnum
{
	OPT_Unknown = -1,

	OPT_GEN_Debugging,
	OPT_GEN_LastFMScrobbler,
	OPT_GEN_NotifyHistory,

	OPT_GEN_DaysMeansNew,
	OPT_GEN_LastVersion,

	OPT_GEN_AutoUpdateDB,
	OPT_GEN_AutoUpdateDBDelay,
	OPT_GEN_MinimizeToTray,
	OPT_GEN_MultimediaKeysActive,

	OPT_GEN_RelativeFontSize,
	OPT_GEN_ForceClearTypeFonts,
	OPT_GEN_ShowToolTipOnMediaChanged,

	OPT_TAG_WriteComments,
	OPT_TAG_WriteLyrics,
	OPT_TAG_WriteRating,
	OPT_TAG_SafetyOption,
	OPT_TAG_SafetyOptionManual,

	OPT_TAG_MP3,
	OPT_TAG_OGG,
	OPT_TAG_OGA,
	OPT_TAG_FLAC,
	OPT_TAG_MPC,
	OPT_TAG_WV,
	OPT_TAG_SPX,
	OPT_TAG_TTA,
	OPT_TAG_M4A,
	OPT_TAG_WMA,
	OPT_TAG_AIF,
	OPT_TAG_WAV,

	OPT_PL_PlayMode,
	OPT_PL_AutoContinueMode,
	OPT_PL_Volume,
	OPT_PL_RememberLastTrack,
	OPT_PL_LastTrack,
	OPT_PL_RememberLastTime,
	OPT_PL_LastTime,
	OPT_PL_AutoPlay,
	OPT_PL_Crossfade,
	OPT_PL_CrossfadeAtStart,
	OPT_PL_Mute,
	OPT_PL_DefaultAction,

	OPT_PL_CrossfadeAtEnd,
	OPT_PL_EqualizerMode,
	OPT_PL_UseSampleFloat,
	OPT_PL_UseFloatDSP,

	OPT_IM_AutoDlArtistBio,
	OPT_IM_AutoDlAlbumReview,
	OPT_IM_AutoDlLyrics,
	OPT_IM_AutoDlAlbumPicture,
	OPT_IM_AutoDlArtistPicture,
	OPT_IM_AutoDlActivationDelay,
	OPT_IM_UseFolderImages,

	OPT_MINIPLR_ShowAtStartup,
	OPT_MINIPLR_Automatic,
	OPT_MINIPLR_ForceOnTop,
	OPT_MINIPLR_ForceOnTopDelay,
	OPT_MINIPLR_Transparency,
	OPT_MINIPLR_ShowTooltips,
	OPT_MINIPLR_ShowExtraInfo,
	OPT_MINIPLR_MaxSize,
	OPT_MINIPLR_DisableTransparencyOnMouseOver,

	OPT_Last
};//* Remember to update the option table in AppOptions.cpp

enum AppStringOptionsEnum
{
	SOPT_Unknown = -1,

	SOPT_GEN_Monitoring,

	SOPT_Last
};

class AppOptions
{
public:
	//settings must outlive this object.
	explicit AppOptions(AppSettings& settings);

	//Loads every integer option from the settings (missing ones get their default).
	void Read();
	//Stores every integer option into the settings (does not save the file).
	void Write();
	//Stores one integer option into the settings right away.
	void Write(AppOptionsEnum opt);

	INT Get(AppOptionsEnum opt) const;
	void Set(AppOptionsEnum opt, INT val);

	//String options are read from / written to the settings directly.
	BOOL GetString(AppStringOptionsEnum opt, LPTSTR bf, UINT bfLen);
	BOOL SetString(AppStringOptionsEnum opt, LPCTSTR value);

	//=== Option table
	static LPCTSTR GetName(AppOptionsEnum opt);		//value name in the settings file
	static LPCTSTR GetCategory(AppOptionsEnum opt);	//section in the settings file
	static INT GetDefault(AppOptionsEnum opt);
	//TRUE when every table entry sits at the index of its enum value.
	static BOOL IsTableConsistent();

private:
	AppSettings& m_settings;
	INT m_values[OPT_Last];
};

#endif
