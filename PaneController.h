#ifndef _PaneController_h_
#define _PaneController_h_

#include "GuiControls/FastSplitter.h"
#include "TSTranslatable.h"
#include "TSSkinnable.h"
#include "IPane.h"
#include "TSPaneContainer.h"


class PaneController:	public TSTranslatable, 
						public TSSkinnable
{
public:
	PaneController();
	virtual ~PaneController();

	void Init(CWnd* pParent);

	void LoadDefaultState();
	BOOL LoadState(LPCTSTR state);
	BOOL SaveState(LPTSTR state, UINT stateLen);

	//=== Relative to the parent
	void SetPosition(INT x, INT y, INT cx, INT cy);
	BOOL SaveSettings();

	//=== Pane Creators (Classes that are able to create new pane instances)
	void AddPaneCreator(IPaneCreator* pPaneCreator);
	IPaneCreator* GetPaneCreator(UINT idx);
	IPaneCreator* GetPaneCreatorByName(LPCTSTR name);
	UINT GetPaneCreatorCount()							{return m_paneCreators.size();}

	//=== TSContainers control
	//ITSMenu* GetPositioningMenu(UINT firstID);
	//ITSMenu* GetAddPaneMenu(UINT firstID);
	//CMenu* GetSwitchPaneMenu(TSPaneContainer* pSrcPane, UINT firstID);

	enum Position
	{
		POS_Up,
		POS_Down,
		POS_Left,
		POS_Right,
		POS_Last
	};
	BOOL AddPane(TSPaneContainer& srcPaneContainer);
	BOOL AddPane(TSPaneContainer& srcPaneContainer, IPaneCreator& newPaneCreator, Position pos, INT splitterPos = -1);

	//BOOL InsertPane(TSPaneContainer& srcPaneContainer, IPaneCreator& newPaneCreator, BOOL bVertical, INT splitterPos = -1);
	//BOOL SwitchPane(TSPaneContainer& srcPaneContainer, IPaneCreator& newPaneCreator);
	void ClosePane(TSPaneContainer& srcPaneContainer);
	void MaxRestorePane(TSPaneContainer& srcPaneContainer);

	




	//=== TSTranslatable
	virtual void ApplyTranslation(ITranslation& translation);
	//=== TSSkinnable
	virtual void ApplySkin(ISkin& skin);

private:
	struct PanePositinion
	{
		UINT ownerSplitterIndex;
		BOOL bIsFirstPane;
	};
	BOOL FindPanePosition(PanePositinion& pos, HWND hwnd);

	//BOOL SplitPane(TSPaneContainer* pOriginalPane, TSPaneContainer* pNewPane, BOOL bVertical, INT splitterPos);
	TSPaneContainer* CreateContainerPane(IPaneCreator* pPaneCreator);
	CFastSplitter* CreateSplitter(CWnd* pParent, CWnd* pFPane, CWnd* pSPane, BOOL bVertical, UINT spitterPos);
	BOOL GetToken(LPCTSTR& posStart, TCHAR* bf, UINT bfLen, WCHAR divider);
	BOOL AppendPaneState(std::tstring& state, HWND hwndPane);


	INT GetContainerIndexByHWND(HWND hwnd);
	INT GetSplitterIndexByHWND(HWND hwnd);

	void ApplySkinToPanes(TSPaneContainer& pane, TSSkin& skin);
	void ApplySkinToSplitters(CFastSplitter& splitter, TSSkin& skin);

	//void FixCloseButton();

	static LPCTSTR SettingsCategory;

	CWnd* m_pParent;
	std::vector<IPaneCreator*> m_paneCreators;
	std::vector<TSPaneContainer*> m_paneContainers;
	std::vector<CFastSplitter*> m_splitters;

	//ITSMenu* m_pmnuAdd;
	CRect m_clientRC;
	TSPaneContainer* m_unusedPaneContainer;
	TSPaneContainer* m_pMaximizedPane;
	CFastSplitter* m_pMaximizedPaneSplitter;
};


#endif
