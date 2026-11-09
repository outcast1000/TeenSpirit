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

	BOOL LoadSkin(LPCTSTR skinPath);

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