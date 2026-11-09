#include "stdafx.h"
#include "TranslationManager.h"
#include <shlwapi.h>

TranslationManager::TranslationManager()
{
	m_fallBackTranslation.SetDefaultTranslation(&m_resourceTranslation);
}
TranslationManager::~TranslationManager()
{
	ASSERT(m_translatables.empty());
}

//=== ITranslationManager
void TranslationManager::RegisterTranslatable(ITranslatable& translatable)
{
	m_translatables.insert(&translatable);
}
void TranslationManager::UnRegisterTranslatable(ITranslatable& translatable)
{
	std::set<ITranslatable*>::iterator it = m_translatables.find(&translatable);
	if (it != m_translatables.end())
		m_translatables.erase(it);
	else
		ASSERT(0);
}
void TranslationManager::UpdateTranslatables(ITranslation& translation)
{
	//=== ApplyTranslation may register or unregister translatables (windows being
	//=== created or destroyed), so iterate over a copy and skip the ones that went away.
	std::vector<ITranslatable*> translatables(m_translatables.begin(), m_translatables.end());
	for (size_t i = 0; i < translatables.size(); i++)
	{
		if (m_translatables.find(translatables[i]) != m_translatables.end())
			translatables[i]->ApplyTranslation(translation);
	}
}

//=== ITranslation
LPCTSTR TranslationManager::GetTranslationInfo(TranslationInfoEnum infoType)
{
	return m_fallBackTranslation.GetTranslationInfo(infoType);
}
LPCTSTR TranslationManager::GetString(UINT stringID)
{
	return m_fallBackTranslation.GetString(stringID);
}

//=== Xtra Interface

void TranslationManager::SetTranslationsFolder(LPCTSTR translationPath)
{
	ASSERT(translationPath != NULL);
	m_translationFolder = translationPath;
	if (!m_translationFolder.empty() && m_translationFolder[m_translationFolder.size() - 1] != '\\')
		m_translationFolder += _T("\\");
}

LPCTSTR TranslationManager::GetTranslationsFolder()
{
	return m_translationFolder.c_str();
}
void TranslationManager::RefreshTranslationsFolder()
{
	m_translations.clear();
	TCHAR searchPath[MAX_PATH];
	_sntprintf(searchPath, MAX_PATH, _T("%s*.lng"), m_translationFolder.c_str());
	CFileFind ff;
	BOOL bCont = ff.FindFile(searchPath);
	while (bCont)
	{
		bCont = ff.FindNextFile();
		FullTranslationInfo lInfo;
		lInfo.path = ff.GetFilePath();
		m_translations.push_back(lInfo);
	}
}


UINT TranslationManager::GetTranslationsCount()
{
	return m_translations.size();
}


//=== Info retrieval
LPCTSTR TranslationManager::GetTranslationInfo(INT idx, TranslationInfoEnum infoType)
{
	if (!IsValidIndex(idx))
	{
		ASSERT(0);
		return NULL;
	}
	if (m_translations[idx].info[ITranslation::TI_Name].empty())
		ReadTranslationInfo(m_translations[idx]);
	return m_translations[idx].info[infoType].c_str();
}

LPCTSTR TranslationManager::GetTranslationPath(INT idx)
{
	if (!IsValidIndex(idx))
	{
		ASSERT(0);
		return NULL;
	}
	return m_translations[idx].path.c_str();
}
const TranslationComparison* TranslationManager::GetTranslationComparison(INT idx)
{
	if (!IsValidIndex(idx))
	{
		ASSERT(0);
		return NULL;
	}
	if (m_translations[idx].comparison.firstCount == 0)
		ReadComparisonInfo(m_translations[idx]);
	return &m_translations[idx].comparison;
}
LPCTSTR TranslationManager::GetTranslationComparisonLog(INT idx)
{
	if (!IsValidIndex(idx))
	{
		ASSERT(0);
		return NULL;
	}
	if (m_translations[idx].comparison.firstCount == 0)
		ReadComparisonInfo(m_translations[idx]);
	return m_translations[idx].comparisonLog.c_str();
}

