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
#include "HTMLUtilities.h"

#include "cStringUtils.h"

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

#ifdef _DEBUG
void InlineHTML2TextTest()
{
	TRACE(_T("@2@TEST@Testing InlineHTML2TextTest\r\n"));
	const LPCTSTR Tests[] =
	{
		_T("Nothing"),
		_T("Simple &quot; Double"),
		_T("&ndash; beggining"),
		_T("Ending &lt;"),
		_T("Al&eacute;x"),
		_T("Al&eacute;x&ndash;&eacute;conomou"),
		_T("Some &#39;&unkonwn;&#39; tag"),
		_T("Unclosing & amps & &")
	};
	TCHAR bf[500];
	for (int i = 0; i < sizeof(Tests)/sizeof(LPCTSTR); i++)
	{
		_tcscpy(bf, Tests[i]);
		INT res = InlineHTML2Text(bf);
		TRACE(_T("@2---Test#%0.2d---\r\n\tsrc:'%s'\r\n\tdst:'%s'\r\n\tModifications: %d\r\n"),
			i, Tests[i], bf, res);
	}
	TRACE(_T("@2@TEST@@2Testing InlineHTML2TextTest END\r\n"));

}
#endif
