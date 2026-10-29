# Assemble the Isowind package into <root>\dist\Isowind and zip it as <root>\dist\Isowind.zip, <root> being the folder
# the repository is in (the bundled mods are there too, in bundled-mods). Run after building OpenMW (openmw,
# openmw-launcher, openmw-wizard) and Isowind.exe; build.ps1 beside this script does all of that and then this.
# Packaging again keeps the test characters in dist\Isowind (the server's data) and backs them up in <root>\backups
# first; the zip is always clean.
#
#   .\make_package.ps1           the package with the bundled mods: dist\Isowind and dist\Isowind.zip
#   .\make_package.ps1 -NoMods   without them, for a public release (their licences don't all allow passing them on):
#                                OpenMW's own look with Isowind's on top, the server requiring only Morrowind and its
#                                expansions (isowind\tools\strip_mods.py, which needs Python 3). Into
#                                dist\nomods\Isowind and dist\nomods\Isowind.zip, so the usual package and the test
#                                characters in it stay as they are; bundled-mods isn't needed.

param([switch]$NoMods)

$ErrorActionPreference = 'Stop'
$src = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path   # the repository
$root = Split-Path -Parent $src
$release = Join-Path $src 'MSVC2022_64\Release'
$distRoot = if ($NoMods) { Join-Path $root 'dist\nomods' } else { Join-Path $root 'dist' }
$dist = Join-Path $distRoot 'Isowind'
# Isowind's own files (the mod, pixel art shader and palettes, profiles, server scripts, readme), shared with the
# Linux and Android builds
$isowind = Join-Path $src 'isowind'

function Mirror($from, $to, [string[]]$extra = @()) {
    # robocopy: exit codes below 8 mean success; a file in use (the game running from dist) fails after two retries
    & robocopy $from $to /MIR /R:2 /W:2 /NFL /NDL /NJH /NJS /NP @extra | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy $from -> $to failed ($LASTEXITCODE)" }
}

# Copies only the files $to doesn't have yet (robocopy skips those changed, newer or older there), deleting nothing
function CopyMissing($from, $to) {
    & robocopy $from $to /E /XC /XN /XO /R:2 /W:2 /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy $from -> $to failed ($LASTEXITCODE)" }
}

Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem

