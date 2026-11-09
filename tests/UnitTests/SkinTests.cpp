//	Skins: TSSkin (colors.ini parsing, partial-skin completion, fonts) and
//	SkinManager (skins folder, startup loading with fallback, skinnables).
#include "stdafx.h"
#include "TestFramework.h"
#include "SkinManager.h"
#include "TSSkinnable.h"
#include <vector>
#include <set>

namespace
{
	std::wstring Dir(const std::wstring& path)
	{
		CreateDirectoryW(path.c_str(), NULL);
		return path;
	}

	//Writes a UTF-16LE ini (with BOM), as the shipped skins are stored.
	void WriteIni(const std::wstring& path, const std::wstring& content)
	{
		HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (f == INVALID_HANDLE_VALUE)
			return;
		DWORD written = 0;
		const WCHAR bom = 0xFEFF;
		WriteFile(f, &bom, sizeof(bom), &written, NULL);
		WriteFile(f, content.c_str(), (DWORD)(content.size() * sizeof(WCHAR)), &written, NULL);
		CloseHandle(f);
	}

	//A skin folder <root><name>\colors.ini with the given content.
	std::wstring MakeSkin(const std::wstring& root, LPCWSTR name, const std::wstring& ini)
	{
		std::wstring dir = Dir(root + name);
		WriteIni(dir + L"\\colors.ini", ini);
		return dir;
	}

	BOOL EndsWith(LPCWSTR str, LPCWSTR suffix)
	{
		size_t a = wcslen(str), b = wcslen(suffix);
		return a >= b && _wcsicmp(str + a - b, suffix) == 0;
	}

	LOGFONT FontOf(TSSkin& skin, SectionsEnum sec, FontStylesEnum style)
	{
		LOGFONT lf = {0};
		CFont* pFont = skin.GetFont(sec, style);
		if (pFont != NULL)
			pFont->GetLogFont(&lf);
		return lf;
	}

	class CountingSkinnable: public ISkinnable
	{
	public:
		CountingSkinnable(): m_count(0)	{}
		virtual void ApplySkin(ISkin&)	{m_count++;}
		INT m_count;
	};

	//Unregisters itself and another skinnable while being skinned.
	class RemovingSkinnable: public ISkinnable
	{
	public:
		RemovingSkinnable(SkinManager& sm, ISkinnable& other): m_sm(sm), m_other(other), m_count(0)	{}
		virtual void ApplySkin(ISkin&)
		{
			m_count++;
			m_sm.UnRegisterSkinnable(*this);
			m_sm.UnRegisterSkinnable(m_other);
		}
		SkinManager& m_sm;
		ISkinnable& m_other;
		INT m_count;
	};

	void TestSkinFolderName()
	{
		TCHAR bf[MAX_PATH];
		TSSkin::GetSkinFolderName(L"C:\\Skins\\Blue", bf, MAX_PATH);
		CHECK(wcscmp(bf, L"Blue") == 0);
		TSSkin::GetSkinFolderName(L"C:\\Skins\\Blue\\", bf, MAX_PATH);	//was an endless loop
		CHECK(wcscmp(bf, L"Blue") == 0);
		TSSkin::GetSkinFolderName(L"Blue", bf, MAX_PATH);					//was left unset
		CHECK(wcscmp(bf, L"Blue") == 0);
		TSSkin::GetSkinFolderName(L"C:\\Skins\\LongName", bf, 5);
		CHECK(wcscmp(bf, L"Long") == 0);
		TSSkin::GetSkinInfoPath(L"C:\\Skins\\Blue", bf, MAX_PATH);
		CHECK(wcscmp(bf, L"C:\\Skins\\Blue\\colors.ini") == 0);
		TSSkin::GetSkinInfoPath(L"C:\\Skins\\Blue\\", bf, MAX_PATH);
		CHECK(wcscmp(bf, L"C:\\Skins\\Blue\\colors.ini") == 0);
	}

