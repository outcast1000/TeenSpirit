# Building Jaangle

## Requirements

- Visual Studio 2026 (or Build Tools 2026) with:
  - **Desktop development with C++** (MSVC v145, x86/x64)
  - **C++ ATL for latest build tools** and **C++ MFC for latest build tools**
  - **Windows 11 SDK** (10.0.26100 or newer)
  - **vcpkg package manager** (bundled component), or a vcpkg clone pointed to by `VCPKG_ROOT`
- Git (vcpkg uses it to fetch the registry baseline)

Third-party C++ dependencies (SQLite and zlib) are declared in `vcpkg.json` and
are restored automatically on the first build into `vcpkg_installed\`.

## Build

Open `Jaangle.sln` in Visual Studio, or from a Developer Command Prompt:

```
msbuild Jaangle.sln -p:Configuration=Release -p:Platform=Win32 -m
```

Output goes to `build\out\Win32\<Configuration>\` (`jaangle.exe` / `jaangledbg.exe`,
`XCrashReport.exe`, `DatabaseTests.exe`). Intermediate files go to `build\obj\`.

Only **Win32** is configured for now. The database no longer needs 32-bit components;
the remaining x64 blockers are the x86-only BASS import libraries and the code itself
(never built for 64-bit).

## Tests

`DatabaseTests.exe` exercises the database layer end to end (records, every query,
case-insensitive matching, two connections in parallel, and importing a real Jet
`music.mdb`). It prints the failures and returns their count:

```
build\out\Win32\Release\DatabaseTests.exe
```

The import test creates a Jet database, so it needs the Jet 4.0 OLE DB provider
(it ships with Windows, as a 32-bit component only).

## Database

The library is stored in `music.db` (SQLite, WAL mode) next to the program data.
On the first start without a `music.db`, an existing `music.mdb` (the Jet database
used up to 0.98f) is imported automatically with all IDs preserved; the `.mdb` is
left untouched. Importing needs the 32-bit build (the Jet provider is 32-bit only).

Text columns use the custom collation `NOCASE_U` (Unicode, case-insensitive) and
`LIKE` is overridden to fold case for all scripts, matching Jet. Generic SQLite
tools can read the file, but queries that compare or sort those columns need the
collation registered (`CommonSources\SQLite\SQLiteDB.cpp`).

## Running

The executable needs `bass.dll` and `bass_fx.dll` (x86) next to it. They are not in
the repository; download BASS and BASS_FX from <https://www.un4seen.com/> (check
their license for redistribution).

## Online-service credentials

See `ApiKeys.h`. Real keys go in the gitignored `ApiKeys.local.h`, enabled by
defining `JAANGLE_LOCAL_KEYS`.

## Known build compromises (to be removed)

- C++14, `/permissive` (conformance mode off) and warning C4996 disabled, to keep the
  legacy code compiling unchanged.
- `/SAFESEH:NO` on Win32: the bundled `Bass\*.lib` import libraries predate SafeSEH.
  Replacing them with current BASS import libraries allows re-enabling it.
- TagLib is the vendored ~1.6 copy in `CommonSources\TagLib`, not the vcpkg port.
