//	Tag reading and writing (Taggers\TagLibTagger.cpp) on small files generated here:
//	MP3 (ID3v2), WAV (ID3v2 chunk) and FLAC (Vorbis comments). The MP3 frames are also
//	checked directly, so other players see the same tags as before.
#include "stdafx.h"
#include "TestFramework.h"
#include "TagLibTagger.h"
#include <taglib.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <popularimeterframe.h>
#include <unsynchronizedlyricsframe.h>

namespace
{
	void WriteBytes(const std::wstring& path, const std::string& data)
	{
		HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD written = 0;
		if (h != INVALID_HANDLE_VALUE)
		{
			WriteFile(h, data.data(), (DWORD)data.size(), &written, NULL);
			CloseHandle(h);
		}
	}
	void PutLE(std::string& s, UINT v, INT bytes)
	{
		for (INT i = 0; i < bytes; i++)
			s += (char)((v >> (8 * i)) & 0xFF);
	}

	//200 frames of MPEG-1 Layer III, 128 kbps, 44.1 kHz (5.2 seconds of silence).
	std::string MakeMp3()
	{
		std::string frame(417, '\0');
		frame[0] = (char)0xFF; frame[1] = (char)0xFB; frame[2] = (char)0x90; frame[3] = (char)0x00;
		std::string s;
		for (INT i = 0; i < 200; i++)
			s += frame;
		return s;
	}

	//PCM WAV, 44.1 kHz, 16 bit stereo, 1 second.
	std::string MakeWav()
	{
		const UINT dataSize = 44100 * 4;
		std::string s("RIFF");
		PutLE(s, 4 + 8 + 16 + 8 + dataSize, 4);
		s += "WAVEfmt ";
		PutLE(s, 16, 4);
		PutLE(s, 1, 2);			//PCM
		PutLE(s, 2, 2);			//channels
		PutLE(s, 44100, 4);
		PutLE(s, 44100 * 4, 4);	//bytes per second
		PutLE(s, 4, 2);			//block align
		PutLE(s, 16, 2);		//bits per sample
		s += "data";
		PutLE(s, dataSize, 4);
		s += std::string(dataSize, '\0');
		return s;
	}

	//FLAC stream with only STREAMINFO: 44.1 kHz, 16 bit stereo, 2 seconds.
	std::string MakeFlac()
	{
		std::string s("fLaC");
		s += (char)0x80; s += (char)0; s += (char)0; s += (char)34;	//last block, STREAMINFO, 34 bytes
		s += (char)0x10; s += (char)0x00; s += (char)0x10; s += (char)0x00;	//block size 4096..4096
		s += std::string(6, '\0');											//frame sizes unknown
		const unsigned long long samples = 88200;
		//20 bits sample rate, 3 bits channels-1, 5 bits bps-1, 36 bits total samples
		unsigned long long v = ((unsigned long long)44100 << 44) | ((unsigned long long)1 << 41) |
			((unsigned long long)15 << 36) | samples;
		for (INT i = 7; i >= 0; i--)
			s += (char)((v >> (8 * i)) & 0xFF);
		s += std::string(16, '\0');											//MD5
		return s;
	}

	TagInfo FullTags()
	{
		TagInfo ti;
		ti.Title = L"\x039F\x03B9 \x03C0\x03B1\x03BB\x03B9\x03AD\x03C2";		//Greek
		ti.Artist = L"\x03A0\x03C5\x03BE \x039B\x03B1\x03BE";
		ti.Album = L"Album \x00C5ngstr\x00F6m \x65E5\x672C";					//Latin-1 + CJK
		ti.Comment = L"A comment";
		ti.Genre = L"Rock";
		ti.Lyrics = L"First line\r\nSecond line \x03B1\x03B2\x03B3";
		ti.AlbumArtist = L"Various \x0391\x03C1\x03C4\x03B9\x03C3\x03C4\x03AD\x03C2";
		ti.Composer = L"Composer \x039C\x03AF\x03BA\x03B7\x03C2";
		ti.Year = 1999;
		ti.TrackNo = 7;
		ti.Rating = 200;
		ti.validFields = TagInfo_Title | TagInfo_Artist | TagInfo_Album | TagInfo_Comment | TagInfo_Genre |
			TagInfo_Lyrics | TagInfo_AlbumArtist | TagInfo_Composer | TagInfo_Year | TagInfo_TrackNo | TagInfo_Rating;
		return ti;
	}