	void TestPartialSkin()
	{
		std::wstring root = Dir(TempPath(L"skins\\"));
		std::wstring longFace(70, L'F');
		std::wstring dir = MakeSkin(root, L"Partial",
			L"[SkinDetails]\r\nName=Partial Test\r\nAuthor=Tester\r\n"
			L"[StatusBar]\r\nBk=#102030\r\nText=#405060\r\n"
			L"[TrackList]\r\nTextSelBk=#AABBCC\r\n"
			L"[Global]\r\nFont_Normal=" + longFace + L";20;0;700;1;0\r\nInt_MaxRelativeFont=8\r\n");

		TSSkin skin;
		CHECK(skin.LoadSkin(dir.c_str()));
		CHECK(skin.GetSkinPath() == dir);
		CHECK(wcscmp(skin.GetSkinInfo(ISkin::SI_Name), L"Partial Test (Partial)") == 0);
		CHECK(wcscmp(skin.GetSkinInfo(ISkin::SI_Author), L"Tester") == 0);
		CHECK(skin.GetColor(CSEC_StatusBar, CVAL_Bk) == RGB(0x10, 0x20, 0x30));
		//Missing colors fall back to the status bar's, or to the old key names.
		CHECK(skin.GetColor(CSEC_TrackList, CVAL_Bk) == RGB(0x10, 0x20, 0x30));
		CHECK(skin.GetColor(CSEC_TrackList, CVAL_Text) == RGB(0x40, 0x50, 0x60));
		CHECK(skin.GetColor(CSEC_TrackList, CVAL_SelBk) == RGB(0xAA, 0xBB, 0xCC));
		CHECK(skin.GetColor(CSEC_Panes, CVAL_CaptionOutline) == RGB(128, 128, 128));
		CHECK(skin.GetIntParameter(CSEC_Toolbar, IPARAM_Height) == 26);	//built-in default

		//The face name is cut to LF_FACESIZE (it overflowed lfFaceName).
		LOGFONT lf = FontOf(skin, CSEC_Global, FSTY_Normal);
		CHECK(wcslen(lf.lfFaceName) == LF_FACESIZE - 1);
		CHECK(lf.lfHeight == 20 && lf.lfWeight == 700 && lf.lfItalic && !lf.lfUnderline);

		//A partial skin is completed on disk.
		std::wstring ini = dir + L"\\colors.ini";
		WCHAR bf[100];
		GetPrivateProfileStringW(L"Toolbar", L"Int_Height", L"", bf, 100, ini.c_str());
		CHECK(wcscmp(bf, L"26") == 0);
		GetPrivateProfileStringW(L"StatusBar", L"Bk", L"", bf, 100, ini.c_str());
		CHECK(wcscmp(bf, L"#102030") == 0);
		GetPrivateProfileStringW(L"TrackList", L"SelBk", L"", bf, 100, ini.c_str());
		CHECK(wcscmp(bf, L"#AABBCC") == 0);

		TSSkin reloaded;
		CHECK(reloaded.LoadSkin(dir.c_str()));
		CHECK(reloaded.GetColor(CSEC_TrackList, CVAL_SelBk) == RGB(0xAA, 0xBB, 0xCC));
		CHECK(wcscmp(reloaded.GetSkinInfo(ISkin::SI_Name), L"Partial Test (Partial)") == 0);

		//Relative font size: clamped to the skin's limits, not applied to the mini player.
		skin.SetRelativeFontSize(2);
		CHECK(skin.GetRelativeFontSize() == 2);
		CHECK(FontOf(skin, CSEC_Global, FSTY_Normal).lfHeight == 22);
		LOGFONT mini = FontOf(skin, CSEC_MiniPlayer, FSTY_Normal);
		skin.SetRelativeFontSize(0);
		CHECK(FontOf(skin, CSEC_MiniPlayer, FSTY_Normal).lfHeight == mini.lfHeight);
		skin.SetRelativeFontSize(50);
		CHECK(skin.GetRelativeFontSize() == 8);
		skin.SetRelativeFontSize(-50);
		CHECK(skin.GetRelativeFontSize() == -6);

		//Completing a skin keeps its stored name (it was saved as "Name (folder)")
		//and non-ASCII text (it was rewritten as ANSI).
		GetPrivateProfileStringW(L"SkinDetails", L"Name", L"", bf, 100, ini.c_str());
		CHECK(wcscmp(bf, L"Partial Test") == 0);
		std::wstring greekDir = MakeSkin(root, L"Greek", L"[SkinDetails]\r\nName=\x039C\x03C0\x03BB\x03B5\r\n");
		TSSkin greek;
		CHECK(greek.LoadSkin(greekDir.c_str()));
		TSSkin greekReloaded;
		CHECK(greekReloaded.LoadSkin(greekDir.c_str()));
		CHECK(wcscmp(greekReloaded.GetSkinInfo(ISkin::SI_Name), L"\x039C\x03C0\x03BB\x03B5 (Greek)") == 0);
		std::wstring sameDir = MakeSkin(root, L"Same", L"[SkinDetails]\r\nName=Same\r\n");
		TSSkin same;
		CHECK(same.LoadSkin(sameDir.c_str()));
		CHECK(wcscmp(same.GetSkinInfo(ISkin::SI_Name), L"Same") == 0);
		CHECK(wcscmp(same.GetSkinInfo(ISkin::SI_DirName), L"Same") == 0);

		//Switching skins doesn't keep the previous skin's details.
		CHECK(wcscmp(reloaded.GetSkinInfo(ISkin::SI_Author), L"Tester") == 0);
		CHECK(reloaded.LoadSkin(sameDir.c_str()));
		CHECK(reloaded.GetSkinInfo(ISkin::SI_Author)[0] == 0);

		TSSkin missing;
		CHECK(!missing.LoadSkin((root + L"DoesNotExist").c_str()));
		CHECK(missing.GetSkinPath()[0] == 0);
	}

