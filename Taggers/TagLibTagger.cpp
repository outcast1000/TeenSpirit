//	/*
// 	*
// 	* Copyright (C) 2003-2010 Alexandros Economou
//	*
//	* This file is part of Jaangle (http://www.jaangle.com)
// 	*
// 	* This Program is free software; you can redistribute it and/or modify
// 	* it under the terms of the GNU General Public License as published by
// 	* the Free Software Foundation; either version 2, or (at your option)
// 	* any later version.
// 	*
// 	* This Program is distributed in the hope that it will be useful,
// 	* but WITHOUT ANY WARRANTY; without even the implied warranty of
// 	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// 	* GNU General Public License for more details.
// 	*
// 	* You should have received a copy of the GNU General Public License
// 	* along with GNU Make; see the file COPYING. If not, write to
// 	* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
// 	* http://www.gnu.org/copyleft/gpl.html
// 	*
//	*/ 
#include "stdafx.h"
#include "TagLibTagger.h"
#include <fileref.h>
#include <tag.h>
#include <tfile.h>
#include <tpropertymap.h>
#include <mpegfile.h>
#include <wavfile.h>
#include <aifffile.h>
#include <trueaudiofile.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <popularimeterframe.h>
#include <textidentificationframe.h>
#include <unsynchronizedlyricsframe.h>

//	Album artist, composer, lyrics and rating are not part of TagLib::Tag.
//	ID3v2 (MP3, WAV, AIFF, TTA): TPE2, TCOM, USLT (the one without a description
//	first) and POPM, as Jaangle has always written them. Other formats: the
//	ALBUMARTIST, COMPOSER and LYRICS properties; no rating.

namespace
{
	//Tags written by old Windows programs often hold text in the local ANSI code page
	//declared as Latin-1. When every character is below 256, decode the bytes with the
	//ANSI code page instead (identity for ASCII).
	void TagLibString2String(std::tstring& dest, const TagLib::String& src)
	{
		dest = src.toWString();
		BOOL bNonAscii = FALSE;
		for (size_t i = 0; i < dest.size(); i++)
		{
			if (dest[i] >= 256)
				return;
			if (dest[i] >= 128)
				bNonAscii = TRUE;
		}
		if (!bNonAscii)
			return;
		std::string bytes(dest.size(), '\0');
		for (size_t i = 0; i < dest.size(); i++)
			bytes[i] = (char)dest[i];
		INT len = MultiByteToWideChar(CP_ACP, 0, bytes.data(), (INT)bytes.size(), NULL, 0);
		std::wstring decoded(len, L'\0');
		MultiByteToWideChar(CP_ACP, 0, bytes.data(), (INT)bytes.size(), &decoded[0], len);
		dest = decoded;
	}
	void GetTag(std::tstring& dest, const TagLib::String& src, INT tagFlag, INT& appendFlag)
	{
		if (!src.isEmpty())
		{
			appendFlag |= tagFlag;
			TagLibString2String(dest, src);
		}
	}

	TagLib::ID3v2::Tag* GetID3v2Tag(TagLib::File* file, bool create)
	{
		if (TagLib::MPEG::File* f = dynamic_cast<TagLib::MPEG::File*>(file))
			return f->ID3v2Tag(create);
		if (TagLib::RIFF::WAV::File* f = dynamic_cast<TagLib::RIFF::WAV::File*>(file))
			return f->ID3v2Tag();
		if (TagLib::RIFF::AIFF::File* f = dynamic_cast<TagLib::RIFF::AIFF::File*>(file))
			return f->tag();
		if (TagLib::TrueAudio::File* f = dynamic_cast<TagLib::TrueAudio::File*>(file))
			return f->ID3v2Tag(create);
		return NULL;
	}

