//	Services extracted from PrgAPI: AppOptions (option table and values over the
//	AppSettings interface) and AppStrings (enum display names over ITranslation).
#include "stdafx.h"
#include "TestFramework.h"
#include "AppOptions.h"
#include "AppStrings.h"
#include "AppSettings/IniAppSettings.h"
#include "ITranslation/ITranslation.h"
#include "resource.h"
#include <map>

namespace
{
	//AppSettings kept in memory; counts writes.
	class MemorySettings: public AppSettings
	{
	public:
		virtual BOOL Load(LPCTSTR)	{return TRUE;}
		virtual BOOL Save()			{return TRUE;}
		virtual BOOL Read(LPCTSTR category, LPCTSTR setting, INT& value, INT defaultVal)
		{
			std::map<std::wstring, std::wstring>::const_iterator it = m_values.find(Key(category, setting));
			value = (it == m_values.end()) ? defaultVal : _wtoi(it->second.c_str());
			return TRUE;
		}
		virtual BOOL Read(LPCTSTR category, LPCTSTR setting, std::basic_string<TCHAR>& value, LPCTSTR defaultVal)
		{
			std::map<std::wstring, std::wstring>::const_iterator it = m_values.find(Key(category, setting));
			if (it != m_values.end())
				value = it->second;
			else
				value = defaultVal != NULL ? defaultVal : L"";
			return TRUE;
		}
		virtual BOOL Write(LPCTSTR category, LPCTSTR setting, LPCTSTR value)
		{
			m_writes++;
			m_values[Key(category, setting)] = value;
			return TRUE;
		}
		virtual BOOL Write(LPCTSTR category, LPCTSTR setting, INT value)
		{
			return Write(category, setting, std::to_wstring(value).c_str());
		}
		BOOL Has(LPCTSTR category, LPCTSTR setting) const	{return m_values.count(Key(category, setting)) > 0;}
		std::wstring Get(LPCTSTR category, LPCTSTR setting) const
		{
			std::map<std::wstring, std::wstring>::const_iterator it = m_values.find(Key(category, setting));
			return it == m_values.end() ? L"" : it->second;
		}
		INT m_writes = 0;
	private:
		static std::wstring Key(LPCTSTR category, LPCTSTR setting)	{return std::wstring(category) + L"|" + setting;}
		std::map<std::wstring, std::wstring> m_values;
	};

	//Returns "#<id>" for the IDs it knows, NULL otherwise.
	class FakeTranslation: public ITranslation
	{
	public:
		virtual LPCTSTR GetTranslationInfo(TranslationInfoEnum)	{return NULL;}
		virtual LPCTSTR GetString(UINT stringID)
		{
			if (m_missing.count(stringID))
				return NULL;
			std::wstring& s = m_strings[stringID];
			s = L"#" + std::to_wstring(stringID);
			return s.c_str();
		}
		std::set<UINT> m_missing;
	private:
		std::map<UINT, std::wstring> m_strings;
	};

	std::wstring Res(INT id)	{return L"#" + std::to_wstring(id);}

	void TestOptionTable()
	{
		CHECK(AppOptions::IsTableConsistent());
		for (INT i = 0; i < OPT_Last; i++)
		{
			CHECK(AppOptions::GetName(AppOptionsEnum(i)) != NULL && AppOptions::GetName(AppOptionsEnum(i))[0] != 0);
			CHECK(AppOptions::GetCategory(AppOptionsEnum(i)) != NULL);
		}
		//The names are the keys in existing jaangle.ini files: they must not change.
		CHECK(wcscmp(AppOptions::GetName(OPT_GEN_Debugging), L"Debugging") == 0);
		CHECK(wcscmp(AppOptions::GetCategory(OPT_GEN_Debugging), L"General") == 0);
		CHECK(wcscmp(AppOptions::GetName(OPT_PL_Volume), L"Volume") == 0);
		CHECK(wcscmp(AppOptions::GetCategory(OPT_PL_Volume), L"Player") == 0);
		CHECK(wcscmp(AppOptions::GetName(OPT_TAG_M4A), L"m4a") == 0);
		CHECK(wcscmp(AppOptions::GetCategory(OPT_TAG_M4A), L"TagManager") == 0);
		CHECK(wcscmp(AppOptions::GetName(OPT_MINIPLR_ForceOnTop), L"ForceOntTop") == 0);
		CHECK(wcscmp(AppOptions::GetCategory(OPT_IM_UseFolderImages), L"InfoManager") == 0);
		CHECK(wcscmp(AppOptions::GetCategory(OPT_MINIPLR_MaxSize), L"MiniPlayer") == 0);
		CHECK(AppOptions::GetDefault(OPT_GEN_DaysMeansNew) == 30);
		CHECK(AppOptions::GetDefault(OPT_PL_Volume) == 90);
		CHECK(AppOptions::GetDefault(OPT_TAG_M4A) == 1);
	}