	void TestSkinManager()
	{
		std::wstring root = Dir(TempPath(L"skinsroot\\"));
		MakeSkin(root, L"Default", L"[SkinDetails]\r\nName=Default\r\n");
		std::wstring blue = MakeSkin(root, L"Blue", L"[SkinDetails]\r\nName=Blue Sky\r\n[StatusBar]\r\nBk=#0000FF\r\n");
		Dir(root + L"NotASkin");
		std::wstring empty = Dir(TempPath(L"skinsempty\\"));

		SkinManager sm;
		CHECK(!sm.SetSkinsFolder((root + L"DoesNotExist").c_str()));
		std::wstring noSlash = root.substr(0, root.size() - 1);
		CHECK(sm.SetSkinsFolder(noSlash.c_str()));
		CHECK(sm.GetSkinsFolder() == root);

		sm.RefreshSkinsFolder();
		CHECK(sm.GetSkinsCount() == 2);
		BOOL bBlue = FALSE, bDefault = FALSE;
		for (UINT i = 0; i < sm.GetSkinsCount(); i++)
		{
			LPCTSTR name = sm.GetSkinInfo(i, ISkin::SI_Name);
			if (wcscmp(name, L"Blue Sky (Blue)") == 0 && EndsWith(sm.GetSkinPath(i), L"\\Blue"))
				bBlue = TRUE;
			if (wcscmp(name, L"Default") == 0 && EndsWith(sm.GetSkinPath(i), L"\\Default"))
				bDefault = TRUE;
		}
		CHECK(bBlue && bDefault);

		//Startup loading
		CHECK(sm.Init(root.c_str(), L"Blue", DEFAULT_QUALITY, 3));
		CHECK(EndsWith(sm.GetSkin().GetSkinPath(), L"\\Blue"));
		CHECK(sm.GetSkin().GetColor(CSEC_StatusBar, CVAL_Bk) == RGB(0, 0, 255));
		CHECK(sm.GetSkin().GetRelativeFontSize() == 3);
		CHECK(sm.GetSkin().GetFontQuality() == DEFAULT_QUALITY);
		CHECK(sm.Init(root.c_str(), blue.c_str(), CLEARTYPE_QUALITY, 0));	//full path
		CHECK(EndsWith(sm.GetSkin().GetSkinPath(), L"\\Blue"));
		CHECK(sm.GetSkin().GetFontQuality() == CLEARTYPE_QUALITY);
		CHECK(sm.Init(root.c_str(), L"Missing", CLEARTYPE_QUALITY, 0));	//falls back
		CHECK(EndsWith(sm.GetSkin().GetSkinPath(), L"\\Default"));
		CHECK(sm.Init(root.c_str(), L"", CLEARTYPE_QUALITY, 0));
		CHECK(EndsWith(sm.GetSkin().GetSkinPath(), L"\\Default"));
		CHECK(!sm.Init(empty.c_str(), L"Blue", CLEARTYPE_QUALITY, 0));	//nothing to load
		CHECK(sm.GetSkin().GetSkinPath()[0] == 0);
		CHECK(sm.GetSkin().GetIntParameter(CSEC_Toolbar, IPARAM_Height) == 26);	//built-in defaults

		//Skinnables
		CHECK(sm.Init(root.c_str(), L"Default", CLEARTYPE_QUALITY, 0));
		CountingSkinnable a, c;
		RemovingSkinnable b(sm, c);
		sm.RegisterSkinnable(a);
		sm.RegisterSkinnable(b);
		sm.RegisterSkinnable(c);
		sm.UpdateSkinnables();	//b unregisters itself and c while the list is walked
		CHECK(a.m_count == 1 && b.m_count == 1 && c.m_count <= 1);
		INT cCount = c.m_count;
		sm.UpdateSkinnables();
		CHECK(a.m_count == 2 && b.m_count == 1 && c.m_count == cCount);

		sm.IncreaseRelativeFontSize();
		CHECK(sm.GetSkin().GetRelativeFontSize() == 1 && a.m_count == 3);
		for (int i = 0; i < 20; i++)
			sm.IncreaseRelativeFontSize();
		CHECK(sm.GetSkin().GetRelativeFontSize() == 8);
		for (int i = 0; i < 40; i++)
			sm.DecreaseRelativeFontSize();
		CHECK(sm.GetSkin().GetRelativeFontSize() == -6);
		INT before = a.m_count;
		sm.ResetRelativeFontSize();
		CHECK(sm.GetSkin().GetRelativeFontSize() == 0 && a.m_count == before + 1);
		sm.ResetRelativeFontSize();		//no change, no update
		CHECK(a.m_count == before + 1);
		sm.UnRegisterSkinnable(a);
	}

