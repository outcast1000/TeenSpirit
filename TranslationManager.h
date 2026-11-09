#ifndef _TRANSLATIONMANAGER_H_
#define _TRANSLATIONMANAGER_H_

#include "ITranslation/ITranslation.h"
#include "ITranslation/EditableTranslation.h"
#include "ITranslation/ResourceTranslation.h"
#include "ITranslation/FallbackTranslation.h"
#include "ITranslation/ITranslationUtilities.h"
#include <set>
#include <vector>


class TranslationManager: public ITranslationManager, public ITranslation
{
public:
	TranslationManager();
	virtual ~TranslationManager();

public:	//=== ITranslationManager
	virtual void RegisterTranslatable(ITranslatable& translatable);
	virtual void UnRegisterTranslatable(ITranslatable& translatable);
	virtual void UpdateTranslatables(ITranslation& translation);
private:
	std::set<ITranslatable*> m_translatables;

public:	//=== ITranslation
	virtual LPCTSTR GetTranslationInfo(TranslationInfoEnum infoType);
	virtual LPCTSTR GetString(UINT stringID);

public:	//=== Translation Folder Management
	void SetTranslationsFolder(LPCTSTR translationPath);
	LPCTSTR GetTranslationsFolder();
	void RefreshTranslationsFolder();
	UINT GetTranslationsCount();


	//=== Info retrieval
	LPCTSTR GetTranslationPath();

	LPCTSTR GetTranslationInfo(INT idx, TranslationInfoEnum infoType);	
	LPCTSTR GetTranslationPath(INT idx);	
	const TranslationComparison* GetTranslationComparison(INT idx);
	LPCTSTR GetTranslationComparisonLog(INT idx);

	ITranslation& GetDefaultTranslation();
	//The module holding the built-in (English) strings. NULL (default): the exe.
	void SetResourceModule(HINSTANCE hInstance)	{m_resourceTranslation.SetResourceInstance(hInstance);}

	//FALSE when the file can't be read; the built-in strings are used then.
	BOOL LoadTranslation(LPCTSTR translationPath);
	BOOL LoadTranslation(INT idx);
	//Loads the translation file for a Windows language from the translations folder.
	//FALSE (no such file): the built-in strings are used.
	BOOL LoadTranslationByLANGID(LANGID langID);
	void UnloadTranslation();

	//Startup. saved is the stored GetTranslationPath(): empty (never chosen) loads
	//the system language, "@..." keeps the built-in strings, a path loads that file
	//or, when it no longer exists there, the file of that name in the translations folder.
	BOOL LoadSavedTranslation(LPCTSTR saved, LANGID systemLangID);

	//The candidate file names (without ".lng") for a Windows language, best first.
	static void GetLangFileNames(LANGID langID, std::vector<LPCTSTR>& names);



	BOOL ExportTranslationFile(INT idx, LPCTSTR pathName);

private:
	FallbackTranslation m_fallBackTranslation;
	EditableTranslation m_fileTranslation;
	ResourceTranslation m_resourceTranslation;
	EditableTranslation m_tempTranslation;
	std::tstring m_translationFolder;
	std::tstring m_loadedTranslationPath;

	static LPCTSTR GetLangNameByLangID(LANGID id);
	BOOL IsValidIndex(INT idx) const	{return idx >= 0 && idx < (INT)m_translations.size();}

	struct FullTranslationInfo
	{
		FullTranslationInfo()						{}
		std::tstring info[ITranslation::TI_Last];
		std::tstring path;
		TranslationComparison comparison;
		std::tstring comparisonLog;
	};
	std::vector<FullTranslationInfo> m_translations;

	BOOL ReadTranslationInfo(FullTranslationInfo& info);
	BOOL ReadComparisonInfo(FullTranslationInfo& info);

};


#endif