	//=== ID3v2
	TagLib::String TextFrame(const TagLib::ID3v2::Tag& tag, const char* id)
	{
		const TagLib::ID3v2::FrameList frames = tag.frameList(id);
		return frames.isEmpty() ? TagLib::String() : frames.front()->toString();
	}
	void SetTextFrame(TagLib::ID3v2::Tag& tag, const char* id, const TagLib::String& value)
	{
		tag.removeFrames(id);
		if (!value.isEmpty())
		{
			TagLib::ID3v2::TextIdentificationFrame* f = new TagLib::ID3v2::TextIdentificationFrame(id, TagLib::String::Latin1);
			f->setText(value);	//Latin-1 is upgraded to Unicode on save when the text needs it
			tag.addFrame(f);
		}
	}
	TagLib::String Lyrics(const TagLib::ID3v2::Tag& tag)
	{
		const TagLib::ID3v2::FrameList frames = tag.frameList("USLT");
		if (frames.isEmpty())
			return TagLib::String();
		for (TagLib::ID3v2::FrameList::ConstIterator it = frames.begin(); it != frames.end(); ++it)
		{
			TagLib::ID3v2::UnsynchronizedLyricsFrame* f = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(*it);
			if (f != NULL && f->description().isEmpty())
				return f->text();
		}
		return frames.front()->toString();
	}
	void SetLyrics(TagLib::ID3v2::Tag& tag, const TagLib::String& value)
	{
		if (value.isEmpty())
		{
			tag.removeFrames("USLT");
			return;
		}
		const TagLib::ID3v2::FrameList frames = tag.frameList("USLT");
		TagLib::ID3v2::UnsynchronizedLyricsFrame* f = frames.isEmpty() ? NULL :
			dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(frames.front());
		if (f == NULL)
		{
			f = new TagLib::ID3v2::UnsynchronizedLyricsFrame(TagLib::String::Latin1);
			tag.addFrame(f);
		}
		f->setText(value);
	}
	UINT Rating(const TagLib::ID3v2::Tag& tag)
	{
		const TagLib::ID3v2::FrameList frames = tag.frameList("POPM");
		TagLib::ID3v2::PopularimeterFrame* f = frames.isEmpty() ? NULL :
			dynamic_cast<TagLib::ID3v2::PopularimeterFrame*>(frames.front());
		return f != NULL ? (UINT)f->rating() : 0;
	}
	void SetRating(TagLib::ID3v2::Tag& tag, INT rating)
	{
		if (rating <= 0)
		{
			tag.removeFrames("POPM");
			return;
		}
		if (rating > 255)
			rating = 255;
		const TagLib::ID3v2::FrameList frames = tag.frameList("POPM");
		TagLib::ID3v2::PopularimeterFrame* f = frames.isEmpty() ? NULL :
			dynamic_cast<TagLib::ID3v2::PopularimeterFrame*>(frames.front());
		if (f == NULL)
		{
			f = new TagLib::ID3v2::PopularimeterFrame();
			tag.addFrame(f);
		}
		f->setRating(rating);
	}

	//=== Other formats
	TagLib::String Property(const TagLib::PropertyMap& props, const char* key)
	{
		TagLib::PropertyMap::ConstIterator it = props.find(key);
		return (it == props.end() || it->second.isEmpty()) ? TagLib::String() : it->second.front();
	}
	void SetProperty(TagLib::PropertyMap& props, const char* key, const TagLib::String& value)
	{
		if (value.isEmpty())
			props.erase(key);
		else
			props.replace(key, TagLib::StringList(value));
	}
}


UINT TagLibTagger::Read(LPCTSTR fileName, TagInfo& info)
{
	TRACEST(_T("TagLibTagger::Read"));
	INT ret = TagInfo_None;
	ASSERT(fileName != NULL);
	try
	{
		TagLib::FileRef f(fileName, true);
		if (f.isNull())
		{
			TRACE(_T("@1 TagLibTagger::Read. FileRef Failed on: %s\r\n"), fileName);
			return TagInfo_None;
		}
		TagLib::Tag *tag = f.tag();
		if (tag == NULL)
		{
			TRACE(_T("@1 TagLibTagger::Read. Get tag failed on: %s\r\n"), fileName);
			return TagInfo_None;
		}

		GetTag(info.Title, tag->title(), TagInfo_Title, ret);
		GetTag(info.Artist, tag->artist(), TagInfo_Artist, ret);
		GetTag(info.Album, tag->album(), TagInfo_Album, ret);
		GetTag(info.Comment, tag->comment(), TagInfo_Comment, ret);
		GetTag(info.Genre, tag->genre(), TagInfo_Genre, ret);

		//create: an empty tag in memory only (Read never saves)
		TagLib::ID3v2::Tag* id3 = GetID3v2Tag(f.file(), true);
		if (id3 != NULL)
		{
			GetTag(info.AlbumArtist, TextFrame(*id3, "TPE2"), TagInfo_AlbumArtist, ret);
			GetTag(info.Composer, TextFrame(*id3, "TCOM"), TagInfo_Composer, ret);
			GetTag(info.Lyrics, Lyrics(*id3), TagInfo_Lyrics, ret);
			info.Rating = Rating(*id3);
		}
		else
		{
			const TagLib::PropertyMap props = f.file()->properties();
			GetTag(info.AlbumArtist, Property(props, "ALBUMARTIST"), TagInfo_AlbumArtist, ret);
			GetTag(info.Composer, Property(props, "COMPOSER"), TagInfo_Composer, ret);
			GetTag(info.Lyrics, Property(props, "LYRICS"), TagInfo_Lyrics, ret);
		}

		info.Year = tag->year();
		if (info.Year > 0 && info.Year < 2100)
			ret |= TagInfo_Year;
		else
			info.Year = 0;
		info.TrackNo = tag->track();
		if (info.TrackNo > 0 && info.TrackNo < 10000)
			ret|=TagInfo_TrackNo;
		else
			info.TrackNo = 0;
		if (info.Rating > 0 && info.Rating <= 255)
			ret|=TagInfo_Rating;
		else
			info.Rating = 0;

		TagLib::AudioProperties *properties = f.audioProperties();
		if (properties)
		{
			info.Length = properties->lengthInSeconds();
			if (info.Length != 0)
				ret |= TagInfo_Length;
			info.Bitrate = properties->bitrate();
			if (info.Bitrate != 0)
				ret |= TagInfo_Bitrate;
		}
	}
	catch (...)
	{
		TRACE(_T("@1 TagLibTagger::Read Exception***\r\n"));
	}

	if (ret == 0)
		TRACE(_T("@1 TagLibTagger::Read Failed ON: %s\r\n"), fileName);
	return ret;
}


