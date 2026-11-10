#include "stdafx.h"
#include "AppResources.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	//{resource ID, size} in APP_ICONS order.
	const INT cIconResources[][2] =
	{
		{IDR_MAINFRAME, 48},		//ICO_Main

		{IDI_INFO,16},				//ICO_Info,
		{IDI_WARN,16},				//ICO_Warning,
		{IDI_TSERROR,16},			//ICO_Error,
		{IDI_PREVIOUS24,24},		//ICO_Prev24,
		{IDI_PLAY24,24},			//ICO_Play24,
		{IDI_PAUSE24,24},			//ICO_Pause24,
		{IDI_STOP24,24},			//ICO_Stop24,
		{IDI_NEXT24,24},			//ICO_Next24

		{IDI_CROSSFADE16,16},		//ICO_Crossfade16,
		{IDI_CROSSFADE_AC16,16},	//ICO_CrossfadeAc16,
		{IDI_EQUALIZER16,16},		//ICO_Equalizer16,
		{IDI_EQUALIZER_AC16,16},	//ICO_EqualizerAc16,

		{IDI_BACK16,16},			//ICO_Back16
		{IDI_FORWARD16,16},			//ICO_Forward16
		{IDI_HOME16,16},			//ICO_Home16
		{IDI_REFRESH16,16},			//ICO_Refresh16
		{IDI_INTERNET16,16},		//ICO_Internet16
		{IDI_ADD16, 16},			//ICO_Add16

		{IDI_ARTISTS_ICON, 16},		//ICO_Artist16,
		{IDI_2STARS, 16},			//ICO_2Stars16,
		{IDI_3STARS, 16},			//ICO_3Stars16,

		{IDI_TOP1016, 16},			//ICO_Top1016,
		{IDI_TOP10016, 16},			//ICO_Top10016,
		{IDI_TOPREST16, 16},		//ICO_TopRest16,
		{IDI_PLAYED16, 16},			//ICO_Played16,
		{IDI_MOUSECLICK16, 16},		//ICO_MouseClick16,

		{IDI_AUDIO16, 16},			//ICO_Audio16,
		{IDI_VIDEO16, 16},			//ICO_Video16,
		{IDI_VIRTUAL16, 16},		//ICO_Virtual16,
		{IDI_COMMENTS16, 16},		//ICO_Comment16,
		{IDI_LYRICS16, 16},			//ICO_Lyrics16,
		{IDI_PERSONAL16, 16},		//ICO_Personal16,
		{IDI_TABLATURE16, 16},		//ICO_Tablature16,

		{IDI_STAREMPTY16, 16},		//ICO_StarEmpty16,
		{IDI_STARGOLD16, 16},		//ICO_StarGold16,
		{IDI_STARGOLDHALF16, 16},	//ICO_StarGoldHalf16,
		{IDI_STARBADEMPTY16, 16},	//ICO_StarEmptyBad16,
		{IDI_STARBAD16, 16},		//ICO_StarBad16,
		{IDI_MINUS16, 16},			//ICO_Minus16,
		{IDI_NEW16, 16},			//ICO_New16

		{IDI_COLLAPSED16, 16},		//ICO_Collapsed16,
		{IDI_EXPANDED16, 16},		//ICO_Expanded16,
		{IDI_LOCALFOLDER16, 16},	//ICO_LocalFolder16,
		{IDI_NETFOLDER16, 16},		//ICO_NetFolder16,
		{IDI_MEDIA16, 16},			//ICO_Media16,
		{IDI_AUDIOCD16, 16},		//ICO_AudioCD16,
		{IDI_REMOVABLE16, 16},		//ICO_Removable16,

		{IDI_PLAY16, 16},			//ICO_Play16,
		{IDI_COPY16, 16},			//ICO_Copy16,
		{IDI_DELETE16, 16},			//ICO_Delete16,
		{IDI_FIND16, 16},			//ICO_Find16,
		{IDI_PROPERTIES16, 16},		//ICO_Properties16,
		{IDI_PROVIDERS16, 16},		//ICO_Providers16,
		{IDI_BUY16, 16},			//ICO_Buy16
		{IDI_OPTIONS16, 16},		//ICO_Options16
		{IDI_WINDOW16, 16},			//ICO_Window16
		{IDI_CLEAR16, 16},			//ICO_Clear16

		{IDI_TEXT16, 16},			//ICO_Text16,
		{IDI_PICTURE16, 16},		//ICO_Picture16,

		{IDI_UP16, 16},				//ICO_Up16,
		{IDI_DOWN16, 16},			//ICO_Down16,

		{IDI_COLLECTIONS16, 16},	//ICO_Collections16,
		{IDI_HISTORY16, 16},		//ICO_History16,
		{IDI_GAMES16, 16},			//ICO_Games16,
		{IDR_MAINFRAME, 16},		//ICO_Main16
		{IDI_SWITCHPANE16, 16},		//ICO_SwitchPane16

		{IDI_MINIMIZE16, 16},		//ICO_Minimize16,
		{IDI_RESTORE16, 16},		//ICO_Restore16,
		{IDI_MAXIMIZE16, 16},		//ICO_Maximize16,
		{IDI_CLOSE16, 16},			//ICO_Close16,

		{IDI_INCREASEFONT16, 16},	//ICO_IncreaseFont16,
		{IDI_DECREASEFONT16, 16},	//ICO_DecreaseFont16,
		{IDI_RESETFONT16, 16},		//ICO_ResetFont16,

		{IDI_ALBUM16, 16},			//ICO_Album16,
		{IDI_INSERT16, 16},			//ICO_Insert16,
		{IDI_REMOVE16, 16},			//ICO_Remove16,

		{IDI_MUTE16, 16},			//ICO_Mute16,
		{IDI_NOMUTE16, 16}			//ICO_Nomute16,
	};
	static_assert(sizeof(cIconResources) / sizeof(cIconResources[0]) == ICO_Last,
		"cIconResources must have one entry per APP_ICONS value");
}

