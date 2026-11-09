////*********************************************************************
//// INCLUDES
////*********************************************************************
#include "StdAfx.h"
#include "SkinManager.h"
#include <shlwapi.h>

LPCTSTR const sColorsIni = _T("colors.ini");
LPCTSTR const sSkinDetails = _T("SkinDetails");

SkinManager::SkinManager()
{
}


SkinManager::~SkinManager()
{
}


//=== ITranslationManager
void SkinManager::RegisterSkinnable(ISkinnable& skinable)
{
	m_skinables.insert(&skinable);
}
void SkinManager::UnRegisterSkinnable(ISkinnable& skinable)
{
	std::set<ISkinnable*>::iterator it = m_skinables.find(&skinable);
	if (it != m_skinables.end())
		m_skinables.erase(it);
	else
		ASSERT(0);
}
void SkinManager::UpdateSkinnables(ISkin& skin)
{
	//=== ApplySkin may register or unregister skinnables (panes being created or
	//=== destroyed), so iterate over a copy and skip the ones that went away.
	std::vector<ISkinnable*> skinnables(m_skinables.begin(), m_skinables.end());
	for (size_t i = 0; i < skinnables.size(); i++)
	{
		if (m_skinables.find(skinnables[i]) != m_skinables.end())
			skinnables[i]->ApplySkin(skin);
	}
}


BOOL SkinManager::SetSkinsFolder(LPCTSTR folder)
{
	ASSERT(folder != NULL);
	if (!PathFileExists(folder))
		return FALSE;
	m_skinFolder = folder;
	if (m_skinFolder[m_skinFolder.size()-1] != TCHAR('\\'))
		m_skinFolder += _T("\\");
	return TRUE;
}


TSSkin& SkinManager::GetSkin()
{
	return m_skin;
}

LPCTSTR SkinManager::GetSkinInfo(INT idx, ISkin::SkinInfoEnum infoType)
{
	if (idx < 0 || idx >= (INT)m_skinsInfo.size())
	{
		ASSERT(0);
		return NULL;
	}
	if (m_skinsInfo[idx].skinInfo.GetSkinInfo(ISkin::SI_Name)[0] == 0)
		TSSkin::LoadSkinInfo(m_skinsInfo[idx].skinInfo, GetSkinPath(idx));
	return m_skinsInfo[idx].skinInfo.GetSkinInfo(infoType);
}

LPCTSTR SkinManager::GetSkinPath(INT idx)
{
	if (idx < 0 || idx >= (INT)m_skinsInfo.size())
	{
		ASSERT(0);
		return NULL;
	}
	return m_skinsInfo[idx].path.c_str();
}

UINT SkinManager::GetSkinsCount()
{
	return m_skinsInfo.size();
}


void SkinManager::RefreshSkinsFolder()
{
	m_skinsInfo.clear();
	TCHAR searchPath[MAX_PATH];
	_sntprintf(searchPath, MAX_PATH, _T("%s*.*"), m_skinFolder.c_str());
	CFileFind ff;
	BOOL bCont = ff.FindFile(searchPath);
	while (bCont)
	{
		bCont = ff.FindNextFile();
		if (ff.IsDirectory() && !ff.IsDots())
		{
			TCHAR colorsPath[MAX_PATH];
			_sntprintf(colorsPath, MAX_PATH, _T("%s\\%s"), (LPCTSTR)ff.GetFilePath(), sColorsIni);
			if (PathFileExists(colorsPath))
			{
				FullSkinInfo item;
				item.path = ff.GetFilePath();
				m_skinsInfo.push_back(item);
			}
		}
	}
}




//BOOL SkinManager::GetSkinInfoCollection(SkinInfoCollection& col)
//{
//	TCHAR searchPath[MAX_PATH];
//	_sntprintf(searchPath, MAX_PATH, _T("%s*.*"), m_skinDir.c_str());
//	BOOL bFound = m_finder.FindFile(searchPath);
//	SkinInfo si;
//	while (bFound)
//	{
//		bFound = m_finder.FindNextFile();
//		if (m_finder.IsDirectory() && !m_finder.IsDots())
//		{
//			TCHAR colorsPath[MAX_PATH];
//			_sntprintf(colorsPath, MAX_PATH, _T("%s\\%s"), m_finder.GetFilePath(), cColorsIni);
//			if (PathFileExists(colorsPath))
//			{
//				if (GetSkinInfo(m_finder.GetFilePath(), si))
//					col.push_back(si);
//			}
//		}
//	}
//	return TRUE;
//}


//BOOL SkinManager::GetSkinInfo(LPCTSTR path, SkinInfo& skinInfo)
//{
//	ASSERT(path != NULL);
//	if (path == NULL)
//		return FALSE;
//	LPCTSTR pos = _tcsrchr(path, '\\');// m_skins[id].rfind('\\');
//	if (pos == NULL)
//		return FALSE;
//	skinInfo.DirName = ++pos;
//	IniAppSettings settings;
//	TCHAR bf[MAX_PATH];
//	_sntprintf(bf, MAX_PATH, _T("%s\\colors.ini"), path);
//	settings.Load(bf);
//	settings.Read(sSkinDetails, _T("Name"), m_skinInfoName, _T(""));
//	settings.Read(sSkinDetails, _T("Author"), m_skinInfoAuthor, _T(""));
//	settings.Read(sSkinDetails, _T("Version"), m_skinInfoVersion, _T(""));
//	settings.Read(sSkinDetails, _T("Email"), m_skinInfoEmail, _T(""));
//	settings.Read(sSkinDetails, _T("Description"), m_skinInfoDescription, _T(""));
//	settings.Read(sSkinDetails, _T("WebSite"), m_skinInfoWebSite, _T(""));
//	settings.Read(sSkinDetails, _T("AutoUpdateURL"), m_skinInfoAutoUpdateURL, _T(""));
//	if (m_skinInfoName.empty())
//		m_skinInfoName = skinInfo.DirName;
//	skinInfo.Name = m_skinInfoName.c_str();
//	skinInfo.Author = m_skinInfoAuthor.c_str();
//	skinInfo.Version = m_skinInfoVersion.c_str();
//	skinInfo.Email = m_skinInfoEmail.c_str();
//	skinInfo.Description = m_skinInfoDescription.c_str();
//	skinInfo.WebSite = m_skinInfoWebSite.c_str();
//	skinInfo.AutoUpdateURL = m_skinInfoAutoUpdateURL.c_str();
//
//	return TRUE;
//}


