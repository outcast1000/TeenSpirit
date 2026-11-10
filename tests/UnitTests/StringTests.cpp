//	String handling that was moved off the unsafe CRT calls: in-place replace,
//	rename patterns, hex encoding and the picture cache's file names (which must
//	not change, or pictures cached by older versions would be lost).
#include "stdafx.h"
#include "TestFramework.h"
#include "cStringUtils.h"
#include "FileNameParser.h"
#include "FileNameTagger.h"
#include "URLEncDecHelper.h"
#include "InformationProviders/CachedImageInfoProvider/CachedImageInfoProvider.h"
#include <shlwapi.h>

namespace
{
	void TestReplaceInl()
	{
		//Growing: the result must leave room for the terminator
		WCHAR bf[6] = L"a$ab";
		CHECK(_tcsReplaceInl(bf, 6, L"$a", L"xyz") == 1);
		CHECK(wcscmp(bf, L"axyzb") == 0);
		WCHAR tooSmall[5] = L"a$ab";
		CHECK(_tcsReplaceInl(tooSmall, 5, L"$a", L"xyz") == -1);
		CHECK(tooSmall[4] == 0);

		//Several replaces up to the exact capacity
		WCHAR many[7] = L"$a$a$a";
		CHECK(_tcsReplaceInl(many, 7, L"$a", L"xx") == 3);
		CHECK(wcscmp(many, L"xxxxxx") == 0);

		//Capacity 0: same length or shorter only
		WCHAR same[] = L"a_b_c";
		CHECK(_tcsReplaceInl(same, 0, L"_", L" ") == 2);
		CHECK(wcscmp(same, L"a b c") == 0);
		WCHAR shrink[] = L"a--b--c";
		CHECK(_tcsReplaceInl(shrink, 0, L"--", L"") == 2);
		CHECK(wcscmp(shrink, L"abc") == 0);
		WCHAR none[] = L"abc";
		CHECK(_tcsReplaceInl(none, 0, L"x", L"yy") == 0);
	}

	void TestStrCopyN()
	{
		WCHAR bf[5];
		CHECK(StrCopyN(bf, L"abcdef", 3) == 3 && wcscmp(bf, L"abc") == 0);		//substring
		CHECK(StrCopyN(bf, L"ab", 4) == 2 && wcscmp(bf, L"ab") == 0);			//source shorter than count
		CHECK(StrCopyN(bf, L"abcdef", 100) == 4 && wcscmp(bf, L"abcd") == 0);	//cut to the buffer
		CHECK(StrCopyN(bf, L"abcdef", 0) == 0 && bf[0] == 0);
		CHECK(StrCopyN(bf, (LPCWSTR)NULL, 3) == 0 && bf[0] == 0);
		WCHAR* p = bf;
		CHECK(StrCopyN(p, 3, L"abcdef", 5) == 2 && wcscmp(bf, L"ab") == 0);		//pointer + size
		bf[0] = L'x';
		CHECK(StrCopyN(p, 0, L"abc", 3) == 0 && bf[0] == L'x');					//no room: untouched
		char a[4];
		CHECK(StrCopyN(a, "c:\\music", 3) == 3 && strcmp(a, "c:\\") == 0);
	}

	void TestTranslatePattern()
	{
		WCHAR bf[MAX_PATH];
		CHECK(FileNameParser::TranslatePattern(bf, MAX_PATH, L"$n - $a - $t", L"Artist", L"Album", L"03", L"Title", L"1999", NULL));
		CHECK(wcscmp(bf, L"03 - Artist - Title") == 0);
		CHECK(FileNameTagger::TranslatePattern(bf, MAX_PATH, L"$n $t ($y)", L"Artist", L"Album", 7, L"Title", 1999, NULL));
		CHECK(wcscmp(bf, L"07 Title (1999)") == 0);

		//The result doesn't fit: fail instead of leaving a placeholder in the name
		WCHAR tiny[16];
		CHECK(!FileNameParser::TranslatePattern(tiny, 16, L"$a - $t", L"A long artist name", L"", L"", L"Title", L"", NULL));
		CHECK(!FileNameParser::TranslatePattern(bf, MAX_PATH, L"", L"A", L"", L"", L"T", L"", NULL));
	}

