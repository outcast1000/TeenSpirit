#ifndef _PlayListListCtrl_h_
#define _PlayListListCtrl_h_ 

#include "TSSkinnable.h"
#include "TSTranslatable.h"
#include "FlexListCtrl.h"
#include "TSConfigurable.h"
class MediaPlayer;
class ITSMenu;

class CPlayListListCtrl : public CFlexListCtrl, public TSSkinnable, public TSTranslatable, public TSConfigurable
{
	DECLARE_DYNAMIC(CPlayListListCtrl)
public:
	CPlayListListCtrl();
	virtual ~CPlayListListCtrl();

public:
	void UpdateList();
	void UpdateActiveItem();
	void OnSelectAll();
	void OnInvertSelection();
	void SetMediaPlayer(MediaPlayer* pPlayer);
	MediaPlayer* GetMediaPlayer()						{return m_pPlayer;}
	virtual void ApplyTranslation(ITranslation& skin);
	virtual void ApplySkin(ISkin& skin);

	enum ConfigurationOptions
	{
		C_ShowNumbers,
		C_Last
	};
	void SetConfigurationOption(ConfigurationOptions idx, INT value);
	INT GetConfigurationOption(ConfigurationOptions idx);

	//---------------------------------------------------
	//ICtrlColors ---------------------------------------
	//#include "ICtrlColors.h" // ,public ICtrlColors
	//---------------------------------------------------
public:
	enum COLORS
	{
		COL_Bk,
		COL_Text,
		COL_TextCur,
		COL_TextSel,
		COL_TextSelBk,
		COL_TextSelCur,
		COL_TextDisabled,
		COL_Last
	};
	virtual void SetColor(UINT idx, COLORREF value);
	virtual COLORREF GetColor(UINT idx);
	//virtual LPCTSTR GetColorName(UINT idx);

	//=== TSConfigurable
	virtual void UpdateConfiguration();
private:
	COLORREF m_colors[COL_Last];
	//---------------------------------------------------
	//---------------------------------------------------
	//---------------------------------------------------

protected://Overrides (DelegatedOleDropTarget)
	//virtual DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	//virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	//virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );

protected:
	virtual void DrawItem(Gdiplus::Graphics& g, INT nItem, Gdiplus::Rect& itemRC);
	virtual void DrawBackground(Gdiplus::Graphics& g, const Gdiplus::Rect& r);
	virtual BOOL ItemAcceptsFileDrop(CPoint& p, INT nItem)	{return TRUE;}
	virtual BOOL OnFileDrop(const std::vector<std::tstring>& files, CPoint& p);
	virtual void OnItemMouseEvent(const ItemMouseEvent& ev);
	virtual void PostCreate();

protected:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	//BOOL ReadHdropData(COleDataObject* pDataObject, CPoint* point = NULL);
	BOOL SwapRows(UINT row1,UINT row2);
private:
	MediaPlayer* m_pPlayer;
	BOOL m_bDragging;
	UINT m_DragItem;
	BOOL m_bOnDelay;
	INT m_MenuInitiator;
	INT m_curCX;
	BOOL m_bInitialized;
	CFont* m_pBoldFont;
	CFont* m_pNormalFont;
	INT m_configOptions[C_Last];

	enum MenuItemsEnum
	{
		MI_First = 0x3000,
		MI_RemoveSelected,
		MI_RemoveOthers,
		MI_RemoveAll,
		MI_RandomizeList,
		MI_RandomizeNext,
		MI_LocateAlbum,
		MI_InsertRandomTracksHolder,
		MI_InsertRandomTracksHolderEnd = MI_InsertRandomTracksHolder + 0xF,
		MI_SearchTracksHolder,
		MI_SearchTracksHolderEnd = MI_SearchTracksHolder + 0xF,
		MI_Last
	};
	enum MenusEnum
	{
		M_Main,
		M_InsertRandomTracks,
		M_SearchItem,
		M_Last
	};
	ITSMenu* m_pMenu[M_Last];
private:
	ITSMenu& GetInternalMenu(MenusEnum mnu);
	void ResetAllMenus(BOOL bDelete);
	void PrepareInsertModeMenu(ITSMenu& mnu);
	void PrepareSearchTracksMenu(ITSMenu& mnu);
	//void RandomizeItems(std::vector<INT>& selItems);
	void RecurseDirectories(LPCTSTR startPath, INT& playListPos);
	void AddPlayListItem(LPCTSTR fileName, INT playListPos);
};

#endif
