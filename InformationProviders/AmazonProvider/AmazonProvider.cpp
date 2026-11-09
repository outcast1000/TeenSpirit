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
#include "AmazonProvider.h"
//#include <memory>
#include "cStringUtils.h"
#include "stlStringUtils.h"
#include "WebPageUtilities.h"
#include <xmllite.h>
#pragma comment( lib, "xmllite.lib" )
#include "XmlLiteUtilities.h"
#include "hmac/hmac_sha2.h"
#include "atlenc.h"
#include "ApiKeys.h"

//Check: 
//http://www.awszone.com/
//http://www.awszone.com/scratchpads/aws/ecs.us/ItemSearch.aws
//http://docs.amazonwebservices.com/AWSEcommerceService/2005-02-23/TourOfEcs.html

LPCTSTR cAWSAccessKeyID = _T(JAANGLE_AWS_ACCESS_KEY_ID);



#ifdef _USRDLL
std::vector<IInfoProvider*>	s_createdIPs;
extern "C" __declspec(dllexport) UINT GetInfoProviderCount()
{
	return 1;
}
extern "C" __declspec(dllexport) IInfoProvider* CreateInfoProvider(UINT idx)
{
	if (idx == 0)
	{
		IInfoProvider* pIP = new AmazonProvider;
		s_createdIPs.push_back(pIP);
		return pIP;
	}
	return NULL;
}
extern "C" __declspec(dllexport) BOOL DeleteInfoProvider(IInfoProvider* pIP)
{
	std::vector<IInfoProvider*>::iterator it = s_createdIPs.begin();
	for (; it != s_createdIPs.end(); it++)
	{
		if (*it == pIP)
		{
			delete pIP;
			s_createdIPs.erase(it);
			return TRUE;
		}
	}
	return FALSE;
}
#endif


#ifdef _UNITTESTING
const int lastSizeOf = 664;