UINT TagLibTagger::Write(LPCTSTR fileName, TagInfo& info)
{
	TRACEST(_T("TagLibTagger::Write"), fileName);
	if (info.validFields == TagInfo_None)
	{
		TRACE(_T("@3 TagLibTagger::Write. No valid fields to tag\r\n"));
		return TagInfo_None;
	}
	try
	{
		TagLib::FileRef f(fileName, false);
		if (f.isNull())
		{
			TRACE(_T("@1 TagLibTagger::Write. FileRef Failed\r\n"));
			return TagInfo_None;
		}
		TagLib::Tag *tag = f.tag();
		if (tag == NULL)
		{
			TRACE(_T("@1 TagLibTagger::Write. Get tag failed\r\n"));
			return TagInfo_None;
		}

		//=== Text is written as Unicode where the format needs it (ID3v1 can't hold it).
		if (info.IsValid(TagInfo_Title))	tag->setTitle(info.Title);
		if (info.IsValid(TagInfo_Album))	tag->setAlbum(info.Album);
		if (info.IsValid(TagInfo_Artist))	tag->setArtist(info.Artist);
		if (info.IsValid(TagInfo_Comment))	tag->setComment(info.Comment);
		if (info.IsValid(TagInfo_Genre))	tag->setGenre(info.Genre);
		if (info.IsValid(TagInfo_Year))		tag->setYear(info.Year);
		if (info.IsValid(TagInfo_TrackNo))	tag->setTrack(info.TrackNo);

		TagLib::ID3v2::Tag* id3 = GetID3v2Tag(f.file(), true);
		if (id3 != NULL)
		{
			if (info.IsValid(TagInfo_Lyrics))		SetLyrics(*id3, info.Lyrics);
			if (info.IsValid(TagInfo_AlbumArtist))	SetTextFrame(*id3, "TPE2", info.AlbumArtist);
			if (info.IsValid(TagInfo_Composer))		SetTextFrame(*id3, "TCOM", info.Composer);
			if (info.IsValid(TagInfo_Rating))		SetRating(*id3, info.Rating);
		}
		else if (info.validFields & (TagInfo_Lyrics | TagInfo_AlbumArtist | TagInfo_Composer))
		{
			TagLib::PropertyMap props = f.file()->properties();
			if (info.IsValid(TagInfo_Lyrics))		SetProperty(props, "LYRICS", info.Lyrics);
			if (info.IsValid(TagInfo_AlbumArtist))	SetProperty(props, "ALBUMARTIST", info.AlbumArtist);
			if (info.IsValid(TagInfo_Composer))		SetProperty(props, "COMPOSER", info.Composer);
			f.file()->setProperties(props);
		}

		if (!f.save())
		{
			TRACE(_T("@1 TagLibTagger::Write. Save tag failed\r\n"));
			return TagInfo_None;
		}
		return info.validFields & (TagInfo_Title|TagInfo_Album|TagInfo_Artist|TagInfo_Year|TagInfo_Comment
			|TagInfo_TrackNo|TagInfo_Genre|TagInfo_Lyrics|TagInfo_Rating);
	}
	catch(...)
	{
		TRACE(_T("@1 TagLibTagger::Write. Exception***\r\n"));
	}
	return TagInfo_None;
}

BOOL TagLibTagger::ClearTags(LPCTSTR fileName)
{
	return FALSE;
}
