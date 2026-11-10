# Downloads the BASS audio library, BASS_FX and the BASS add-ons the player loads,
# from the official site (https://www.un4seen.com), into build\deps\bass\.
# The DLLs are copied next to the executables by the build (see Directory.Build.targets).
#
# BASS is free for non-commercial use only; see https://www.un4seen.com/bass.html#license.
# Its DLLs are therefore not stored in the repository.
#
# Usage:  powershell -ExecutionPolicy Bypass -File tools\fetch-bass.ps1 [-UpdateSdk]
#   -UpdateSdk  also copies the headers and import libraries into Bass\ (x86) and Bass\x64\ (tracked).
param([switch]$UpdateSdk)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$deps = Join-Path $root 'build\deps\bass'
$dl = Join-Path $deps 'download'
$x86 = Join-Path $deps 'x86'
$x64 = Join-Path $deps 'x64'
New-Item -ItemType Directory -Force $dl, $x86, $x64 | Out-Null

$packages = @(
    'files/bass24.zip',
    'files/z/0/bass_fx24.zip',
    'files/basswma24.zip',
    'files/basscd24.zip',
    'files/bassflac24.zip',
    'files/bassmidi24.zip',
    'files/bassape24.zip',
    'files/basswv24.zip',
    'files/z/2/bass_mpc24.zip',
    'files/z/2/bass_ac324.zip',
    'files/z/2/bass_aac24.zip'
)

Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach ($p in $packages) {
    $name = [IO.Path]::GetFileNameWithoutExtension($p)
    $zip = Join-Path $dl "$name.zip"
    Write-Host "Downloading $name"
    Invoke-WebRequest -UseBasicParsing -Uri "https://www.un4seen.com/$p" -OutFile $zip
    $out = Join-Path $dl $name
    if (Test-Path $out) { Remove-Item -Recurse -Force $out }
    [IO.Compression.ZipFile]::ExtractToDirectory($zip, $out)

    # x86 DLL at the package root, x64 under x64\
    Get-ChildItem $out -Filter *.dll | Copy-Item -Destination $x86 -Force
    $x64dir = Join-Path $out 'x64'
    if (Test-Path $x64dir) { Get-ChildItem $x64dir -Filter *.dll | Copy-Item -Destination $x64 -Force }

    if ($UpdateSdk -and ($name -eq 'bass24' -or $name -eq 'bass_fx24')) {
        $c = Get-ChildItem $out -Directory | Where-Object { $_.Name -ieq 'c' } | Select-Object -First 1
        Get-ChildItem $c.FullName -Filter *.h | Copy-Item -Destination (Join-Path $root 'Bass') -Force
        # Prefer the SafeSEH import library when the package has one (bass_fx).
        $safeseh = Join-Path $c.FullName 'safeseh'
        $libDir = if (Test-Path $safeseh) { $safeseh } else { $c.FullName }
        Get-ChildItem $libDir -Filter *.lib | Copy-Item -Destination (Join-Path $root 'Bass') -Force
        $c64 = Join-Path $c.FullName 'x64'
        New-Item -ItemType Directory -Force (Join-Path $root 'Bass\x64') | Out-Null
        Get-ChildItem $c64 -Filter *.lib | Copy-Item -Destination (Join-Path $root 'Bass\x64') -Force
    }
}
Get-ChildItem $x86 -Filter *.dll | ForEach-Object { '{0,-14} {1}' -f $_.Name, $_.VersionInfo.FileVersion }