	//Records registrations.
	class FakeSkinManager: public ISkinManager
	{
	public:
		virtual void RegisterSkinnable(ISkinnable& s)		{m_registered.insert(&s); m_registers++;}
		virtual void UnRegisterSkinnable(ISkinnable& s)		{m_registered.erase(&s); m_unregisters++;}
		virtual void UpdateSkinnables(ISkin&)				{}
		FakeSkinManager(): m_registers(0), m_unregisters(0)	{}
		std::set<ISkinnable*> m_registered;
		INT m_registers;
		INT m_unregisters;
	};

	class SkinnableWidget: public TSSkinnable
	{
	public:
		explicit SkinnableWidget(ISkinManager& sm): TSSkinnable(sm), m_count(0)	{}
		virtual void ApplySkin(ISkin&)	{m_count++;}
		INT m_count;
	};

	void TestSkinnableMixin()
	{
		FakeSkinManager sm, other;
		{
			SkinnableWidget w(sm);
			CHECK(sm.m_registered.count(&w) == 1);
			{
				SkinnableWidget copy(w);		//a copy registers too (it didn't, but unregistered)
				CHECK(sm.m_registered.count(&copy) == 1 && sm.m_registered.size() == 2);
				SkinnableWidget elsewhere(other);
				elsewhere = w;					//assignment keeps each one where it is
				CHECK(other.m_registered.count(&elsewhere) == 1 && sm.m_registered.size() == 2);
			}
			CHECK(sm.m_registered.size() == 1 && other.m_registered.empty());
		}
		CHECK(sm.m_registered.empty() && sm.m_registers == 2 && sm.m_unregisters == 2);

		//With the real manager: skinned on update, gone after destruction.
		SkinManager real;
		TSSkin& skin = real.GetSkin();
		{
			SkinnableWidget w(real);
			real.UpdateSkinnables(skin);
			CHECK(w.m_count == 1);
		}
		real.UpdateSkinnables(skin);	//must not touch the destroyed widget
	}