BOOL AmazonProvider::UnitTest()
{
	if (lastSizeOf != sizeof(AmazonProvider))
		TRACE(_T("TestAmazonProvider. Object Size Changed. Was: %d - Is: %d\r\n"), lastSizeOf, sizeof(AmazonProvider));

	AmazonProvider g;
	HINTERNET hNet = InternetOpen(_T("UnitTest"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);
	g.SetInternetHandle(hNet);
	//Successful Requests
	IInfoProvider::Request req(IInfoProvider::SRV_AlbumImage);
	req.artist = _T("Asian Dub Foundation");
	req.album = _T("R.A.F.I.");
	UNITTEST(g.OpenRequest(req));
	IInfoProvider::Result res;
	LPCTSTR tmpFile = _T("c:\\test.jpg");
	while (g.GetNextResult(res))
	{
		UNITTEST(res.IsValid());
		LPCTSTR fName = res.main;
		VERIFY(MoveFile(fName, tmpFile));
		INT a = GetLastError();
		ShellExecute(0, _T("open"), tmpFile, 0,0,0);
		int ret = MessageBox(0, _T("more pictures?"), _T("???"), MB_YESNO);
		VERIFY(DeleteFile(tmpFile));
		if (ret != IDYES)
			break;
	}

	req.service = SRV_AlbumReview;
	UNITTEST(g.OpenRequest(req));
	while (g.GetNextResult(res))
	{
		UNITTEST(res.IsValid());
		if (MessageBox(NULL, res.main, res.additionalInfo, MB_YESNO) != IDYES)
			break;
	}

	//Bad Request
	req.artist = _T("Asian Dub Foundation");
	req.album = _T("Kapoio asxeto");
	UNITTEST(g.OpenRequest(req));

	req.service = SRV_AlbumImage;

	while (g.GetNextResult(res))
	{
		UNITTEST(res.IsValid());
		LPCTSTR fName = res.main;
		VERIFY(MoveFile(fName, tmpFile));
		INT a = GetLastError();
		ShellExecute(0, _T("open"), tmpFile, 0,0,0);
		int ret = MessageBox(0, _T("more pictures?"), _T("???"), MB_YESNO);
		VERIFY(DeleteFile(tmpFile));
		if (ret != IDYES)
			break;
	}

	req.service = SRV_AlbumReview;
	while (g.GetNextResult(res))
	{
		UNITTEST(res.IsValid());
		if (MessageBox(NULL, res.main, res.additionalInfo, MB_YESNO) != IDYES)
			break;
	}



	g.SetInternetHandle(0);
	InternetCloseHandle(hNet);
	return TRUE;
}
#endif

static INT AmazonProviderInstances = 0;

AmazonProvider::AmazonProvider():
m_hNet(NULL),
m_curResult(0),
m_request(SRV_First)
{
	//I ve change it because .tmp files were left in the app dir (when closing the application while downloading)
	TCHAR tmpPath[MAX_PATH];
	GetTempPath(MAX_PATH, tmpPath);
	_sntprintf(m_TempFile, MAX_PATH, _T("%samz%d.tmp"), tmpPath, AmazonProviderInstances);
	AmazonProviderInstances++;
}

IInfoProvider* AmazonProvider::Clone() const
{
	IInfoProvider* pIP = new AmazonProvider;
	pIP->SetInternetHandle(GetInternetHandle());
	IConfigurableHelper::TransferConfiguration(*this, *pIP);
	return pIP;
}

AmazonProvider::~AmazonProvider()
{
	DeleteFile(m_TempFile);//Delete the file if it exists
}

BOOL AmazonProvider::CanHandle(ServiceEnum service) const
{
	switch (service)
	{
	case IInfoProvider::SRV_AlbumReview:
	case IInfoProvider::SRV_AlbumImage:
		return TRUE;
	}
	return FALSE;
}

BOOL AmazonProvider::OpenRequest(const Request& request)
{
	m_curResult = 0;
	ASSERT(request.IsValid());
	if (!request.IsValid())
		return FALSE;
	switch (request.service)
	{
	case IInfoProvider::SRV_AlbumReview:
	case IInfoProvider::SRV_AlbumImage:
		m_artist = request.artist;
		m_album = request.album;
		m_request.service = request.service;
		m_request.artist = m_artist.c_str();
		m_request.album = m_album.c_str();
		return TRUE;
	}
	return FALSE;
}

BOOL AmazonProvider::GetNextResult(Result& result)
{
	m_result.clear();
	switch (m_request.service)
	{
	case SRV_AlbumImage:
		GetAlbumPicture();
		m_curResult++;
		break;
	case SRV_AlbumReview:
		GetAlbumReview();
		m_curResult++;
		break;
	}
	result.main = m_result.c_str();
	result.additionalInfo = _T("amazon.com");
	result.service = m_request.service;
	ASSERT(result.IsValid());
	return !m_result.empty();
}

BOOL AmazonProvider::GetAlbumPicture()
{
	if (m_curResult > 0)
		return FALSE;
	std::basic_string<TCHAR> query;
	CreateAmazonURL(query, cAWSAccessKeyID, _T("Images"), m_artist.c_str(), m_album.c_str());
	std::string page;
	if (!DownloadWebPage(page, m_hNet, query.c_str()))
		return FALSE;
	CComPtr<IStream> pReadOnlyMemStream;
	CComPtr<IXmlReader> pReader;
	HRESULT hr;
	if (FAILED(ReadOnlyMemStream::Create((LPBYTE) &page[0], page.size(), FALSE, &pReadOnlyMemStream)))
		return FALSE;
	if (FAILED(hr = CreateXmlReader(__uuidof(IXmlReader), (void**) &pReader, NULL)))
		return FALSE;
	if (FAILED(hr = pReader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit)))
		return FALSE;
	if (FAILED(hr = pReader->SetInput(pReadOnlyMemStream)))
		return FALSE;
	XmlLiteReaderHelper hlp(*pReader);
	INT curDepth = hlp.GetCurrentDepth();
	if (hlp.FindElement(_T("ItemSearchResponse"), curDepth))
	{
		if (hlp.FindElement(_T("Items"), curDepth + 1))
		{
			BOOL bIsValid = FALSE;
			if (hlp.FindElement(_T("Request"), curDepth + 2))
			{
				if (hlp.FindElement(_T("IsValid"), curDepth + 3))
				{
					LPCTSTR ret = hlp.GetElementText();
					if (ret != NULL)
						bIsValid = (_tcsicmp(_T("True"), ret) == 0);
				}
			}
			if (bIsValid)
			{
				while (hlp.FindElement(_T("Item"), curDepth + 2))
				{
					if (hlp.FindElement(_T("LargeImage"), curDepth + 3))
					{
						if (hlp.FindElement(_T("URL"), curDepth + 4))
						{
							LPCTSTR ret = hlp.GetElementText();
							if (ret != NULL)
							{
								if (DownloadToFile(m_TempFile, m_hNet, ret))
								{
									m_result = m_TempFile;
									return TRUE;
								}
								else
									TRACE(_T("@1 AmazonProvider::DownloadPicture Failed\r\n"));
							}
						}
					}
				}
			}
		}
	}
	return FALSE;
}


