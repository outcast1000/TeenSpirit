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
#include "cStringUtils.h"

TCHAR * _tcsltrim(TCHAR * pszSource)
{
	TCHAR * cp = pszSource;
	if (cp && *cp)
	{
		// find first non-whitespace character
		while (_istspace(*cp))
			cp++;
		if (cp != pszSource)
			memcpy(pszSource, cp, (_tcslen(cp)+1)*sizeof(TCHAR));
	}
	return pszSource;
}


///////////////////////////////////////////////////////////////////////////////
//
// _tcsrtrim()
//
// Purpose:     Removes (trims) trailing whitespace characters from a string
//
// Parameters:  pszSource - Pointer to the null-terminated string to be trimmed. 
//                          On return, pszSource will hold the trimmed string
//
// Returns:     TCHAR *   - pointer to trimmed string
//
TCHAR * _tcsrtrim(TCHAR * pszSource)
{
	TCHAR * cp = pszSource;

	if (cp && *cp)
	{
		BOOL bNonSpaceSeen = FALSE;

		// check if string is blank
		while (*cp)
		{
			if (!_istspace(*cp))
				bNonSpaceSeen = TRUE;
			cp++;
		}

		if (bNonSpaceSeen)
		{
			cp--;

			// find last non-whitespace character
			while ((cp >= pszSource) && (_istspace(*cp)))
				*cp-- = _T('\0');
		}
		else
		{
			// string contains only whitespace characters
			*pszSource = _T('\0');
		}
	}

	return pszSource;
}

TCHAR* _tcstrim(TCHAR* pszSource)
{
	return _tcsltrim(_tcsrtrim(pszSource));
}


TCHAR* _tcsproper(TCHAR* str)
{
	//Remember to set locale
	//#include <locale.h>
	//setlocale( LC_TYPE, "" );
	if (str[0]==0)
		return str;
	str[0] = _totupper(str[0]);
	for (TCHAR* p = str + 1 ; *p ; p++ )
	{
		//if (*(p-1) == ' ' || *str == '.')
		if (_tcschr(_T(".,()!; "), *(p-1)) != NULL)	
			*p = _totupper(*p);
		else
			*p = _totlower(*p);
	}
	return str;
}


int _tchrrep(TCHAR find, TCHAR replace, LPTSTR str)
{
	int ret = 0;
	TCHAR* f = _tcschr(str, find);
	while (f != NULL)
	{
		ret++;
		*f = replace;
		f = _tcschr(f, find);
	}
	return ret;
}
int _tchrrep(LPCTSTR findOne, TCHAR replace, LPTSTR str)
{
	int ret = 0;
	const TCHAR* f = findOne;
	while (*f != NULL)
	{
		ret += _tchrrep(*f, replace, str);
		f++;
	}
	return ret;
}


const char* stristrex(const char* pszMain, const char* pszSub, int range)
{
	size_t subLen = strlen(pszSub);
	for (size_t pos = 0; pos < range - subLen; pos++)
	{
		if (pszMain[pos] == 0)
			return NULL;
		if (_strnicmp(&pszMain[pos], pszSub, subLen) == 0)
			return &pszMain[pos];
	}
	return NULL;
}
const WCHAR* wcsistrex(const WCHAR* pszMain, const WCHAR* pszSub, int range)
{
	size_t subLen = wcslen(pszSub);
	for (size_t pos = 0; pos < range - subLen; pos++)
	{
		if (_wcsnicmp(&pszMain[pos], pszSub, subLen) == 0)
			return &pszMain[pos];
	}
	return NULL;
}




//stristr / wcsistr: case-insensitive strstr (CharLower). NULL when either string is NULL or
//pszSub is empty. (They were x86 assembly by Ralph Walden, which x64 can't compile.)
namespace
{
	inline char LowerCharA(char c)		{return (char)(UINT_PTR)CharLowerA((LPSTR)(UINT_PTR)(BYTE)c);}
	inline WCHAR LowerCharW(WCHAR c)	{return (WCHAR)(UINT_PTR)CharLowerW((LPWSTR)(UINT_PTR)c);}

	template <class T, class Lower>
	const T* FindNoCase(const T* pszMain, const T* pszSub, Lower lower)
	{
		if (pszMain == NULL || pszSub == NULL || pszSub[0] == 0)
			return NULL;
		const T first = lower(pszSub[0]);
		for (; *pszMain != 0; pszMain++)
		{
			if (lower(*pszMain) != first)
				continue;
			const T* m = pszMain + 1;
			const T* s = pszSub + 1;
			while (*s != 0 && (*m == *s || lower(*m) == lower(*s)))
			{
				m++;
				s++;
			}
			if (*s == 0)
				return pszMain;
		}
		return NULL;
	}
}

const char* __fastcall stristr(const char* pszMain, const char* pszSub)
{
	return FindNoCase(pszMain, pszSub, LowerCharA);
}