	void TestOptionValues()
	{
		MemorySettings settings;
		AppOptions opts(settings);
		//=== Defaults before and after reading empty settings
		CHECK(opts.Get(OPT_PL_Volume) == 90);
		opts.Read();
		for (INT i = 0; i < OPT_Last; i++)
			CHECK(opts.Get(AppOptionsEnum(i)) == AppOptions::GetDefault(AppOptionsEnum(i)));
		CHECK(settings.m_writes == 0);

		//=== Set only changes memory; Write stores everything
		opts.Set(OPT_PL_Volume, 42);
		CHECK(opts.Get(OPT_PL_Volume) == 42);
		CHECK(!settings.Has(L"Player", L"Volume"));
		opts.Write();
		CHECK(settings.m_writes == OPT_Last);
		CHECK(settings.Get(L"Player", L"Volume") == L"42");
		CHECK(settings.Get(L"General", L"DaysMeansNew") == L"30");

		//=== Write(opt) stores a single value
		opts.Set(OPT_GEN_LastVersion, 1234);
		INT writes = settings.m_writes;
		opts.Write(OPT_GEN_LastVersion);
		CHECK(settings.m_writes == writes + 1);
		CHECK(settings.Get(L"General", L"LastVersion") == L"1234");

		//=== Another instance reads the stored values
		AppOptions opts2(settings);
		opts2.Read();
		CHECK(opts2.Get(OPT_PL_Volume) == 42);
		CHECK(opts2.Get(OPT_GEN_LastVersion) == 1234);
		CHECK(opts2.Get(OPT_PL_Crossfade) == AppOptions::GetDefault(OPT_PL_Crossfade));

		//=== String options
		TCHAR bf[100];
		CHECK(opts.GetString(SOPT_GEN_Monitoring, bf, 100));
		CHECK(bf[0] == 0);
		CHECK(opts.SetString(SOPT_GEN_Monitoring, L"abc"));
		CHECK(settings.Get(L"General", L"AppMonitoring") == L"abc");
		CHECK(opts.GetString(SOPT_GEN_Monitoring, bf, 100));
		CHECK(wcscmp(bf, L"abc") == 0);
		CHECK(!opts.GetString(SOPT_GEN_Monitoring, bf, 3));	//too small
	}

	void TestOptionsIniRoundTrip()
	{
		std::wstring path = TempPath(L"options.ini");
		DeleteFileW(path.c_str());
		{
			IniAppSettings ini;
			ini.Load(path.c_str());
			AppOptions opts(ini);
			opts.Read();
			opts.Set(OPT_PL_Volume, 55);
			opts.Set(OPT_TAG_M4A, 0);
			opts.Write();
			opts.SetString(SOPT_GEN_Monitoring, L"x=1");
			ini.Save();
		}
		//=== The file uses the same sections and keys as before
		WCHAR v[100];
		GetPrivateProfileStringW(L"Player", L"Volume", L"", v, 100, path.c_str());
		CHECK(wcscmp(v, L"55") == 0);
		GetPrivateProfileStringW(L"TagManager", L"m4a", L"", v, 100, path.c_str());
		CHECK(wcscmp(v, L"0") == 0);
		{
			IniAppSettings ini;
			ini.Load(path.c_str());
			AppOptions opts(ini);
			opts.Read();
			CHECK(opts.Get(OPT_PL_Volume) == 55);
			CHECK(opts.Get(OPT_TAG_M4A) == 0);
			CHECK(opts.Get(OPT_GEN_DaysMeansNew) == 30);
			TCHAR bf[100];
			CHECK(opts.GetString(SOPT_GEN_Monitoring, bf, 100) && wcscmp(bf, L"x=1") == 0);
		}
		DeleteFileW(path.c_str());
	}