BOOL AmazonProvider::GetAlbumReview()
{
	if (m_curResult > 0)
		return FALSE;
	m_result.clear();
	std::basic_string<TCHAR> query;
	CreateAmazonURL(query, cAWSAccessKeyID, _T("EditorialReview,Tracks"), m_artist.c_str(), m_album.c_str());
	std::string page;
	if (!DownloadWebPage(page, m_hNet, query.c_str()))
		return FALSE;
	CComPtr<IStream> pReadOnlyMemStream;
	CComPtr<IXmlReader> pReader;
	HRESULT hr;
	if (FAILED(ReadOnlyMemStream::Create((LPBYTE) &page[0], page.size(), FALSE, &pReadOnlyMemStream)))
		return FALSE;
	if (FAILED(hr = CreateXmlReader(__uuidof(IXmlReader), (void**) &pReader, NULL)))
		return FALSE;
	if (FAILED(hr = pReader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit)))
		return FALSE;
	if (FAILED(hr = pReader->SetInput(pReadOnlyMemStream)))
		return FALSE;
	XmlLiteReaderHelper hlp(*pReader);
	INT curDepth = hlp.GetCurrentDepth();
	if (hlp.FindElement(_T("ItemSearchResponse"), curDepth))
	{
		if (hlp.FindElement(_T("Items"), curDepth + 1))
		{
			BOOL bIsValid = FALSE;
			if (hlp.FindElement(_T("Request"), curDepth + 2))
			{
				if (hlp.FindElement(_T("IsValid"), curDepth + 3))
				{
					LPCTSTR ret = hlp.GetElementText();
					if (ret != NULL)
						bIsValid = (_tcsicmp(_T("True"), ret) == 0);
				}
			}
			if (bIsValid)
			{
				if (hlp.FindElement(_T("Item"), curDepth + 2))
				{
					if (hlp.FindElement(_T("EditorialReviews"), curDepth + 3))
					{
						if (hlp.FindElement(_T("EditorialReview"), curDepth + 4))
						{
							if (hlp.FindElement(_T("Content"), curDepth + 5))
							{
								LPCTSTR ret = hlp.GetElementText();
								if (ret != NULL)
								{
									if (_tcsistr(ret, _T("</td>")) == 0)
									{
										m_result = ret;
										replace(m_result, _T("<i>"), _T(""));
										replace(m_result, _T("</i>"), _T(""));
										replace(m_result, _T("<I>"), _T(""));
										replace(m_result, _T("</I>"), _T(""));
										size_t pos = m_result.find('<');
										if (pos != std::tstring::npos && pos > 0)
											m_result.resize(pos);
									}
								}
							}
						}
					}
					if (hlp.FindElement(_T("Tracks"), curDepth + 3))
					{
						while (hlp.FindElement(_T("Disc"), curDepth + 4))
						{
							m_result += _T("\r\n");
							while (hlp.FindElement(_T("Track"), curDepth + 5))
							{
								if (pReader->MoveToAttributeByName(_T("Number"), NULL) == S_OK)
								{
									LPCTSTR sNum = NULL;
									if (pReader->GetValue(&sNum, NULL) == S_OK && sNum != NULL)
									{
										INT number = _ttoi(sNum);
										LPCTSTR sName = hlp.GetElementText();
										if (sName != NULL)
										{
											TCHAR bf[500];
											_sntprintf(bf, 500, _T("%d. %s\r\n"), number, sName);
											m_result += bf;
										}
									}
								}
							}
						}
					}
					return !m_result.empty();
				}
			}
		}
	}
	return FALSE;
}