const WCHAR* __fastcall wcsistr(const WCHAR* pszMain, const WCHAR* pszSub)
{
	return FindNoCase(pszMain, pszSub, LowerCharW);
}



/*

const char* strrfind(const char* searchableString, const char* startPos, char charToSearch)
{
	ASSERT(searchableString != NULL && startPos != NULL);
	ASSERT(startPos > searchableString);
	while (*startPos != charToSearch)
	{
		startPos--;
		if (startPos <= searchableString)
			return NULL;
	}
	return startPos;
}
const wchar_t* wcsrfind(const wchar_t* searchableString, const wchar_t* startPos, wchar_t charToSearch)
{
	ASSERT(searchableString != NULL && startPos != NULL);
	ASSERT(startPos > searchableString);
	while (*startPos != charToSearch)
	{
		startPos--;
		if (startPos == searchableString)
			return NULL;
	}
	return startPos;
}
*/


LPWSTR HTMLText2Unicode(LPCSTR html, UINT size/* = -1*/)
{
	LPWSTR ret = NULL;
	//Changed to make it work
	//There is a problem for non latin users (like me) when downloading
	//English / french / spanish lyrics..
	//To overcome this problem you must check the page's codepage
	//In HTML's 
	//<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1"> = Latin1 (28591)
	//<meta http-equiv="Content-Type" content="text/html; charset=windows-1253"> = Greek (1253)
	//<meta http-equiv="Content-Type" content="text/html; charset=windows-1252"> = Latin1 (1252)
	//<meta http-equiv="Content-Type" content="text/html; charset=utf-8" /> = utf8 (65001)
	//If nothing then use Latin1

	UINT codePage = 28591;
	const CHAR* startPos = stristrex(html, " charset=", size == -1 ? 2000 : min(2000, size));
	if (startPos != 0)
	{
		startPos += 9;
		const CHAR* endPos = strchr(startPos, '"');
		if (endPos != NULL && endPos - startPos < 30)
		{
			CHAR bf[30];
			strncpy_s(bf, startPos, endPos - startPos);
			_strlwr_s(bf);
			if (strncmp(bf, "windows-", 8) == 0 && endPos - startPos > 8)
			{
				codePage = atol(&bf[8]);
			}
			else if (strncmp(bf, "iso-8859-", 9) == 0 && endPos - startPos > 9)
			{
				codePage = 28590 +  atol(&bf[9]);
			}
			else if (strncmp(bf, "utf-8", 5) == 0)
				codePage = 65001;
			else if (strncmp(bf, "utf-7", 5) == 0)
				codePage = 65000;
			else
			{
				TRACE(_T("@1 HTMLText2Unicode. Unknown charset: '%s'\r\n"), (LPCTSTR)CA2CT(bf));
			}
		}
		else
			TRACE(_T("@1 HTMLText2Unicode. Can't find END charset\r\n"));
	}
	else
		TRACE(_T("@1 HTMLText2Unicode. Can't find charset\r\n"));

	UINT wLen = MultiByteToWideChar(codePage, 0, html, size, NULL, 0);
	if (wLen > 0)
	{
		ret = new TCHAR[wLen + 1];
		MultiByteToWideChar(codePage, 0, html, size, ret, wLen);
		ret[wLen] = 0;
	}
	return ret;
}

struct htmlConvertinonMap
{
	LPCTSTR html;
	TCHAR ch;
};

const htmlConvertinonMap htmlConvertinonMappings[] = 
{
	{_T("&quot;"), '\"'},
	{_T("&amp;"), '&'},
	{_T("&ndash;"), '-'},
	{_T("&eacute;"), _T('é')},
	{_T("&lt;"), '<'},
	{_T("&rt;"), '>'},
	{_T("&#39;"), '\''},
	{_T("&#180;"), _T('´')},
	{_T("&acute;"), _T('´')}

};

INT InlineHTML2Text(LPTSTR htmlText)
{
	INT convertions = 0;
	LPCTSTR bReadPos = htmlText;
	LPTSTR bWritePos = htmlText;
	while (*bReadPos != NULL)
	{
		if (*bReadPos == TCHAR('&'))
		{
			BOOL bFound = FALSE;
			for (int i = 0; i < sizeof(htmlConvertinonMappings)/sizeof(htmlConvertinonMap); i++)
			{
				size_t len = _tcslen(htmlConvertinonMappings[i].html);
				if (_tcsnicmp(bReadPos, htmlConvertinonMappings[i].html, len) == 0)
				{
					convertions++;
					*bWritePos = htmlConvertinonMappings[i].ch;
					bReadPos += len-1;
					bFound = TRUE;
					break;
				}
			}
			if (!bFound)
			{
				TRACE(_T("@2InlineHTML2Text. Can't Find '%.10s' Code.\r\n"), bReadPos);
				*bWritePos = *bReadPos;
			}
		}
		else
			*bWritePos = *bReadPos;
		bReadPos++;
		bWritePos++;
	}
	*bWritePos = 0;
	return convertions;
}