	//Every skin shipped in Skins\ (copied next to UnitTests.exe by the build) loads.
	void TestShippedSkins()
	{
		WCHAR exeDir[MAX_PATH];
		GetModuleFileNameW(NULL, exeDir, MAX_PATH);
		*(wcsrchr(exeDir, L'\\') + 1) = 0;
		std::wstring shipped = std::wstring(exeDir) + L"Skins\\";
		//Work on copies: loading completes partial skins on disk.
		std::wstring root = Dir(TempPath(L"shippedskins\\"));
		INT copied = 0;
		WIN32_FIND_DATAW fd;
		HANDLE h = FindFirstFileW((shipped + L"*").c_str(), &fd);
		if (h != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == L'.')
					continue;
				std::wstring src = shipped + fd.cFileName + L"\\colors.ini";
				std::wstring dst = Dir(root + fd.cFileName) + L"\\colors.ini";
				if (CopyFileW(src.c_str(), dst.c_str(), FALSE))
				{
					SetFileAttributesW(dst.c_str(), FILE_ATTRIBUTE_NORMAL);
					copied++;
				}
			} while (FindNextFileW(h, &fd));
			FindClose(h);
		}
		CHECK(copied > 0);
		if (copied == 0)
		{
			wprintf(L"  (no skins found in %s)\n", shipped.c_str());
			return;
		}

		SkinManager sm;
		CHECK(sm.SetSkinsFolder(root.c_str()));
		sm.RefreshSkinsFolder();
		CHECK((INT)sm.GetSkinsCount() == copied);
		for (UINT i = 0; i < sm.GetSkinsCount(); i++)
		{
			LPCTSTR path = sm.GetSkinPath(i);
			BOOL bLoaded = sm.LoadSkin(path);
			CHECK(bLoaded);
			if (!bLoaded)
			{
				wprintf(L"    skin not loaded: %s\n", path);
				continue;
			}
			CHECK(sm.GetSkin().GetSkinInfo(ISkin::SI_Name)[0] != 0);
			CHECK(sm.GetSkinInfo(i, ISkin::SI_Name)[0] != 0);
			LOGFONT lf = FontOf(sm.GetSkin(), CSEC_Global, FSTY_Normal);
			CHECK(lf.lfFaceName[0] != 0 && lf.lfHeight != 0);
			CHECK(sm.GetSkin().GetIntParameter(CSEC_Toolbar, IPARAM_Height) > 0);
		}
		CHECK(sm.Init(root.c_str(), L"Default", CLEARTYPE_QUALITY, 0));
		CHECK(EndsWith(sm.GetSkin().GetSkinPath(), L"\\Default"));
	}
}

void RunSkinTests()
{
	wprintf(L"Skin tests\n");
	TestSkinFolderName();
	TestPartialSkin();
	TestSkinManager();
	TestSkinnableMixin();
	TestShippedSkins();
}
