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
#include "ITranslationUtilities.h"
#include <memory>
#include <shlwapi.h>

BOOL ITranslationUtilities::CompareTranslations(ITranslation& first, ITranslation& second, TranslationComparison& stats, std::basic_string<TCHAR>& log)
{
	BOOL bTranslated = FALSE;
	stats = TranslationComparison();
	TCHAR bf[100];
	for (int i = 0; i < 1000; i++)
	{
		LPCTSTR firstRet = first.GetString(i);
		LPCTSTR secRet = second.GetString(i);
		if (firstRet != NULL)
		{
			stats.firstCount++;
			if (secRet != NULL)
			{
				stats.secondCount++;
				if (_tcscmp(firstRet, secRet) == 0)
				{
					log += _T("Same string found: ");
					log += _itot(i, bf, 10);
					log += _T(" = ");
					log += firstRet;
					log += _T("\r\n");
					stats.same++;
				}
			}
			else
			{
				stats.notInSecond++;
				log += _T("Untranslated string found: ");
				log += _itot(i, bf, 10);
				log += _T(" = ");
				log += firstRet;
				log += _T("\r\n");
			}
		}
		else
		{
			if (secRet != NULL)
			{
				stats.secondCount++;
				stats.notInFirst++;
				log += _T("Unused string found: ");
				log += _itot(i, bf, 10);
				log += _T(" = ");
				log += secRet;
				log += _T("\r\n");
			}
		}
	}
	return TRUE;
}


LPCTSTR const LNG_Name = _T("NAME");
LPCTSTR const LNG_LocalizedName = _T("LOCALIZEDNAME");
LPCTSTR const LNG_AuthorName = _T("AUTH");
LPCTSTR const LNG_Email = _T("EMAIL");
LPCTSTR const LNG_Description = _T("DESC");
LPCTSTR const LNG_WebSite = _T("WWW");
LPCTSTR const LNG_Version = _T("VERSION");
LPCTSTR const LNG_AppVersion = _T("APPVERSION");
LPCTSTR const LNG_AutoUpdateURL = _T("AUTOUPDATEURL");


BOOL ITranslationUtilities::ImportTranslationFile(LPCTSTR pathName, EditableTranslation& destTranslation, BOOL bInfoOnly/* = FALSE*/)
{
	ASSERT(pathName != NULL);
	if (_taccess(pathName, 4) == 0)
	{
		std::unique_ptr<TCHAR[]> dta(GetTranslationFile(pathName));	//allocated with new TCHAR[]
		if (dta.get() != NULL)
		{
			destTranslation.ClearStrings();
			if (!bInfoOnly)
			{
				if (!GetTranslationStrings(dta.get(), destTranslation))
					return FALSE;
			}
			//=== The default name is the file name without its extension
			TCHAR fileTitle[MAX_PATH];
			_tcsncpy(fileTitle, PathFindFileName(pathName), MAX_PATH - 1);
			fileTitle[MAX_PATH - 1] = 0;
			PathRemoveExtension(fileTitle);
			destTranslation.SetTranslationInfo(ITranslation::TI_Name, fileTitle);
			//li.Path = pathName;
			return GetTranslationInfo(dta.get(), destTranslation);
		}
	}
	return FALSE;
}

