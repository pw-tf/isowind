# Builds Isowind on Windows, the whole way: CrabNet (the multiplayer networking), OpenMW's dependencies, the engine,
# Isowind.exe, and the package (make_package.ps1). Run it from PowerShell, from anywhere; it works out the folders.
#
# Expects the layout make_package.ps1 expects:
#   <root>\openmw-src\       this repository (this script is isowind\windows\build.ps1 in it)
#   <root>\launcher\         Isowind.cs and make_icon.py (the Windows launcher's source, kept outside the repository)
#   <root>\bundled-mods\     the third-party mods (make_package.ps1 wants them)
#   <root>\make_package.ps1
# and makes <root>\crabnet, <root>\openmw-src\MSVC2022_64 (the build), <root>\launcher\Isowind.exe, <root>\dist.
# (The repository folder may have another name; only make_package.ps1 insists on openmw-src.)
#
# Needs on PATH: git (Git for Windows, whose bash runs OpenMW's dependency script), cmake, and Visual Studio 2022
# with the C++ workload. The dependency pack (a .7z) is extracted with 7-Zip if it is on the PATH, otherwise with
# Windows' own tar.exe (which reads .7z on Windows 11). For Isowind.exe: Python 3 with Pillow (py -m pip install
# Pillow) for the icon; the C# compiler comes with Windows.
#
#   .\build.ps1                    everything, incrementally (an existing build folder is kept)
#   .\build.ps1 -Clean             a fresh build folder
#   .\build.ps1 -SkipDependencies  don't check or re-extract OpenMW's dependency downloads (faster once they're there)
#   .\build.ps1 -SkipLauncher      no Isowind.exe
#   .\build.ps1 -SkipPackage       no dist\Isowind.zip
#   .\build.ps1 -Configuration Debug -VsVersion 2026

[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo')] [string]$Configuration = 'Release',
    [ValidateSet('2022', '2026')] [string]$VsVersion = '2022',
    [switch]$Clean,
    [switch]$SkipDependencies,
    [switch]$SkipLauncher,
    [switch]$SkipPackage
)

$ErrorActionPreference = 'Stop'
$src = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path   # the repository
$root = Split-Path -Parent $src
$crabnet = Join-Path $root 'crabnet'
$launcher = Join-Path $root 'launcher'
$buildDir = "MSVC${VsVersion}_64"
$CrabNetCommit = '19e6619'   # the one every Isowind build uses (see .github\workflows\linux.yml)

function Step($text) { Write-Host "`n==> $text" -ForegroundColor Cyan }

# Runs a program and stops on a failure exit code (PowerShell doesn't on its own)
function Run {
    param([string]$Program, [string[]]$Arguments, [string]$In = $root)
    Push-Location $In
    try {
        & $Program @Arguments
        if ($LASTEXITCODE -ne 0) { throw "$Program $($Arguments -join ' ') failed ($LASTEXITCODE)" }
    }
    finally { Pop-Location }
}

function Require($name, $hint) {
    if (-not (Get-Command $name -ErrorAction SilentlyContinue)) { throw "$name isn't on the PATH. $hint" }
}

Step 'Tools'
Require git 'Install Git for Windows.'
Require cmake 'Install CMake and tick "add to PATH".'
$windowsTar = if ($env:windir) { Join-Path $env:windir 'System32\tar.exe' } else { $null }
if (-not (Get-Command 7z -ErrorAction SilentlyContinue) -and -not ($windowsTar -and (Test-Path $windowsTar))) {
    throw 'Neither 7-Zip (7z on the PATH) nor Windows'' tar.exe is available to extract the dependency pack.'
}
# Git for Windows' bash, which OpenMW's dependency script is written for (not WSL's): bin\bash.exe in the Git
# folder, found by walking up from wherever git.exe is (Git\cmd, Git\mingw64\bin, ...)
$bash = $null
$gitDir = Split-Path -Parent (Get-Command git).Source
while ($gitDir -and -not $bash) {
    if (Test-Path (Join-Path $gitDir 'bin\bash.exe')) { $bash = Join-Path $gitDir 'bin\bash.exe' }
    else { $gitDir = Split-Path -Parent $gitDir }
}
if (-not $bash -and (Test-Path 'C:\Program Files\Git\bin\bash.exe')) { $bash = 'C:\Program Files\Git\bin\bash.exe' }
if (-not $bash) { throw "Git for Windows' bash.exe wasn't found in the Git folder above git.exe; install Git for Windows." }
if (-not (Test-Path (Join-Path $src 'CI\before_script.msvc.sh'))) { throw "$src doesn't look like the Isowind repository" }
Write-Host "Repository: $src"
Write-Host "Root:       $root"