	void TestCustomWrite()
	{
		std::wstring src = TempPath(L"rename-me.mp3");
		std::wstring dst = TempPath(L"Artist - Title.mp3");
		DeleteFileW(dst.c_str());
		HANDLE h = CreateFileW(src.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		CHECK(h != INVALID_HANDLE_VALUE);
		CloseHandle(h);

		FileNameTagger tagger;
		WCHAR newPath[MAX_PATH];
		CHECK(tagger.CustomWrite(newPath, src.c_str(), L"$a - $t", L"Artist", L"Album", 1, L"Title", 2000) != 0);
		CHECK(dst == newPath);
		CHECK(PathFileExistsW(dst.c_str()));
		CHECK(!PathFileExistsW(src.c_str()));

		//A new path longer than MAX_PATH: the file is not renamed
		std::wstring longTitle(MAX_PATH, L'x');
		CHECK(tagger.CustomWrite(newPath, dst.c_str(), L"$t", L"Artist", L"Album", 1, longTitle.c_str(), 2000) == 0);
		CHECK(PathFileExistsW(dst.c_str()));
		//folder + title + ".mp3" exactly MAX_PATH characters: no room for the terminator
		std::wstring nearTitle(MAX_PATH - (dst.size() - wcslen(L"Artist - Title.mp3")) - 4, L'y');
		CHECK(tagger.CustomWrite(newPath, dst.c_str(), L"$t", L"Artist", L"Album", 1, nearTitle.c_str(), 2000) == 0);
		CHECK(PathFileExistsW(dst.c_str()));
		DeleteFileW(dst.c_str());
	}

	void TestEncodeHex()
	{
		const BYTE data[] = {0x00, 0x0F, 0xAB, 0xFF};
		LPSTR hex = URLEncDecHelper::EncodeHex(data, 4);
		CHECK(strcmp(hex, "000FABFF") == 0);
		delete[] hex;
		hex = URLEncDecHelper::EncodeHex(data, 0);
		CHECK(hex[0] == 0);
		delete[] hex;
	}

	void TestPictureCacheNames()
	{
		std::wstring storage = TempPath(L"picture-cache\\");
		CreateDirectoryW(storage.c_str(), NULL);
		std::wstring src = TempPath(L"picture.jpg");
		HANDLE h = CreateFileW(src.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		DWORD written = 0;
		WriteFile(h, "jpeg", 4, &written, NULL);
		CloseHandle(h);

		//Names are AR_/AL_ + MD5 of the lowercase UTF-16 "artist" / "artist-album", a leading "The " dropped
		std::wstring artistFile = storage + L"AR_A18EAFB708F4C534F4F8232061EDC7A900.jpg";
		std::wstring albumFile = storage + L"AL_7D9C6F4CA4611ACFE9DEAA1C444B6C5C00.jpg";
		DeleteFileW(artistFile.c_str());
		DeleteFileW(albumFile.c_str());

		CachedImageInfoProvider cache;
		cache.SetStoragePath(storage.c_str());
		IInfoProvider::Request req(IInfoProvider::SRV_ArtistImage);
		req.artist = L"The Beatles";
		CHECK(cache.OpenRequest(req));
		IInfoProvider::Result res;
		res.service = IInfoProvider::SRV_ArtistImage;
		res.main = src.c_str();
		CHECK(cache.AddResult(res));
		CHECK(PathFileExistsW(artistFile.c_str()));

		IInfoProvider::Request albumReq(IInfoProvider::SRV_AlbumImage);
		albumReq.artist = L"The Beatles";
		albumReq.album = L"Abbey Road";
		CHECK(cache.OpenRequest(albumReq));
		res.service = IInfoProvider::SRV_AlbumImage;
		CHECK(cache.AddResult(res));
		CHECK(PathFileExistsW(albumFile.c_str()));

		//...and they are found again
		CHECK(cache.OpenRequest(req));
		IInfoProvider::Result found;
		CHECK(cache.GetNextResult(found));
		CHECK(found.main != NULL && artistFile == found.main);

		DeleteFileW(artistFile.c_str());
		DeleteFileW(albumFile.c_str());
		DeleteFileW(src.c_str());
	}
}

void RunStringTests()
{
	wprintf(L"String tests\n");
	TestReplaceInl();
	TestStrCopyN();
	TestTranslatePattern();
	TestCustomWrite();
	TestEncodeHex();
	TestPictureCacheNames();
}