void SkinManager::UpdateSkinnables()
{
	UpdateSkinnables(GetSkin());
}

BOOL SkinManager::LoadSkin(LPCTSTR skinPath)
{
	if (PathFileExists(skinPath))
	{
		return GetSkin().LoadSkin(skinPath);
	}
	std::tstring path(m_skinFolder);
	path += skinPath;
	return GetSkin().LoadSkin(path.c_str());
}

BOOL SkinManager::Init(LPCTSTR skinsFolder, LPCTSTR skinName, INT fontQuality, INT relativeFontSize)
{
	ASSERT(skinsFolder != NULL);
	SetSkinsFolder(skinsFolder);
	BOOL bLoaded = FALSE;
	if (skinName != NULL && skinName[0] != 0)
	{
		bLoaded = LoadSkin(skinName);
		if (!bLoaded)
			TRACE(_T("@1 SkinManager::Init Skin not loaded: '%s'\r\n"), skinName);
	}
	if (!bLoaded)
		bLoaded = LoadSkin(_T("Default"));
	m_skin.SetFontQuality(fontQuality);
	m_skin.SetRelativeFontSize(relativeFontSize);
	return bLoaded;
}









//COLORREF SkinManager::GetColor(LPCTSTR Category, LPCTSTR Value, COLORREF def)
//{
//	if (m_pAppSettings != NULL)
//	{
//		TCHAR defVal[30];
//		if (def != 0xFFFFFFFF)
//			COLORREF2String(def, defVal, 30);
//		else
//			defVal[0] = 0;
//		m_pAppSettings->Read(Category, Value, m_tmpBuffer, defVal);
//		if (!m_tmpBuffer.empty())
//		{
//			COLORREF val;
//			if (String2COLORREF(m_tmpBuffer.c_str(), val))
//				return val;
//		}
//	}
//	return def;
//}
//void SkinManager::SetColor(LPCTSTR Category, LPCTSTR Value, COLORREF value)
//{
//	if (m_pAppSettings != NULL)
//	{
//		TCHAR defVal[30];
//		COLORREF2String(value, defVal, 30);
//		m_pAppSettings->Write(Category, Value, defVal);
//	}
//}



//BOOL SkinManager::LoadSkin(LPCTSTR skinDir)
//{
//	ASSERT(skinDir != NULL);
//	m_skin.Loas
//	m_loadedSkinPath = fileName;
//	IniAppSettings appSettings;
//	if (!appSettings.Load(fileName))
//		return FALSE;
//	std::tstring tmpBuffer;
//	ColorSectionsMap::iterator secit = m_colors.begin();
//	for (;secit != m_colors.end(); secit++)
//	{
//		ColorValuesMap::iterator valit = secit->second.begin();
//		for (;valit != secit->second.end(); valit++)
//		{
//			if (appSettings.Read(
//				cColorSectionNames[secit->first], 
//				cColorNames[valit->first], 
//				tmpBuffer, _T("")))
//			{
//				COLORREF clr;
//				if (String2COLORREF(tmpBuffer.c_str(), clr) == TRUE)
//				{
//					valit->second = clr;
//				}
//				else
//					TRACE(_T("@1 SkinManager::LoadSkin Cannot decode value: '%s'\r\n"), tmpBuffer.c_str());
//			}
//			else
//			{
//				TRACE(_T("@2 SkinManager::LoadSkin Value Not Set: '%s-%s'\r\n"), cColorSectionNames[secit->first], cColorNames[valit->first]);
//			}
//		}
//
//	}
//	return TRUE;
//}



const INT cMaxRelativeFontVariation = 10;

void SkinManager::IncreaseRelativeFontSize()
{
	TSSkin& skin = GetSkin();
	INT rfd = skin.GetRelativeFontSize();
	if (rfd < cMaxRelativeFontVariation)
	{
		skin.SetRelativeFontSize(rfd + 1);
		UpdateSkinnables(skin);
	}
}
void SkinManager::DecreaseRelativeFontSize()
{
	TSSkin& skin = GetSkin();
	INT rfd = skin.GetRelativeFontSize();
	if (rfd > -cMaxRelativeFontVariation)
	{
		skin.SetRelativeFontSize(rfd - 1);
		UpdateSkinnables(skin);
	}
}
void SkinManager::ResetRelativeFontSize()
{
	TSSkin& skin = GetSkin();
	INT rfd = skin.GetRelativeFontSize();
	if (rfd != 0)
	{
		skin.SetRelativeFontSize(0);
		UpdateSkinnables(skin);
	}
}