	//Text read back through the ANSI code page workaround (see TagLibTagger.cpp).
	std::wstring AnsiDecoded(const std::wstring& s)
	{
		std::string bytes;
		for (size_t i = 0; i < s.size(); i++)
			bytes += (char)s[i];
		WCHAR bf[200];
		INT n = MultiByteToWideChar(CP_ACP, 0, bytes.data(), (INT)bytes.size(), bf, 200);
		return std::wstring(bf, n);
	}

	BOOL Same(const std::wstring& a, const std::wstring& b)
	{
		return wcscmp(a.c_str(), b.c_str()) == 0;	//c_str: tolerate a trailing NUL inside the string
	}

	void CheckRoundTrip(LPCWSTR label, const std::wstring& path, BOOL bExtendedFields, BOOL bRating)
	{
		wprintf(L"  %s\n", label);
		TagLibTagger tagger;
		TagInfo w = FullTags();
		UINT written = tagger.Write(path.c_str(), w);
		CHECK(written != TagInfo_None);
		CHECK((written & (TagInfo_Title | TagInfo_Artist | TagInfo_Album | TagInfo_Year | TagInfo_TrackNo)) ==
			(TagInfo_Title | TagInfo_Artist | TagInfo_Album | TagInfo_Year | TagInfo_TrackNo));

		TagInfo r;
		UINT read = tagger.Read(path.c_str(), r);
		CHECK((read & TagInfo_Title) && Same(r.Title, w.Title));
		CHECK((read & TagInfo_Artist) && Same(r.Artist, w.Artist));
		CHECK((read & TagInfo_Album) && Same(r.Album, w.Album));
		CHECK((read & TagInfo_Comment) && Same(r.Comment, w.Comment));
		CHECK((read & TagInfo_Genre) && Same(r.Genre, w.Genre));
		CHECK((read & TagInfo_Year) && r.Year == 1999);
		CHECK((read & TagInfo_TrackNo) && r.TrackNo == 7);
		if (bExtendedFields)
		{
			CHECK((read & TagInfo_Lyrics) && Same(r.Lyrics, w.Lyrics));
			CHECK((read & TagInfo_AlbumArtist) && Same(r.AlbumArtist, w.AlbumArtist));
			CHECK((read & TagInfo_Composer) && Same(r.Composer, w.Composer));
		}
		if (bRating)
			CHECK((read & TagInfo_Rating) && r.Rating == 200);
		else
			CHECK((read & TagInfo_Rating) == 0);

		//Updating one field keeps the others; empty values are not written.
		TagInfo w2;
		w2.Title = L"New title";
		w2.validFields = TagInfo_Title;
		CHECK(tagger.Write(path.c_str(), w2) & TagInfo_Title);
		TagInfo r2;
		tagger.Read(path.c_str(), r2);
		CHECK(Same(r2.Title, L"New title"));
		CHECK(Same(r2.Artist, w.Artist));
		CHECK(r2.TrackNo == 7);
		if (bExtendedFields)
			CHECK(Same(r2.AlbumArtist, w.AlbumArtist) && Same(r2.Lyrics, w.Lyrics));
		if (bRating)
			CHECK(r2.Rating == 200);

		//Text with only characters below 256 is decoded with the ANSI code page.
		TagInfo w3;
		w3.Title = L"Caf\x00E9 \x00C0 la carte";
		w3.validFields = TagInfo_Title;
		tagger.Write(path.c_str(), w3);
		TagInfo r3;
		tagger.Read(path.c_str(), r3);
		CHECK(Same(r3.Title, AnsiDecoded(w3.Title)));
	}