LPCTSTR AmazonProvider::GetModuleInfo(ModuleInfo mi) const
{
	switch (mi)
	{
	case IPI_UniqueID:		return _T("AMZN");
	case IPI_Name:			return _T("Amazon");
	case IPI_Author:		return _T("Alex Economou");
	case IPI_VersionStr:	return _T("1.2");
	case IPI_VersionNum:	return _T("3");
	case IPI_Description:	return _T("Downloads info from amazon.com");
	case IPI_HomePage:		return _T("http://teenspirit.artificialspirit.com");
	case IPI_Email:			return _T("alex@artificialspirit.com");
	}
	return NULL;
}

void AmazonProvider::CreateAmazonURL(std::basic_string<TCHAR>& query, 
	LPCTSTR AWSAccessKeyID, LPCTSTR respGroup, LPCTSTR artist, LPCTSTR album)
{
	ASSERT(AWSAccessKeyID != NULL);
	ASSERT(respGroup != NULL);
	ASSERT(artist != NULL);
	ASSERT(album != NULL);
	std::tstring fixedArtist, fixedAlbum;
	URLEncode(fixedArtist, artist);
	URLEncode(fixedAlbum, album);
	std::tstring fixedRespGroup;
	URLEncode(fixedRespGroup, respGroup);

	SYSTEMTIME st;
	GetSystemTime(&st);
	TCHAR timeStamp[200];
	_sntprintf(timeStamp, 200, _T("%d-%02d-%02dT%02d%%3A%02d%%3A%02d"), 
		(INT)st.wYear, (INT)st.wMonth, (INT)st.wDay, 
		(INT)st.wHour, (INT)st.wMinute, (INT)st.wSecond);

	//LPCTSTR query = _T("http://www.amazon.com/onca/xml?");

	//=== See http://docs.amazonwebservices.com/AWSECommerceService/latest/DG/index.html?RequestAuthenticationArticle.html

	std::tstring params;

	params += _T("AWSAccessKeyId=");
	params += AWSAccessKeyID;

	params += _T("&Artist=");
	params += fixedArtist;

	std::tstring fixedAssociateTag = _T("httpwwwartifi-20");
	//URLEncode(fixedAssociateTag,  _T("httpwwwartifi-20"));
	params += _T("&AssociateTag=");
	params += fixedAssociateTag;

	params += _T("&Operation=ItemSearch");


	params += _T("&ResponseGroup=");
	params += fixedRespGroup;
	
	params += _T("&SearchIndex=Music");
	
	params += _T("&Service=AWSECommerceService");
	
	params += _T("&Timestamp=");
	params += timeStamp;

	params += _T("&Title=");
	params += fixedAlbum;

	params += _T("&Version=2009-01-06");


	std::tstring stringToSign(	_T("GET\n")
		_T("www.amazon.com\n")
		_T("/onca/xml\n"));
	stringToSign += params;
	CHAR sig[32 + 1];
	CHAR utf8[2000];
	INT utf8Len = WideCharToMultiByte(CP_UTF8, 0, stringToSign.c_str(), stringToSign.size(), utf8, 2000, 0, 0);
	utf8[utf8Len] = 0;
	LPCSTR secretKey = JAANGLE_AWS_SECRET_KEY;
	hmac_sha256(	(unsigned char *)secretKey, strlen(secretKey), 
					(unsigned char *)utf8, utf8Len, 
					(unsigned char *)sig, 32);
	CHAR base64[60];
	int base64len = 60;
	if (Base64Encode((const BYTE*)sig, 32, base64, &base64len, ATL_BASE64_FLAG_NOCRLF))
	{
		base64[base64len] = 0;
		std::basic_string<CHAR> urlEncoded(base64);
		replace<CHAR>(urlEncoded, "+", "%2B");
		replace<CHAR>(urlEncoded, "=", "%3D");
		query = _T("http://www.amazon.com/onca/xml?");
		query += params;
		query += _T("&Signature=");
		query += CA2CT(urlEncoded.c_str());
	}
	else
		ASSERT(0);



}