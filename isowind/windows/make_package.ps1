# Assemble the Isowind package into <root>\dist\Isowind and zip it as <root>\dist\Isowind.zip, <root> being the folder
# the repository is in (the bundled mods are there too, in bundled-mods). Run after building OpenMW (openmw,
# openmw-launcher, openmw-wizard) and Isowind.exe; build.ps1 beside this script does all of that and then this.

$ErrorActionPreference = 'Stop'
$src = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path   # the repository
$root = Split-Path -Parent $src
$release = Join-Path $src 'MSVC2022_64\Release'
$dist = Join-Path $root 'dist\Isowind'
# Isowind's own files (the mod, pixel art shader and palettes, profiles, server scripts, readme), shared with the
# Linux and Android builds
$isowind = Join-Path $src 'isowind'

function Mirror($from, $to, [string[]]$extra = @()) {
    # robocopy: exit codes below 8 mean success; a file in use (the game running from dist) fails after two retries
    & robocopy $from $to /MIR /R:2 /W:2 /NFL /NDL /NJH /NJS /NP @extra | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy $from -> $to failed ($LASTEXITCODE)" }
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
Mirror (Join-Path $root 'bundled-mods') (Join-Path $dist 'mods')
# Mirrored, so textures built while testing the package (Bethesda-derived) never end up in it.
Mirror (Join-Path $isowind 'pixelart') (Join-Path $dist 'pixelart')

# Profiles, with the modern key bindings.
foreach ($look in 'pixelart', 'normal') {
    $to = Join-Path $dist "profiles\$look"
    Mirror (Join-Path $isowind "profiles\$look") $to
    Copy-Item (Join-Path $isowind 'profiles\input_v3.xml') (Join-Path $to 'input_v3.xml') -Force
}

# Multiplayer server (engine\tes3mp-server.exe, started from here by Isowind.exe's "Host a server"): its settings, and
# TES3MP's CoreScripts with Isowind's changes, without anyone's saved players or world.
$server = Join-Path $dist 'server'
# (/MIR also clears what testing the package left: saved players, the world; its logs go too)
Mirror (Join-Path $isowind 'server\CoreScripts') (Join-Path $server 'CoreScripts')
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
Copy-Item (Join-Path $PSScriptRoot 'Isowind.cs'), (Join-Path $PSScriptRoot 'make_icon.py') $source -Force
Copy-Item (Join-Path $isowind 'tools\make_palettes.py'), (Join-Path $isowind 'tools\retro_textures.py') $source -Force
@"
Isowind source
==============
isowind-openmw-0.51.0.patch   Changes to OpenMW: apply to https://gitlab.com/OpenMW/openmw at tag openmw-0.51.0.
                              Includes the multiplayer port of TES3MP (https://github.com/TES3MP/TES3MP, GPL v3).
Isowind.cs, make_icon.py      The Isowind.exe launcher (C#, .NET Framework 4) and its icon.
make_palettes.py              Builds pixelart\textures\isowind\palettes.dds.
retro_textures.py             Python version of the texture reduction Isowind.exe does.
The top-down mod (Lua) and shaders are in the mod and pixelart folders as they are.
"@ | Set-Content -Encoding utf8 (Join-Path $source 'README.txt')

# Zip: only what the package is made of, under an Isowind folder (not whatever else ended up in dist\Isowind, such
# as a copy unzipped there to play).
$zip = Join-Path $root 'dist\Isowind.zip'
if (Test-Path $zip) { Remove-Item $zip -Force }
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open($zip, 'Create')
try {
    foreach ($item in 'engine', 'licenses', 'mod', 'mods', 'pixelart', 'profiles', 'server', 'source', 'Isowind.exe', 'README.txt') {
        $path = Join-Path $dist $item
        $files = if (Test-Path $path -PathType Container) { Get-ChildItem $path -Recurse -File } else { Get-Item $path }
        foreach ($file in $files) {
            $entry = 'Isowind/' + $file.FullName.Substring($dist.Length + 1).Replace('\', '/')
            # A host's own choices (Host a server) stay theirs
            if ($entry -eq 'Isowind/server/tes3mp-server.cfg') { continue }
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $entry, 'Optimal') | Out-Null
        }
    }
} finally { $archive.Dispose() }
"Package: $dist"
"Zip: $zip ({0:N0} MB)" -f ((Get-Item $zip).Length / 1MB)