BOOL ITranslationUtilities::ExportTranslationFile(LPCTSTR pathName, ITranslation* pSrcTranslation, ITranslation* pReferenceLanguage/* = NULL*/)
{
	ASSERT(pathName != NULL);
	ASSERT(pSrcTranslation!=NULL ||pReferenceLanguage!=NULL);
	CFile f;
	if (!f.Open(pathName, CFile::modeCreate | CFile::modeWrite))
		return FALSE;
#ifdef _UNICODE
	//Write Unicode header
	BYTE b = 0xff;
	f.Write(&b, 1);
	b = 0xfe;
	f.Write(&b, 1);
#endif
	const TCHAR* const Info = _T("#**************How To Translate**************\r\n")
		_T("#1. All the lines that start with '#' are comments and will be ignored\r\n")
		_T("#2. The lines that starts with @ store info about the tranlation file. Specifically:\r\n")
		_T("#   @NAME=Language Name in English.\r\n")
		_T("#   @LOCALIZEDNAME=Native Language Name.\r\n")
		_T("#   @AUTH=Author Name.\r\n")
		_T("#   @EMAIL=Author's Email\r\n")
		_T("#   @DESC=Description of this language file\r\n")
		_T("#   @WWW=Author's Website\r\n")
		_T("#   @VERSION=Language File Version\r\n")
		_T("#   @APPVERSION=Application version\r\n")
		_T("#   @AUTOUPDATEURL= (future feature that will support live lang pack update)\r\n")
		_T("#3. All The following lines which start with (number)= are the ones that you\r\n")
		_T("#   must translate. Untranslated strings are initially commented.\r\n")
		_T("#   Already translated strings are (optionally) kept for reference as a comment\r\n")
		_T("#   CAUTION!!!\r\n")
		_T("#   Translation strings are ONE-LINERS.\r\n")
		_T("#   DO NOT CHANGE %s (strings) %d (integers) and other %_ as they placeholders.\r\n")
		_T("#   DO NOT CHANGE the numbers before the'='.\r\n")
		_T("#   Watch for special characters like '\\r\\n' or '\\n' which means new line.\r\n")
		_T("#   Take care of the HTML Tags.\r\n")
		_T("\r\n");
	f.Write(Info, _tcslen(Info) * sizeof(TCHAR));

	ITranslation* pTrans = pSrcTranslation != NULL ? pSrcTranslation : pReferenceLanguage;
	SaveHeader(f, LNG_Name, pTrans->GetTranslationInfo(ITranslation::TI_Name));
	SaveHeader(f, LNG_LocalizedName, pTrans->GetTranslationInfo(ITranslation::TI_LocalizedName));
	SaveHeader(f, LNG_AuthorName, pTrans->GetTranslationInfo(ITranslation::TI_Author));
	SaveHeader(f, LNG_Email, pTrans->GetTranslationInfo(ITranslation::TI_Email));
	SaveHeader(f, LNG_Description, pTrans->GetTranslationInfo(ITranslation::TI_Description));
	SaveHeader(f, LNG_WebSite, pTrans->GetTranslationInfo(ITranslation::TI_WebSite));
	SaveHeader(f, LNG_Version, pTrans->GetTranslationInfo(ITranslation::TI_Version));
	SaveHeader(f, LNG_AppVersion, pTrans->GetTranslationInfo(ITranslation::TI_AppVersion));
	SaveHeader(f, LNG_AutoUpdateURL, pTrans->GetTranslationInfo(ITranslation::TI_AutoUpdateURL));
	f.Write(_T("\r\n"), 2 * sizeof(TCHAR));


	BOOL bTranslated = FALSE;
	std::tstring strLoader;
	for (int i = 0; i < 1000; i++)
	{
		LPCTSTR srcStr = pSrcTranslation ? pSrcTranslation->GetString(i) : NULL;
		LPCTSTR refStr = pReferenceLanguage ? pReferenceLanguage->GetString(i) : NULL;
		if (refStr == NULL)
			continue;
		TCHAR header[10];
		_sntprintf(header, 10, _T("%d="), i);
		header[9] = 0;
		strLoader = refStr;
		Replace(strLoader, _T("\r"), _T("\\r"));
		Replace(strLoader, _T("\n"), _T("\\n"));
		Replace(strLoader, _T("\t"), _T("\\t"));
		f.Write(_T("#"), sizeof(TCHAR));
		f.Write(header, _tcslen(header) * sizeof(TCHAR));
		f.Write(strLoader.c_str(), strLoader.size() * sizeof(TCHAR));
		f.Write(_T("\r\n"), 2 * sizeof(TCHAR));
		if (srcStr != NULL)
		{
			strLoader = srcStr;
			Replace(strLoader, _T("\r"), _T("\\r"));
			Replace(strLoader, _T("\n"), _T("\\n"));
			Replace(strLoader, _T("\t"), _T("\\t"));
			f.Write(header, _tcslen(header) * sizeof(TCHAR));
			f.Write(strLoader.c_str(), strLoader.size() * sizeof(TCHAR));
			f.Write(_T("\r\n"), 2 * sizeof(TCHAR));
		}
	}
	f.Close();
	return TRUE;
}

