# TeenSpirit (Jaangle)

TeenSpirit, later renamed **Jaangle**, is a music player and library organizer for
Windows, originally developed 2003–2010. This repository modernizes the 0.98f-rc1
source so it builds and runs on current Windows with current tools.

## Features

- Music library organized by artist, album, genre, year and date added, with fast
  search (also across accented and non-Latin names).
- Playback through BASS (MP3, OGG, FLAC, WMA, AAC/M4A, APE, WavPack, MPC, AC3, MIDI,
  audio CD) with crossfading and equalizer; video through DirectShow.
- Tag reading and editing (TagLib), ratings, play history and statistics.
- Album covers, artist pictures, lyrics and biographies downloaded automatically
  (Cover Art Archive, Deezer, LRCLIB, Last.fm), plus pictures from the music folders,
  comments and personal notes per track.
- Last.fm scrobbling, mini player, skins and 17 translations.

## Status

Modernized so far:

- Builds with Visual Studio 2026 and vcpkg (32-bit and 64-bit).
- Library database moved from Microsoft Jet (`music.mdb`) to SQLite (`music.db`).
- Background threads reworked (no forced thread termination, no polling).
- Current BASS 2.4 runtime; long-dead online services removed.

## Building and running

See [BUILDING.md](BUILDING.md). In short: install Visual Studio 2026 (C++, ATL/MFC,
Windows SDK, vcpkg), run `tools\fetch-bass.ps1` once, then build `Jaangle.sln`.
The program runs from `build\out\<Platform>\<Configuration>\` and keeps its settings and
database next to the executable.

## Reporting problems

Please open an [issue](https://github.com/outcast1000/TeenSpirit/issues).

## License

GNU General Public License, version 2 or (at your option) any later version — see the
headers of the source files. Third-party components keep their own licenses
(TagLib: LGPL/MPL; BASS: free for non-commercial use, not included in this repository).