	void TestStrings()
	{
		FakeTranslation tr;
		CHECK(AppStrings::Get(tr, IDS_ARTIST) == Res(IDS_ARTIST));
		tr.m_missing.insert(IDS_ARTIST);
		CHECK(wcscmp(AppStrings::Get(tr, IDS_ARTIST), L"") == 0);	//missing string -> ""
		tr.m_missing.clear();

		CHECK(AppStrings::ForItemType(tr, IT_Artist) == Res(IDS_ARTIST));
		CHECK(AppStrings::ForItemType(tr, IT_Track) == Res(IDS_TITLE));
		CHECK(AppStrings::ForItemType(tr, IT_QuickSearch) == Res(IDS_UNKNOWN));
		CHECK(AppStrings::ForInfoItemType(tr, IIT_TrackLyrics) == Res(IDS_LYRICS));
		CHECK(AppStrings::ForInfoItemType(tr, IIT_AlbumPicture) == Res(IDS_ALBUMPICTURE));
		CHECK(AppStrings::ForInfoItemType(tr, IIT_MonthPicture) == Res(IDS_UNKNOWN));	//was read past the table
		CHECK(AppStrings::ForCollectionType(tr, CTYPE_Virtual) == Res(IDS_VIRTUALTRACKS));
		CHECK(AppStrings::ForCollectionType(tr, CTYPE_HTTPLinks) == Res(IDS_UNKNOWN));
		CHECK(AppStrings::ForRandomMode(tr, RTM_Newest) == Res(IDS_NEW));
		CHECK(AppStrings::ForTrackTextType(tr, TTT_Album) == Res(IDS_ALBUM));
		CHECK(AppStrings::ForPlayListMode(tr, MPNM_AutoContinue) == Res(IDS_AUTOCONTINUE));
		CHECK(AppStrings::ForTrackType(tr, TTYPE_mp3) == Res(IDS_AUDIO));
		CHECK(AppStrings::ForTrackType(tr, TTYPE_mpg) == Res(IDS_VIDEO));
		CHECK(AppStrings::ForTrackType(tr, TTYPE_http) == Res(IDS_VIRTUALTRACKS));
		CHECK(AppStrings::ForStars(tr, 0) == Res(IDS_CLEAR));
		CHECK(AppStrings::ForStars(tr, 5) == Res(IDS_OUTSTANDING));
		CHECK(AppStrings::ForCaseStyle(tr, CASE_Proper) == Res(IDS_PROPERCASE));
		CHECK(AppStrings::ForRecordSortOption(tr, RSO_Title) == Res(IDS_NAME));
		CHECK(AppStrings::ForRecordSortOption(tr, RSO_TrackCount) == Res(IDS_NUMBEROFTRACKS));
		CHECK(AppStrings::ForRecordSortOption(tr, RSO_LastDate) == Res(IDS_LASTAPPEARANCE));	//was read past the table
		CHECK(AppStrings::ForSectionFilter(tr, SFILT_New) == Res(IDS_NEW));
		CHECK(AppStrings::ForSectionType(tr, ST_MonthAdded) == Res(IDS_MONTH));
		CHECK(wcscmp(AppStrings::ForSectionType(tr, ST_Directory), L"") == 0);
	}
}

void RunAppServicesTests()
{
	wprintf(L"AppServices tests\n");
	TestOptionTable();
	TestOptionValues();
	TestOptionsIniRoundTrip();
	TestStrings();
}
