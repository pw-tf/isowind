Isowind
=======

Morrowind as a top-down / isometric game, with an optional pixel art look and cooperative multiplayer:
[OpenMW](https://openmw.org) 0.51 with TES3MP's multiplayer, a camera mod and menus made for a controller.
This repository is OpenMW's source with Isowind's changes; OpenMW's own README follows further down.

* `isowind/` - the mod (`mod/`), the pixel art look, the profiles, the server files and the player's `README.txt`
* `isowind/windows/` - the Windows build script, `Isowind.exe`'s source and the packaging script
* `isowind/linux/` - `isowind.sh`, the launcher window and the Linux packaging script
* `.github/workflows/linux.yml` - the Linux build, which is the reference recipe

Building on Windows
-------------------

`isowind\windows\build.ps1` does the whole chain: CrabNet (the multiplayer networking), OpenMW's dependency
pack, the engine, `Isowind.exe` and the package.

You need, on the PATH:

* Git for Windows (its bash runs OpenMW's dependency script)
* CMake
* Visual Studio 2022 with the "Desktop development with C++" workload (2026 works, with `-VsVersion 2026`)
* Python 3 (python.org or the Microsoft Store), for the launcher's icon; Pillow is installed for you if it is missing
* GitHub CLI, logged in (`winget install GitHub.cli`, then `gh auth login`), for the bundled mods: they come from
  this repository's private `bundled-mods` release. Or put the mod folders in `<root>\bundled-mods` yourself.
* 7-Zip, unless you are on Windows 11, whose own `tar.exe` extracts the dependency pack

Clone the repository into a folder of its own. Everything that isn't source goes beside the repository, in its
parent folder (`<root>`):

    <root>\
        <repository>\             this clone; the engine builds in MSVC2022_64 inside it
        crabnet\                  cloned and built by the script
        bundled-mods\             the third-party mods, fetched by the script
        dist\Isowind\             the package, and dist\Isowind.zip beside it
        backups\                  test characters and saves, zipped before each packaging (the last five)

Then, in PowerShell, from the repository:

    Set-ExecutionPolicy -Scope CurrentUser RemoteSigned   # once, if PowerShell refuses to run scripts
    .\isowind\windows\build.ps1

The first run clones and builds CrabNet, downloads and unpacks the dependency pack, configures and builds the
engine (the final link with link-time optimisation alone takes ten to twenty minutes), compiles `Isowind.exe`,
fetches the bundled mods and packages. Later runs are incremental; after pulling changes:

    .\isowind\windows\build.ps1 -SkipDependencies

The engine ends up in `MSVC2022_64\Release`, `Isowind.exe` in `MSVC2022_64\launcher`, and the package in
`<root>\dist\Isowind` with `<root>\dist\Isowind.zip` beside it.

Rebuilding keeps the test characters. The server's players, cells and world (`dist\Isowind\server\CoreScripts\data`)
and the host's `server\tes3mp-server.cfg` stay in the package folder; single-player saves are in
`Documents\My Games\OpenMW\saves`, which packaging never touches. `make_package.ps1` still zips both into
`<root>\backups\isowind-data-<date>-<time>.zip` first, and keeps the last five. The zip takes the server's data from
git (the empty folders and the default ban and plugin lists), so it never carries anyone's characters. To update a
copy elsewhere (Bottles, another PC), unzip the new `Isowind.zip` over the old folder rather than deleting the folder:
the players stay, and only the ban list goes back to empty.

| Switch                  | What it does                                                                                   |
|-------------------------|------------------------------------------------------------------------------------------------|
| `-SkipDependencies`     | Doesn't check or re-extract the dependency downloads. Use it on every build after the first.  |
| `-NoLto`                | No link-time optimisation: the final link takes a minute or two instead of ten to twenty. The game runs a little slower, so it is for working on the code, not for a release. |
| `-SkipLauncher`         | Doesn't build `Isowind.exe`.                                                                   |
| `-SkipPackage`          | Doesn't run `make_package.ps1`.                                                                |
| `-Clean`                | A fresh build folder instead of keeping the existing one.                                      |
| `-Configuration Debug`  | `Release` (the default), `Debug` or `RelWithDebInfo`. Only a Release build is packaged.         |
| `-VsVersion 2026`       | Visual Studio 2026 instead of 2022; the build folder is then `MSVC2026_64`.                    |

The pieces, if you need one on its own:

* `make_package.ps1` assembles `<root>\dist\Isowind` from `MSVC2022_64\Release`, the mod, the bundled mods, the
  pixel art, profiles, server, licenses, `README.txt` and the GPL source patch against OpenMW's `openmw-0.51.0`
  tag (fetched if the clone lacks it), then zips it. It finds the repository from its own location.
* `make_icon.py` draws the icon; `Isowind.cs` compiles with .NET Framework 4's `csc.exe`, which comes with Windows.

If something goes wrong:

* **RakNet not found** at configure: the script lays CrabNet out as `<root>\crabnet\root` (`include\raknet`,
  `lib\RakNetLibStatic.lib`, `lib\RakNetLibStaticd.lib`) and sets `RAKNET_ROOT` to it, which
  `cmake\FindRakNet.cmake` honours; without a debug library the release one serves Debug too.
* **Windows Defender quarantines `openmw.exe`**: a fresh, unsigned binary can trip its heuristics. Restore it from
  Protection history, exclude `<root>` in Windows Security, and report the file to Microsoft as a false positive.
  The zip step fails while the file is quarantined.
* **No bundled mods**: `gh auth login` and build again, or put the mods in `<root>\bundled-mods`.

Building on Linux
-----------------

The same steps as `.github/workflows/linux.yml`, which builds every push on Ubuntu 24.04. Everything that isn't
source goes in `build-deps/` and `build/` inside the clone (git ignores both).

**1. The libraries and tools.**

* Debian / Ubuntu: `sudo CI/install_debian_deps.sh gcc openmw-deps openmw-deps-dynamic`
* Arch: `sudo pacman -S --needed base-devel cmake git boost openscenegraph mygui sdl2-compat openal ffmpeg luajit lz4
  icu sqlite yaml-cpp recastnavigation unshield`

To play you also want Python 3 with Tk for the launcher window (`python3-tk` / `tk`) and Pillow for the pixel art
look (`python3-pil` / `python-pillow`; without it `isowind.sh` installs it into a private folder of its own). To
package: [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy/releases) on the PATH (its AppImage; set
`APPIMAGE_EXTRACT_AND_RUN=1` if FUSE is missing), and the GitHub CLI logged in (`gh auth login`) for the bundled mods.

**2. CrabNet**, the multiplayer's networking, at the commit the Windows build uses:

    git clone https://github.com/TES3MP/CrabNet.git build-deps/crabnet
    git -C build-deps/crabnet checkout 19e6619
    sed -i '1i #include <cstdint>' build-deps/crabnet/Source/Utils/_FindFirst.h   # newer compilers want it
    cmake -S build-deps/crabnet -B build-deps/crabnet/build -DCMAKE_BUILD_TYPE=Release -DCRABNET_ENABLE_DLL=OFF \
        -DCRABNET_ENABLE_SAMPLES=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    cmake --build build-deps/crabnet/build -j$(nproc)

**3. The engine** (the game, the server and the importer; not the editor, launcher or tools):

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_OPENCS=OFF -DBUILD_BSATOOL=OFF -DBUILD_ESMTOOL=OFF -DBUILD_ESSIMPORTER=OFF -DBUILD_NIFTEST=OFF \
        -DBUILD_NAVMESHTOOL=OFF -DBUILD_BULLETOBJECTTOOL=OFF -DBUILD_LAUNCHER=OFF -DBUILD_WIZARD=OFF \
        -DRakNet_INCLUDES=$PWD/build-deps/crabnet/include \
        -DRakNet_LIBRARY_RELEASE=$PWD/build-deps/crabnet/build/lib/libRakNetLibStatic.a \
        -DRakNet_LIBRARY_DEBUG=$PWD/build-deps/crabnet/build/lib/libRakNetLibStatic.a \
        <your system's options, below>
    cmake --build build -j$(nproc)

* Debian / Ubuntu: `-DUSE_SYSTEM_TINYXML=ON -DOPENMW_USE_SYSTEM_RECASTNAVIGATION=ON`
* Arch: `-DUSE_SYSTEM_TINYXML=OFF -DOPENMW_USE_SYSTEM_BULLET=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5`. Arch has no
  TinyXML package, its Bullet is built for single precision where OpenMW wants double (so OpenMW builds its own),
  and CMake 4 wants the policy minimum for the older projects OpenMW pulls in.

`build/openmw` and `build/tes3mp-server` are the game and the server.

**4. The package**, laid out like the Windows one with `isowind.sh` in place of `Isowind.exe`:

    gh release download bundled-mods -p bundled-mods.zip -D build-deps
    unzip -q build-deps/bundled-mods.zip -d build-deps/bundled-mods
    isowind/linux/package.sh build build-deps/Isowind build-deps/bundled-mods

**5. Play:** `build-deps/Isowind/isowind.sh` (or `isowind.sh gui` for the launcher window; the other commands are in
"Running with a controller" below). The first time it looks for Morrowind in Steam's usual places; otherwise run it
as `MORROWIND=/path/to/Morrowind build-deps/Isowind/isowind.sh`, the folder with `Data Files` and `Morrowind.ini`.

After changes, `cmake --build build -j$(nproc)` and `package.sh` again. Packaging into the same folder again keeps its
server data and `server/tes3mp-server.cfg`, as on Windows, after backing them and `~/.local/share/openmw/saves` up
into `~/.local/share/isowind/backups` (the last five); a release goes into a new folder, as the CI's does.
`package.sh` refuses a folder that has files in it and isn't a package. While working on the code, a package's
`engine/usr/bin/openmw` and `tes3mp-server` can be links to the ones in `build/`, so that a rebuild needs no
packaging; the server's `CoreScripts` are a copy, though, and must be refreshed after server script changes.

**Playing with others** takes the same engine source. The version players are matched on is a hash of the
engine's folders (`apps`, `components`, `files`, `extern`, `cmake`, `CMakeLists.txt`) as committed, taken each time
the build runs, so build again after committing or pulling engine changes, even when the code on disk was already
built. Isowind's mod and server scripts don't count.

Running with a controller
-------------------------

The menus made for a controller are the `controller menus` setting in the look's `settings.cfg`
(`profiles\pixelart`, `profiles\normal`); the launchers set it, the game itself never does.

* Windows: `Isowind.exe` has a **Controller** tick box, ticked when a controller is connected as it opens.
* Linux: `isowind.sh gui` is the same window. The commands (`isowind.sh`, `isowind.sh normal`, `isowind.sh join
  ADDRESS`, `isowind.sh host`) go by what is plugged in, and `--controller` or `--no-controller` anywhere among the
  arguments overrides that. A controller that comes with a stream, as Moonlight's does, isn't there to be seen,
  so a streamed entry wants the flag:

      ./isowind.sh pixelart --controller
      ./isowind.sh join 203.0.113.7:25565 --controller

The in-game controller layout is in `isowind/README.txt`.


OpenMW
======

OpenMW is an open-source open-world RPG game engine that supports playing Morrowind by Bethesda Softworks. You need to own the game for OpenMW to play Morrowind.

OpenMW also comes with OpenMW-CS, a replacement for Bethesda's Construction Set.

* Version: 0.51.0
* License: GPLv3 (see [LICENSE](https://gitlab.com/OpenMW/openmw/-/raw/master/LICENSE) for more information)
* Website: https://www.openmw.org
* IRC: #openmw on irc.libera.chat
* Discord: https://discord.gg/bWuqq2e


Font Licenses:
* DejaVuLGCSansMono.ttf: custom (see [files/data/fonts/DejaVuFontLicense.txt](https://gitlab.com/OpenMW/openmw/-/raw/master/files/data/fonts/DejaVuFontLicense.txt) for more information)
* DemonicLetters.ttf: SIL Open Font License (see [files/data/fonts/DemonicLettersFontLicense.txt](https://gitlab.com/OpenMW/openmw/-/raw/master/files/data/fonts/DemonicLettersFontLicense.txt) for more information)
* MysticCards.ttf: SIL Open Font License (see [files/data/fonts/MysticCardsFontLicense.txt](https://gitlab.com/OpenMW/openmw/-/raw/master/files/data/fonts/MysticCardsFontLicense.txt) for more information)

Current Status
--------------

The main quests in Morrowind, Tribunal and Bloodmoon are all completable. Some issues with side quests are to be expected (but rare). Check the [bug tracker](https://gitlab.com/OpenMW/openmw/-/issues/?milestone_title=openmw-1.0) for a list of issues we need to resolve before the "1.0" release. Even before the "1.0" release, however, OpenMW boasts some new [features](https://wiki.openmw.org/index.php?title=Features), such as improved graphics and user interfaces.

Pre-existing modifications created for the original Morrowind engine can be hit-and-miss. The OpenMW script compiler performs more thorough error-checking than Morrowind does, meaning that a mod created for Morrowind may not necessarily run in OpenMW. Some mods also rely on quirky behaviour or engine bugs in order to work. We are considering such compatibility issues on a case-by-case basis - in some cases adding a workaround to OpenMW may be feasible, in other cases fixing the mod will be the only option. If you know of any mods that work or don't work, feel free to add them to the [Mod status](https://wiki.openmw.org/index.php?title=Mod_status) wiki page.

Getting Started
---------------

* [Official forums](https://forum.openmw.org/)
* [Installation instructions](https://openmw.readthedocs.io/en/latest/manuals/installation/index.html)
* [Build from source](https://wiki.openmw.org/index.php?title=Development_Environment_Setup)
* [Testing the game](https://wiki.openmw.org/index.php?title=Testing)
* [How to contribute](https://wiki.openmw.org/index.php?title=Contribution_Wanted)
* [Report a bug](https://gitlab.com/OpenMW/openmw/issues) - read the [guidelines](https://wiki.openmw.org/index.php?title=Bug_Reporting_Guidelines) before submitting your first bug!
* [Known issues](https://gitlab.com/OpenMW/openmw/issues?label_name%5B%5D=Bug)

The data path
-------------

The data path tells OpenMW where to find your Morrowind files. If you run the launcher, OpenMW should be able to pick up the location of these files on its own, if both Morrowind and OpenMW are installed properly (installing Morrowind under WINE is considered a proper install).

Command line options
--------------------

    Syntax: openmw <options>
    Allowed options:
      --config arg                          additional config directories
      --replace arg                         settings where the values from the
                                            current source should replace those
                                            from lower-priority sources instead of
                                            being appended
      --user-data arg                       set user data directory (used for
                                            saves, screenshots, etc)
      --resources arg (=resources)          set resources directory
      --help                                print help message
      --version                             print version information and quit
      --data arg (=data)                    set data directories (later directories
                                            have higher priority)
      --data-local arg                      set local data directory (highest
                                            priority)
      --fallback-archive arg (=fallback-archive)
                                            set fallback BSA archives (later
                                            archives have higher priority)
      --start arg                           set initial cell
      --content arg                         content file(s): esm/esp, or
                                            omwgame/omwaddon/omwscripts
      --groundcover arg                     groundcover content file(s): esm/esp,
                                            or omwgame/omwaddon
      --no-sound [=arg(=1)] (=0)            disable all sounds
      --script-all [=arg(=1)] (=0)          compile all scripts (excluding dialogue
                                            scripts) at startup
      --script-all-dialogue [=arg(=1)] (=0) compile all dialogue scripts at startup
      --script-console [=arg(=1)] (=0)      enable console-only script
                                            functionality
      --script-run arg                      select a file containing a list of
                                            console commands that is executed on
                                            startup
      --script-warn [=arg(=1)] (=1)         handling of warnings when compiling
                                            scripts
                                            0 - ignore warnings
                                            1 - show warnings but consider script as
                                            correctly compiled anyway
                                            2 - treat warnings as errors
      --load-savegame arg                   load a save game file on game startup
                                            (specify an absolute filename or a
                                            filename relative to the current
                                            working directory)
      --skip-menu [=arg(=1)] (=0)           skip main menu on game startup
      --new-game [=arg(=1)] (=0)            run new game sequence (ignored if
                                            skip-menu=0)
      --encoding arg (=win1252)             Character encoding used in OpenMW game
                                            messages:

                                            win1250 - Central and Eastern European
                                            such as Polish, Czech, Slovak,
                                            Hungarian, Slovene, Bosnian, Croatian,
                                            Serbian (Latin script), Romanian and
                                            Albanian languages

                                            win1251 - Cyrillic alphabet such as
                                            Russian, Bulgarian, Serbian Cyrillic
                                            and other languages

                                            win1252 - Western European (Latin)
                                            alphabet, used by default
      --fallback arg                        fallback values
      --no-grab [=arg(=1)] (=0)             Don't grab mouse cursor
      --export-fonts [=arg(=1)] (=0)        Export Morrowind .fnt fonts to PNG
                                            image and XML file in current directory
      --activate-dist arg (=-1)             activation distance override
      --random-seed arg (=<impl defined>)   seed value for random number generator
