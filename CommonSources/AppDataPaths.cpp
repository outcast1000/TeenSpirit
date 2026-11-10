#include "stdafx.h"
#include "AppDataPaths.h"
#include <shlobj.h>

const wchar_t* const AppDataPaths::cPortableMarker = L"portable.txt";
const wchar_t* const AppDataPaths::cAppFolder = L"Jaangle";

namespace
{
	std::wstring WithSlash(std::wstring path)
	{
		if (!path.empty() && path.back() != L'\\' && path.back() != L'/')
			path.push_back(L'\\');
		return path;
	}
}

AppDataLayout AppDataPaths::Resolve(const std::wstring& exePath, const std::wstring& appDataFolder, bool bPortableMarker)
{
	AppDataLayout l;
	const std::wstring::size_type slash = exePath.find_last_of(L"\\/");
	const std::wstring exeDir = slash == std::wstring::npos ? std::wstring() : exePath.substr(0, slash + 1);
	std::wstring exeName = slash == std::wstring::npos ? exePath : exePath.substr(slash + 1);
	const std::wstring::size_type dot = exeName.find_last_of(L'.');
	if (dot != std::wstring::npos)
		exeName.resize(dot);

	l.bPortable = bPortableMarker || appDataFolder.empty();
	if (l.bPortable)
	{
		l.dataRoot = exeDir;
		l.settingsFile = exeDir + exeName + L".ini";
		l.storageRoot = exeDir + L"Storage\\";
		l.userRoot = exeDir + L"User\\";
		l.databaseRoot = exeDir;
	}
	else
	{
		l.dataRoot = WithSlash(appDataFolder) + cAppFolder + L"\\";
		l.settingsFile = l.dataRoot + L"Settings.ini";
		l.storageRoot = l.dataRoot + L"Storage\\";
		l.userRoot = l.dataRoot;
		l.databaseRoot = l.storageRoot + L"Database\\";
	}
	return l;
}

const AppDataLayout& AppDataPaths::Get()
{
	static const AppDataLayout layout = []()
	{
		wchar_t exePath[MAX_PATH] = {0};
		GetModuleFileNameW(NULL, exePath, MAX_PATH);
		std::wstring marker(exePath);
		const std::wstring::size_type slash = marker.find_last_of(L'\\');
		marker.resize(slash == std::wstring::npos ? 0 : slash + 1);
		marker += cPortableMarker;
		const bool bPortable = GetFileAttributesW(marker.c_str()) != INVALID_FILE_ATTRIBUTES;

		std::wstring appData;
		if (!bPortable)
		{
			PWSTR p = NULL;
			if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &p)))
				appData = p;
			CoTaskMemFree(p);
		}
		//No AppData folder (should not happen): fall back to portable.
		AppDataLayout l = Resolve(exePath, appData, bPortable);
		SHCreateDirectoryExW(NULL, l.dataRoot.c_str(), NULL);
		return l;
	}();
	return layout;
}