LPTSTR ITranslationUtilities::GetTranslationFile(LPCTSTR pathName)
{
	//=== Returns the whole file as a null-terminated string (delete[] it), or NULL.
	//=== UTF-16LE and UTF-8 files are recognized by their BOM; anything else is ANSI.
	ASSERT(pathName != NULL);
	CFile f;
	if (!f.Open(pathName, CFile::modeRead | CFile::shareDenyWrite))
		return NULL;
	ULONGLONG len64 = f.GetLength();
	if (len64 < 2 || len64 > 16 * 1024 * 1024)
		return NULL;
	UINT fLen = (UINT) len64;
	std::unique_ptr<BYTE[]> fileData(new BYTE[fLen]);
	if (f.Read(fileData.get(), fLen) != fLen)
		return NULL;
	f.Close();
	const BYTE* data = fileData.get();
	if (fLen >= 2 && data[0] == 0xff && data[1] == 0xfe)
	{
		UINT chars = (fLen - 2) / sizeof(WCHAR);
		LPTSTR ret = new TCHAR[chars + 1];
		memcpy(ret, &data[2], chars * sizeof(WCHAR));
		ret[chars] = 0;
		return ret;
	}
	UINT codePage = CP_ACP;
	if (fLen >= 3 && data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf)
	{
		codePage = CP_UTF8;
		data += 3;
		fLen -= 3;
	}
	INT chars = fLen > 0 ? MultiByteToWideChar(codePage, 0, (LPCSTR)data, fLen, NULL, 0) : 0;
	LPTSTR ret = new TCHAR[chars + 1];
	if (chars > 0)
		MultiByteToWideChar(codePage, 0, (LPCSTR)data, fLen, ret, chars);
	ret[chars] = 0;
	return ret;
}

void ITranslationUtilities::SaveHeader(CFile& f, LPCTSTR header, LPCTSTR info)
{
	ASSERT(header != NULL);
	ASSERT(_tcslen(header) < 20);
	f.Write(_T("@"), sizeof(TCHAR));
	f.Write(header, _tcslen(header) * sizeof(TCHAR));
	f.Write(_T("="), sizeof(TCHAR));
	if (info != NULL)
		f.Write(info, _tcslen(info) * sizeof(TCHAR));
	f.Write(_T("\r\n"), 2 * sizeof(TCHAR));
}

namespace
{
	//=== Splits the file into lines (CR, LF or CRLF endings) and calls
	//=== fn(lineStart, lineLength) for each non-empty one.
	template <class Fn>
	void ForEachLine(LPCTSTR data, Fn fn)
	{
		LPCTSTR pos = data;
		while (*pos != 0)
		{
			LPCTSTR end = pos;
			while (*end != 0 && *end != '\r' && *end != '\n')
				end++;
			if (end > pos)
				fn(pos, (size_t)(end - pos));
			pos = end;
			while (*pos == '\r' || *pos == '\n')
				pos++;
		}
	}
}

