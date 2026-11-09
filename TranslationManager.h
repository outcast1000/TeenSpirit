#ifndef _TRANSLATIONMANAGER_H_
#define _TRANSLATIONMANAGER_H_

#include "ITranslation/ITranslation.h"
#include "ITranslation/EditableTranslation.h"
#include "ITranslation/ResourceTranslation.h"
#include "ITranslation/FallbackTranslation.h"
#include "ITranslation/ITranslationUtilities.h"
#include <set>


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
	BOOL LoadTranslation(LPCTSTR translationPath);
	BOOL LoadTranslation(INT idx);
	BOOL LoadTranslationByLANGID(LANGID langID);
	void UnloadTranslation();



	BOOL ExportTranslationFile(INT idx, LPCTSTR pathName);

private:
	FallbackTranslation m_fallBackTranslation;
	EditableTranslation m_fileTranslation;
	ResourceTranslation m_resourceTranslation;
	EditableTranslation m_tempTranslation;
	std::tstring m_translationFolder;
	std::tstring m_loadedTranslationPath;

	static LPCTSTR GetLangNameByLangID(LANGID id);

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