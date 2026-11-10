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




//stristr
// If this code works, it was written by Ralph Walden. If it doesn't work, I don't know who wrote it.
#pragma warning(disable : 4035) // no return value

const char* __fastcall stristr(const char* pszMain, const char* pszSub)
{
	pszMain;    // compiler thinks these are unreferenced because
	pszSub;     // they are in ecx and edx registers

	char* pszTmp1;
	char* pszTmp2;
	char  lowerch, upperch;

	// We keep the first character of pszSub in lowerch and upperch (lower and
	// upper case). First we loop trying to find a match for this character. Once
	// we have found a match, we start with the second character of both pszMain
	// and pszSub and walk through both strings doing a CharLower on both
	// characters before comparing. If we make it all the way through pszSub with
	// matches, then we bail with a pointer to the string's location in pszMain.

	_asm {
		mov esi, ecx    // pszMain
			mov edi, edx    // pszSub

			// Check for NULL pointers

			test ecx, ecx
			je short NoMatch // NULL pointer for pszMain
			test edx, edx
			je short NoMatch // NULL pointer for pszSub

			sub eax, eax
			mov al, [edi]
		push eax
			call DWORD PTR CharLower
			mov lowerch, al
			push eax
			call DWORD PTR CharUpper
			mov upperch, al

			push edi    // increment the second string pointer
			call DWORD PTR CharNext
			mov  edi, eax

			mov pszTmp2, edi
			mov edi, DWORD PTR CharNext // faster to call through a register

Loop1:
		mov al, [esi]
		test al, al
			je short NoMatch        // end of main string, so no match
			cmp al, lowerch
			je short CheckString    // lowercase match?
			cmp al, upperch
			je short CheckString    // upppercase match?
			push esi
			call edi                // Call CharNext to update main string pointer
			mov esi, eax
			jmp short Loop1

CheckString:
		mov pszTmp1, esi    // save current pszMain pointer in case its a match
			push esi
			call edi            // first character of both strings match,
			mov  esi, eax       // so move to next pszMain character
			mov edi, pszTmp2
			mov al, [edi]
		jmp short Branch1

Loop3:
		push esi
			call DWORD PTR CharNext    // CharNext to change pszMain pointer
			mov  esi, eax
			push edi
			call DWORD PTR CharNext    // CharNext to change pszSub pointer
			mov  edi, eax

			mov al, [edi]
Branch1:
		test al, al
			je short Match       // zero in sub string, means we've got a match
			cmp al, [esi]
		je short Loop3

			// Doesn't match, but might be simply a case mismatch. Lower-case both
			// characters and compare again

			sub ecx, ecx
			mov cl, al  // character from pszSub
			push ecx
			call DWORD PTR CharLower
			mov cl, al
			sub eax, eax
			mov al,  [esi]   // character from pszMain
		push ecx    // preserve register
			push eax
			call DWORD PTR CharLower
			pop ecx
			cmp al, cl
			je short Loop3  // we still have a match, keep checking

			// No match, put everything back, update pszMain to the next character
			// and try again from the top

			mov esi, pszTmp1
			mov  edi, DWORD PTR CharNext
			push esi
			call edi
			mov  esi, eax
			jmp short Loop1

Match:
		mov eax, pszTmp1
			jmp short Done  // Don't just return -- always let the C portion of the code handle the return

NoMatch:
		sub eax, eax
Done:
	}

	// Note lack of return in the C portion of the code. Return value is always in
	// eax register which we have set by the time we get here
}

const WCHAR* __fastcall wcsistr(const WCHAR* pszMain, const WCHAR* pszSub)
{
	pszMain;    // compiler thinks these are unreferenced
	pszSub;

	WCHAR* pszTmp1;
	WCHAR* pszTmp2;
	WCHAR  lowerch, upperch;

	// We keep the first character of pszSub in lowerch and upperch (lower and
	// upper case). First we loop trying to find a match for this character. Once
	// we have found a match, we start with the second character of both pszMain
	// and pszSub and walk through both strings doing a CharLower on both
	// characters before comparing. If we make it all the way through pszSub with
	// matches, then we bail with a pointer to the strings location in pszMain.

	_asm {
		mov esi, ecx    // pszMain
			mov edi, edx    // pszSub

			// Check for NULL pointers

			test ecx, ecx
			je short NoMatch // NULL pointer for pszMain
			test edx, edx
			je short NoMatch // NULL pointer for pszSub

			sub eax, eax
			mov ax, [edi]
		push eax
			call DWORD PTR CharLowerW
			mov lowerch, ax
			push eax
			call DWORD PTR CharUpperW
			mov upperch, ax

			lea edi, [edi+2]

		mov pszTmp2, edi

Loop1:
		mov ax, [esi]
		test ax, ax
			je short NoMatch        // end of main string, so no match
			cmp ax, lowerch
			je short CheckString    // lowercase match?
			cmp ax, upperch
			je short CheckString    // upppercase match?
			lea esi, [esi+2]
		jmp short Loop1

CheckString:
		mov pszTmp1, esi    // save current pszMain pointer
			lea esi, [esi+2]
		mov edi, pszTmp2
			mov ax, [edi]
		jmp short Branch1

Loop3:
		lea esi, [esi+2]
		lea edi, [edi+2]

		mov ax, [edi]
Branch1:
		test ax, ax
			je short Match       // zero in main string, means we've got a match
			cmp ax, [esi]
		je short Loop3

			// Doesn't match, but might be simply a case mismatch. Lower-case both
			// characters and compare again

			sub ecx, ecx
			mov cx, ax  // character from pszSub
			push ecx
			call DWORD PTR CharLowerW
			mov cx, ax
			sub eax, eax
			mov ax, [esi]   // character from pszMain
		push ecx        // preserve register
			push eax
			call DWORD PTR CharLowerW
			pop ecx
			cmp ax, cx
			je short Loop3  // we still have a match, keep checking

			// No match, put everything back, update pszMain to the next character
			// and try again from the top

			mov esi, pszTmp1
			lea esi, [esi+2]
		jmp short Loop1

Match:
		mov eax, pszTmp1
			jmp short Done

NoMatch:
		sub eax, eax
Done:
	}
	// Note lack of return in the C portion of the code. Return value is always in
	// eax register which we have set by the time we get here
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

