# Building Jaangle

## Requirements

- Visual Studio 2026 (or Build Tools 2026) with:
  - **Desktop development with C++** (MSVC v145, x86/x64)
  - **C++ ATL for latest build tools** and **C++ MFC for latest build tools**
  - **Windows 11 SDK** (10.0.26100 or newer)
  - **vcpkg package manager** (bundled component), or a vcpkg clone pointed to by `VCPKG_ROOT`
- Git (vcpkg uses it to fetch the registry baseline)

Third-party C++ dependencies (currently only zlib) are declared in `vcpkg.json` and
are restored automatically on the first build into `vcpkg_installed\`.

## Build

Open `Jaangle.sln` in Visual Studio, or from a Developer Command Prompt:

```
msbuild Jaangle.sln -p:Configuration=Release -p:Platform=Win32 -m
```

Output goes to `build\out\Win32\<Configuration>\` (`jaangle.exe` / `jaangledbg.exe`,
`XCrashReport.exe`). Intermediate files go to `build\obj\`.

Only **Win32** is supported for now: the database layer uses Jet OLEDB 4.0 and the
JRO type library, which exist only as 32-bit components.

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