# Backup of what testing made: the server's saved players, cells and world and the host's settings, which the package
# folder keeps (below), and the single-player saves in Documents\My Games\OpenMW (OpenMW's user data; packaging never
# touches them). Zipped into <root>\backups\isowind-data-<date>-<time>.zip all the same, the last five kept; not while
# there is nothing yet but the server's defaults.
$backups = Join-Path $root 'backups'
$serverData = Join-Path $dist 'server\CoreScripts\data'
$backupFrom = [ordered]@{}   # folder in the zip = folder here
# (none for the package without the mods: it's for a release, not played from)
if (-not $NoMods) {
    $backupFrom['server/CoreScripts/data'] = $serverData
    $documents = [Environment]::GetFolderPath('MyDocuments')
    if ($documents) { $backupFrom['saves'] = Join-Path $documents 'My Games\OpenMW\saves' }
}
$backup = @()
foreach ($name in $backupFrom.Keys) {
    if (-not (Test-Path $backupFrom[$name])) { continue }
    $base = (Get-Item $backupFrom[$name] -Force).FullName
    $backup += @(Get-ChildItem $base -Recurse -File -Force | ForEach-Object {
        [pscustomobject]@{ File = $_.FullName; Entry = "$name/" + $_.FullName.Substring($base.Length + 1).Replace('\', '/') }
    })
}
$hostSettings = Join-Path $dist 'server\tes3mp-server.cfg'
if (-not $NoMods -and (Test-Path $hostSettings)) { $backup +=[pscustomobject]@{ File = $hostSettings; Entry = 'server/tes3mp-server.cfg' } }
$defaults = 'banlist.json', 'requiredDataFiles.json' | ForEach-Object { "server/CoreScripts/data/$_" }
if ($backup | Where-Object { $_.Entry -notin $defaults -and $_.Entry -notlike '*/.gitkeep' }) {
    New-Item -ItemType Directory -Force $backups | Out-Null
    $backupZip = Join-Path $backups ('isowind-data-{0:yyyyMMdd-HHmmss}.zip' -f (Get-Date))
    $archive = [IO.Compression.ZipFile]::Open($backupZip, 'Create')
    try {
        foreach ($file in $backup) {
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.File, $file.Entry, 'Optimal') | Out-Null
        }
    } finally { $archive.Dispose() }
    Get-ChildItem $backups -Filter 'isowind-data-*.zip' | Where-Object { $_.Name -match '^isowind-data-\d{8}-\d{6}\.zip$' } |
        Sort-Object Name -Descending | Select-Object -Skip 5 | Remove-Item -Force
    "Backup: $backupZip"
}

# Engine: the build output (OpenMW, its launcher and wizard, libraries, resources) with this project's shaders and
# layouts laid over it, so they are current even if the build didn't copy them.
Mirror $release (Join-Path $dist 'engine') @('/XF', '*.pdb', '*.ilk', '*.exp', '*.lib', 'openmw-cs.exe', '*_tests.exe',
    'benchmarks*.exe', 'navmeshtool.exe', 'bulletobjecttool.exe', 'niftest.exe')
Mirror (Join-Path $src 'files\shaders') (Join-Path $dist 'engine\resources\shaders') @('/XF', 'CMakeLists.txt')
foreach ($layout in 'openmw_layers.xml', 'openmw_settings_window.layout', 'openmw_hud.layout', 'core.skin') {
    Copy-Item (Join-Path $src "files\data\mygui\$layout") (Join-Path $dist "engine\resources\vfs\mygui\$layout") -Force
}

# Top-down mod, pixel art shader and palettes (the textures are built on the player's machine from their own files).
Mirror (Join-Path $isowind 'mod') (Join-Path $dist 'mod')
# Bundled mods (credited in Isowind.exe's Licenses and README.txt) with only the options Isowind uses, kept in
# bundled-mods (mod\add-mods is where new ones are dropped to look at); the profiles list their data folders.
if (-not $NoMods) { Mirror (Join-Path $root 'bundled-mods') (Join-Path $dist 'mods') }
# Mirrored, so textures built while testing the package (Bethesda-derived) never end up in it.
Mirror (Join-Path $isowind 'pixelart') (Join-Path $dist 'pixelart')

# Profiles, with the modern key bindings.
foreach ($look in 'pixelart', 'normal') {
    $to = Join-Path $dist "profiles\$look"
    Mirror (Join-Path $isowind "profiles\$look") $to
    Copy-Item (Join-Path $isowind 'profiles\input_v3.xml') (Join-Path $to 'input_v3.xml') -Force
}

# Multiplayer server (engine\tes3mp-server.exe, started from here by Isowind.exe's "Host a server"): its settings, and
# TES3MP's CoreScripts with Isowind's changes. What hosting from the package made is the host's and stays, so a rebuild
# keeps the test characters: the saved players, cells and world in CoreScripts\data (/XD: neither copied nor cleared
# by /MIR) and tes3mp-server.cfg. The zip leaves them out (below); the logs go.
$server = Join-Path $dist 'server'
Mirror (Join-Path $isowind 'server\CoreScripts') (Join-Path $server 'CoreScripts') @('/XD', 'data')
# Of the repository's data, what the folder doesn't have yet (the empty folders, the ban list), and the plugin list
# always, as it goes with the bundled mods
CopyMissing (Join-Path $isowind 'server\CoreScripts\data') $serverData
Copy-Item (Join-Path $isowind 'server\CoreScripts\data\requiredDataFiles.json') $serverData -Force
if (Test-Path (Join-Path $server 'logs')) { Remove-Item -Recurse -Force (Join-Path $server 'logs') }
Copy-Item (Join-Path $isowind 'server\tes3mp-server-default.cfg') $server -Force

# License texts (Isowind.exe's Licenses window lists who made what): OpenMW's GPL and authors; the bundled mods carry
# their own in mods\.
$licenses = Join-Path $dist 'licenses'
New-Item -ItemType Directory -Force $licenses | Out-Null
Copy-Item (Join-Path $src 'LICENSE') (Join-Path $licenses 'GPL-3.0.txt') -Force
Copy-Item (Join-Path $src 'AUTHORS.md') (Join-Path $licenses 'OpenMW-AUTHORS.md') -Force

# Launcher (built by build.ps1 into MSVC2022_64\launcher, or beside the repository in launcher\) and readme.
$launcherExe = @((Join-Path $src 'MSVC2022_64\launcher\Isowind.exe'), (Join-Path $root 'launcher\Isowind.exe')) |
    Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $launcherExe) { throw 'No Isowind.exe: build it first (build.ps1)' }
Copy-Item $launcherExe $dist -Force
Copy-Item (Join-Path $isowind 'README.txt') $dist -Force

# Without the bundled mods: the profiles, the server's plugin list and the readme made for OpenMW alone
if ($NoMods) {
    $python = (Get-Command py -ErrorAction SilentlyContinue), (Get-Command python -ErrorAction SilentlyContinue) |
        Where-Object { $_ } | Select-Object -First 1
    if (-not $python) { throw 'Python 3 is needed for the package without the mods (strip_mods.py): install it from python.org' }
    & $python.Source (Join-Path $isowind 'tools\strip_mods.py') $dist
    if ($LASTEXITCODE -ne 0) { throw 'strip_mods.py failed (above)' }
}

