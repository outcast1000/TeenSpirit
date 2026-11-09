#pragma once

#include "IPane.h"
#include "TSTranslatable.h"
#include "IStateChangeListener.h"
#include "MainTreeCtrl.h"
#include "GuiControls/BCMenu.h"

static PaneInfo SectionsPaneInfo = {_T("SectionsPane"), _T("Sections Pane"), _T(""), TRUE};


class SectionsPane :	public IPane,
						public TSTranslatable,
						public IStateChangeListener		//=== It requires state change events
{
public:
	SectionsPane();
	virtual ~SectionsPane();

	virtual BOOL Init(HWND hwndParent);
	virtual void UnInit();

	virtual BOOL Show();
	virtual void Hide();
	virtual void MovePane(INT x, INT y, INT cx, INT cy);

	virtual LPCTSTR GetTitle(UINT captionNum);
	virtual BOOL GetButtonInfo(PaneButtonInfo& bInfo, UINT idx);
	virtual ITSMenu* GetMenu(UINT idx);

	virtual BOOL OnButton(UINT idx);

	virtual const PaneInfo& GetPaneInfo()						{return SectionsPaneInfo;}

	virtual BOOL LoadState(IPaneState& paneState);
	virtual BOOL SaveState(IPaneState& paneState);

	//=== public ITranslatable
	virtual void ApplyTranslation(ITranslation& translation)	{ResetAllMenus();}

	//=== public IStateChangeListener
	virtual BOOL OnStateChanged(UINT stateMessage);
	virtual LPCTSTR GetStateListenerName()						{return SectionsPaneInfo.friendlyName;}


private:
	enum MenuCommandsEnum
	{
		MENU_First = 0x2000,

		MENU_InsertRandomTracksHolder,
		MENU_InsertRandomTracksHolderEnd = MENU_InsertRandomTracksHolder + 0xF,
		MENU_SectionModesHolder,
		MENU_SectionModesHolderEnd = MENU_SectionModesHolder + 0xF,
		MENU_SectionFiltersHolder,
		MENU_SectionFiltersHolderEnd = MENU_SectionFiltersHolder + 0xF,
		MENU_SectionStylesHolder,
		MENU_SectionStylesHolderEnd = MENU_SectionStylesHolder + 0xF,
		MENU_SectionGroupingHolder,
		MENU_SectionGroupingHolderEnd = MENU_SectionGroupingHolder + 0xF,

		MENU_SectionSortsHolder,

		MENU_SectionSortsArtName,
		MENU_SectionSortsArtRating,
		MENU_SectionSortsArtCount,
		MENU_SectionSortsArtDesc,

		MENU_SectionSortsAlbName,
		MENU_SectionSortsAlbRating,
		MENU_SectionSortsAlbCount,
		MENU_SectionSortsAlbYear,
		MENU_SectionSortsAlbDesc,

		MENU_SectionSortsGenName,
		MENU_SectionSortsGenCount,
		MENU_SectionSortsGenDesc,

		MENU_SectionSortsColName,
		MENU_SectionSortsColCount,
		MENU_SectionSortsColDesc,

		MENU_SectionSortsYeaName,
		MENU_SectionSortsYeaCount,
		MENU_SectionSortsYeaDesc,

		MENU_SectionSortsMonName,
		MENU_SectionSortsMonCount,
		MENU_SectionSortsMonDesc,

		MENU_SectionSortsHolderEnd,
		MENU_Last
	};


	enum MENUS
	{
		MNU_SectionOptionsMenu,
		MNU_SectionModesMenu,
		MNU_SectionSortsMenu,
		MNU_SectionSortArtistMenu,
		MNU_SectionSortAlbumMenu,
		MNU_SectionSortGenreMenu,
		MNU_SectionSortCollectionMenu,
		MNU_SectionSortYearMenu,
		MNU_SectionSortMonthAddedMenu,
		MNU_SectionFilterMenu,
		MNU_SectionGroupingMenu,
		MNU_SectionStylesMenu,
		MNU_Last
	};
	ITSMenu* m_pMenu[MNU_Last];
	TCHAR m_stringBuffer[500];
	CMainTreeCtrl m_treeControl;

private:
	ITSMenu& GetInternalMenu(MENUS mnu);
	void ResetAllMenus();
	void ReadSortOption(IPaneState& paneState, SectionTypeEnum st);
	void WriteSortOption(IPaneState& paneState, SectionTypeEnum st);

};