AppResources::AppResources(HINSTANCE resModule):
m_resModule(resModule)
{
	memset(m_pFonts, 0, sizeof(m_pFonts));
	memset(m_icons, 0, sizeof(m_icons));
}

AppResources::~AppResources()
{
	if (m_il16.m_hImageList != NULL)
		m_il16.DeleteImageList();
	for (int i = 0; i < FONT_Last; i++)
		delete m_pFonts[i];
	for (int i = 0; i < ICO_Last; i++)
	{
		if (m_icons[i] != NULL)
			DestroyIcon(m_icons[i]);
	}
}

BOOL AppResources::GetFontSpec(APP_FONTS font, LOGFONT& lf)
{
	if (font < 0 || font >= FONT_Last)
		return FALSE;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight = 13;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
	_tcsncpy_s(lf.lfFaceName, _T("Tahoma"), _TRUNCATE);

	switch (font)
	{
	case FONT_Dialogs:
		break;
	case FONT_DialogsUnderline:
		lf.lfUnderline = TRUE;
		break;
	case FONT_DialogsItalics:
		lf.lfItalic = TRUE;
		break;
	case FONT_DialogsBold:
		lf.lfWeight = FW_BOLD;
		break;
	case FONT_DialogsBig:
		lf.lfHeight = 18;
		break;
	case FONT_DialogsBigBold:
		lf.lfHeight = 18;
		lf.lfWeight = FW_BOLD;
		break;
	case FONT_Fixed:
		lf.lfHeight = 15;
		lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
		_tcsncpy_s(lf.lfFaceName, _T("Courier New"), _TRUNCATE);
		break;
	}
	return TRUE;
}

CFont* AppResources::GetFont(APP_FONTS font)
{
	LOGFONT lf;
	if (!GetFontSpec(font, lf))
	{
		ASSERT(0);
		return NULL;
	}
	if (m_pFonts[font] == NULL)
	{
		CFont* pFont = new CFont;
		pFont->CreateFontIndirect(&lf);
		m_pFonts[font] = pFont;
	}
	return m_pFonts[font];
}

BOOL AppResources::GetIconResource(APP_ICONS icon, INT& resID, INT& size)
{
	if (icon < 0 || icon >= ICO_Last)
		return FALSE;
	resID = cIconResources[icon][0];
	size = cIconResources[icon][1];
	return TRUE;
}

HICON AppResources::GetIcon(APP_ICONS icon)
{
	INT resID = 0, size = 0;
	if (!GetIconResource(icon, resID, size))
		return NULL;
	if (m_icons[icon] == NULL)
	{
		m_icons[icon] = (HICON) ::LoadImage(m_resModule,
			MAKEINTRESOURCE(resID),
			IMAGE_ICON,
			size,
			size,
			LR_VGACOLOR);
		ASSERT(m_icons[icon] != NULL);
	}
	return m_icons[icon];
}

CImageList& AppResources::GetImageList16()
{
	if (m_il16.m_hImageList == NULL)
	{
		m_il16.Create(16, 16, ILC_MASK | ILC_COLOR32, 5, 1);
		for (int i = 0; i < ICO_Last; i++)
			m_il16.Add(GetIcon((APP_ICONS)i));
	}
	return m_il16;
}

APP_ICONS AppResources::IconForTrackType(TrackTypesEnum tt)
{
	if (tt < TTYPE_FIRSTVIDEO)
		return ICO_Audio16;
	else if (tt < TTYPE_FIRSTVIRTUAL)
		return ICO_Video16;
	return ICO_Virtual16;
}

APP_ICONS AppResources::IconForRank(INT rank)
{
	if (rank >= 1 && rank <= 10)
		return ICO_Top1016;
	else if (rank > 10 && rank <= 100)
		return ICO_Top10016;
	return ICO_TopRest16;
}

APP_ICONS AppResources::IconForStars(INT stars)
{
	switch (stars)
	{
	case 0:
		return ICO_Last;
	case 1:
		return ICO_StarBad16;
	default:
		return ICO_StarGold16;
	}
}

APP_ICONS AppResources::IconForCollectionType(CollectionTypesEnum colType)
{
	switch (colType)
	{
	case CTYPE_LocalFolder:		return ICO_LocalFolder16;
	case CTYPE_NetFolder:		return ICO_NetFolder16;
	case CTYPE_Media:			return ICO_Media16;
	case CTYPE_AudioCD:			return ICO_AudioCD16;
	case CTYPE_Virtual:			return ICO_Virtual16;
	case CTYPE_Removable:		return ICO_Removable16;
	}
	return ICO_Last;
}

APP_ICONS AppResources::IconForInfoItemType(InfoItemTypeEnum iit)
{
	switch (iit)
	{
	case IIT_AlbumPicture:
	case IIT_ArtistPicture:
		return ICO_Picture16;
	case IIT_AlbumReview:
	case IIT_ArtistBio:
		return ICO_Text16;
	case IIT_TrackLyrics:
		return ICO_Lyrics16;
	case IIT_TrackPersonal:
		return ICO_Personal16;
	case IIT_TrackComment:
		return ICO_Comment16;
	case IIT_TrackTablatures:
		return ICO_Tablature16;
	}
	return ICO_Last;
}
