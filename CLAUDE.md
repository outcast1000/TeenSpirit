# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

TeenSpirit (renamed Jaangle ~2010): a legacy MFC music player/library organizer (2003–2010), being
modernized to build and run on current Windows. See `README.md` for features and `BUILDING.md` for
full build/run/database/threading details — keep `BUILDING.md` updated when those change.

## Commands

MSBuild lives in the VS 2026 Build Tools on F: (not on PATH by default):

```
"F:\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe" Jaangle.sln -p:Configuration=Release -p:Platform=Win32 -m
```

- Only **Win32** is configured (Debug and Release). Projects: `TeenSpirit.vcxproj` (the app),
  `CrashReport\XCrashReport.vcxproj`, `tests\UnitTests\UnitTests.vcxproj`.
- Output: `build\out\Win32\<Config>\` (`jaangle.exe` / `jaangledbg.exe`, `UnitTests.exe`);
  intermediates in `build\obj\` (set by `Directory.Build.props`).
- vcpkg dependencies (sqlite3, taglib, zlib, nlohmann-json) in `vcpkg.json` restore on first build.
- One-time: `powershell -ExecutionPolicy Bypass -File tools\fetch-bass.ps1` downloads BASS DLLs to
  `build\deps\bass\` (not in repo — licensing). The build copies them plus `Skins\`, `Langs\`,
  `WebServices\` next to the exe, so the exe runs from the output dir. Settings (`jaangle.ini`) and
  `music.db` live next to the exe.
- The app's close button minimizes to tray; quit via tray Exit.

### Tests

`build\out\Win32\<Config>\UnitTests.exe` — custom framework (`tests\UnitTests\TestFramework.h`),
prints failures, exit code = failure count. No per-test filter: suites are called from
`tests\UnitTests\main.cpp` (`RunDatabaseTests`, `RunJobQueueTests`, ...); comment out calls there to
run a subset. The test project compiles selected app sources directly (listed in
`UnitTests.vcxproj`) — add a source file there when testing new code. `JAANGLE_LIVE_TESTS=1` also
queries the real online APIs (`JAANGLE_LASTFM_API_KEY` for Last.fm); all must pass. The Jet import
test needs the 32-bit Jet 4.0 OLE DB provider.

## Architecture

- **`PrgAPI`** (`PrgAPI.h`, accessed via `PRGAPI()`, ~600 call sites) is a singleton god object that
  owns and lazily creates every service: `SQLManager`, `MediaPlayer`, `CollectionManager`,
  `InfoDownloadManager`, `InfoProviderFactory`, `StateManager`, `SkinManager`,
  `TranslationManager`, `LastFMServices`, trackers, hotkeys, menus, dialogs, and the shared WinInet
  handle. It is being decoupled incrementally: services move out into classes that take their
  dependencies explicitly (`AppOptions` over `AppSettings`, `AppStrings` over `ITranslation`, `AppResources` for fonts and
  icons over a resource module), with
  PrgAPI keeping thin delegating methods so call sites don't change. New code should take those
  dependencies directly instead of calling `PRGAPI()`.
- **`StateManager`** is the app-wide event bus: components implement `IStateChangeListener`
  (`OnStateChanged(UINT)`) and receive `SM_*` messages (`SM_MediaChanged`, `SM_DatabaseUpdated`,
  `SM_CurrentSectionChanged`, ...) via `SendMessage`/`PostMessage`.
- **UI**: `TeenSpiritDlg` is the main window. Its content is a tree of panes managed by
  `PaneController` / `TSPaneContainer`; each pane type implements `IPane`
  (`CommonSources\IPane.h`) and is registered through an `IPaneCreator` (`*PaneCreator.cpp`:
  SectionsPane, TrackListPane, PlayListPane, InfoPane, WebPane, VideoPane, VisualizationPane).
  Skinning (`TSSkinnable`) and translation (`TSTranslatable`, `Langs\*.lng`) are mixins.
- **Database** (`Database\`): `SQLManager` over SQLite (`CommonSources\SQLite\SQLiteDB.cpp`) with
  record types in `DataRecords.h`. Text columns use the custom `NOCASE_U` collation and an overridden
  `LIKE` (Jet-compatible case folding). `JetImporter` imports a legacy `music.mdb` once (32-bit only,
  uses ADO). Background jobs (e.g. `CollectionUpdateJob`) use their own `SQLManager` connection.
- **Playback** (`MediaPlayers\`): `MediaPlayer` drives a `MediaPlayerEngine` — BASS engines
  (`BassEngine`, `FadeBassEngine` for crossfade), `DShowEngine` for video, `MultiEngine` choosing by
  file type. `MediaTransportControls` (owned by `TeenSpiritDlg`) mirrors the current track and
  play state into the Windows media controls; their buttons arrive as `WM_HOTKEY`
  (`VK_MEDIA_*`). Tag reading/writing in `Taggers\TagLibTagger.cpp` (TagLib 2 from vcpkg; album artist,
  composer, lyrics and rating are ID3v2 frames for MP3/WAV/AIFF/TTA, properties elsewhere;
  covered by `tests\UnitTests\TaggerTests.cpp`).
- **Online info** (`InformationProviders\`): `IInfoProvider` implementations created by
  `InfoProviderFactory`; `OnlineInfoProvider.cpp` holds the current web-API providers (Cover Art
  Archive/MusicBrainz, LRCLIB, Deezer, Last.fm API 2.0). `InfoDownloadManager` queues requests.
  `WebServices\*.wsxml` are web-search URL templates. API keys: `ApiKeys.h`; real keys go in the
  gitignored `ApiKeys.local.h` (define `JAANGLE_LOCAL_KEYS`).
- **Threads**: background work runs on `CommonSources\MultiThreadJobQueue` (condition variables,
  COM STA per worker) and the Last.fm worker. Never use `TerminateThread`: threads are asked to stop
  and joined; in-flight network calls are cancelled by closing the shared WinInet handle, which
  `PrgAPI` does before destroying services. Job code must not call `PRGAPI()` or touch UI: it gets
  what it needs when created and queues results/messages for the UI thread (e.g. `CollectionManager`
  flushes scan messages in `ConsumeResult`, called from the main dialog's timer).
- `Trackers\` record plays (local history + Last.fm scrobbling via `ITracker`/`MultiTracker`).
- `CommonSources\` is a grab bag of reusable controls/utilities (Flex* controls, HTMLLite, ALog
  logging, etc.).

## Conventions and gotchas

- C++17 with `/permissive-` (ConformanceMode on) in all three projects; the build is warning-free,
  keep it that way. Still deliberate for the legacy code: C4996 disabled (unsafe CRT string calls),
  Unicode, static MFC, `WINVER` = Windows 7.
- `.gitattributes` sets `* -text` (no line-ending conversion — preserve each file's existing
  endings). Some files are **UTF-16LE with BOM** in the working tree (all `*.lng`, `changelog.txt`,
  and a few `.cpp` files listed in `.gitattributes`); edit them in a way that keeps that encoding.
- `.svn/` still exists on disk; never commit it.
- Workflow: commit directly to `main` and push (no PRs).
