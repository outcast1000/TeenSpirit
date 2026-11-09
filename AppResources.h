#ifndef _AppResources_h_
#define _AppResources_h_

//	The application's shared fonts and icons. Created lazily and owned here; the
//	returned CFont*/HICON stay valid until the AppResources is destroyed.
//	The icons come from the resource module given to the constructor, so the
//	class can be used (and tested) without PrgAPI.
//	The mapping functions (IconFor*) are pure and need no instance.

#include "TeenSpiritEnums.h"

enum APP_FONTS
{
	FONT_Dialogs,
	FONT_DialogsBold,
	FONT_DialogsUnderline,
	FONT_DialogsItalics,
	FONT_DialogsBig,
	FONT_DialogsBigBold,
	FONT_Fixed,
	FONT_Last
};

enum APP_ICONS
{
	ICO_Main,

	ICO_Info,
	ICO_Warning,
	ICO_Error,
	ICO_Prev24,
	ICO_Play24,
	ICO_Pause24,
	ICO_Stop24,
	ICO_Next24,
	ICO_Crossfade16,
	ICO_CrossfadeAc16,
	ICO_Equalizer16,
	ICO_EqualizerAc16,
	ICO_Back16,
	ICO_Forward16,
	ICO_Home16,
	ICO_Refresh16,
	ICO_Internet16,
	ICO_Add16,
	ICO_Artist16,
	ICO_2Stars16,
	ICO_3Stars16,
	ICO_Top1016,
	ICO_Top10016,
	ICO_TopRest16,
	ICO_Played16,
	ICO_MouseClick16,
	ICO_Audio16,
	ICO_Video16,
	ICO_Virtual16,
	ICO_Comment16,
	ICO_Lyrics16,
	ICO_Personal16,
	ICO_Tablature16,

	ICO_StarEmpty16,
	ICO_StarGold16,
	ICO_StarGoldHalf16,
	ICO_StarEmptyBad16,
	ICO_StarBad16,
	ICO_Minus16,
	ICO_New16,
	ICO_Collapsed16,
	ICO_Expanded16,
	ICO_LocalFolder16,
	ICO_NetFolder16,
	ICO_Media16,
	ICO_AudioCD16,
	ICO_Removable16,

	ICO_Play16,
	ICO_Copy16,
	ICO_Delete16,
	ICO_Find16,
	ICO_Properties16,
	ICO_Providers16,
	ICO_Buy16,
	ICO_Options16,
	ICO_Window16,
	ICO_Clear16,

	ICO_Text16,
	ICO_Picture16,

	ICO_Up16,
	ICO_Down16,

	ICO_Collections16,
	ICO_History16,
	ICO_Games16,
	ICO_Main16,

	ICO_SwitchPane16,

	ICO_Minimize16,
	ICO_Restore16,
	ICO_Maximize16,
	ICO_Close16,

	ICO_IncreaseFont16,
	ICO_DecreaseFont16,
	ICO_ResetFont16,

	ICO_Album16,
	ICO_Insert16,
	ICO_Remove16,

	ICO_Mute16,
	ICO_Nomute16,

	ICO_Last
};

class AppResources
{
public:
	//resModule: the module holding the icon resources (AfxGetResourceHandle() in the app).
	explicit AppResources(HINSTANCE resModule);
	~AppResources();

	CFont* GetFont(APP_FONTS font);
	HICON GetIcon(APP_ICONS icon);		//NULL for ICO_Last
	//All the icons, in APP_ICONS order (image index == APP_ICONS value).
	CImageList& GetImageList16();

	//---Pure mappings. ICO_Last means "no icon".
	static APP_ICONS IconForTrackType(TrackTypesEnum tt);
	static APP_ICONS IconForRank(INT rank);
	static APP_ICONS IconForStars(INT stars);
	static APP_ICONS IconForCollectionType(CollectionTypesEnum colType);
	static APP_ICONS IconForInfoItemType(InfoItemTypeEnum iit);

	//The resource ID and size (pixels) of an icon. FALSE for an out-of-range icon.
	static BOOL GetIconResource(APP_ICONS icon, INT& resID, INT& size);
	//The LOGFONT a font is created from. FALSE for an out-of-range font.
	static BOOL GetFontSpec(APP_FONTS font, LOGFONT& lf);

private:
	AppResources(const AppResources&);
	AppResources& operator=(const AppResources&);

	HINSTANCE m_resModule;
	CFont* m_pFonts[FONT_Last];
	HICON m_icons[ICO_Last];
	CImageList m_il16;
};

#endif