BOOL TranslationManager::ReadTranslationInfo(FullTranslationInfo& info)
{
	EditableTranslation t;
	if (ITranslationUtilities::ImportTranslationFile(info.path.c_str(), t, TRUE))
	{
		for (INT i = 0; i < ITranslation::TI_Last; i++)
		{
			ITranslation::TranslationInfoEnum it = ITranslation::TranslationInfoEnum(i);
			LPCTSTR text = t.GetTranslationInfo(it);
			info.info[i] = text == NULL ? _T("") : text;
		}
		if (info.info[TI_Name].empty())
		{
			TCHAR name[MAX_PATH];
			_tcsncpy(name, PathFindFileName(info.path.c_str()), MAX_PATH - 1);
			name[MAX_PATH - 1] = 0;
			PathRemoveExtension(name);
			info.info[TI_Name] = name;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL TranslationManager::ReadComparisonInfo(FullTranslationInfo& info)
{
	EditableTranslation translation;
	if (ITranslationUtilities::ImportTranslationFile(info.path.c_str(), translation, FALSE))
	{

		if (ITranslationUtilities::CompareTranslations(m_resourceTranslation, translation, info.comparison, info.comparisonLog))
		{
			return TRUE;
			//==Comparison successs
		}
	}
	return FALSE;
}


//=== Other

ITranslation& TranslationManager::GetDefaultTranslation()		
{
	return m_resourceTranslation;
}

LPCTSTR TranslationManager::GetTranslationPath()
{
	if (m_loadedTranslationPath.empty())
		return _T("@RES@");
	return m_loadedTranslationPath.c_str();
}


BOOL TranslationManager::LoadTranslation(LPCTSTR langPath)
{
	ASSERT(langPath != NULL);
	if (m_loadedTranslationPath != langPath)
	{
		UnloadTranslation();
		if (ITranslationUtilities::ImportTranslationFile(langPath, m_fileTranslation, FALSE))
		{
			m_loadedTranslationPath = langPath;
			m_fallBackTranslation.SetTranslation(&m_fileTranslation);
			return TRUE;
		}
		//=== Stay on the built-in strings. (The path used to be kept: GetTranslationPath
		//=== reported the failed file and loading it again returned TRUE.)
		m_fileTranslation.ClearStrings();
		TRACE(_T("@1 TranslationManager::LoadTranslation Import failed: '%s'\r\n"), langPath);
		return FALSE;
	}
	return TRUE;
}

BOOL TranslationManager::LoadTranslation(INT idx)
{
	if (!IsValidIndex(idx))
	{
		ASSERT(0);
		return FALSE;
	}
	return LoadTranslation(m_translations[idx].path.c_str());
}

void TranslationManager::GetLangFileNames(LANGID langID, std::vector<LPCTSTR>& names)
{
	names.clear();
	//=== Shipped files whose names differ from the language name, or that are
	//=== specific to a sublanguage
	WORD primary = PRIMARYLANGID(langID);
	WORD sub = SUBLANGID(langID);
	switch (primary)
	{
	case LANG_CHINESE:
		if (sub == SUBLANG_CHINESE_TRADITIONAL || sub == SUBLANG_CHINESE_HONGKONG || sub == SUBLANG_CHINESE_MACAU)
			names.push_back(_T("TraChinese"));
		break;
	case LANG_PORTUGUESE:
		if (sub == SUBLANG_PORTUGUESE_BRAZILIAN)
			names.push_back(_T("Brazilian Portuguese"));
		break;
	case LANG_SERBIAN:	//=== Also Croatian and Bosnian (same primary language)
		if (sub == SUBLANG_SERBIAN_CYRILLIC || sub == SUBLANG_SERBIAN_SERBIA_CYRILLIC
			|| sub == SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC || sub == SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC)
			names.push_back(_T("serbian_cir"));
		break;
	case LANG_FINNISH:
		names.push_back(_T("Suomi"));
		break;
	case LANG_INDONESIAN:
		names.push_back(_T("Indonesia"));
		break;
	}
	LPCTSTR langName = GetLangNameByLangID(langID);
	if (langName != NULL)
		names.push_back(langName);
}

BOOL TranslationManager::LoadTranslationByLANGID(LANGID langID)
{
	std::vector<LPCTSTR> names;
	GetLangFileNames(langID, names);
	for (size_t i = 0; i < names.size(); i++)
	{
		TCHAR path[MAX_PATH];
		_sntprintf(path, MAX_PATH, _T("%s%s.lng"), GetTranslationsFolder(), names[i]);
		path[MAX_PATH - 1] = 0;
		if (PathFileExists(path) && LoadTranslation(path))
			return TRUE;
	}
	UnloadTranslation();
	return FALSE;
}

BOOL TranslationManager::LoadSavedTranslation(LPCTSTR saved, LANGID systemLangID)
{
	if (saved == NULL || saved[0] == 0)
		return LoadTranslationByLANGID(systemLangID);	//=== Never chosen: the system language
	if (saved[0] == '@')
	{
		UnloadTranslation();						//=== The built-in strings were chosen
		return TRUE;
	}
	if (PathFileExists(saved))
		return LoadTranslation(saved);
	//=== The application folder moved: look for the file in the translations folder
	std::tstring path(m_translationFolder);
	path += PathFindFileName(saved);
	if (PathFileExists(path.c_str()))
		return LoadTranslation(path.c_str());
	TRACE(_T("@1 TranslationManager::LoadSavedTranslation Not found: '%s'\r\n"), saved);
	UnloadTranslation();
	return FALSE;
}

void TranslationManager::UnloadTranslation()
{
	m_fallBackTranslation.SetTranslation(NULL);
	m_fileTranslation.ClearStrings();
	m_loadedTranslationPath.clear();
}

LPCTSTR TranslationManager::GetLangNameByLangID(LANGID id)
{
	LPCTSTR cLangName[] =
	{
		_T("Neutral"),//0x00
		_T("Arabic"),
		_T("Bulgarian"),
		_T("Catalan"),
		_T("Chinese"),
		_T("Czech"),
		_T("Danish"),
		_T("German"),
		_T("Greek"),//0x08
		_T("English"),
		_T("Spanish"),
		_T("Finnish"),
		_T("French"),
		_T("Hebrew"),
		_T("Hungarian"),
		_T("Icelandic"),
		_T("Italian"),//0x10
		_T("Japanese"),
		_T("Korean"),
		_T("Dutch"),
		_T("Norwegian"),
		_T("Polish"),
		_T("Portuguese"),
		_T("Romansh"),
		_T("Romanian"),//0x18
		_T("Russian"),
		_T("Serbian"),
		_T("Slovak"),
		_T("Albanian"),
		_T("Swedish"),
		_T("Thai"),
		_T("Turkish"),
		_T("Urdu"),//0x20
		_T("Indonesian"),
		_T("Ukrainian"),
		_T("Belarusian"),
		_T("Slovenian"),
		_T("Estonian"),
		_T("Latvian"),
		_T("Lithuanian"),
		_T("Tajik"),//0x28
		_T("Persian"),
		_T("Vietnamese"),
		_T("Armenian"),
		_T("Azeri"),
		_T("Basque")//0x2d

	};
	static_assert(sizeof(cLangName) / sizeof(cLangName[0]) == 0x2e, "cLangName covers 0x00 - 0x2d");
	WORD index = PRIMARYLANGID(id);
	if (index <= 0x2d)
		return cLangName[index];
	return NULL;
}

BOOL TranslationManager::ExportTranslationFile(INT idx, LPCTSTR path)
{
	ASSERT(idx < (INT)GetTranslationsCount());
	if (idx >= (INT)GetTranslationsCount())
		return FALSE;
	if (idx >= 0)
	{
		EditableTranslation translation;
		if (!ITranslationUtilities::ImportTranslationFile(m_translations[idx].path.c_str(), translation, FALSE))
			return FALSE;
		if (!ITranslationUtilities::ExportTranslationFile(path, &translation, &m_resourceTranslation))
			return FALSE;
	}
	else
	{
		if (!ITranslationUtilities::ExportTranslationFile(path, NULL, &m_resourceTranslation))
			return FALSE;
	}
	return TRUE;
}