	void TestMp3()
	{
		std::wstring path = TempPath(L"tagger.mp3");
		WriteBytes(path, MakeMp3());
		TagLibTagger tagger;
		TagInfo r;
		UINT read = tagger.Read(path.c_str(), r);
		CHECK((read & TagInfo_Length) && r.Length == 5);
		CHECK((read & TagInfo_Bitrate) && r.Bitrate == 128);
		CHECK((read & TagInfo_Title) == 0);

		CheckRoundTrip(L"mp3", path, TRUE, TRUE);

		//The frames other programs read
		TagInfo w = FullTags();
		tagger.Write(path.c_str(), w);
		{
			TagLib::MPEG::File f(path.c_str(), false);
			TagLib::ID3v2::Tag* id3 = f.ID3v2Tag();
			CHECK(id3 != NULL);
			if (id3 != NULL)
			{
				const TagLib::ID3v2::FrameList tpe2 = id3->frameList("TPE2");
				const TagLib::ID3v2::FrameList tcom = id3->frameList("TCOM");
				const TagLib::ID3v2::FrameList uslt = id3->frameList("USLT");
				const TagLib::ID3v2::FrameList popm = id3->frameList("POPM");
				CHECK(!tpe2.isEmpty() && Same(tpe2.front()->toString().toWString(), w.AlbumArtist));
				CHECK(!tcom.isEmpty() && Same(tcom.front()->toString().toWString(), w.Composer));
				CHECK(uslt.size() == 1);
				if (!uslt.isEmpty())
				{
					TagLib::ID3v2::UnsynchronizedLyricsFrame* f =
						dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(uslt.front());
					CHECK(f != NULL && Same(f->text().toWString(), w.Lyrics) && f->description().isEmpty());
				}
				CHECK(popm.size() == 1);
				if (!popm.isEmpty())
				{
					TagLib::ID3v2::PopularimeterFrame* f = dynamic_cast<TagLib::ID3v2::PopularimeterFrame*>(popm.front());
					CHECK(f != NULL && f->rating() == 200);
				}
			}
		}
		//The audio is untouched
		TagInfo r2;
		tagger.Read(path.c_str(), r2);
		CHECK(r2.Length == 5 && r2.Bitrate == 128);

		//Rating 0 removes the POPM frame
		TagInfo w0;
		w0.Rating = 0;
		w0.validFields = TagInfo_Rating;
		tagger.Write(path.c_str(), w0);
		TagInfo r0;
		CHECK((tagger.Read(path.c_str(), r0) & TagInfo_Rating) == 0);
	}

	void TestWav()
	{
		std::wstring path = TempPath(L"tagger.wav");
		WriteBytes(path, MakeWav());
		TagLibTagger tagger;
		TagInfo r;
		UINT read = tagger.Read(path.c_str(), r);
		CHECK((read & TagInfo_Length) && r.Length == 1);
		CHECK((read & TagInfo_Bitrate) && r.Bitrate == 1411);
		CheckRoundTrip(L"wav", path, TRUE, TRUE);
	}

	void TestFlac()
	{
		std::wstring path = TempPath(L"tagger.flac");
		WriteBytes(path, MakeFlac());
		TagLibTagger tagger;
		TagInfo r;
		UINT read = tagger.Read(path.c_str(), r);
		CHECK((read & TagInfo_Length) && r.Length == 2);
		//TagLib 1.6 only had album artist, composer and lyrics for ID3v2 tags.
		CheckRoundTrip(L"flac", path, TAGLIB_MAJOR_VERSION >= 2, FALSE);
	}

	void TestMissingFile()
	{
		TagLibTagger tagger;
		TagInfo r;
		CHECK(tagger.Read(TempPath(L"does-not-exist.mp3").c_str(), r) == TagInfo_None);
		std::wstring path = TempPath(L"not-audio.mp3");
		WriteBytes(path, std::string(5000, 'x'));
		TagInfo w = FullTags();
		tagger.Write(path.c_str(), w);	//must not crash
	}
}

void RunTaggerTests()
{
	wprintf(L"TestTagger (TagLib %d.%d.%d)\n", TAGLIB_MAJOR_VERSION, TAGLIB_MINOR_VERSION, TAGLIB_PATCH_VERSION);
	TestMp3();
	TestWav();
	TestFlac();
	TestMissingFile();
}
