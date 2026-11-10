//	Where the program keeps its settings and data.
//
//	Portable: a "portable.txt" file next to the executable (the build puts one in
//	build\out; a portable zip ships one). Everything stays next to the executable:
//	settings in <exe name>.ini, Storage\, User\ and music.db.
//
//	Installed (no marker, e.g. under Program Files): %APPDATA%\Jaangle\ with the layout
//	of the old installed Jaangle 0.98, so its data is picked up as it is:
//	Settings.ini, Storage\, Storage\Database\music.db and LastTracks.m3u (user root).
//
//	The ini keys StoragePath, UserPath and DatabasePath still override the defaults.
#ifndef _AppDataPaths_h_
#define _AppDataPaths_h_

#include <string>

struct AppDataLayout
{
	bool bPortable = false;
	std::wstring dataRoot;		//settings folder; crash log and debug log go here. Ends with '\'.
	std::wstring settingsFile;
	std::wstring storageRoot;	//Ends with '\'
	std::wstring userRoot;		//Ends with '\'
	std::wstring databaseRoot;	//Folder of music.db. Ends with '\'
};

namespace AppDataPaths
{
	extern const wchar_t* const cPortableMarker;	//"portable.txt"
	extern const wchar_t* const cAppFolder;			//"Jaangle"

	//Pure: computes the layout. exePath is the executable's full path, appDataFolder the
	//user's roaming AppData folder (used only when not portable).
	AppDataLayout Resolve(const std::wstring& exePath, const std::wstring& appDataFolder, bool bPortableMarker);

	//The layout for this process (computed once; creates the data root).
	const AppDataLayout& Get();
}

#endif
