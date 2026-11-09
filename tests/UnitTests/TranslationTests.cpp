//	Translations: the .lng parser (ITranslationUtilities) and TranslationManager
//	(loading with fallback to the built-in strings, system-language lookup, startup).
#include "stdafx.h"
#include "TestFramework.h"
#include "TranslationManager.h"
#include "resource.h"
#include <vector>

namespace
{
	std::wstring Dir(const std::wstring& path)
	{
		CreateDirectoryW(path.c_str(), NULL);
		return path;
	}

	void WriteBytes(const std::wstring& path, const void* data, size_t len)
	{
		HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (f == INVALID_HANDLE_VALUE)
			return;
		DWORD written = 0;
		WriteFile(f, data, (DWORD)len, &written, NULL);
		CloseHandle(f);
	}

	void WriteUtf16(const std::wstring& path, const std::wstring& content)
	{
		std::wstring s(1, (WCHAR)0xFEFF);
		s += content;
		WriteBytes(path, s.c_str(), s.size() * sizeof(WCHAR));
	}

	void WriteUtf8(const std::wstring& path, const std::wstring& content, BOOL bBom)
	{
		INT len = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), (INT)content.size(), NULL, 0, NULL, NULL);
		std::string s(bBom ? "\xEF\xBB\xBF" : "");
		std::string body(len, 0);
		WideCharToMultiByte(CP_UTF8, 0, content.c_str(), (INT)content.size(), &body[0], len, NULL, NULL);
		s += body;
		WriteBytes(path, s.c_str(), s.size());
	}

	BOOL Is(LPCTSTR a, LPCTSTR b)
	{
		return a != NULL && b != NULL && wcscmp(a, b) == 0;
	}

	BOOL EndsWith(LPCWSTR str, LPCWSTR suffix)
	{
		size_t a = wcslen(str), b = wcslen(suffix);
		return a >= b && _wcsicmp(str + a - b, suffix) == 0;
	}

	//The built app exe as a resource-only module (it holds the English strings).
	HMODULE LoadAppModule()
	{
		WCHAR path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		*(wcsrchr(path, L'\\') + 1) = 0;
#ifdef _DEBUG
		wcscat(path, L"jaangledbg.exe");
#else
		wcscat(path, L"jaangle.exe");
#endif
		return LoadLibraryExW(path, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
	}

	std::wstring ExeDir()
	{
		WCHAR path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		*(wcsrchr(path, L'\\') + 1) = 0;
		return path;
	}

	class CountingTranslatable: public ITranslatable
	{
	public:
		CountingTranslatable(): m_count(0)					{}
		virtual void ApplyTranslation(ITranslation&)		{m_count++;}
		INT m_count;
	};

	class RemovingTranslatable: public ITranslatable
	{
	public:
		RemovingTranslatable(TranslationManager& tm, ITranslatable& other): m_tm(tm), m_other(other), m_count(0)	{}
		virtual void ApplyTranslation(ITranslation&)
		{
			m_count++;
			m_tm.UnRegisterTranslatable(*this);
			m_tm.UnRegisterTranslatable(m_other);
		}
		TranslationManager& m_tm;
		ITranslatable& m_other;
		INT m_count;
	};

	void TestParser()
	{
		std::wstring dir = Dir(TempPath(L"lng\\"));
		const std::wstring body =
			L"#comment=ignored\r\n"
			L"@NAME=Testish\r\n"
			L"@AUTH=\x0391\x03BB\x03AD\x03BE\r\n"
			L"@LOCALIZEDNAME=\r\n"
			L"@UNKNOWNKEY=x\r\n"
			L"\r\n"
			L"1=One\r\n"
			L"#2=Commented\r\n"
			L"3=Line\\r\\nBreak\\tTab\r\n"
			L"0=Zero is invalid\r\n"
			L"123456=Too long an id\r\n"
			L"junk line without equals\r\n"
			L"4=Still read after junk\r\n"
			L"5=Last line without newline";

		EditableTranslation t;
		std::wstring utf16 = dir + L"utf16.lng";
		WriteUtf16(utf16, body);
		CHECK(ITranslationUtilities::ImportTranslationFile(utf16.c_str(), t, FALSE));
		CHECK(Is(t.GetString(1), L"One"));
		CHECK(t.GetString(2) == NULL);
		CHECK(Is(t.GetString(3), L"Line\r\nBreak\tTab"));
		CHECK(t.GetString(0) == NULL);
		CHECK(Is(t.GetString(4), L"Still read after junk"));
		CHECK(Is(t.GetString(5), L"Last line without newline"));
		CHECK(t.GetStringCount() == 4);
		CHECK(Is(t.GetTranslationInfo(ITranslation::TI_Name), L"Testish"));
		CHECK(Is(t.GetTranslationInfo(ITranslation::TI_Author), L"\x0391\x03BB\x03AD\x03BE"));
		CHECK(t.GetTranslationInfo(ITranslation::TI_LocalizedName) == NULL);

		//Same content as UTF-8 (with BOM), with LF-only and CR-only line endings.
		std::wstring lf(body), cr(body);
		for (size_t p = lf.find(L"\r\n"); p != std::wstring::npos; p = lf.find(L"\r\n", p))
			lf.replace(p, 2, L"\n");
		for (size_t p = cr.find(L"\r\n"); p != std::wstring::npos; p = cr.find(L"\r\n", p))
			cr.replace(p, 2, L"\r");
		std::wstring utf8 = dir + L"utf8.lng";
		WriteUtf8(utf8, lf, TRUE);
		EditableTranslation t8;
		CHECK(ITranslationUtilities::ImportTranslationFile(utf8.c_str(), t8, FALSE));
		CHECK(Is(t8.GetString(3), L"Line\r\nBreak\tTab"));
		CHECK(Is(t8.GetString(5), L"Last line without newline"));
		CHECK(t8.GetStringCount() == 4);
		CHECK(Is(t8.GetTranslationInfo(ITranslation::TI_Author), L"\x0391\x03BB\x03AD\x03BE"));
		std::wstring crFile = dir + L"cr.lng";
		WriteUtf16(crFile, cr);
		EditableTranslation tcr;
		CHECK(ITranslationUtilities::ImportTranslationFile(crFile.c_str(), tcr, FALSE));
		CHECK(tcr.GetStringCount() == 4 && Is(tcr.GetString(4), L"Still read after junk"));

		//ANSI (no BOM): the whole file is read and terminated.
		std::wstring ansi = dir + L"ansi.lng";
		const char ansiBody[] = "@NAME=Ansi\r\n7=Seven";
		WriteBytes(ansi, ansiBody, sizeof(ansiBody) - 1);
		EditableTranslation ta;
		CHECK(ITranslationUtilities::ImportTranslationFile(ansi.c_str(), ta, FALSE));
		CHECK(Is(ta.GetString(7), L"Seven"));
		CHECK(Is(ta.GetTranslationInfo(ITranslation::TI_Name), L"Ansi"));

		//Info lines that used to hang the parser: "@" without '=', '=' only on a later line.
		std::wstring hang = dir + L"My Lang.v2.lng";
		WriteUtf16(hang, L"@BROKEN\r\n@NAME=\r\n@ALSOBROKEN\r\n8=Eight=8\r\n@");
		EditableTranslation th;
		CHECK(ITranslationUtilities::ImportTranslationFile(hang.c_str(), th, FALSE));
		CHECK(Is(th.GetString(8), L"Eight=8"));
		CHECK(Is(th.GetTranslationInfo(ITranslation::TI_Name), L"My Lang.v2"));	//empty @NAME: the file name

		//Info only
		EditableTranslation ti;
		CHECK(ITranslationUtilities::ImportTranslationFile(utf16.c_str(), ti, TRUE));
		CHECK(ti.GetStringCount() == 0 && Is(ti.GetTranslationInfo(ITranslation::TI_Name), L"Testish"));

		EditableTranslation tm;
		CHECK(!ITranslationUtilities::ImportTranslationFile((dir + L"missing.lng").c_str(), tm, FALSE));
		std::wstring tiny = dir + L"tiny.lng";
		WriteBytes(tiny, "x", 1);
		CHECK(!ITranslationUtilities::ImportTranslationFile(tiny.c_str(), tm, FALSE));
	}

	void TestLangFileNames()
	{
		std::vector<LPCTSTR> n;
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT), n);
		CHECK(n.size() == 1 && Is(n[0], L"Greek"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_FINNISH, SUBLANG_DEFAULT), n);
		CHECK(n.size() == 2 && Is(n[0], L"Suomi") && Is(n[1], L"Finnish"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_INDONESIAN, SUBLANG_DEFAULT), n);
		CHECK(n.size() == 2 && Is(n[0], L"Indonesia"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL), n);
		CHECK(n.size() == 2 && Is(n[0], L"TraChinese") && Is(n[1], L"Chinese"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), n);
		CHECK(n.size() == 1 && Is(n[0], L"Chinese"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN), n);
		CHECK(n.size() == 2 && Is(n[0], L"Brazilian Portuguese") && Is(n[1], L"Portuguese"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE), n);
		CHECK(n.size() == 1 && Is(n[0], L"Portuguese"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_CYRILLIC), n);
		CHECK(n.size() == 2 && Is(n[0], L"serbian_cir") && Is(n[1], L"Serbian"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_LATIN), n);
		CHECK(n.size() == 1 && Is(n[0], L"Serbian"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_BASQUE, SUBLANG_DEFAULT), n);
		CHECK(n.size() == 1 && Is(n[0], L"Basque"));
		TranslationManager::GetLangFileNames(MAKELANGID(LANG_HINDI, SUBLANG_DEFAULT), n);	//0x39: not in the table
		CHECK(n.empty());
	}

	void TestManager(HMODULE hApp)
	{
		std::wstring folder = Dir(TempPath(L"langs\\"));
		WriteUtf16(folder + L"Greek.lng", L"@NAME=Greek\r\n@AUTH=Tester\r\n" + std::to_wstring(IDS_ARTIST) + L"=\x039A\x03B1\x03BB\x03BB\x03B9\x03C4\x03AD\x03C7\x03BD\x03B7\x03C2\r\n");
		WriteUtf16(folder + L"Suomi.lng", L"@NAME=Finnish\r\n1=Yksi\r\n");
		WriteUtf16(folder + L"TraChinese.lng", L"@NAME=Traditional Chinese\r\n1=Tra\r\n");
		WriteUtf16(folder + L"Chinese.lng", L"@NAME=Chinese\r\n1=Sim\r\n");
		WriteBytes(folder + L"Broken.lng", "x", 1);
		WriteUtf16(folder + L"NotALanguage.txt", L"1=x\r\n");

		TranslationManager tm;
		tm.SetResourceModule(hApp);
		std::wstring noSlash = folder.substr(0, folder.size() - 1);
		tm.SetTranslationsFolder(noSlash.c_str());
		CHECK(tm.GetTranslationsFolder() == folder);

		tm.RefreshTranslationsFolder();
		CHECK(tm.GetTranslationsCount() == 5);
		BOOL bGreek = FALSE;
		for (UINT i = 0; i < tm.GetTranslationsCount(); i++)
		{
			if (EndsWith(tm.GetTranslationPath(i), L"\\Greek.lng"))
				bGreek = Is(tm.GetTranslationInfo(i, ITranslation::TI_Name), L"Greek")
					&& Is(tm.GetTranslationInfo(i, ITranslation::TI_Author), L"Tester");
		}
		CHECK(bGreek);

		//Built-in strings
		LPCTSTR builtIn = tm.GetDefaultTranslation().GetString(IDS_ARTIST);
		CHECK(builtIn != NULL && builtIn[0] != 0);
		LPCTSTR builtInTitle = tm.GetDefaultTranslation().GetString(IDS_TITLE);
		CHECK(Is(tm.GetString(IDS_ARTIST), builtIn));
		CHECK(Is(tm.GetTranslationPath(), L"@RES@"));

		//A translation, falling back to the built-in strings it doesn't have
		std::wstring greek = folder + L"Greek.lng";
		CHECK(tm.LoadTranslation(greek.c_str()));
		CHECK(tm.GetTranslationPath() == greek);
		CHECK(Is(tm.GetString(IDS_ARTIST), L"\x039A\x03B1\x03BB\x03BB\x03B9\x03C4\x03AD\x03C7\x03BD\x03B7\x03C2"));
		CHECK(Is(tm.GetString(IDS_TITLE), builtInTitle));
		CHECK(Is(tm.GetTranslationInfo(ITranslation::TI_Name), L"Greek"));

		//A failed load returns to the built-in strings and isn't remembered
		std::wstring broken = folder + L"Broken.lng";
		CHECK(!tm.LoadTranslation(broken.c_str()));
		CHECK(Is(tm.GetTranslationPath(), L"@RES@"));
		CHECK(Is(tm.GetString(IDS_ARTIST), builtIn));
		CHECK(!tm.LoadTranslation(broken.c_str()));	//was TRUE the second time
		CHECK(Is(tm.GetTranslationPath(), L"@RES@"));

		//System language
		CHECK(tm.LoadTranslationByLANGID(MAKELANGID(LANG_FINNISH, SUBLANG_DEFAULT)));
		CHECK(EndsWith(tm.GetTranslationPath(), L"\\Suomi.lng") && Is(tm.GetString(1), L"Yksi"));
		CHECK(tm.LoadTranslationByLANGID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)));
		CHECK(Is(tm.GetString(1), L"Tra"));
		CHECK(tm.LoadTranslationByLANGID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)));
		CHECK(Is(tm.GetString(1), L"Sim"));
		CHECK(!tm.LoadTranslationByLANGID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)));	//no English.lng
		CHECK(Is(tm.GetTranslationPath(), L"@RES@"));

		//Startup
		LANGID greekID = MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT);
		CHECK(tm.LoadSavedTranslation(L"", greekID));				//never chosen: system language
		CHECK(tm.GetTranslationPath() == greek);
		CHECK(tm.LoadSavedTranslation(L"@RES@", greekID));			//built-in chosen
		CHECK(Is(tm.GetTranslationPath(), L"@RES@") && Is(tm.GetString(IDS_ARTIST), builtIn));
		CHECK(tm.LoadSavedTranslation((folder + L"Suomi.lng").c_str(), greekID));
		CHECK(Is(tm.GetString(1), L"Yksi"));
		CHECK(tm.LoadSavedTranslation(L"C:\\Old\\Install\\Langs\\Greek.lng", greekID));	//app folder moved
		CHECK(tm.GetTranslationPath() == greek);
		CHECK(!tm.LoadSavedTranslation(L"C:\\Old\\Install\\Langs\\Klingon.lng", greekID));
		CHECK(Is(tm.GetTranslationPath(), L"@RES@"));

		//Export a translation and read it back
		INT greekIdx = -1;
		for (UINT i = 0; i < tm.GetTranslationsCount(); i++)
			if (EndsWith(tm.GetTranslationPath(i), L"\\Greek.lng"))
				greekIdx = i;
		std::wstring exported = Dir(TempPath(L"lngexport\\")) + L"Exported.lng";
		CHECK(greekIdx >= 0 && tm.ExportTranslationFile(greekIdx, exported.c_str()));
		EditableTranslation back;
		CHECK(ITranslationUtilities::ImportTranslationFile(exported.c_str(), back, FALSE));
		CHECK(Is(back.GetString(IDS_ARTIST), L"\x039A\x03B1\x03BB\x03BB\x03B9\x03C4\x03AD\x03C7\x03BD\x03B7\x03C2"));
		CHECK(back.GetStringCount() == 1);	//untranslated strings are exported commented out
		CHECK(Is(back.GetTranslationInfo(ITranslation::TI_Author), L"Tester"));

		//Translatables
		CountingTranslatable a, c;
		RemovingTranslatable b(tm, c);
		tm.RegisterTranslatable(a);
		tm.RegisterTranslatable(b);
		tm.RegisterTranslatable(c);
		tm.UpdateTranslatables(tm);		//b unregisters itself and c while the list is walked
		CHECK(a.m_count == 1 && b.m_count == 1 && c.m_count <= 1);
		INT cCount = c.m_count;
		tm.UpdateTranslatables(tm);
		CHECK(a.m_count == 2 && b.m_count == 1 && c.m_count == cCount);
		tm.UnRegisterTranslatable(a);
	}

	//Every translation shipped in Langs\ (copied next to UnitTests.exe by the build) parses.
	void TestShippedTranslations(HMODULE hApp)
	{
		TranslationManager tm;
		tm.SetResourceModule(hApp);
		tm.SetTranslationsFolder((ExeDir() + L"Langs\\").c_str());
		tm.RefreshTranslationsFolder();
		CHECK(tm.GetTranslationsCount() > 0);
		if (tm.GetTranslationsCount() == 0)
			wprintf(L"  (no translations found in %sLangs)\n", ExeDir().c_str());
		for (UINT i = 0; i < tm.GetTranslationsCount(); i++)
		{
			LPCTSTR path = tm.GetTranslationPath(i);
			EditableTranslation t;
			BOOL bOk = ITranslationUtilities::ImportTranslationFile(path, t, FALSE);
			CHECK(bOk && t.GetStringCount() > 100);
			if (!bOk || t.GetStringCount() <= 100)
				wprintf(L"    %s: %u strings\n", path, t.GetStringCount());
			CHECK(tm.GetTranslationInfo(i, ITranslation::TI_Name)[0] != 0);
			const TranslationComparison* pComp = tm.GetTranslationComparison(i);
			CHECK(pComp != NULL && pComp->firstCount > 100 && pComp->secondCount > 100);
			CHECK(tm.LoadTranslation((INT)i));
		}
	}
}

void RunTranslationTests()
{
	wprintf(L"Translation tests\n");
	TestParser();
	TestLangFileNames();
	HMODULE hApp = LoadAppModule();
	CHECK(hApp != NULL);
	if (hApp == NULL)
	{
		wprintf(L"  (app exe not found next to UnitTests.exe)\n");
		return;
	}
	TestManager(hApp);
	TestShippedTranslations(hApp);
	FreeLibrary(hApp);
}
