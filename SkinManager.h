#ifndef _SKINMANAGER_H
#define _SKINMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <set>

#include "TSSkin.h"

//*********************************************************************
// SkinManager
//*********************************************************************

class AppSettings;

class SkinManager : public ISkinManager
{
public:
	SkinManager();
	virtual ~SkinManager();

public:
	virtual void RegisterSkinnable(ISkinnable& skinnable);
	virtual void UnRegisterSkinnable(ISkinnable& skinnable);
	virtual void UpdateSkinnables(ISkin& skin);

private:
	std::set<ISkinnable*> m_skinables;


public:
	TSSkin& GetSkin();



public:	//=== Skins Folder Management
	BOOL SetSkinsFolder(LPCTSTR translationPath);
	LPCTSTR GetSkinsFolder()	{return m_skinFolder.c_str();}
	void RefreshSkinsFolder();
	UINT GetSkinsCount();


	//=== Info retrieval
	LPCTSTR GetSkinInfo(INT idx, ISkin::SkinInfoEnum infoType);	
	LPCTSTR GetSkinPath(INT idx);	

	//=== MACROS
	void IncreaseRelativeFontSize();
	void DecreaseRelativeFontSize();
	void ResetRelativeFontSize();

	void UpdateSkinnables();

	//skinPath: a full path or a folder name inside the skins folder.
	BOOL LoadSkin(LPCTSTR skinPath);

	//Startup: sets the skins folder and loads skinName (a path or folder name),
	//falling back to the "Default" skin when it is empty or fails to load.
	//FALSE when no skin could be loaded (the built-in defaults stay in use).
	BOOL Init(LPCTSTR skinsFolder, LPCTSTR skinName, INT fontQuality, INT relativeFontSize);

private:
	void InitDefaultSkin();

private:
	std::tstring m_skinFolder;
	std::tstring m_tmpBuffer;
	TSSkin m_skin;

	struct FullSkinInfo
	{
		TSSkinInfo skinInfo;
		std::basic_string<TCHAR> path;
	};
	std::vector<FullSkinInfo> m_skinsInfo;


};
#endif