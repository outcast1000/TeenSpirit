//	Version resource of a module (by default the running executable).
#pragma once

struct FileVersion
{
	WORD Major;
	WORD Minor;
	WORD Build;
	WORD SubBuild;
	FileVersion():
	Major(0),Minor(0),Build(0),SubBuild(0)	{}
	FileVersion(WORD major, WORD minor=0, WORD build=0, WORD subBuild=0):
	Major(major),Minor(minor),Build(build),SubBuild(subBuild)	{}
	BOOL operator==(const FileVersion& rgt) const
	{
		return Major==rgt.Major && Minor==rgt.Minor && Build==rgt.Build && SubBuild==rgt.SubBuild;
	}
	BOOL operator>(const FileVersion& rgt) const
	{
		if (Major!=rgt.Major)
			return Major > rgt.Major;
		if (Minor!=rgt.Minor)
			return Minor > rgt.Minor;
		if (Build!=rgt.Build)
			return Build > rgt.Build;
		return SubBuild > rgt.SubBuild;
	}
	//bfLen should be at least 24
	void toString(LPTSTR bf, UINT bfLen) const
	{
		_sntprintf_s(bf, bfLen, _TRUNCATE, _T("%d.%d.%d.%d"), Major, Minor, Build, SubBuild);
		if (bfLen > 0)
			bf[bfLen - 1] = 0;
	}
};

BOOL GetFileVersion(FileVersion& fv, HMODULE module = 0);
