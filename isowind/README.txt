ISOWIND - Morrowind, seen from above
=====================================

A modified OpenMW that plays Morrowind as a top-down / isometric game, with an optional pixel art look.

You need
--------
- Morrowind (Game of the Year edition, from Steam or GOG) installed.
- Windows 10 or 11, 64-bit, or 64-bit Linux (Steam Deck included).

Getting started
---------------
1. Unzip this whole folder somewhere you can write to, e.g. your Desktop or D:\Games\Isowind
   (not Program Files).
2. Run Isowind.exe.
   Windows may warn that the app is unrecognised: click "More info", then "Run anyway".
3. Pick a look and press Play.
   - The very first time, the OpenMW launcher opens instead: follow its wizard and point it at your
     Morrowind installation (it finds Steam/GOG installs), then close it and press Play again.
   - The first time you play the pixel art look, Isowind builds its low-res textures from your own
     Morrowind files. That takes a minute; after that it starts straight away.

On Linux, unpack the folder and use isowind.sh instead of Isowind.exe, from a terminal:
   ./isowind.sh                 play (pixel art; ./isowind.sh normal for the normal look)
   ./isowind.sh join ADDRESS    play on a server
   ./isowind.sh host            run a server in that terminal (it asks its settings first)
   ./isowind.sh gui             a launcher window like Isowind.exe (also what a file manager opens)
   Add --controller (or --no-controller) to get the menus made for a controller (or Morrowind's own)
   whatever is plugged in; without it, isowind.sh picks by what is. A controller that comes with a
   stream, as Moonlight's does, wants --controller.
   The first time it finds Morrowind in Steam's usual places and sets up OpenMW for it; otherwise run it as
   MORROWIND=/path/to/Morrowind ./isowind.sh (the folder with "Data Files" in it). The pixel art look
   needs Python 3; it fetches Pillow for itself if your system doesn't have it.

Your saves are shared with any normal OpenMW you have.

Multiplayer (early test version)
--------------------------------
Several people play the same world together: you see each other, fight the same creatures, share
doors, chests and items, and the server keeps everyone's character, inventory and quests.

Joining a friend's server
1. Make sure Morrowind with Tribunal and Bloodmoon is set up and plays (press Play once), with no
   other mods turned on in the OpenMW Launcher - the server only lets in players whose game files
   match its own.
2. In Isowind.exe, type the server address your friend gives you under Multiplayer (an IP address
   like 203.0.113.7, optionally with a port like 203.0.113.7:25565) and press Join.
3. Type a character name. A new name creates your character on that server (you also pick a
   password, which you need to log in as that character again); an existing name logs you in.
The address is remembered for next time.

Hosting a server
1. Press "Host a server" in Isowind.exe and choose its settings:
   - Name, port (25565 unless you change it), most players, and a password (empty: anyone can join).
   - Game: Co-op is one shared world, as in single player. MMO gives everyone their own loot
     (containers, corpses, plants, items lying around) and their own quests, and enemies and NPCs
     come back after they die, players there or not: weak ones after 1.5 minutes, normal ones 5,
     tough ones 15, bosses 20 ("Enemy respawns" speeds that up or slows it down). Plants and ore
     you harvested grow back for you after 10 minutes; chests don't refill.
     Quests that count kills (like the Fighters Guild's rats) count the kills you made yourself,
     or your party made; someone else killing them first doesn't take them from you, they come back.
   - PvP (whether players can hurt each other), difficulty and how fast the time of day goes.
   Then a server window opens; close it to stop the server.
2. Press Join (the address is set to this computer) to play on it yourself.
3. Friends need to reach your computer, one of two ways:
   - Port forwarding: in your router, forward the server's UDP port (25565) to this computer, and
     allow tes3mp-server.exe through the Windows firewall when it asks. Friends use your internet
     address (search "what is my IP").
   - A VPN that puts you on one network, such as Tailscale, ZeroTier or Radmin VPN (free). Everyone
     joins the same network and friends use this computer's address in it.
Your choices are kept in server\tes3mp-server.cfg (over server\tes3mp-server-default.cfg), and
everything the server saves (characters, the world) in server\CoreScripts\data. Its log is in
server\logs. Keep the same game (co-op or MMO) for a world: an MMO world switched to co-op forgets
what each player looted, so it's all back.

In multiplayer
Enter          Talk in the chat (with a keyboard; a controller has no button for it for now);
               F2 hides or shows the chat window.
The world never pauses, not even in menus. Saving and loading are off: the server keeps everything.
When you die you wake up at the nearest temple.

Parties (MMO servers): players in a party share their quests as on a co-op server - journal,
topics, quest progress, kill credit and what quests change in the world - while each keeps their
own loot. Type in the chat:
   /party invite NAME     invite a player (their name, or the number before it in the player list)
   /party accept          join the party you were invited to (or /party decline)
   /party                 who is in your party
   /p MESSAGE             talk to your party
   /party leave           leave it; you keep the quests you have

Looks
-----
Pixel art   The world is drawn at low resolution with chunky pixels, retro textures, banded light
            and hard shadows.
Normal      Morrowind's usual graphics.
The top-down camera and controls are the same in both.

Controls
--------
WASD              Move (W is always up the screen). Running is the default; hold Shift to walk.
Mouse             Move the cursor. With a weapon or spell ready you aim at it while your legs run
                  the way you move.
Left click        Attack / cast when ready, otherwise use whatever is under the cursor
                  (doors, people, chests, items).
Right click       Power attack: your weapon's strongest attack type.
E                 Use what is under the cursor, or the nearest thing in front of you.
Space             Jump
F / R             Ready weapon / ready magic
Tab               Inventory
Mouse wheel       Zoom. All the way out is the top-down view; zoom in and it becomes Morrowind's third person
                  view, all the way in first person.
F7                Hop between the top-down view and the normal one.
1 - 0             Quick keys, shown along the bottom of the screen. To set one, open the inventory (Tab),
                  point at an item, spell or enchanted item and press the number. Right-click a slot for
                  Morrowind's own quick key menu.

With a controller (it takes over as soon as a stick or button is used, the mouse as soon as it moves):
Left stick        Move                            Right stick       Aim, or point at what X uses
A                 Jump                            B                 Ready weapon
X                 Use / talk                      Y                 Ready magic
Right trigger     Attack / cast                   Left trigger      Power attack
Bumpers           Previous / next quick key       D-pad up / down   Zoom (hold to keep going)
D-pad left        (nothing for now)
D-pad right       Quest log                       Start             Menu
Back              Inventory (hold: journal)
Left stick in     Sneak                           Right stick in    Rest
Menus are made for the controller: the D-pad and A choose, B goes back, and where several windows are open
side by side (the inventory screen, a chest, a shop) the bumpers switch between them, the one in use lit,
while the triggers switch the tabs within a window. The right stick pressed in shows an item's info, and in
the inventory and magic windows Y puts the item or spell on a quick key. In Options the bumpers switch tabs,
the D-pad steps a pointer through the settings (up / down) and adjusts the one it is on (left / right), and A
works it. The legend along the bottom of each menu says what the buttons do there. A keyboard appears on the
screen wherever there is something to type (your name on a server, the chat, a save's name); press Y to bring
it up if it doesn't. Whether you get these menus or Morrowind's own is decided as the game starts, from
whether a controller is plugged in (Isowind.exe's launcher and isowind.sh's --controller flag decide too); the
mouse works in them as well.

The minimap turns so that up on it is up on the screen, and shows other players in multiplayer (as
does the map). Zoom it with the mouse wheel over it while a menu is open, or in Options > Interface,
where you can also keep it pointing north.

Walls, roofs and trees between you and the camera open up in a circle around you, so you can always
see yourself. Indoors you see the room like a dollhouse: walls toward the camera end at waist height,
and ceilings and the floor above are removed. Doors are never hidden. Hovering over a creature or person shows a ring under them: blue if peaceful (light
blue when in reach), red if hostile (orange when in weapon reach).

Options
-------
In the game's Options there is an extra "Interface" tab with the HUD size.
The "OpenMW Launcher" button in Isowind.exe opens the regular OpenMW launcher, for your Morrowind folder,
mods and other options.

If something goes wrong
-----------------------
- The game's log is in profiles\pixelart\openmw.log (or profiles\normal\openmw.log). Send it along with
  a description of what happened.
- If the pixel art textures look wrong, delete the folder pixelart\textures and play again to rebuild
  them.
- If saving fails, Windows Defender's "Controlled folder access" may be blocking engine\openmw.exe from
  writing to Documents: allow it under Windows Security > Ransomware protection.

Isowind is built on OpenMW (https://openmw.org), which is free software under the GNU GPL v3. Its
multiplayer is a port of TES3MP (https://tes3mp.com, GNU GPL v3), and the server runs TES3MP's
CoreScripts. The source changes are in the source folder.

Included mods (in the mods folder, on in both looks):
- Rafael's Shader Pack, by Rafael with Phal, Hrnchamd, Dexter and Wazabear (GNU GPL v3): ambient
  occlusion, sky, god rays, wet surfaces and tonemapping. Toggle shaders with F2 in game.
- Weapon Sheathing 1.6, by Greatness7 and the artists credited in its readme: lowered weapons stay
  on the hip or back.
- Questman, by fixer_m: a quest log. Press X in game (D-pad right on a controller).
- No Flickering, by Batkan1234: steady lights.
- Morrowind Optimization Patch, by Remiros, Greatness7 and contributors: faster, fixed meshes.
- Glow in the Dahrk 3.3.0, by Melchior Dahrk and NullCascade: windows glow at night (with Telvanni
  dormers on Vvardenfell).
- Graphic Herbalism 1.04, by Greatness7 and Stuporstar: plants show as picked instead of opening a
  container.
- Project Atlas 0.7.5, by FloorBelow, Greatness7, Melchior Dahrk, Remiros and the Atlas team:
  texture atlases, so towns draw faster.
- Expansion Delay 1.3, by Half11: Tribunal's assassins and Bloodmoon's rumours wait until you're
  further along.
Multiplayer servers expect every player to have the same plugins (No Flickering, Lake Fjalding
Anti-Suck, chuzei_helm_no_neck, GITD_Telvanni_Dormers, Expansion Delay); the profiles load them.
Isowind.exe's Licenses button lists everything Isowind is made from; the texts are in licenses.
