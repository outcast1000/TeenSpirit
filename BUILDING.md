# Building Jaangle

## Requirements

- Visual Studio 2026 (or Build Tools 2026) with:
  - **Desktop development with C++** (MSVC v145, x86/x64)
  - **C++ ATL for latest build tools** and **C++ MFC for latest build tools**
  - **Windows 11 SDK** (10.0.26100 or newer)
  - **vcpkg package manager** (bundled component), or a vcpkg clone pointed to by `VCPKG_ROOT`
- Git (vcpkg uses it to fetch the registry baseline)

Third-party C++ dependencies (SQLite, TagLib 2, zlib, nlohmann-json) are declared in `vcpkg.json` and
are restored automatically on the first build into `vcpkg_installed\<Platform>\` (one tree per
platform, since vcpkg's manifest mode removes the packages of other triplets).

## Build

Open `Jaangle.sln` in Visual Studio, or from a Developer Command Prompt:

```
msbuild Jaangle.sln -p:Configuration=Release -p:Platform=x64 -m
```

Platforms are **Win32** and **x64** (Debug and Release each). Output goes to
`build\out\<Platform>\<Configuration>\` (`jaangle.exe` / `jaangledbg.exe`,
`XCrashReport.exe`, `UnitTests.exe`). Intermediate files go to `build\obj\`.

Both builds have the same features. The visualization pane draws a built-in spectrum
(`MediaPlayers\NativeVisualization.cpp`); Winamp visualization plugins are no longer
supported.

In x64 builds warning C4267 (`size_t` to `int`) is disabled (see `Directory.Build.targets`);
the pointer-truncation warnings are on in both.

## Tests

`UnitTests.exe` (project `tests\UnitTests`) prints the failures and returns their count:

- **Database**: records, every query, case-insensitive matching, two connections in
  parallel, reopening, and that a leftover `music.mdb` is ignored.
- **Job queue**: concurrency limit, COM on workers, pause/resume, abort, start delay,
  no CPU use while idle, and shutdown never terminating threads.
- **Last.fm worker**: started on demand and joined on destruction.
- **Web search templates**: `WebServices\*.wsxml` load and expand to encoded URLs.
- **Tagger**: tags written and read back on generated MP3, WAV and FLAC files, including
  the ID3v2 frames (album artist, composer, lyrics, rating) other players read.
- **Online info providers**: API response parsing (offline). Set `JAANGLE_LIVE_TESTS=1`
  to also query the real services (and `JAANGLE_LASTFM_API_KEY` for Last.fm).

```
build\out\x64\Release\UnitTests.exe
```

## Continuous integration

`.github/workflows/ci.yml` builds Debug and Release for Win32 and x64 on the
`windows-2025-vs2026` GitHub runner (the only image with the v145 toolset) and runs
`UnitTests.exe` on every push to `main` and on pull requests. vcpkg binaries are cached
between runs (per triplet). The Release executables and PDBs of both platforms are
uploaded as build artifacts (without the BASS DLLs), and the Release x64 job also fetches
BASS and builds the MSI.

The live online-API tests run weekly (Monday 06:00 UTC) and on a manual run with the
**live** option ("Run workflow" in the Actions tab). The Last.fm part uses the
`JAANGLE_LASTFM_API_KEY` repository secret and is skipped when it isn't set.

### Releases

`.github/workflows/release.yml` publishes a GitHub release when a `v*` tag is pushed.
The tag must match `VersionNo.h` (major.minor.build), since the MSI upgrades by the exe's
file version. Bump `VersionNo.h`, commit, then:

```
git tag v0.98.973
git push origin v0.98.973
```

It builds Release for Win32 and x64 with BASS, runs the unit tests and attaches to the
release: `Jaangle-<version>-x64.msi`, a portable zip per platform (exe, BASS DLLs,
`Skins\`, `Langs\`, `WebServices\` and `portable.txt`) and a zip with `jaangle.pdb` per
platform. Release notes are generated from the commits since the previous tag.

## Database

The library is stored in `music.db` (SQLite, WAL mode); see below for where.
The Jet database used up to 0.98f (`music.mdb`) is no longer read: the one-time import
was removed after converting the existing library (it is in the git history, last in
commit 1af84ba, should another `.mdb` ever need converting; it needs the 32-bit build).

Text columns use the custom collation `NOCASE_U` (Unicode, case-insensitive) and
`LIKE` is overridden to fold case for all scripts, matching Jet. Generic SQLite
tools can read the file, but queries that compare or sort those columns need the
collation registered (`CommonSources\SQLite\SQLiteDB.cpp`).

## Running

The player needs the BASS audio library DLLs. They are not in the repository (BASS is
free for non-commercial use only, see <https://www.un4seen.com/bass.html#license>).
Fetch them once from the official site:

```
powershell -ExecutionPolicy Bypass -File tools\fetch-bass.ps1
```

This downloads BASS 2.4, BASS_FX and the format add-ons (WMA, CD, FLAC, MIDI, APE,
WavPack, MPC, AC3, AAC), x86 and x64, into `build\deps\bass\`. The build then copies the
ones for its platform, together with `Skins\`, `Langs\` and `WebServices\`, next to the
executable, so `build\out\<Platform>\<Configuration>\jaangle.exe` can be started directly.
`-UpdateSdk` also refreshes the tracked headers and import libraries in `Bass\` (x86) and
`Bass\x64\`.

## Where settings and data live

Decided at startup by `CommonSources\AppDataPaths.cpp`:

- **Portable**: a `portable.txt` next to the executable. Settings (`jaangle.ini`, or
  `jaangledbg.ini` for the Debug build), `music.db`, `Storage\` and `User\` are kept next to
  it. The build copies `build\portable.txt` into `build\out\...`, so development runs
  never touch the installed program's data.
- **Installed** (no marker): `%APPDATA%\Jaangle\` with the layout of the old installed
  Jaangle 0.98, whose data is picked up as it is: `Settings.ini`, `LastTracks.m3u`,
  `Storage\` (pictures) and `Storage\Database\music.db`.

The crash log (`crashlog.txt`) and `debug.log` go to the same folder. The ini keys
`StoragePath`, `UserPath` and `DatabasePath` (`[General]`) still override the defaults.
A saved skin path that no longer exists (e.g. the old install under Program Files (x86))
is looked up again by its folder name in the program's `Skins\`.

## Installer

`installer\Jaangle.wixproj` (WiX 5, restored from NuGet) builds an x64 MSI of the
Release build: per machine into `Program Files\Jaangle`, with a Start menu shortcut and
major upgrades by version (the exe's file version). It contains `jaangle.exe`, the BASS
DLLs and the repository's `Skins\`, `Langs\` and `WebServices\`. Uninstalling leaves
`%APPDATA%\Jaangle` in place. The crash reporter (`XCrashReport.exe`) is not included:
the program does not start it under that name and its upload service is gone.

```
msbuild Jaangle.sln -p:Configuration=Release -p:Platform=x64 -m
msbuild installer\Jaangle.wixproj -restore -p:Configuration=Release -p:Platform=x64
```

Output: `build\out\installer\Jaangle-x64.msi` (unsigned, so SmartScreen warns on first
run). CI builds it in the Release x64 job and uploads it as the `jaangle-x64-installer`
artifact.

## Windows integration

- **Media controls** (Windows 10+, `MediaTransportControls.cpp`): the current track,
  its cover and the play state appear in the Windows media overlay and lock screen, and
  their buttons control the player. Windows then delivers the keyboard media keys too, so
  the program no longer registers them as global hotkeys (it still does on Windows 7/8).
  WinRT is called through WRL and its DLLs are delay-loaded, so the program still starts
  on Windows 7.
- **High DPI**: the program is DPI-unaware (the UI is laid out in pixels) and Windows
  scales it; the manifest enables GDI scaling so text stays sharp. Images are stretched.

## Threads

Background work runs on `MultiThreadJobQueue` workers (COM initialized as STA on
each) and on the Last.fm worker. Threads are never terminated: on shutdown they are
asked to stop and waited for. Network calls in progress are cancelled by closing the
shared WinInet handle, which `PrgAPI` does before destroying the services.

## Online information

Album covers, lyrics, artist pictures and biographies are downloaded from public
web APIs (`InformationProviders\OnlineInfoProvider.cpp`):

| Information | Service | Key |
|---|---|---|
| Album covers | MusicBrainz + Cover Art Archive | none |
| Lyrics | LRCLIB | none |
| Artist pictures | Deezer | none |
| Artist biographies, album descriptions | Last.fm API 2.0 | free API key |

The Last.fm key can be entered in the program (Options, information providers,
Last.fm), or compiled in: see `ApiKeys.h` (real keys go in the gitignored
`ApiKeys.local.h`, enabled by defining `JAANGLE_LOCAL_KEYS`).

## Compiler settings

C++17 with `/permissive-`, Unicode, static MFC, `WINVER` = Windows 7. The build is
warning-free, and deprecated CRT calls are not suppressed (no `_CRT_SECURE_NO_WARNINGS`,
C4996 on), so an unsafe string call (`_tcscpy`, `sprintf`, ...) shows up as a warning.
Use the secure overloads, e.g. `_sntprintf_s(buffer, _TRUNCATE, ...)` and
`_tcsncpy_s(buffer, src, _TRUNCATE)` (they take the size from an array), or
`StrCopyN` (`CommonSources\cStringUtils.h`) for substrings.