# Source: OpenMW is GPL v3, so ship the changes (against the openmw-0.51.0 tag) and this project's own code.
$source = Join-Path $dist 'source'
New-Item -ItemType Directory -Force $source | Out-Null
# Everything since the openmw-0.51.0 tag, committed or not (new files included). The tag is OpenMW's, not this
# repository's: it is fetched on its own (shallow, just that tag) from OpenMW's GitLab the first time
$tag = 'openmw-0.51.0'
& git -C $src rev-parse --verify --quiet "$tag^{commit}" | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Fetching OpenMW's $tag tag for the source patch (once)..."
    & git -C $src fetch --depth 1 https://gitlab.com/OpenMW/openmw.git "refs/tags/${tag}:refs/tags/$tag"
}
& git -C $src rev-parse --verify --quiet "$tag^{commit}" | Out-Null
if ($LASTEXITCODE -eq 0) {
    & git -C $src add --intent-to-add -A -- . ':!MSVC2022_64' ':!deps' ':!android' ':!isowind' ':!.github'
    & git -C $src diff $tag -- . ':!MSVC2022_64' ':!deps' ':!android' ':!isowind' ':!.github' | Set-Content -Encoding utf8 (Join-Path $source "isowind-$tag.patch")
    & git -C $src reset --quiet
}
else {
    Write-Warning "Couldn't get OpenMW's $tag tag (offline?): the source folder names the repository and commit instead of carrying the patch"
    $commit = (& git -C $src rev-parse HEAD)
    "The changes to OpenMW are in the Isowind repository, https://github.com/pw-tf/isowind, at commit $commit`r`n(the patch against OpenMW's $tag tag couldn't be made when this package was built)" |
        Set-Content -Encoding utf8 (Join-Path $source "isowind-$tag.patch.txt")
}
Copy-Item (Join-Path $PSScriptRoot 'Isowind.cs'), (Join-Path $PSScriptRoot 'logo.png'), (Join-Path $PSScriptRoot 'make_icon.py') $source -Force
Copy-Item (Join-Path $isowind 'tools\make_palettes.py'), (Join-Path $isowind 'tools\retro_textures.py') $source -Force
@"
Isowind source
==============
isowind-openmw-0.51.0.patch   Changes to OpenMW: apply to https://gitlab.com/OpenMW/openmw at tag openmw-0.51.0.
                              Includes the multiplayer port of TES3MP (https://github.com/TES3MP/TES3MP, GPL v3).
Isowind.cs, logo.png,         The Isowind.exe launcher (C#, .NET Framework 4), its logo and its icon.
make_icon.py
make_palettes.py              Builds pixelart\textures\isowind\palettes.dds.
retro_textures.py             Python version of the texture reduction Isowind.exe does.
The top-down mod (Lua) and shaders are in the mod and pixelart folders as they are.
"@ | Set-Content -Encoding utf8 (Join-Path $source 'README.txt')

# Zip: only what the package is made of, under an Isowind folder (not whatever else ended up in dist\Isowind, such
# as a copy unzipped there to play, or what playing and hosting from it left: the launcher's last choices in
# profiles\, the host's tes3mp-server.cfg, crash dumps in server\).
$zip = Join-Path $distRoot 'Isowind.zip'
if (Test-Path $zip) { Remove-Item $zip -Force }
# The server's data from the repository as git has it (the empty folders, the ban list, the plugin list), not the
# package folder's, which has the players, cells and world of whoever tested it
$cleanData = @(& git -C $src ls-files -- 'isowind/server/CoreScripts/data')
if ($LASTEXITCODE -ne 0 -or $cleanData.Count -eq 0) {
    $repoData = Join-Path $isowind 'server\CoreScripts\data'
    Write-Warning "git couldn't list the server's data files: the zip takes the .gitkeep files and data\*.json from $repoData"
    $cleanData = @(Get-ChildItem $repoData -Recurse -File -Force |
        Where-Object { $_.Name -eq '.gitkeep' -or ($_.DirectoryName -eq $repoData -and $_.Extension -eq '.json') } |
        ForEach-Object { 'isowind/server/CoreScripts/data/' + $_.FullName.Substring($repoData.Length + 1).Replace('\', '/') })
}
$archive = [IO.Compression.ZipFile]::Open($zip, 'Create')
try {
    $items = 'engine', 'licenses', 'mod', 'mods', 'pixelart', 'profiles\pixelart', 'profiles\normal',
        'server\CoreScripts', 'server\tes3mp-server-default.cfg', 'source', 'Isowind.exe', 'README.txt'
    if ($NoMods) { $items = $items | Where-Object { $_ -ne 'mods' } }
    foreach ($item in $items) {
        $path = Join-Path $dist $item
        $files = if (Test-Path $path -PathType Container) { Get-ChildItem $path -Recurse -File } else { Get-Item $path }
        foreach ($file in $files) {
            $entry = 'Isowind/' + $file.FullName.Substring($dist.Length + 1).Replace('\', '/')
            if ($entry -like 'Isowind/server/CoreScripts/data/*') { continue }
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $entry, 'Optimal') | Out-Null
        }
    }
    foreach ($file in $cleanData) {
        $entry = 'Isowind/server/' + $file.Substring('isowind/server/'.Length)
        $from = Join-Path $src $file
        # (the plugin list as strip_mods.py made it, without the mods)
        if ($NoMods -and $file -like '*/requiredDataFiles.json') { $from = Join-Path $serverData 'requiredDataFiles.json' }
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $from, $entry, 'Optimal') | Out-Null
    }
} finally { $archive.Dispose() }
"Package: $dist"
"Zip: $zip ({0:N0} MB)" -f ((Get-Item $zip).Length / 1MB)
