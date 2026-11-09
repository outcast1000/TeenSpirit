# Building Jaangle

## Requirements

- Visual Studio 2026 (or Build Tools 2026) with:
  - **Desktop development with C++** (MSVC v145, x86/x64)
  - **C++ ATL for latest build tools** and **C++ MFC for latest build tools**
  - **Windows 11 SDK** (10.0.26100 or newer)
  - **vcpkg package manager** (bundled component), or a vcpkg clone pointed to by `VCPKG_ROOT`
- Git (vcpkg uses it to fetch the registry baseline)

Third-party C++ dependencies (SQLite, TagLib 2, zlib, nlohmann-json) are declared in `vcpkg.json` and
are restored automatically on the first build into `vcpkg_installed\`.

## Build

Open `Jaangle.sln` in Visual Studio, or from a Developer Command Prompt:

```
msbuild Jaangle.sln -p:Configuration=Release -p:Platform=Win32 -m
```

Output goes to `build\out\Win32\<Configuration>\` (`jaangle.exe` / `jaangledbg.exe`,
`XCrashReport.exe`, `UnitTests.exe`). Intermediate files go to `build\obj\`.

Only **Win32** is configured for now. The database no longer needs 32-bit components;
the remaining x64 blockers are the x86-only BASS import libraries and the code itself
(never built for 64-bit).

## Tests

`UnitTests.exe` (project `tests\UnitTests`) prints the failures and returns their count:

- **Database**: records, every query, case-insensitive matching, two connections in
  parallel, and importing a real Jet `music.mdb`.
- **Job queue**: concurrency limit, COM on workers, pause/resume, abort, start delay,
  no CPU use while idle, and shutdown never terminating threads.
- **Last.fm worker**: started on demand and joined on destruction.
- **Web search templates**: `WebServices\*.wsxml` load and expand to encoded URLs.
- **Tagger**: tags written and read back on generated MP3, WAV and FLAC files, including
  the ID3v2 frames (album artist, composer, lyrics, rating) other players read.
- **Online info providers**: API response parsing (offline). Set `JAANGLE_LIVE_TESTS=1`
  to also query the real services (and `JAANGLE_LASTFM_API_KEY` for Last.fm).

```
build\out\Win32\Release\UnitTests.exe
```

The import test creates a Jet database, so it needs the Jet 4.0 OLE DB provider
(it ships with Windows, as a 32-bit component only).

## Continuous integration

`.github/workflows/ci.yml` builds Debug and Release (Win32) on the `windows-2025-vs2026`
GitHub runner (the only image with the v145 toolset) and runs `UnitTests.exe` on every
push to `main` and on pull requests. vcpkg binaries are cached between runs. The Release
executables and PDBs are uploaded as a build artifact (without the BASS DLLs).

The live online-API tests run weekly (Monday 06:00 UTC) and on a manual run with the
**live** option ("Run workflow" in the Actions tab). The Last.fm part uses the
`JAANGLE_LASTFM_API_KEY` repository secret and is skipped when it isn't set.

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

The player needs the BASS audio library DLLs. They are not in the repository (BASS is
free for non-commercial use only, see <https://www.un4seen.com/bass.html#license>).
Fetch them once from the official site:

```
powershell -ExecutionPolicy Bypass -File tools\fetch-bass.ps1
```

This downloads BASS 2.4, BASS_FX and the format add-ons (WMA, CD, FLAC, MIDI, APE,
WavPack, MPC, AC3, AAC) into `build\deps\bass\`. The build then copies them, together
with `Skins\`, `Langs\` and `WebServices\`, next to the executable, so `build\out\Win32\<Configuration>\jaangle.exe`
can be started directly. `-UpdateSdk` also refreshes the tracked headers and import
libraries in `Bass\`.

The program keeps its settings (`jaangle.ini`) and database (`music.db`) next to the
executable.

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

## Known build compromises (to be removed)

- C++14, `/permissive` (conformance mode off) and warning C4996 disabled, to keep the
  legacy code compiling unchanged.
