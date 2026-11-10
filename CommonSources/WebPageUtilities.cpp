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
#include "WebPageUtilities.h"
#include "stlStringUtils.h"
#include "Wininet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _UNITTESTING

LPCTSTR repEntitiesTest[] = 
{
	_T("No Entities test"), 
	_T("No Entities test"),

	_T("Only & and && tests for &"),
	_T("Only & and && tests for &"),
	
	_T("This is a test &rt;&amp;&lt;&#0039;&#039;&#39; for &amp; at the end &#0039;"),
	_T("This is a test >&<''' for & at the end '"),
	
	_T("Unknown &xxx; Not Valid (overflow) &#912345; Not Valid (Length) &rt df df dse etkaokwws; Not Valid-Valid &rt&amp; Not Valid - End &rt"), 
	_T("Unknown &xxx; Not Valid (overflow) &#912345; Not Valid (Length) &rt df df dse etkaokwws; Not Valid-Valid &rt& Not Valid - End &rt"), 
};

BOOL TestWebPageUtilities()
{
	UNITTESTSTART;
	for (int i = 0; i < sizeof(repEntitiesTest)/sizeof(LPCTSTR); i+=2)
	{
		std::wstring test(repEntitiesTest[i]);
		ReplaceHtmlEntities(test);
		UNITTEST(test == repEntitiesTest[i+1]);
	}
	//UNITTEST(ReplaceHtmlEntities(std::wstring(_T("Only & and && tests for &"))) == 0);
	//UNITTEST(ReplaceHtmlEntities(std::wstring(_T("This is a test &rt;&amp;&lt;&#0039;&#039;&#39; for &amp; at the end &#0039;"))) == 8);
	//UNITTEST(ReplaceHtmlEntities(std::wstring(_T(""))) == 1);


	HINTERNET hNet = InternetOpen(_T("UnitTest"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);
	std::string page;
	if (hNet)
	{
		page.clear();
		UNITTEST(!DownloadWebPage(page, hNet, _T("123567")));
		UNITTEST(!DownloadWebPage(page, hNet, _T("")));
		UNITTEST(page.empty());
		if (DownloadWebPage(page, hNet, _T("http://www.in.gr/")))
			UNITTEST(!page.empty());

		std::wstring pageU;
		if (DownloadWebPageUnicode(pageU, hNet, _T("http://www.in.gr/")))
		{
			UNITTEST(!page.empty());
			std::wstring UnicodeAnsiPage;
			INT cp = GetWindowsCodePageA(page.c_str());
			if (cp == 0)
				cp = 1252;
			Ansi2Unicode(UnicodeAnsiPage, page, cp);
			UNITTEST(UnicodeAnsiPage == pageU);
		}
		InternetCloseHandle(hNet);
	}

	page = "<html content=\"text/html; charset=windows-452\">This is a &amp; test</html>";
	UNITTEST(GetWindowsCodePageA(page.c_str()) == 452);
	UNITTEST(GetWindowsCodePageA("content=\"text/html; charset=UTF-8\">") == 65001);
	UNITTEST(GetWindowsCodePageA("content=\"text/html; charset=utf-8\">") == 65001);
	UNITTEST(GetWindowsCodePageA("content=\"text/html; charset=iso-8859-7\"") == 28597);
	UNITTEST(GetWindowsCodePageA("content=\"text/html; charset=iso-8859-1\"") == 28591);

	std::wstring pageU;
	Ansi2Unicode(pageU, page);
	UNITTEST(ReplaceHtmlEntities(pageU) == 1);
	UNITTEST(pageU == _T("<html content=\"text/html; charset=windows-452\">This is a & test</html>"));

	std::tstring text;
	GetTextFromHtmlFragment(text, pageU.c_str(), &pageU.c_str()[page.size() - 1]);
	UNITTEST(text == _T("This is a & test"));
	int a = (int) text.size();
	LPCTSTR s = text.c_str();



	return TRUE;
}

#endif


//void TraceFailure(LPCTSTR msg)
//{
//#ifdef	_DEBUG
//	LPVOID     lpMsgBuffer;
//	DWORD dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
//		NULL,
//		::GetLastError(),
//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//		reinterpret_cast<LPTSTR>(&lpMsgBuffer),
//		0,
//		NULL);
//	TRACE(_T("LastFMServices::SetFailure. ['%s'] Error: '%s'\r\n"), msg, (LPCTSTR)lpMsgBuffer);
//	LocalFree(lpMsgBuffer);		
//#endif
//}

void HandleInternetError(LPCTSTR functionName)
{
	INT le = GetLastError();
	LPCTSTR errMsg = NULL;
#ifdef DEBUG
	switch (le)
	{
	case 12002:
		errMsg = _T("ERROR_INTERNET_TIMEOUT");
		break;
	case 12017:
		errMsg = _T("ERROR_INTERNET_OPERATION_CANCELLED (App Closing)");
		break;
	case 12029:
		errMsg = _T("ERROR_INTERNET_CANNOT_CONNECT");
		break;
	case 12030:
		errMsg = _T("ERROR_INTERNET_CONNECTION_ABORTED (App Closing)");
		break;
	case 12007:
		errMsg = _T("ERROR_INTERNET_NAME_NOT_RESOLVED");
		break;
	case 6://ERROR_INVALID_HANDLE:
		errMsg = _T("***********************ERROR_INVALID_HANDLE");
		break;
	default:
		ASSERT(0);
		break;//Unknown Internet Error
	}
#endif
	if (errMsg != NULL)
		TRACE(_T("@1 %s Failure: '%s'\r\n"), functionName, errMsg);
	else
		TRACE(_T("@1 %s Failure: [%d]\r\n"), functionName, le);
	SetLastError(0);
}


BOOL ReadInternetFile2StringA(HINTERNET hFile, std::string& page)
{
	page.clear();
	DWORD pageSize = 0;
	DWORD pageSizeLen = sizeof(pageSize);
	if (::HttpQueryInfoA(hFile, HTTP_QUERY_CONTENT_LENGTH, &pageSize, &pageSizeLen, NULL))
	{
		if (pageSize > 0)
			page.reserve(pageSize + 10);
	}

	const int bufferSize = 8192;
	CHAR bf[bufferSize];
	unsigned long nSize = 0;
	BOOL bReadSuccess = TRUE;
	while(bReadSuccess)
	{
		bReadSuccess = InternetReadFile(hFile, bf, bufferSize, &nSize);
		if (bReadSuccess)
		{
			if (nSize == 0)
				return TRUE;
			page.append(bf, nSize);
		}
		else
			HandleInternetError(_T("ReadInternetFile2StringA. InternetReadFile"));
	}
	return FALSE;
}

BOOL ReadInternetFile2StringW(HINTERNET hFile, std::string& page)
{
	//=== CAREFULL* InternetReadFile MAY FAIL HERE INVALIDATING THE WHOLE INTERNET
	//		HANDLE IF THE URL IS TOOO LARGE > 1000 BYTES.
	page.clear();
	DWORD pageSize = 0;
	DWORD pageSizeLen = sizeof(pageSize);
	if (::HttpQueryInfoW(hFile,HTTP_QUERY_CONTENT_LENGTH, &pageSize, &pageSizeLen, NULL))
	{
		if (pageSize > 0)
			page.reserve(pageSize + 10);
	}
	const int bufferSize = 8192;
	CHAR bf[bufferSize];
	unsigned long nSize = 0;
	BOOL bReadSuccess = TRUE;
	while(bReadSuccess)
	{
		bReadSuccess = InternetReadFile(hFile, bf, bufferSize, &nSize);
		if (bReadSuccess)
		{
			if (nSize == 0)
				return TRUE;
			page.append(bf, nSize);
		}
		else
			HandleInternetError(_T("ReadInternetFile2StringW. InternetReadFile"));
	}
	return FALSE;
}


BOOL DownloadWebPageA(std::string& page, HINTERNET hInternet, LPCSTR url)
{
	TRACEST(_T("DownloadWebPageA"), (LPCTSTR)CA2CT(url));
	BOOL bDownloadSuccess = FALSE;
	if (hInternet == NULL)
		return FALSE;
	HINTERNET hFile = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (hFile)
	{   
		bDownloadSuccess = ReadInternetFile2StringA(hFile, page);
		InternetCloseHandle(hFile);
	}
	else
		HandleInternetError(_T("DownloadWebPage. InternetOpenUrlA"));
	return bDownloadSuccess;
}

BOOL DownloadWebPageW(std::string& page, HINTERNET hInternet, LPCWSTR url)
{
	TRACEST(_T("DownloadWebPageW"), (LPCTSTR)CW2CT(url));
	BOOL bDownloadSuccess = FALSE;
	if (hInternet == NULL)
		return FALSE;
	HINTERNET hFile = InternetOpenUrlW(hInternet, url, NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (hFile)
	{   
		bDownloadSuccess = ReadInternetFile2StringW(hFile, page);
		InternetCloseHandle(hFile);
	}
	else
		HandleInternetError(_T("DownloadWebPage. InternetOpenUrlW"));
	return bDownloadSuccess;
}

BOOL DownloadWebPagePostA(std::string& page, HINTERNET hInternet, LPCSTR url, LPCSTR postData)
{
	TRACEST(_T("DownloadWebPagePostA"), (LPCTSTR)CA2CT(url));
	BOOL bRet = FALSE;
	URL_COMPONENTSA comp;
	memset(&comp, 0, sizeof(URL_COMPONENTSA));
	comp.dwStructSize = sizeof(URL_COMPONENTSA);
	CHAR hostName[100], URI[100];
	comp.dwHostNameLength = comp.dwUrlPathLength = 100;
	comp.lpszHostName = hostName;
	comp.lpszUrlPath = URI;
	if (InternetCrackUrlA(url, 0, ICU_ESCAPE, &comp))
	{
		HINTERNET hConnection = InternetConnectA(hInternet, 
			comp.lpszHostName, 
			comp.nPort, 
			NULL, 
			NULL, 
			INTERNET_SERVICE_HTTP, 
			INTERNET_FLAG_KEEP_CONNECTION, 
			0);
		if (hConnection)
		{
			LPCSTR sAcceptTypes[] = {"*/*",NULL};
			HINTERNET hRequest=::HttpOpenRequestA(hConnection,
				"POST", // HTTP Verb
				comp.lpszUrlPath, // Object Name
				HTTP_VERSIONA, // Version
				NULL, // Reference
				sAcceptTypes, // Accept Type
				INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_FORMS_SUBMIT,
				0); // context call-back point
			if(hRequest)
			{
				static LPCSTR sHttpAccept = "Accept: */*\r\n";
				::HttpAddRequestHeadersA(hRequest, sHttpAccept, (DWORD)strlen(sHttpAccept), HTTP_ADDREQ_FLAG_ADD_IF_NEW);

				static LPCSTR szContentType = "Content-Type: application/x-www-form-urlencoded\r\n";
				if(::HttpSendRequestA(hRequest,	szContentType, (DWORD)strlen(szContentType), (LPVOID)postData, (DWORD)strlen(postData)))
					bRet = ReadInternetFile2StringA(hRequest, page);
				else
					HandleInternetError(_T("DownloadWebPagePostA. HttpSendRequestA"));
				InternetCloseHandle(hRequest);

			}
			else
				HandleInternetError(_T("DownloadWebPagePostA. HttpOpenRequestA"));
			InternetCloseHandle(hConnection);

		}
		else
			HandleInternetError(_T("DownloadWebPagePostA. InternetConnectA"));
	}
	else
		HandleInternetError(_T("DownloadWebPagePostA. InternetCrackUrlA"));
	return bRet;
}

BOOL DownloadWebPagePostW(std::string& page, HINTERNET hInternet, LPCWSTR url, LPCSTR postData)
{
	TRACEST(_T("DownloadWebPagePostW"), (LPCTSTR)CW2CT(url));
	BOOL bRet = FALSE;
	URL_COMPONENTSW comp;
	memset(&comp, 0, sizeof(URL_COMPONENTSW));
	comp.dwStructSize = sizeof(URL_COMPONENTSW);
	WCHAR hostName[100], URI[100];
	comp.dwHostNameLength = comp.dwUrlPathLength = 100;
	comp.lpszHostName = hostName;
	comp.lpszUrlPath = URI;
	if (InternetCrackUrlW(url, 0, ICU_ESCAPE, &comp))
	{
		HINTERNET hConnection = InternetConnectW(hInternet, 
			comp.lpszHostName, 
			comp.nPort, 
			NULL, 
			NULL, 
			INTERNET_SERVICE_HTTP, 
			INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE, 
			0);
		if (hConnection)
		{
			LPCTSTR sAcceptTypes[] = {L"*/*",NULL};
			HINTERNET hRequest=::HttpOpenRequestW(hConnection,
				L"POST", // HTTP Verb
				comp.lpszUrlPath, // Object Name
				HTTP_VERSION, // Version
				NULL, // Reference
				sAcceptTypes, // Accept Type
				INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_FORMS_SUBMIT,
				0); // context call-back point
			if(hRequest)
			{
				static LPCWSTR sHttpAccept = L"Accept: */*\r\n";
				::HttpAddRequestHeadersW(hRequest, sHttpAccept, (DWORD)wcslen(sHttpAccept), HTTP_ADDREQ_FLAG_ADD_IF_NEW);

				static LPCWSTR szContentType = L"Content-Type: application/x-www-form-urlencoded\r\n";
				if(::HttpSendRequestW(hRequest,	szContentType, (DWORD)wcslen(szContentType), (LPVOID)postData, (DWORD)strlen(postData)))
					bRet = ReadInternetFile2StringW(hRequest, page);
				else
					HandleInternetError(_T("DownloadWebPagePostW. HttpSendRequestW"));
				InternetCloseHandle(hRequest);
			}
			else
				HandleInternetError(_T("DownloadWebPagePostW. InternetConnectW"));
			InternetCloseHandle(hConnection);
		}
		else
			HandleInternetError(_T("DownloadWebPagePostW. InternetConnectW"));
	}
	else
		HandleInternetError(_T("DownloadWebPagePostW. InternetCrackUrlW"));
	return bRet;
}





BOOL DownloadWebPageUnicode(std::wstring& page, HINTERNET hInternet, LPCWSTR url)
{
	TRACEST(_T("DownloadWebPageUnicode"), (LPCTSTR)CW2CT(url));
	BOOL bDownloadSuccess = FALSE;
	if (hInternet == NULL)
		return FALSE;
	HINTERNET hFile = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (hFile)
	{   
		page.clear();
		DWORD dwSize = 0;
		INT headerCP = 0;//Latin1
		INT pageCP = 0;//Latin1
		INT workingCP = 1252;//Latin1
		if(!HttpQueryInfo(hFile, HTTP_QUERY_RAW_HEADERS_CRLF, 0, &dwSize, 0))
		{
			if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
			{
				SetLastError(0);
				LPVOID lpOutBuffer = new CHAR[dwSize];
				if (HttpQueryInfo(hFile, HTTP_QUERY_RAW_HEADERS_CRLF, lpOutBuffer, &dwSize, 0))
				{
					//TRACE((LPCTSTR)lpOutBuffer);
					headerCP = GetWindowsCodePageW((LPCTSTR)lpOutBuffer);
				}
				delete[] lpOutBuffer;
			}		
		}

		DWORD pageSize = 0;
		DWORD pageSizeLen = sizeof(pageSize);
		if (::HttpQueryInfo(hFile, HTTP_QUERY_CONTENT_LENGTH, &pageSize, &pageSizeLen, NULL))
		{
			if (pageSize > 0)
				page.reserve(pageSize + 10);
		}
		if (page.capacity() < 1000)
			page.reserve(50000);
		const int bufferSize = 8192;
		const int bufferSizeU = 2 * bufferSize;
		CHAR bf[bufferSize + 1];
		TCHAR bfU[bufferSizeU];
		unsigned long nSize = 0;
		BOOL bReadSuccess = TRUE;
		BOOL bFirstTime = TRUE;
		while(bReadSuccess)
		{
			bReadSuccess = InternetReadFile(hFile, bf, bufferSize, &nSize);
			if (bReadSuccess)
			{
				if (nSize == 0)
				{
					//TRACE(_T("@3 DownloadWebPage. InternetReadFile Finished\r\n"));
					bDownloadSuccess = TRUE;
					break;
				}
				if (bFirstTime)
				{
					bFirstTime = FALSE;
					bf[nSize] = 0;
					pageCP = GetWindowsCodePageA(bf);
					if (headerCP != pageCP)
					{
						TRACE(_T("@3 DownloadWebPage. CodePage Differs (header:%d - page:%d)\r\n"),
							headerCP, pageCP);
					}
					if (pageCP > 0)	//===Choose the Codepage detected from page if available
						workingCP = pageCP;
					else if (headerCP > 0) //===Else select the Codepage detected from headers if available
						workingCP = headerCP;
					//===Otherwise keep the original 1252 (latin 1)
				}
				if (nSize > 0)
				{
					INT bfLen = MultiByteToWideChar(workingCP, 0, bf, nSize, 0, 0);
					if (bfLen < bufferSizeU)
					{
						MultiByteToWideChar(workingCP, 0, bf, nSize, bfU, bfLen);
						page.append(bfU, bfLen);
					}
					else
					{
						TRACE(_T("@1 DownloadWebPageUnicode. Unicode buffer not enough\r\n"));
						bReadSuccess = FALSE;
					}
				}
			}
			else
				HandleInternetError(_T("DownloadWebPageUnicode. InternetReadFile"));
		}
		InternetCloseHandle(hFile);
	}
	else
		HandleInternetError(_T("DownloadWebPageUnicode. InternetOpenUrl"));
	return bDownloadSuccess;
}



BOOL DownloadToFile(LPCTSTR fileName, HINTERNET hInternet, LPCWSTR url)
{
	TRACEST(_T("DownloadToFile"), (LPCTSTR)CW2CT(url));
	BOOL bDownloadSuccess = FALSE;
	if (hInternet == NULL)
		return FALSE;
	HINTERNET hFile = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (hFile)
	{   
		const int bufferSize = 8192;
		BYTE bf[bufferSize];
		unsigned long nSize = 0;
		BOOL bReadSuccess = TRUE;
		HANDLE f = CreateFile(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (f != INVALID_HANDLE_VALUE)
		{
			while(bReadSuccess)
			{
				bReadSuccess = InternetReadFile(hFile, bf, bufferSize, &nSize);
				if (bReadSuccess)
				{
					if (nSize == 0)
					{
						TRACE(_T("@3 DownloadToFile. InternetReadFile Finished\r\n"));
						bDownloadSuccess = TRUE;
						break;
					}
					DWORD bWritten = 0;
					if (WriteFile(f, bf, nSize, &bWritten, NULL) == 0)
					{
						bReadSuccess = FALSE;
						TRACE(_T("@3 DownloadToFile. WriteFile Failed [%d]\r\n"), GetLastError());
					}
				}
				else
					HandleInternetError(_T("DownloadToFile. InternetReadFile"));
			}
			CloseHandle(f);
		}
		else
			TRACE(_T("@3 DownloadToFile. CreateFile Failed [%d]\r\n"), GetLastError());
		InternetCloseHandle(hFile);
	}
	else
		HandleInternetError(_T("DownloadToFile. InternetOpenUrl"));
	return bDownloadSuccess;
}

UINT GetWindowsCodePageA(LPCSTR html)
{
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

	UINT codePage = 0;
	LPCSTR startPos = strstr(html, " charset=");
	if (startPos != NULL)
	{
		startPos += 9;
		LPCSTR endPos = strchr(startPos + 1, '"');
		if (endPos != NULL && endPos - startPos < 30)
		{
			CHAR bf[30];
			strncpy_s(bf, startPos, endPos - startPos);
			_strlwr_s(bf);
			if (_strnicmp(bf, "windows-", 8) == 0 && endPos - startPos > 8)
			{
				codePage = atol(&bf[8]);
			}
			else if (_strnicmp(bf, "iso-8859-", 9) == 0 && endPos - startPos > 9)
			{
				codePage = 28590 +  atol(&bf[9]);
			}
			else if (_strnicmp(bf, "utf-8", 5) == 0)
				codePage = 65001;
			else if (_strnicmp(bf, "utf-7", 5) == 0)
				codePage = 65000;
			else
			{
				TRACE(_T("@1 GetWindowsCodePageA. Unknown charset: '%s'\r\n"), (LPCTSTR)CA2CT(bf));
			}
		}
		else
			TRACE(_T("@1 GetWindowsCodePageA. Can't find END charset\r\n"));
	}
	else
		TRACE(_T("@1 GetWindowsCodePageA. Can't find charset\r\n"));

	return codePage;
}

UINT GetWindowsCodePageW(LPCWSTR html)
{
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

	UINT codePage = 0;
	LPCWSTR startPos = wcsstr(html, _T(" charset="));
	if (startPos != NULL)
	{
		startPos += 9;
		LPCWSTR endPos = wcschr(startPos, '"');
		if (endPos == NULL)
			endPos = wcschr(startPos, 13);
		if (endPos != NULL && endPos - startPos < 30)
		{
			WCHAR bf[30];
			wcsncpy_s(bf, startPos, endPos - startPos);
			_wcslwr_s(bf);
			if (_wcsnicmp(bf, _T("windows-"), 8) == 0 && endPos - startPos > 8)
			{
				codePage = _wtol(&bf[8]);
			}
			else if (_wcsnicmp(bf, _T("iso-8859-"), 9) == 0 && endPos - startPos > 9)
			{
				codePage = 28590 +  _wtol(&bf[9]);
			}
			else if (_wcsnicmp(bf, _T("utf-8"), 5) == 0)
				codePage = 65001;
			else if (_wcsnicmp(bf, _T("utf-7"), 5) == 0)
				codePage = 65000;
			else
			{
				TRACE(_T("@1 GetWindowsCodePageW. Unknown charset: '%s'\r\n"), bf);
			}
		}
		else
			TRACE(_T("@1 GetWindowsCodePageW. Can't find END charset\r\n"));
	}
	else
		TRACE(_T("@1 GetWindowsCodePageW. Can't find charset\r\n"));

	return codePage;
}

struct HtmlStringEntityMap
{
	LPCTSTR html;
	wchar_t res;
};

const HtmlStringEntityMap strentity2wchar_t[] = 
{
	{_T("quot;"), 34},
	{_T("amp;"), 38},
	{_T("apos;"), 39},
	{_T("lt;"), 60},
	{_T("rt;"), 62},
	{_T("ndash;"), 8211},
	{_T("mdash;"), 8212},
	{_T("laquo;"), 34},
	{_T("raquo;"), 34},
	{_T("lsquo;"), 8216},
	{_T("rsquo;"), 8217}
};

//Starting with 160
const LPCTSTR ISO88591_Entities[] =
{
	_T("nbsp;"), //&#160;
	_T("iexcl;"), //&#161;
	_T("cent;"), //&#162;
	_T("pound;"), //&#163;
	_T("curren;"), //&#164;
	_T("yen;"), //&#165;
	_T("brvbar;"), //&#166;
	_T("sect;"), //&#167;
	_T("uml;"), //&#168;
	_T("copy;"), //&#169;

	_T("ordf;"), //&#170;
	_T("laquo;"), //&#171;
	_T("not;"), //&#172;
	_T("shy;"), //&#173;
	_T("reg;"), //&#174;
	_T("macr;"), //&#175;
	_T("deg;"), //&#176;
	_T("plusmn;"), //&#177;
	_T("sup2;"), //&#178;
	_T("sup3;"), //&#179;

	_T("acute;"), //&#180;
	_T("micro;"), //&#181;
	_T("para;"), //&#182;
	_T("middot;"), //&#183;
	_T("cedil;"), //&#184;
	_T("sup1;"), //&#185;
	_T("ordm;"), //&#186;
	_T("raquo;"), //&#187;
	_T("frac14;"), //&#188;
	_T("frac12;"), //&#189;

	_T("frac34;"), //&#190;
	_T("iquest;"), //&#191;
	_T("Agrave;"), //&#192;
	_T("Aacute;"), //&#193;
	_T("Acirc;"), //&#194;
	_T("Atilde;"), //&#195;
	_T("Auml;"), //&#196;
	_T("Aring;"), //&#197;
	_T("AElig;"), //&#198;
	_T("Ccedil;"), //&#199;

	_T("Egrave;"), //&#200;
	_T("Eacute;"), //&#201;
	_T("Ecirc;"), //&#202;
	_T("Euml;"), //&#203;
	_T("Igrave;"), //&#204;
	_T("Iacute;"), //&#205;
	_T("Icirc;"), //&#206;
	_T("Iuml;"), //&#207;
	_T("ETH;"), //&#208;
	_T("Ntilde;"), //&#209;

	_T("Ograve;"), //&#210;
	_T("Oacute;"), //&#211;
	_T("Ocirc;"), //&#212;
	_T("Otilde;"), //&#213;
	_T("Ouml;"), //&#214;
	_T("times;"), //&#215;
	_T("Oslash;"), //&#216;
	_T("Ugrave;"), //&#217;
	_T("Uacute;"), //&#218;
	_T("Ucirc;"), //&#219;

	_T("Uuml;"), //&#220;
	_T("Yacute;"), //&#221;
	_T("THORN;"), //&#222;
	_T("szlig;"), //&#223;
	_T("agrave;"), //&#224;
	_T("aacute;"), //&#225;
	_T("acirc;"), //&#226;
	_T("atilde;"), //&#227;
	_T("auml;"), //&#228;
	_T("aring;"), //&#229;

	_T("aelig;"), //&#230;
	_T("ccedil;"), //&#231;
	_T("egrave;"), //&#232;
	_T("eacute;"), //&#233;
	_T("ecirc;"), //&#234;
	_T("euml;"), //&#235;
	_T("igrave;"), //&#236;
	_T("iacute;"), //&#237;
	_T("icirc;"), //&#238;
	_T("iuml;"), //&#239;

	_T("eth;"), //&#240;
	_T("ntilde;"), //&#241;
	_T("ograve;"), //&#242;
	_T("oacute;"), //&#243;
	_T("ocirc;"), //&#244;
	_T("otilde;"), //&#245;
	_T("ouml;"), //&#246;
	_T("divide;"), //&#247;
	_T("oslash;"), //&#248;
	_T("ugrave;"), //&#249;

	_T("uacute;"), //&#250;
	_T("ucirc;"), //&#251;
	_T("uuml;"), //&#252;
	_T("yacute;"), //&#253;
	_T("thorn;"), //&#254;
	_T("yuml;") //&#255;
};

//Check http://www.w3.org/TR/REC-html40/sgml/entities.html#iso-88591
//Implemented:
//	1. FULL: ISO 8859-1 characters (24.2)
//	2. NONE: symbols, mathematical symbols, and Greek letters (24.3)
//	3. PARTIAL: markup-significant and internationalization characters (24.4)

INT ReplaceHtmlEntities(std::basic_string<TCHAR>& htmlText)
{
	INT replaces = 0;
	size_t startPos = htmlText.find(L"&", 0);
	while (startPos != std::wstring::npos)
	{
		size_t nextEndPos = htmlText.find(L";", startPos + 1);
		if (nextEndPos == std::wstring::npos)
		{
			//=== Last &... wasn't an entity & we ve found EOF
			break;
		}
		BOOL bEntityIsValid = TRUE;
		size_t nextStartPos = htmlText.find(L"&", startPos + 1);
		if (nextStartPos != std::wstring::npos)
		{
			if (nextStartPos < nextEndPos)
			{
				//=== We ve found &..&..; pattern which is illegal
				bEntityIsValid = FALSE;
			}
		}
		if (bEntityIsValid)
			bEntityIsValid = ((nextEndPos - startPos) <= 8);
		if (bEntityIsValid)
		{
			wchar_t repVal = 0;
			LPCTSTR entityStart = &htmlText[startPos+1];
			if (*entityStart/*htmlText[startPos+1]*/ == '#')
			{
				//=== It is a int entity
				TCHAR intbf[10];
				INT copyChars = (INT) min(nextEndPos - startPos - 2, 9);
				_tcsncpy_s(intbf, &htmlText[startPos+2], copyChars);
				INT val = _wtoi(intbf);
				if (val > 0 && val < 0xFFFE)
					repVal = wchar_t(val);
			}
			else
			{
				//=== It is a string entity
				//=== Check the ISO88591 entities
				for (int i = 0; i < sizeof(ISO88591_Entities)/sizeof(LPCTSTR); i++)
				{
					if (_tcsncmp(ISO88591_Entities[i], entityStart, _tcslen(ISO88591_Entities[i])) == 0)
					{
						repVal = (wchar_t) (i + 160);
						break;
					}
				}

				//=== Check the "other" mapped entities
				if (repVal == 0)
				{
					for (int i = 0; i < sizeof(strentity2wchar_t)/sizeof(HtmlStringEntityMap); i++)
					{
						if (_tcsncmp(strentity2wchar_t[i].html, entityStart, _tcslen(strentity2wchar_t[i].html)) == 0)
						{
							repVal = strentity2wchar_t[i].res;
							break;
						}
					}
				}
			}
			if (repVal != 0)
			{
				htmlText.erase(startPos + 1, nextEndPos - startPos);
				htmlText[startPos] = repVal;
				replaces++;
			}
			nextStartPos = htmlText.find(L"&", startPos + 1);
		}
		startPos = nextStartPos;

	}
	//for (int i = 0; i < sizeof(html2text)/sizeof(HtmlEntityMap); i++)
	//	replaces += replace(htmlText, html2text[i].html, html2text[i].text);
	return replaces;
}

void GetTextFromHtmlFragment(std::wstring& text, LPCTSTR startFragment, LPCTSTR endFragment)
{
	ASSERT(startFragment != NULL);
	ASSERT(endFragment != NULL);
	text.clear();
	BOOL bInTag = FALSE;
	for (LPCTSTR pos = startFragment; pos < endFragment; pos++)
	{
		if (*pos == 0)
			break;
		switch (*pos)
		{
		case '<':
			bInTag = TRUE;
			break;
		case '>':
			bInTag = FALSE;
			break;
		default:
			if (!bInTag)
				text += *pos;
		}
	}
	if (!text.empty())
	{
		ReplaceHtmlEntities(text);
		trim(text, _T(" \t\n\r"));
	}
}

inline CHAR toHex(const BYTE &x)
{
	return x > 9 ? _T('A') + x-10: _T('0') + x;
}

BOOL URLEncodeA(std::string& encoded, LPCSTR source)
{
	LPCSTR pInBuf = source;
	while (*pInBuf)
	{
		if(isalnum(*pInBuf))
			encoded += *pInBuf;
		else
		{
			if(isspace(*pInBuf) && (*pInBuf != 13) && (*pInBuf!=10))
				encoded += '+';
			else
			{
				encoded += '%';
				encoded +=  toHex(*pInBuf>> 4);
				encoded +=  toHex(*pInBuf%16);
			}
		}
		pInBuf++;
	}
	return TRUE;
}

BOOL URLEncodeW(std::wstring& encoded, LPCWSTR source)
{
	ASSERT(source != NULL);
	if (source == NULL)
		return FALSE;
	if (*source == 0)
		return TRUE;
	//=== Convert the whole string at once: converting one WCHAR at a time turns
	//=== each half of a surrogate pair (emoji, CJK Ext. B...) into U+FFFD.
	INT len = WideCharToMultiByte(CP_UTF8, 0, source, -1, NULL, 0, NULL, NULL);
	if (len <= 0)
	{
		TRACE(_T("URLEncodeW WideCharToMultiByte Failed (%d)\r\n"), GetLastError());
		return FALSE;
	}
	std::string utf8(len, 0);
	WideCharToMultiByte(CP_UTF8, 0, source, -1, &utf8[0], len, NULL, NULL);
	utf8.resize(len - 1);	//=== drop the terminating NUL
	for (size_t i = 0; i < utf8.size(); i++)
	{
		BYTE tmp = (BYTE) utf8[i];
		if ((tmp >= 'A' && tmp <= 'Z') ||
			(tmp >= 'a' && tmp <= 'z') ||
			(tmp >= '0' && tmp <= '9'))
			encoded += (WCHAR) tmp;
		else
		{
			encoded += '%';
			encoded += toHex(tmp >> 4);
			encoded += toHex(tmp % 16);
		}
	}
	return TRUE;
}