BOOL ITranslationUtilities::GetTranslationStrings(LPCTSTR translationFileData, EditableTranslation& destLanguage)
{
	//=== String lines are "<id>=<text>" (id: 1 to 5 characters, > 0). '#' and '@' lines are skipped.
	ForEachLine(translationFileData, [&destLanguage](LPCTSTR line, size_t len)
	{
		if (line[0] == '#' || line[0] == '@')
			return;
		LPCTSTR eqPos = wmemchr(line, '=', len);
		if (eqPos == NULL || eqPos == line || eqPos - line > 5)
		{
			TRACE(_T("@1 ITranslationUtilities::GetTranslationStrings. LngFile error. Near:'%.20s'\r\n"), line);
			return;
		}
		TCHAR num[6];
		_tcsncpy(num, line, eqPos - line);
		num[eqPos - line] = 0;
		INT resNum = _ttoi(num);
		if (resNum <= 0)
		{
			TRACE(_T("@1 ITranslationUtilities::GetTranslationStrings. Invalid ID. Near:'%.20s'\r\n"), line);
			return;
		}
		std::tstring text(eqPos + 1, line + len);
		FixString(text);
		destLanguage.SetString(resNum, text.c_str());
	});
	return TRUE;
}
void ITranslationUtilities::FixString(std::tstring& str)
{
	Replace(str, _T("\\r"), _T("\r"));
	Replace(str, _T("\\n"), _T("\n"));
	Replace(str, _T("\\t"), _T("\t"));
}

void ITranslationUtilities::Replace(std::tstring& str, LPCTSTR oldString, LPCTSTR newString)
{
	size_t oldLen = _tcslen(oldString);
	size_t newLen = _tcslen(newString);
	size_t pos = str.find(oldString);
	while (pos != std::tstring::npos)
	{
		str.replace(pos, oldLen, newString);
		pos = str.find(oldString, pos + newLen);
	}
}

BOOL ITranslationUtilities::GetTranslationInfo(LPCTSTR translationFileData, EditableTranslation& translation)
{
	//=== Info lines are "@<KEY>=<value>"; unknown keys are ignored.
	ForEachLine(translationFileData, [&translation](LPCTSTR line, size_t len)
	{
		if (line[0] != '@')
			return;
		LPCTSTR eqPos = wmemchr(line, '=', len);
		if (eqPos == NULL || eqPos - line - 1 >= 20 || eqPos == line + 1)
		{
			TRACE(_T("@1 ITranslationUtilities::GetTranslationInfo (Unknown info field). Near:'%.20s'\r\n"), line);
			return;
		}
		TCHAR key[20];
		_tcsncpy(key, &line[1], eqPos - line - 1);
		key[eqPos - line - 1] = 0;
		ITranslation::TranslationInfoEnum ti = ITranslation::TI_Last;
		if (_tcscmp(key, LNG_Name) == 0)
			ti = ITranslation::TI_Name;
		else if (_tcscmp(key, LNG_LocalizedName) == 0)
			ti = ITranslation::TI_LocalizedName;
		else if (_tcscmp(key, LNG_AuthorName) == 0)
			ti = ITranslation::TI_Author;
		else if (_tcscmp(key, LNG_Email) == 0)
			ti = ITranslation::TI_Email;
		else if (_tcscmp(key, LNG_Description) == 0)
			ti = ITranslation::TI_Description;
		else if (_tcscmp(key, LNG_WebSite) == 0)
			ti = ITranslation::TI_WebSite;
		else if (_tcscmp(key, LNG_Version) == 0)
			ti = ITranslation::TI_Version;
		else if (_tcscmp(key, LNG_AutoUpdateURL) == 0)
			ti = ITranslation::TI_AutoUpdateURL;
		else if (_tcscmp(key, LNG_AppVersion) == 0)
			ti = ITranslation::TI_AppVersion;
		size_t valueLen = line + len - eqPos - 1;
		if (ti == ITranslation::TI_Last)
			TRACE(_T("@2 ITranslationUtilities::GetTranslationInfo. Unknown Key: '%s'\r\n"), key);
		else if (valueLen > 0)		//=== An empty value keeps the default (e.g. the file name)
			translation.SetTranslationInfo(ti, eqPos + 1, (UINT)valueLen);
	});
	return TRUE;
}