# ---- CrabNet ---------------------------------------------------------------------------------------------------
Step "CrabNet (TES3MP's networking) at $CrabNetCommit"
if (-not (Test-Path (Join-Path $crabnet '.git'))) {
    Run git @('clone', 'https://github.com/TES3MP/CrabNet.git', $crabnet)
}
Run git @('-C', $crabnet, 'checkout', '--quiet', $CrabNetCommit)
$rakLib = Get-ChildItem (Join-Path $crabnet 'build') -Recurse -Filter 'RakNetLibStatic.lib' -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "\\$Configuration\\" } | Select-Object -First 1
if (-not $rakLib) {
    Run cmake @('-S', $crabnet, '-B', (Join-Path $crabnet 'build'), '-A', 'x64',
        '-DCRABNET_ENABLE_DLL=OFF', '-DCRABNET_ENABLE_SAMPLES=OFF', '-DCMAKE_POLICY_VERSION_MINIMUM=3.5')
    Run cmake @('--build', (Join-Path $crabnet 'build'), '--config', $Configuration, '--parallel')
    $rakLib = Get-ChildItem (Join-Path $crabnet 'build') -Recurse -Filter 'RakNetLibStatic.lib' |
        Where-Object { $_.FullName -match "\\$Configuration\\" } | Select-Object -First 1
    if (-not $rakLib) { throw 'CrabNet built but RakNetLibStatic.lib is nowhere under crabnet\build' }
}
Write-Host "RakNet: $($rakLib.FullName)"
# The dependency script runs the engine's CMake configure itself, and that needs RakNet; cmake\FindRakNet.cmake
# looks under $env:RAKNET_ROOT for include\raknet and lib\RakNetLibStatic.lib, so lay CrabNet out that way
$rakRoot = Join-Path $crabnet 'root'
New-Item -ItemType Directory -Force (Join-Path $rakRoot 'lib'), (Join-Path $rakRoot 'include') | Out-Null
Copy-Item (Join-Path $crabnet 'include\*') (Join-Path $rakRoot 'include') -Recurse -Force
Copy-Item $rakLib.FullName (Join-Path $rakRoot 'lib\RakNetLibStatic.lib') -Force
$rakDebug = Get-ChildItem (Join-Path $crabnet 'build') -Recurse -Filter 'RakNetLibStatic.lib' |
    Where-Object { $_.FullName -match '\\Debug\\' } | Select-Object -First 1
if ($rakDebug) { Copy-Item $rakDebug.FullName (Join-Path $rakRoot 'lib\RakNetLibStaticd.lib') -Force }
$env:RAKNET_ROOT = $rakRoot.Replace('\', '/')
Write-Host "RAKNET_ROOT: $env:RAKNET_ROOT"

# ---- OpenMW's dependencies and the build folder -----------------------------------------------------------------
Step "OpenMW's dependencies and the $buildDir build folder (CI\before_script.msvc.sh)"
$scriptArgs = @('-v', $VsVersion, '-p', 'Win64', '-c', $Configuration)
if (-not $Clean) { $scriptArgs += '-k' }
if ($SkipDependencies) { $scriptArgs += @('-d', '-e') }
Run $bash (@('CI/before_script.msvc.sh') + $scriptArgs) $src
$cache = Join-Path $src "$buildDir\CMakeCache.txt"
if (-not (Test-Path $cache) -or (Select-String -Path $cache -Pattern 'RakNet_INCLUDES.*NOTFOUND' -Quiet)) {
    throw "The engine's configure didn't find RakNet under RAKNET_ROOT=$env:RAKNET_ROOT (see the output above)"
}

# ---- The engine --------------------------------------------------------------------------------------------------
Step "Building the engine ($Configuration)"
Run cmake @('--build', $buildDir, '--config', $Configuration, '--parallel') $src
$release = Join-Path $src "$buildDir\$Configuration"
foreach ($exe in 'openmw.exe', 'tes3mp-server.exe', 'openmw-launcher.exe', 'openmw-wizard.exe') {
    if (-not (Test-Path (Join-Path $release $exe))) { Write-Warning "$exe isn't in $release" }
}

# ---- Isowind.exe -------------------------------------------------------------------------------------------------
if (-not $SkipLauncher) {
    Step 'Isowind.exe'
    if (-not (Test-Path (Join-Path $launcher 'Isowind.cs'))) {
        Write-Warning "No $launcher\Isowind.cs: Isowind.exe isn't built (the launcher's source lives outside the repository)"
    }
    else {
        $csc = Join-Path $env:windir 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
        if (-not (Test-Path $csc)) { $csc = Join-Path $env:windir 'Microsoft.NET\Framework\v4.0.30319\csc.exe' }
        if (-not (Test-Path $csc)) { throw "The C# compiler (.NET Framework 4's csc.exe) wasn't found under $env:windir\Microsoft.NET" }
        $python = (Get-Command py -ErrorAction SilentlyContinue), (Get-Command python -ErrorAction SilentlyContinue) |
            Where-Object { $_ } | Select-Object -First 1
        if (-not $python) { throw 'Python 3 (with Pillow) is needed for the icon: install it, then py -m pip install Pillow' }
        Run $python.Source @('make_icon.py', 'isowind.ico') $launcher
        Run $csc @('/nologo', '/target:winexe', '/out:Isowind.exe', '/win32icon:isowind.ico', 'Isowind.cs') $launcher
        Write-Host "Launcher: $launcher\Isowind.exe"
    }
}

# ---- The package ---------------------------------------------------------------------------------------------------
if (-not $SkipPackage) {
    Step 'The package (make_package.ps1)'
    $package = Join-Path $root 'make_package.ps1'
    if (-not (Test-Path $package)) { Write-Warning "No ${package}: nothing packaged" }
    elseif ($Configuration -ne 'Release') { Write-Warning "make_package.ps1 packages $buildDir\Release; this was a $Configuration build, so it isn't run" }
    elseif (-not (Test-Path (Join-Path $root 'bundled-mods'))) { Write-Warning "No $root\bundled-mods: make_package.ps1 would fail, so it isn't run" }
    else { & $package }
}

Step 'Done'
Write-Host "Engine: $release"
