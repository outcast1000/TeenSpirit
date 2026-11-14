# TeenSpirit (Jaangle)

TeenSpirit, later renamed **Jaangle**, is a music player and library organizer for Windows.
It was written in C++ with MFC between 2003 and 2010, back when local music collections were
the norm, and then went unmaintained. This repository renovates it: the original
application, with its features and look, now builds with current tools and runs on current
64-bit Windows. Most of the changes are under the hood (see [Renovation](#renovation)).

## Features

**Library**
- Scans your music folders ("collections") into a local database and keeps it up to date.
- Browses by artist, album, genre, year, collection or month added, with several grouping and
  display styles, and fast search (quick, full-text and advanced), including accented and
  non-Latin names.
- Ratings, comments and personal notes per track; play history with statistics per track and
  artist.

**Playback**
- Audio through the BASS library: MP3, OGG, FLAC, WMA, AAC/M4A, APE, WavPack, MPC, AC3, MIDI
  and audio CD.
- Crossfading between tracks, a 3- or 8-band equalizer, and shuffle, loop and
  auto-continue modes: when the playlist runs out, it continues with random tracks from the
  whole library, from the same artist, album, genre, year or collection, or from the newest
  additions.
- Video through DirectShow, plus a visualization pane.
- Playlists can be saved and loaded, or filled with random tracks, a whole album or search
  results.

**Information**
- Downloads album covers, artist pictures, lyrics and biographies automatically (Cover Art
  Archive/MusicBrainz, Deezer, LRCLIB, Last.fm) and uses the pictures found in the music
  folders.
- Web-search shortcuts for the current artist, album or track (YouTube, Wikipedia, Discogs,
  Genius, Bandcamp, AllMusic and others), shown in a built-in web pane.

**Tags and files**
- Reads and edits tags with TagLib, including album artist, composer, lyrics and rating.
- Fills tags from file names, and renames files from tags, using patterns.

**Interface**
- The main window is laid out in panes (sections, track list, playlist, info, web, video,
  visualization) that you can arrange.
- Skins (49 included) and translations into more than 25 languages.
- A mini player, a tray icon, configurable global hotkeys, and the keyboard media keys.
- On Windows 10 and later, the Windows media overlay and lock screen show the current track
  and control playback.
- Last.fm scrobbling.
- A music quiz built from your own library.

## Architecture

- **C++17 / MFC (static), Win32 API**, Unicode, 32- and 64-bit builds.
- **UI**: a main dialog hosting a tree of panes, each a pluggable `IPane`; skinning and
  translation are mixins; an app-wide event bus (`StateManager`) broadcasts state changes
  (track changed, database updated, ...) to the components.
- **Database**: SQLite with a custom case-insensitive Unicode collation, accessed through
  `SQLManager`.
- **Playback**: a `MediaPlayer` driving interchangeable engines: BASS (with crossfade),
  DirectShow for video, and a dispatcher that picks one by file type.
- **Background work**: a job queue with worker threads (collection scanning, online lookups)
  that hand their results back to the UI thread, plus a Last.fm worker.
- **Online information**: one provider per web API behind a common interface, with a
  download queue and a local picture cache.
- Unit tests for the database, tag reading and writing, the job queue and the online
  providers.

More detail in [BUILDING.md](BUILDING.md) and [CLAUDE.md](CLAUDE.md).

## Renovation

The 2010 code base was brought forward without changing what the program does:

- Builds with Visual Studio 2026, `/permissive-` and no warnings, with dependencies from
  vcpkg; 64-bit in addition to the original 32-bit.
- The library database moved from Microsoft Jet (`music.mdb`) to SQLite (`music.db`); an
  existing library was converted.
- Tag handling moved to TagLib 2; the BASS runtime is current.
- Background threads reworked: no forced thread termination and no polling; network calls
  can be cancelled on exit.
- Online services that have long since shut down were replaced with current APIs or removed.
- Unsafe C runtime calls replaced throughout.
- Windows media controls integration.
- A setup program (WiX), portable builds, settings in `%APPDATA%` when installed, and
  automated builds and releases on GitHub Actions.

## Download

The [Releases](https://github.com/outcast1000/TeenSpirit/releases) page has, for each
release:

- a **setup program** (64-bit) that installs into Program Files and keeps settings and the
  library in `%APPDATA%\Jaangle`;
- **portable zips** (64-bit and 32-bit): unzip anywhere and run `jaangle.exe`; settings and
  the library stay in that folder.

The files are not code-signed, so Windows SmartScreen warns the first time you run them.

## Building and running

See [BUILDING.md](BUILDING.md). In short: install Visual Studio 2026 (C++, ATL/MFC,
Windows SDK, vcpkg), run `tools\fetch-bass.ps1` once, then build `Jaangle.sln`.
The program runs from `build\out\<Platform>\<Configuration>\` and keeps its settings and
database next to the executable there (portable).

## Reporting problems

Please open an [issue](https://github.com/outcast1000/TeenSpirit/issues).

## License

GNU General Public License, version 2 or (at your option) any later version — see the
headers of the source files. Third-party components keep their own licenses
(TagLib: LGPL/MPL; BASS: free for non-commercial use, not included in this repository).
