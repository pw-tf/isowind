// Isowind launcher.
//
// Picks the look (pixel art or normal), starts the Isowind build of OpenMW with the matching profile, and opens the
// regular OpenMW launcher for setting up Morrowind, mods and options. The first time pixel art is played it builds
// the low-resolution, reduced-colour textures from the player's own Morrowind files (they are Bethesda's, so they are
// not shipped).
//
// Layout next to this exe:
//   engine\             the Isowind build of OpenMW (openmw.exe, openmw-launcher.exe, ...)
//   mod\                top-down camera and controls (OpenMW Lua)
//   pixelart\           pixel art shader and palettes; textures\ is generated here
//   profiles\pixelart   settings for the pixel art look (passed to openmw.exe with --config)
//   profiles\normal     settings for the normal look
//   server\             the multiplayer server's settings and scripts (TES3MP CoreScripts); engine\tes3mp-server.exe
//                       runs from here when hosting
//
// Built with the C# compiler that comes with Windows (.NET Framework 4), so: C# 5, no newer syntax.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

static class Program
{
    [STAThread]
    static void Main(string[] args)
    {
        // "Isowind.exe --build-textures": build the pixel art textures without the window, for testing and
        // troubleshooting. The result goes to pixelart\textures-build.log.
        if (args.Length > 0 && args[0] == "--build-textures")
        {
            string root = AppDomain.CurrentDomain.BaseDirectory;
            Directory.CreateDirectory(Path.Combine(root, "pixelart"));
            string error = TextureBuilder.Build(root, delegate { });
            File.WriteAllText(Path.Combine(root, "pixelart", "textures-build.log"), error ?? "Pixel art textures ready.");
            Environment.ExitCode = error == null ? 0 : 1;
            return;
        }
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new LauncherForm());
    }
}

class LauncherForm : Form
{
    internal static readonly Color Back = Color.FromArgb(24, 21, 18);
    internal static readonly Color Panel = Color.FromArgb(38, 33, 28);
    internal static readonly Color Gold = Color.FromArgb(202, 165, 96);
    internal static readonly Color Sand = Color.FromArgb(223, 201, 159);
    internal static readonly Color Dim = Color.FromArgb(150, 135, 105);

    readonly string root = AppDomain.CurrentDomain.BaseDirectory;
    readonly RadioButton pixelArt;
    readonly RadioButton normal;
    readonly ComboBox character;
    readonly Button play;
    readonly Button openmwLauncher;
    readonly TextBox serverAddress;
    readonly Button join;
    readonly Button host;
    readonly Label status;
    ServerForm serverForm;
    readonly ProgressBar progress;

    string EngineDir { get { return Path.Combine(root, "engine"); } }
    string TexturesDir { get { return Path.Combine(root, "pixelart", "textures"); } }
    string TexturesMarker { get { return Path.Combine(TexturesDir, "isowind-generated.txt"); } }
    string ModeFile { get { return Path.Combine(root, "profiles", "launcher-mode.txt"); } }
    string ServerFile { get { return Path.Combine(root, "profiles", "launcher-server.txt"); } }
    // The character picked last: its saves folder's name, or "new"
    string CharacterFile { get { return Path.Combine(root, "profiles", "launcher-character.txt"); } }
    string ServerDir { get { return Path.Combine(root, "server"); } }
    internal const int DefaultPort = 25565;

    // Where OpenMW keeps the player's openmw.cfg (Morrowind's location, load order).
    internal static string UserConfigDir
    {
        get
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "My Games", "OpenMW");
        }
    }

    public LauncherForm()
    {
        Text = "Isowind";
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(460, 606);
        BackColor = Back;
        ForeColor = Sand;
        Font = new Font("Segoe UI", 10f);
        try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

        // The logo (logo.png, embedded by the compiler's /resource), or the name in text if the exe was built without it
        Control title;
        var logo = typeof(LauncherForm).Assembly.GetManifestResourceStream("logo.png");
        if (logo != null)
        {
            title = new PictureBox
            {
                Image = Image.FromStream(logo),
                SizeMode = PictureBoxSizeMode.CenterImage,
                Bounds = new Rectangle(0, 18, 460, 100),
            };
        }
        else
        {
            title = new Label
            {
                Text = "ISOWIND",
                Font = new Font("Georgia", 30f, FontStyle.Bold),
                ForeColor = Gold,
                AutoSize = false,
                TextAlign = ContentAlignment.MiddleCenter,
                Bounds = new Rectangle(0, 18, 460, 100),
            };
        }

        var look = new Panel { BackColor = Panel, Bounds = new Rectangle(30, 132, 400, 120) };
        var lookTitle = new Label { Text = "Look", ForeColor = Gold, Bounds = new Rectangle(14, 8, 300, 22) };
        pixelArt = new RadioButton
        {
            Text = "Pixel art  -  low resolution, retro textures and lighting",
            Bounds = new Rectangle(14, 36, 380, 26),
            ForeColor = Sand,
        };
        normal = new RadioButton
        {
            Text = "Normal  -  Morrowind's usual graphics",
            Bounds = new Rectangle(14, 66, 380, 26),
            ForeColor = Sand,
        };
        var hint = new Label
        {
            Text = "The top-down camera works in both. The zoom (or F7) in game switches camera.",
            ForeColor = Dim,
            Font = new Font("Segoe UI", 8.5f),
            Bounds = new Rectangle(14, 96, 380, 18),
        };
        look.Controls.AddRange(new Control[] { lookTitle, pixelArt, normal, hint });

        // Who Play plays: a character with saves (their newest one), or a new one
        var characterPanel = new Panel { BackColor = Panel, Bounds = new Rectangle(30, 264, 400, 44) };
        var characterTitle = new Label
        {
            Text = "Character",
            ForeColor = Gold,
            Bounds = new Rectangle(14, 11, 80, 22),
            TextAlign = ContentAlignment.MiddleLeft,
        };
        character = HostForm.MakeChoice(new string[0]);
        character.Bounds = new Rectangle(94, 10, 292, 24);
        character.DropDownWidth = 420;
        characterPanel.Controls.AddRange(new Control[] { characterTitle, character });
        FillCharacters();

        play = MakeButton("Play", new Rectangle(30, 320, 400, 52), 16f);
        play.Click += delegate { OnPlay(); };

        // Multiplayer: join a server by its address, or host one on this computer
        var multiplayer = new Panel { BackColor = Panel, Bounds = new Rectangle(30, 384, 400, 122) };
        var multiplayerTitle = new Label { Text = "Multiplayer", ForeColor = Gold, Bounds = new Rectangle(14, 8, 300, 22) };
        var addressLabel = new Label
        {
            Text = "Server address",
            ForeColor = Sand,
            Bounds = new Rectangle(14, 38, 110, 22),
            TextAlign = ContentAlignment.MiddleLeft,
        };
        serverAddress = new TextBox
        {
            Bounds = new Rectangle(126, 37, 160, 24),
            BackColor = Back,
            ForeColor = Sand,
            BorderStyle = BorderStyle.FixedSingle,
        };
        join = MakeButton("Join", new Rectangle(296, 34, 90, 30), 10.5f);
        join.Click += delegate { OnJoin(); };
        host = MakeButton("Host a server", new Rectangle(14, 74, 140, 30), 9.5f);
        host.Click += delegate { OnHost(); };
        var hostHint = new Label
        {
            Text = "Friends need the server's UDP port\nforwarded to you, or a shared VPN",
            ForeColor = Dim,
            Font = new Font("Segoe UI", 8f),
            Bounds = new Rectangle(162, 72, 230, 34),
        };
        multiplayer.Controls.AddRange(new Control[] { multiplayerTitle, addressLabel, serverAddress, join, host, hostHint });
        serverAddress.KeyDown += delegate(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                e.SuppressKeyPress = true;
                OnJoin();
            }
        };

        openmwLauncher = MakeButton("OpenMW Launcher  (Morrowind, mods, options)",
            new Rectangle(30, 518, 300, 34), 9f);
        openmwLauncher.Click += delegate { StartOpenMWLauncher(); };
        var licenses = MakeButton("Licenses", new Rectangle(340, 518, 90, 34), 9.5f);
        licenses.Click += delegate { ShowLicenses(); };

        progress = new ProgressBar { Bounds = new Rectangle(30, 560, 400, 12), Visible = false };
        status = new Label { ForeColor = Dim, Bounds = new Rectangle(30, 576, 400, 22), Font = new Font("Segoe UI", 9f) };

        Controls.AddRange(new Control[] { title, look, characterPanel, play, multiplayer, openmwLauncher, licenses,
            progress, status });

        try
        {
            if (File.Exists(ServerFile))
                serverAddress.Text = File.ReadAllText(ServerFile).Trim();
        }
        catch { }
        if (!File.Exists(Path.Combine(EngineDir, "tes3mp-server.exe")) || !Directory.Exists(ServerDir))
            host.Enabled = false;

        bool normalMode = false;
        try { normalMode = File.Exists(ModeFile) && File.ReadAllText(ModeFile).Trim() == "normal"; } catch { }
        pixelArt.Checked = !normalMode;
        normal.Checked = normalMode;

        if (!File.Exists(Path.Combine(EngineDir, "openmw.exe")))
            status.Text = "engine\\openmw.exe is missing - unpack the whole Isowind folder.";
        else if (!MorrowindConfigured())
            status.Text = "First time? Press Play to set up your Morrowind installation.";
    }

    // Who made what Isowind is built from and ships with, and under which terms. The full texts are in licenses\. The
    // bundled mods are listed only when the package has them (make_package.ps1 -NoMods makes one without).
    const string Credits =
        "Isowind is a modified OpenMW with a multiplayer port of TES3MP. It needs your own copy of The Elder Scrolls III: " +
        "Morrowind (Bethesda Softworks); none of the game's files are included.\r\n\r\n" +
        "ENGINE\r\n" +
        "OpenMW - openmw.org - by the OpenMW team and contributors (licenses\\OpenMW-AUTHORS.md). " +
        "GNU General Public License v3 (licenses\\GPL-3.0.txt). Isowind's changes are under the same license; " +
        "the source is in the source folder.\r\n\r\n" +
        "TES3MP - tes3mp.com - by David Cernat, Stanislav Zhukov and contributors. The multiplayer code in Isowind is " +
        "ported from it, and the server runs its CoreScripts. GNU General Public License v3.\r\n\r\n";

    const string ModCredits =
        "BUNDLED MODS\r\n" +
        "Rafael's Shader Pack - by Rafael, with Phal, Hrnchamd, Dexter and Wazabear. GNU General Public License v3 " +
        "(mods\\RafaelsShaderPack\\LICENSE). Includes SMAA by Jorge Jimenez, Jose I. Echevarria, Belen Masia, " +
        "Fernando Navarro and Diego Gutierrez, work by Fernando Garcia Linan, and NVIDIA code, under their notices in " +
        "the shader files.\r\n\r\n" +
        "Weapon Sheathing 1.6 - by Greatness7; OpenMW implementation by akortunov and the OpenMW team. Meshes by " +
        "akortunov, Greatness7, Heinrich, London Rook, Lord Berandas, Melchior Dahrk, MementoMoritius, Petethegoat, " +
        "PikachunoTM and Remiros. Free to use with credit to the authors and not for a fee " +
        "(mods\\WeaponSheathing\\WeaponSheathing.txt).\r\n\r\n" +
        "Questman - by fixer_m (Nexus Mods, Morrowind mod 59493). A quest log; press X in game (D-pad right on a " +
        "controller).\r\n\r\n" +
        "No Flickering - by Batkan1234. Steady lights instead of flickering ones (mods\\NoFlickering).\r\n\r\n" +
        "Morrowind Optimization Patch - by Remiros and Greatness7, with Axeljk, Borok, Half11, Hemaris, Lamb Shark, " +
        "Melchior Dahrk, Nich, Revenorror, Sophie and Vegetto. Faster, fixed meshes " +
        "(mods\\MorrowindOptimizationPatch).\r\n\r\n" +
        "Glow in the Dahrk 3.3.0 - by Melchior Dahrk and NullCascade, with Greatness7, Siberian Crab, Hrnchamd, " +
        "Wolli and Atrayonis. Windows that glow at night (mods\\GlowInTheDahrk).\r\n\r\n" +
        "Graphic Herbalism 1.04 (MWSE and OpenMW edition) - by Greatness7 and Stuporstar. Plants show as picked " +
        "(mods\\GraphicHerbalism).\r\n\r\n" +
        "Project Atlas 0.7.5 - by FloorBelow, Greatness7, Lord Berandas, Melchior Dahrk, MwGek, Petethegoat, " +
        "Pop000100, R-Zero, Remiros, revenorror, RubberMan, Sataniel, Stuporstar, vtastek and Wollibeebee. " +
        "Texture atlases for faster drawing (mods\\ProjectAtlas).\r\n\r\n" +
        "Expansion Delay 1.3 - by Half11. Tribunal's assassins and Bloodmoon's rumours wait until later " +
        "(mods\\ExpansionDelay).\r\n\r\n";

    const string LibraryCredits =
        "LIBRARIES (used by OpenMW and its launcher)\r\n" +
        "OpenSceneGraph (OpenSceneGraph Public License), Bullet Physics (zlib), MyGUI (MIT), SDL 2 and SDL_image " +
        "(zlib), OpenAL Soft (LGPL 2.1), FFmpeg (LGPL 2.1), LuaJIT (MIT), Qt 6 (LGPL 3, for the OpenMW launcher and " +
        "wizard), RakNet (BSD), Recast & Detour (zlib), SQLite (public domain), yaml-cpp (MIT), ICU (Unicode " +
        "License), Boost (Boost Software License), FreeType (FreeType License), zlib, libpng, libjpeg-turbo, " +
        "LibTIFF, libxml2, Collada DOM, fmt (MIT), Brotli, zstd, XZ Utils, bzip2, libiconv and uriparser, each " +
        "under its own license.";

    void ShowLicenses()
    {
        var form = new Form
        {
            Text = "Isowind - Licenses",
            FormBorderStyle = FormBorderStyle.FixedSingle,
            MaximizeBox = false,
            MinimizeBox = false,
            StartPosition = FormStartPosition.CenterParent,
            ClientSize = new Size(560, 520),
            BackColor = Back,
            ForeColor = Sand,
            Font = new Font("Segoe UI", 9.5f),
            Icon = Icon,
        };
        var text = new TextBox
        {
            Multiline = true,
            ReadOnly = true,
            ScrollBars = ScrollBars.Vertical,
            Text = Credits + (Directory.Exists(Path.Combine(root, "mods")) ? ModCredits : "") + LibraryCredits,
            Bounds = new Rectangle(16, 16, 528, 438),
            BackColor = Panel,
            ForeColor = Sand,
            BorderStyle = BorderStyle.None,
        };
        var folder = MakeButton("Open the licenses folder", new Rectangle(16, 468, 220, 34), 9.5f);
        folder.Click += delegate
        {
            string dir = Path.Combine(root, "licenses");
            try { Process.Start("explorer.exe", "\"" + dir + "\""); } catch { }
        };
        var close = MakeButton("Close", new Rectangle(444, 468, 100, 34), 9.5f);
        close.Click += delegate { form.Close(); };
        form.Controls.AddRange(new Control[] { text, folder, close });
        form.Shown += delegate { text.SelectionLength = 0; close.Focus(); };
        form.ShowDialog(this);
    }

    internal static Button MakeButton(string text, Rectangle bounds, float size)
    {
        var button = new Button
        {
            Text = text,
            Bounds = bounds,
            FlatStyle = FlatStyle.Flat,
            BackColor = Panel,
            ForeColor = Sand,
            Font = new Font("Segoe UI", size, size > 12f ? FontStyle.Bold : FontStyle.Regular),
            Cursor = Cursors.Hand,
        };
        button.FlatAppearance.BorderColor = Gold;
        button.FlatAppearance.MouseOverBackColor = Color.FromArgb(60, 50, 38);
        return button;
    }

    // OpenMW needs Morrowind's data files; the regular OpenMW launcher (and its wizard) record them in the user's
    // openmw.cfg.
    static bool MorrowindConfigured()
    {
        string cfg = Path.Combine(UserConfigDir, "openmw.cfg");
        if (!File.Exists(cfg))
            return false;
        foreach (string line in File.ReadAllLines(cfg))
        {
            if (line.Trim().Equals("content=Morrowind.esm", StringComparison.OrdinalIgnoreCase))
                return true;
        }
        return false;
    }

    void StartOpenMWLauncher()
    {
        Start(Path.Combine(EngineDir, "openmw-launcher.exe"), "");
    }

    bool Start(string exe, string arguments)
    {
        return Start(exe, arguments, EngineDir, false);
    }

    bool Start(string exe, string arguments, string workingDir, bool ownWindow)
    {
        try
        {
            var info = new ProcessStartInfo(exe, arguments) { WorkingDirectory = workingDir, UseShellExecute = ownWindow };
            Process.Start(info);
            return true;
        }
        catch (Exception e)
        {
            MessageBox.Show(this, "Could not start " + Path.GetFileName(exe) + ":\n" + e.Message, "Isowind",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            return false;
        }
    }

    // "host", "host:port": the address and port to connect to, or null if it doesn't look like one
    static string ParseServerAddress(string text)
    {
        text = (text ?? "").Trim();
        if (text.Length == 0)
            return null;
        string address = text;
        int port = DefaultPort;
        int colon = text.LastIndexOf(':');
        if (colon > 0 && text.IndexOf(':') == colon)
        {
            address = text.Substring(0, colon);
            if (!int.TryParse(text.Substring(colon + 1), out port) || port <= 0 || port > 65535)
                return null;
        }
        if (address.Length == 0 || address.IndexOf(' ') >= 0)
            return null;
        return address + ":" + port;
    }

    // The characters in OpenMW's saves folder, the last played first, then "New character"; the one picked last is
    // picked again (the last played if none was)
    void FillCharacters()
    {
        string picked = null;
        try { picked = File.Exists(CharacterFile) ? File.ReadAllText(CharacterFile).Trim() : null; } catch { }
        character.Items.Clear();
        foreach (var each in Character.ReadAll())
            character.Items.Add(each);
        character.Items.Add(new Character());
        character.SelectedIndex = 0;
        for (int i = 0; i < character.Items.Count; i++)
        {
            string folder = ((Character)character.Items[i]).Folder;
            if (picked != null && picked.Equals(folder == null ? "new" : Path.GetFileName(folder),
                    StringComparison.OrdinalIgnoreCase))
                character.SelectedIndex = i;
        }
    }

    // The character picked, remembered for next time
    Character RememberPick()
    {
        var picked = (Character)character.SelectedItem;
        try
        {
            File.WriteAllText(CharacterFile, picked.Folder == null ? "new" : Path.GetFileName(picked.Folder));
        }
        catch { }
        return picked;
    }

    void OnPlay()
    {
        var picked = RememberPick();
        if (picked.Folder == null)
            Launch("--skip-menu --new-game");
        else if (picked.FromServer)
            Launch("--skip-menu --server-character \"" + picked.Folder + "\"");
        else
            Launch("--skip-menu --load-savegame \"" + picked.NewestSave + "\"");
    }

    void OnJoin()
    {
        string address = ParseServerAddress(serverAddress.Text);
        if (address == null)
        {
            MessageBox.Show(this,
                "Type the address of the server to join: its IP address (like 203.0.113.7) or name, optionally with " +
                "a port (like 203.0.113.7:" + DefaultPort + ").\n\nTo join a server on this computer, use 127.0.0.1.",
                "Isowind - multiplayer", MessageBoxButtons.OK, MessageBoxIcon.Information);
            serverAddress.Focus();
            return;
        }
        try { File.WriteAllText(ServerFile, serverAddress.Text.Trim()); } catch { }
        // The character picked comes along, if the server takes characters from single-player
        var picked = RememberPick();
        if (picked.Folder == null)
            Launch("--connect=" + address);
        else
            Launch("--connect=" + address + " --character \"" + picked.NewestSave + "\"");
    }

    void OnHost()
    {
        if (serverForm != null)
        {
            serverForm.Activate();
            return;
        }
        if (Process.GetProcessesByName("tes3mp-server").Length > 0)
        {
            MessageBox.Show(this, "A server is already running on this computer.", "Isowind - multiplayer",
                MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }
        int port;
        using (var form = new HostForm(ServerDir))
        {
            if (form.ShowDialog(this) != DialogResult.OK)
                return;
            port = form.Port;
        }
        serverForm = new ServerForm(EngineDir, ServerDir);
        serverForm.FormClosed += delegate
        {
            serverForm = null;
            // The game was started with the server running: the launcher only waited for the server to stop
            if (!Visible)
                Close();
        };
        serverForm.Show();
        if (!serverForm.StartServer())
            return;
        serverAddress.Text = port == DefaultPort ? "127.0.0.1" : "127.0.0.1:" + port;
        status.Text = "Server started (its window shows what happens). Press Join to play on it.";
    }

    // Starts the game with the look picked; extraArguments: what to play (a save, a new game or a server)
    async void Launch(string extraArguments)
    {
        string mode = pixelArt.Checked ? "pixelart" : "normal";
        try { File.WriteAllText(ModeFile, mode); } catch { }

        if (!MorrowindConfigured())
        {
            MessageBox.Show(this,
                "OpenMW doesn't know where Morrowind is installed yet.\n\n" +
                "The OpenMW launcher will open now. Follow its first-run wizard and point it at your Morrowind " +
                "installation (Steam or GOG), then close it and press Play here again.",
                "Isowind - first time setup", MessageBoxButtons.OK, MessageBoxIcon.Information);
            StartOpenMWLauncher();
            return;
        }

        if (mode == "pixelart" && !File.Exists(TexturesMarker))
        {
            play.Enabled = openmwLauncher.Enabled = pixelArt.Enabled = normal.Enabled = join.Enabled =
                character.Enabled = false;
            progress.Visible = true;
            bool ok = await Task.Run(() => GenerateTextures());
            progress.Visible = false;
            play.Enabled = openmwLauncher.Enabled = pixelArt.Enabled = normal.Enabled = join.Enabled =
                character.Enabled = true;
            if (!ok)
                return;
        }

        string profile = Path.Combine(root, "profiles", mode);
        string arguments = "--config \"" + profile + "\"";
        if (extraArguments != null)
            arguments += " " + extraArguments;
        if (Start(Path.Combine(EngineDir, "openmw.exe"), arguments))
            CloseOrHide();
    }

    // Closing the launcher with the server's window open would stop the server: it hides until that window closes
    void CloseOrHide()
    {
        if (serverForm != null)
            Hide();
        else
            Close();
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        if (serverForm != null && e.CloseReason == CloseReason.UserClosing)
        {
            e.Cancel = true;
            Hide();
            return;
        }
        base.OnFormClosing(e);
    }

    // Sets key = value in [section] of a settings file (OpenMW's settings.cfg, the server's tes3mp-server.cfg),
    // keeping the rest of it
    internal static void SetConfigValue(string file, string section, string key, string value)
    {
        var lines = File.Exists(file) ? File.ReadAllLines(file).ToList() : new List<string>();
        string line = key + " = " + value;
        int start = lines.FindIndex(l => l.Trim() == "[" + section + "]");
        if (start < 0)
        {
            if (lines.Count > 0)
                lines.Add("");
            lines.Add("[" + section + "]");
            lines.Add(line);
        }
        else
        {
            int end = lines.FindIndex(start + 1, l => l.Trim().StartsWith("["));
            if (end < 0)
                end = lines.Count;
            int existing = lines.FindIndex(start + 1, end - start - 1, l =>
            {
                int equals = l.IndexOf('=');
                return equals > 0 && !l.TrimStart().StartsWith("#")
                    && l.Substring(0, equals).Trim().Equals(key, StringComparison.OrdinalIgnoreCase);
            });
            if (existing >= 0)
                lines[existing] = line;
            else
            {
                // after the section's last setting, before the blank lines that end it
                while (end > start + 1 && lines[end - 1].Trim().Length == 0)
                    end--;
                lines.Insert(end, line);
            }
        }
        File.WriteAllLines(file, lines);
    }

    // The settings of settings files read in order, later ones winning: "section/key" -> value
    internal static Dictionary<string, string> ReadConfig(params string[] files)
    {
        var values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (string file in files)
        {
            if (!File.Exists(file))
                continue;
            string section = "";
            foreach (string raw in File.ReadAllLines(file))
            {
                string line = raw.Trim();
                if (line.Length == 0 || line.StartsWith("#"))
                    continue;
                if (line.StartsWith("[") && line.EndsWith("]"))
                {
                    section = line.Substring(1, line.Length - 2).Trim();
                    continue;
                }
                int equals = line.IndexOf('=');
                if (equals > 0)
                    values[section + "/" + line.Substring(0, equals).Trim()] = line.Substring(equals + 1).Trim();
            }
        }
        return values;
    }

    void Report(string text, int done, int total)
    {
        BeginInvoke((Action)delegate
        {
            status.Text = text;
            progress.Maximum = Math.Max(total, 1);
            progress.Value = Math.Min(done, progress.Maximum);
        });
    }

    bool GenerateTextures()
    {
        string error = TextureBuilder.Build(root, Report);
        if (error != null)
        {
            BeginInvoke((Action)delegate
            {
                status.Text = "";
                MessageBox.Show(this, error, "Isowind", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            });
        }
        return error == null;
    }
}

// A single-player character: a folder in OpenMW's saves folder, as its newest save has them (the character folder
// with no saves, Folder null: "New character")
class Character
{
    public string Folder;
    public string NewestSave;
    public DateTime LastPlayed;
    public string Name = "";
    public int Level = 1;
    public string ClassName = "";
    public string Cell = "";
    // Made on a multiplayer server and never played offline: NewestSave is what the server wrote back
    public bool FromServer;

    // Save format versions up to this one have record ids as plain strings (ESM::MaxStringRefIdFormatVersion)
    const uint MaxStringRefIdFormatVersion = 23;
    // What a multiplayer server writes back beside a character's saves (apps/openmw/mwmp/PortableCharacter.hpp)
    const string ServerProgressFile = "isowind-server-progress.dat";

    static string SavesDir { get { return Path.Combine(LauncherForm.UserConfigDir, "saves"); } }

    public override string ToString()
    {
        if (Folder == null)
            return "New character";
        string text = Name + (FromServer ? " (from multiplayer)" : "") + "  -  level " + Level;
        if (ClassName.Length > 0)
            text += " " + ClassName;
        text += "  -  " + LastPlayed.ToString("d MMM");
        if (Cell.Length > 0)
            text += ", " + Cell;
        return text;
    }

    // The characters that have saves, the last played first
    internal static List<Character> ReadAll()
    {
        var characters = new List<Character>();
        try
        {
            if (!Directory.Exists(SavesDir))
                return characters;
            foreach (string folder in Directory.GetDirectories(SavesDir))
            {
                // The newest save it can read (OpenMW's own order: the files' times)
                Character found = null;
                foreach (var save in new DirectoryInfo(folder).GetFiles("*.omwsave")
                    .OrderByDescending(f => f.LastWriteTimeUtc))
                {
                    found = ReadSave(save.FullName);
                    if (found == null)
                        continue;
                    found.NewestSave = save.FullName;
                    found.LastPlayed = save.LastWriteTime;
                    break;
                }

                // What a multiplayer server wrote back (the game puts it on the character when it's next loaded; one
                // made on a server has only that): shown when it's newer
                var progressFile = new FileInfo(Path.Combine(folder, ServerProgressFile));
                if (progressFile.Exists && (found == null || progressFile.LastWriteTime > found.LastPlayed))
                {
                    var fromServer = ReadSave(progressFile.FullName);
                    if (fromServer != null)
                    {
                        fromServer.NewestSave = found != null ? found.NewestSave : progressFile.FullName;
                        fromServer.FromServer = found == null;
                        fromServer.LastPlayed = progressFile.LastWriteTime;
                        found = fromServer;
                    }
                }
                if (found == null)
                    continue;
                found.Folder = folder;
                characters.Add(found);
            }
        }
        catch { }
        return characters.OrderByDescending(c => c.LastPlayed).ToList();
    }

    // A save's name, level, class and cell, from its SAVE record (after the TES3 header; the screenshot follows
    // them), or null if it isn't a save
    static Character ReadSave(string path)
    {
        try
        {
            byte[] data;
            using (var file = File.OpenRead(path))
            {
                data = new byte[Math.Min(file.Length, 1 << 18)];
                int read = 0, count;
                while (read < data.Length && (count = file.Read(data, read, data.Length - read)) > 0)
                    read += count;
            }
            if (data.Length < 16 || Ascii(data, 0) != "TES3")
                return null;
            int headerEnd = 16 + (int)BitConverter.ToUInt32(data, 4);
            uint format = 0;
            for (int pos = 16; pos + 8 <= headerEnd && pos + 12 <= data.Length;
                pos += 8 + (int)BitConverter.ToUInt32(data, pos + 4))
            {
                if (Ascii(data, pos) == "FORM")
                    format = BitConverter.ToUInt32(data, pos + 8);
            }
            if (headerEnd + 16 > data.Length || Ascii(data, headerEnd) != "SAVE")
                return null;

            var fields = new Dictionary<string, byte[]>();
            long end = Math.Min(data.Length, headerEnd + 16L + BitConverter.ToUInt32(data, headerEnd + 4));
            for (int pos = headerEnd + 16; pos + 8 <= end;)
            {
                string name = Ascii(data, pos);
                int size = (int)BitConverter.ToUInt32(data, pos + 4);
                if (name == "SCRN" || pos + 8L + size > end)
                    break;
                if (!fields.ContainsKey(name))
                {
                    var value = new byte[size];
                    Array.Copy(data, pos + 8, value, 0, size);
                    fields[name] = value;
                }
                pos += 8 + size;
            }
            if (!fields.ContainsKey("PLNA"))
                return null;

            var character = new Character { Name = Text(fields["PLNA"], 0) };
            if (fields.ContainsKey("PLLE") && fields["PLLE"].Length >= 4)
                character.Level = BitConverter.ToInt32(fields["PLLE"], 0);
            if (fields.ContainsKey("PLCL"))
            {
                byte[] id = fields["PLCL"];
                if (format <= MaxStringRefIdFormatVersion)
                    character.ClassName = Text(id, 0);
                else if (id.Length > 0 && id[0] == 2) // an unsized string id
                    character.ClassName = Text(id, 1);
                else if (id.Length > 5 && id[0] == 1) // a sized one: its length, then the string
                    character.ClassName = Text(id, 5);
            }
            else if (fields.ContainsKey("PLCN"))
                character.ClassName = Text(fields["PLCN"], 0);
            if (fields.ContainsKey("PLCE"))
                character.Cell = Text(fields["PLCE"], 0);
            return character;
        }
        catch
        {
            return null;
        }
    }

    static string Ascii(byte[] data, int offset)
    {
        return Encoding.ASCII.GetString(data, offset, 4);
    }

    // Saves keep text in Morrowind's own encoding (Windows-1252 for the English game)
    static string Text(byte[] value, int offset)
    {
        return Encoding.GetEncoding(1252).GetString(value, offset, value.Length - offset).TrimEnd('\0');
    }
}

// "Host a server": the server's settings, kept in server\tes3mp-server.cfg (over tes3mp-server-default.cfg). The
// [Isowind] ones are read by the server's CoreScripts (scripts\custom\isowindHost.lua).
class HostForm : Form
{
    static readonly string[] DifficultyNames = { "Very easy", "Easy", "Normal", "Hard", "Very hard" };
    static readonly int[] DifficultyValues = { -100, -50, 0, 50, 100 };
    static readonly string[] SpeedValues = { "0.5", "1", "2", "4" };

    readonly string serverDir;
    readonly TextBox serverName;
    readonly NumericUpDown port;
    readonly NumericUpDown maxPlayers;
    readonly TextBox password;
    readonly RadioButton coop;
    readonly RadioButton mmo;
    readonly CheckBox pvp;
    readonly CheckBox portableCharacters;
    readonly ComboBox difficulty;
    readonly ComboBox respawnSpeed;
    readonly ComboBox timeSpeed;

    public int Port { get { return (int)port.Value; } }

    string OverridesFile { get { return Path.Combine(serverDir, "tes3mp-server.cfg"); } }

    public HostForm(string serverDir)
    {
        this.serverDir = serverDir;
        Text = "Isowind - host a server";
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        MinimizeBox = false;
        StartPosition = FormStartPosition.CenterParent;
        ClientSize = new Size(460, 490);
        BackColor = LauncherForm.Back;
        ForeColor = LauncherForm.Sand;
        Font = new Font("Segoe UI", 9.5f);

        var values = LauncherForm.ReadConfig(Path.Combine(serverDir, "tes3mp-server-default.cfg"), OverridesFile);
        Func<string, string, string> get = delegate(string key, string fallback)
        {
            string value;
            return values.TryGetValue(key, out value) && value.Length > 0 ? value : fallback;
        };

        int y = 16;
        serverName = new TextBox { Text = get("General/hostname", "Isowind server"), MaxLength = 60 };
        AddRow("Server name", serverName, ref y);

        port = new NumericUpDown { Minimum = 1024, Maximum = 65535, Value = ParseInt(get("General/port", ""),
            LauncherForm.DefaultPort, 1024, 65535), Width = 90 };
        AddRow("Port", port, ref y);

        maxPlayers = new NumericUpDown { Minimum = 1, Maximum = 64, Value = ParseInt(get("General/maximumPlayers", ""),
            16, 1, 64), Width = 90 };
        AddRow("Most players", maxPlayers, ref y);

        string savedPassword;
        values.TryGetValue("General/password", out savedPassword);
        password = new TextBox { Text = savedPassword ?? "", MaxLength = 40 };
        AddRow("Password", password, ref y);
        AddHint("Leave empty for anyone to join.", ref y);

        coop = new RadioButton { Text = "Co-op  -  one shared world, as in single player", AutoSize = true };
        AddRow("Game", coop, ref y);
        mmo = new RadioButton { Text = "MMO  -  own loot and quests, enemies come back", AutoSize = true };
        AddRow("", mmo, ref y);
        bool isMmo = get("Isowind/mode", "coop").Equals("mmo", StringComparison.OrdinalIgnoreCase);
        coop.Checked = !isMmo;
        mmo.Checked = isMmo;

        pvp = new CheckBox { Text = "Players can hurt each other (PvP)", AutoSize = true,
            Checked = get("Isowind/pvp", "false").Equals("true", StringComparison.OrdinalIgnoreCase) };
        AddRow("", pvp, ref y);

        portableCharacters = new CheckBox { Text = "Players can bring their single-player characters", AutoSize = true,
            Checked = get("Isowind/portableCharacters", "false").Equals("true", StringComparison.OrdinalIgnoreCase) };
        AddRow("", portableCharacters, ref y);
        AddHint("Taken as their saves have them: no checks on what they bring.", ref y);

        difficulty = MakeChoice(DifficultyNames);
        int savedDifficulty = ParseInt(get("Isowind/difficulty", "0"), 0, -500, 500);
        difficulty.SelectedIndex = Nearest(DifficultyValues.Select(v => (double)v).ToArray(), savedDifficulty);
        AddRow("Difficulty", difficulty, ref y);

        respawnSpeed = MakeChoice(new[] { "Half as fast", "Normal  (1.5 to 20 minutes)", "Twice as fast",
            "Four times as fast" });
        respawnSpeed.SelectedIndex = SpeedIndex(get("Isowind/respawnSpeed", "1"));
        AddRow("Enemy respawns", respawnSpeed, ref y);
        AddHint("MMO only. They come back even with players there.", ref y);

        timeSpeed = MakeChoice(new[] { "Half as fast", "Morrowind's  (an hour in 2 minutes)", "Twice as fast",
            "Four times as fast" });
        timeSpeed.SelectedIndex = SpeedIndex(get("Isowind/timeSpeed", "1"));
        AddRow("Time of day", timeSpeed, ref y);

        coop.CheckedChanged += delegate { respawnSpeed.Enabled = mmo.Checked; };
        respawnSpeed.Enabled = mmo.Checked;

        var start = LauncherForm.MakeButton("Start the server", new Rectangle(16, ClientSize.Height - 50, 200, 34),
            10.5f);
        start.Click += delegate { OnStart(); };
        var cancel = LauncherForm.MakeButton("Cancel", new Rectangle(ClientSize.Width - 116, ClientSize.Height - 50,
            100, 34), 9.5f);
        cancel.Click += delegate { DialogResult = DialogResult.Cancel; };
        Controls.AddRange(new Control[] { start, cancel });
        AcceptButton = start;
        CancelButton = cancel;
    }

    void AddRow(string label, Control control, ref int y)
    {
        if (label.Length > 0)
            Controls.Add(new Label { Text = label, ForeColor = LauncherForm.Gold, Bounds = new Rectangle(16, y + 3, 120,
                22) });
        control.Location = new Point(140, y);
        if (control is TextBox || control is ComboBox)
            control.Width = 300;
        if (control is TextBox || control is NumericUpDown)
        {
            control.BackColor = LauncherForm.Panel;
            control.ForeColor = LauncherForm.Sand;
        }
        if (control is TextBox)
            ((TextBox)control).BorderStyle = BorderStyle.FixedSingle;
        Controls.Add(control);
        y += Math.Max(control.Height, 24) + 8;
    }

    void AddHint(string text, ref int y)
    {
        Controls.Add(new Label { Text = text, ForeColor = LauncherForm.Dim, Font = new Font("Segoe UI", 8.5f),
            Bounds = new Rectangle(140, y - 6, 306, 18) });
        y += 16;
    }

    internal static ComboBox MakeChoice(string[] items)
    {
        // Drawn here: Windows draws a drop-down list white whatever its colours
        var box = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, FlatStyle = FlatStyle.Flat,
            BackColor = LauncherForm.Panel, ForeColor = LauncherForm.Sand, DrawMode = DrawMode.OwnerDrawFixed,
            ItemHeight = 20 };
        box.Items.AddRange(items);
        box.DrawItem += delegate(object sender, DrawItemEventArgs e)
        {
            bool highlighted = (e.State & DrawItemState.Selected) != 0 && (e.State & DrawItemState.ComboBoxEdit) == 0;
            using (var back = new SolidBrush(highlighted ? Color.FromArgb(60, 50, 38) : LauncherForm.Panel))
                e.Graphics.FillRectangle(back, e.Bounds);
            if (e.Index >= 0)
                TextRenderer.DrawText(e.Graphics, box.Items[e.Index].ToString(), box.Font, e.Bounds,
                    box.Enabled ? LauncherForm.Sand : LauncherForm.Dim,
                    TextFormatFlags.VerticalCenter | TextFormatFlags.Left | TextFormatFlags.NoPrefix |
                    TextFormatFlags.EndEllipsis);
        };
        return box;
    }

    static int ParseInt(string text, int fallback, int min, int max)
    {
        int value;
        return int.TryParse(text, out value) ? Math.Max(min, Math.Min(max, value)) : fallback;
    }

    static int SpeedIndex(string text)
    {
        double value;
        if (!double.TryParse(text, System.Globalization.NumberStyles.Float,
            System.Globalization.CultureInfo.InvariantCulture, out value))
            value = 1;
        return Nearest(SpeedValues.Select(v => double.Parse(v, System.Globalization.CultureInfo.InvariantCulture))
            .ToArray(), value);
    }

    static int Nearest(double[] options, double value)
    {
        int best = 0;
        for (int i = 1; i < options.Length; i++)
            if (Math.Abs(options[i] - value) < Math.Abs(options[best] - value))
                best = i;
        return best;
    }

    void OnStart()
    {
        string name = serverName.Text.Trim();
        if (name.Length == 0)
        {
            MessageBox.Show(this, "Give the server a name.", Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
            serverName.Focus();
            return;
        }
        try
        {
            if (!File.Exists(OverridesFile))
                File.WriteAllText(OverridesFile, "# This server's settings, over those in tes3mp-server-default.cfg " +
                    "(Isowind.exe's Host a server writes them)\r\n");
            string file = OverridesFile;
            LauncherForm.SetConfigValue(file, "General", "hostname", name);
            LauncherForm.SetConfigValue(file, "General", "port", Port.ToString());
            LauncherForm.SetConfigValue(file, "General", "maximumPlayers", ((int)maxPlayers.Value).ToString());
            LauncherForm.SetConfigValue(file, "General", "password", password.Text.Trim());
            LauncherForm.SetConfigValue(file, "Isowind", "mode", mmo.Checked ? "mmo" : "coop");
            LauncherForm.SetConfigValue(file, "Isowind", "pvp", pvp.Checked ? "true" : "false");
            LauncherForm.SetConfigValue(file, "Isowind", "portableCharacters",
                portableCharacters.Checked ? "true" : "false");
            LauncherForm.SetConfigValue(file, "Isowind", "difficulty",
                DifficultyValues[difficulty.SelectedIndex].ToString());
            LauncherForm.SetConfigValue(file, "Isowind", "respawnSpeed", SpeedValues[respawnSpeed.SelectedIndex]);
            LauncherForm.SetConfigValue(file, "Isowind", "timeSpeed", SpeedValues[timeSpeed.SelectedIndex]);
        }
        catch (Exception e)
        {
            MessageBox.Show(this, "Could not save the server's settings:\n" + e.Message, Text, MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return;
        }
        DialogResult = DialogResult.OK;
    }
}

// The server's window: runs engine\tes3mp-server.exe in it (its log shown here, no console of its own) and talks to
// its scripts (server\CoreScripts\scripts\custom\isowindAdmin.lua) through two files in the server's folder: a status
// it writes every second (the players, the bans, the time, the results of what was asked) and commands appended here.
// Closing it stops the server, saved.
class ServerForm : Form
{
    const string StatusFile = "isowind-admin-status.txt";
    const string CommandsFile = "isowind-admin-commands.txt";
    static readonly string[] RankNames = { "Player", "Moderator", "Admin", "Server owner" };
    static readonly string[] WeatherNames = { "Clear", "Cloudy", "Foggy", "Overcast", "Rain", "Thunder", "Ash",
        "Blight", "Snow", "Blizzard" };

    class Player
    {
        public int Pid;
        public string Name, Ip, Cell, Region, Level;
        public int Rank, Ping;
    }

    readonly string engineDir;
    readonly string serverDir;
    readonly string serverName;
    readonly int port;
    readonly int maxPlayers;

    Process process;
    bool restarting;
    bool stopping;
    int nextCommandId = 1;
    int lastResultShown;
    long lastStatusTime;
    DateTime lastStatusChange = DateTime.MinValue;
    List<Player> players = new List<Player>();
    bool updatingSelection;

    readonly Label summary;
    readonly ListView playerList;
    readonly Panel playerActions;
    readonly ComboBox rank;
    readonly TextBox tellText;
    readonly ComboBox teleportTarget;
    readonly ComboBox weather;
    readonly TextBox log;
    readonly TextBox sayText;
    readonly ListBox bannedNames;
    readonly ListBox bannedIps;
    readonly TextBox banText;
    readonly NumericUpDown hour;
    readonly TextBox cellText;
    readonly Panel[] pages;
    readonly Button[] pageButtons;
    readonly Button stopButton;
    readonly Button restartButton;
    readonly System.Windows.Forms.Timer refresh;

    public ServerForm(string engineDir, string serverDir)
    {
        this.engineDir = engineDir;
        this.serverDir = serverDir;
        var values = LauncherForm.ReadConfig(Path.Combine(serverDir, "tes3mp-server-default.cfg"),
            Path.Combine(serverDir, "tes3mp-server.cfg"));
        string value;
        serverName = values.TryGetValue("General/hostname", out value) && value.Length > 0 ? value : "Isowind server";
        if (!values.TryGetValue("General/port", out value) || !int.TryParse(value, out port))
            port = LauncherForm.DefaultPort;
        if (!values.TryGetValue("General/maximumPlayers", out value) || !int.TryParse(value, out maxPlayers))
            maxPlayers = 16;

        Text = "Isowind server - " + serverName;
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(940, 640);
        MinimumSize = new Size(956, 679);
        BackColor = LauncherForm.Back;
        ForeColor = LauncherForm.Sand;
        Font = new Font("Segoe UI", 9.5f);
        try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

        summary = new Label { ForeColor = LauncherForm.Gold, Font = new Font("Segoe UI", 10.5f, FontStyle.Bold),
            Bounds = new Rectangle(16, 12, 908, 24), Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };
        Controls.Add(summary);

        // The players, and what can be done to the one picked
        Controls.Add(new Label { Text = "Players", ForeColor = LauncherForm.Gold, Bounds = new Rectangle(16, 44, 300,
            20) });
        playerList = new ListView { View = View.Details, FullRowSelect = true, MultiSelect = false, HideSelection = false,
            BackColor = LauncherForm.Panel, ForeColor = LauncherForm.Sand, BorderStyle = BorderStyle.FixedSingle,
            Bounds = new Rectangle(16, 66, 520, 330), OwnerDraw = true,
            Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left };
        playerList.Columns.Add("Name", 150);
        playerList.Columns.Add("Staff rank", 100);
        playerList.Columns.Add("Ping", 50);
        playerList.Columns.Add("Level", 50);
        playerList.Columns.Add("Cell", 166);
        // The last column to the list's right edge, so no white header shows past it
        playerList.HandleCreated += delegate { playerList.Columns[4].Width = -2; };
        playerList.DrawColumnHeader += delegate(object sender, DrawListViewColumnHeaderEventArgs e)
        {
            using (var back = new SolidBrush(LauncherForm.Back))
                e.Graphics.FillRectangle(back, e.Bounds);
            TextRenderer.DrawText(e.Graphics, e.Header.Text, playerList.Font, e.Bounds, LauncherForm.Gold,
                TextFormatFlags.VerticalCenter | TextFormatFlags.Left);
        };
        playerList.DrawItem += delegate(object sender, DrawListViewItemEventArgs e) { e.DrawDefault = true; };
        playerList.DrawSubItem += delegate(object sender, DrawListViewSubItemEventArgs e) { e.DrawDefault = true; };
        playerList.SelectedIndexChanged += delegate
        {
            if (!updatingSelection)
                ShowSelectedPlayer();
        };
        Controls.Add(playerList);

        playerActions = new Panel { BackColor = LauncherForm.Panel, Bounds = new Rectangle(16, 406, 520, 168),
            Anchor = AnchorStyles.Bottom | AnchorStyles.Left };
        rank = HostFormChoice(RankNames);
        rank.Bounds = new Rectangle(120, 12, 150, 24);
        rank.SelectedIndexChanged += delegate
        {
            Player player = SelectedPlayer();
            if (!updatingSelection && player != null && rank.SelectedIndex >= 0 && rank.SelectedIndex != player.Rank)
                Send("rank", player.Pid.ToString(), rank.SelectedIndex.ToString());
        };
        var kick = LauncherForm.MakeButton("Kick", new Rectangle(290, 10, 100, 28), 9.5f);
        kick.Click += delegate { OnPlayerAction("kick", "Kick {0} from the server?"); };
        var ban = LauncherForm.MakeButton("Ban", new Rectangle(400, 10, 100, 28), 9.5f);
        ban.Click += delegate
        {
            OnPlayerAction("banplayer", "Ban {0}? Their account and the addresses they played from are banned.");
        };
        tellText = MakeTextBox(new Rectangle(120, 52, 270, 24));
        var tell = LauncherForm.MakeButton("Tell", new Rectangle(400, 50, 100, 28), 9.5f);
        tell.Click += delegate { OnTell(); };
        teleportTarget = HostFormChoice(new string[0]);
        teleportTarget.Bounds = new Rectangle(120, 92, 270, 24);
        var teleport = LauncherForm.MakeButton("Teleport", new Rectangle(400, 90, 100, 28), 9.5f);
        teleport.Click += delegate { OnTeleport(); };
        weather = HostFormChoice(WeatherNames);
        weather.Bounds = new Rectangle(120, 132, 270, 24);
        weather.SelectedIndex = 0;
        var setWeather = LauncherForm.MakeButton("Set", new Rectangle(400, 130, 100, 28), 9.5f);
        setWeather.Click += delegate
        {
            Player player = SelectedPlayer();
            if (player != null && weather.SelectedIndex >= 0)
                Send("weather", player.Pid.ToString(), weather.SelectedIndex.ToString());
        };
        playerActions.Controls.AddRange(new Control[] {
            ActionLabel("Staff rank", 14), rank, kick, ban,
            ActionLabel("Message", 54), tellText, tell,
            ActionLabel("Teleport to", 94), teleportTarget, teleport,
            ActionLabel("Weather here", 134), weather, setWeather });
        Controls.Add(playerActions);

        // On the right: the server's log, the bans and the world, a page at a time
        string[] pageNames = { "Log", "Bans", "World" };
        pages = new Panel[pageNames.Length];
        pageButtons = new Button[pageNames.Length];
        for (int i = 0; i < pageNames.Length; i++)
        {
            int page = i;
            pageButtons[i] = LauncherForm.MakeButton(pageNames[i], new Rectangle(552 + i * 102, 40, 96, 28), 9.5f);
            pageButtons[i].Anchor = AnchorStyles.Top | AnchorStyles.Right;
            pageButtons[i].Click += delegate { ShowPage(page); };
            pages[i] = new Panel { Bounds = new Rectangle(552, 76, 372, 498), BackColor = LauncherForm.Back,
                Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Right };
            Controls.Add(pageButtons[i]);
            Controls.Add(pages[i]);
        }

        log = new TextBox { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, WordWrap = true,
            BackColor = LauncherForm.Panel, ForeColor = LauncherForm.Sand, BorderStyle = BorderStyle.FixedSingle,
            Font = new Font("Consolas", 8.5f), Bounds = new Rectangle(0, 0, 372, 456),
            Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right };
        sayText = MakeTextBox(new Rectangle(0, 470, 262, 24));
        sayText.Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
        sayText.KeyDown += delegate(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter) { e.SuppressKeyPress = true; OnSay(); }
        };
        var say = LauncherForm.MakeButton("To everyone", new Rectangle(270, 468, 102, 28), 9f);
        say.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
        say.Click += delegate { OnSay(); };
        pages[0].Controls.AddRange(new Control[] { log, sayText, say });

        bannedNames = MakeList(new Rectangle(0, 22, 372, 150));
        var unbanName = LauncherForm.MakeButton("Unban", new Rectangle(272, 178, 100, 28), 9.5f);
        unbanName.Click += delegate
        {
            if (bannedNames.SelectedItem != null)
                Send("unbanname", bannedNames.SelectedItem.ToString());
        };
        bannedIps = MakeList(new Rectangle(0, 236, 372, 120));
        var unbanIp = LauncherForm.MakeButton("Unban", new Rectangle(272, 362, 100, 28), 9.5f);
        unbanIp.Click += delegate
        {
            if (bannedIps.SelectedItem != null)
                Send("unbanip", bannedIps.SelectedItem.ToString());
        };
        banText = MakeTextBox(new Rectangle(0, 432, 372, 24));
        var banName = LauncherForm.MakeButton("Ban account", new Rectangle(0, 464, 140, 28), 9.5f);
        banName.Click += delegate { OnBan("banname"); };
        var banIp = LauncherForm.MakeButton("Ban address", new Rectangle(150, 464, 140, 28), 9.5f);
        banIp.Click += delegate { OnBan("banip"); };
        pages[1].Controls.AddRange(new Control[] {
            PageLabel("Banned accounts", 0), bannedNames, unbanName,
            PageLabel("Banned addresses", 214), bannedIps, unbanIp,
            PageLabel("Ban an account (its name) or an IP address", 410), banText, banName, banIp });

        hour = new NumericUpDown { Minimum = 0, Maximum = 23, Bounds = new Rectangle(0, 24, 80, 24),
            BackColor = LauncherForm.Panel, ForeColor = LauncherForm.Sand };
        var setHour = LauncherForm.MakeButton("Set the time", new Rectangle(92, 22, 130, 28), 9.5f);
        setHour.Click += delegate { Send("hour", ((int)hour.Value).ToString()); };
        cellText = MakeTextBox(new Rectangle(0, 106, 372, 24));
        var pickedCell = LauncherForm.MakeButton("The picked player's", new Rectangle(0, 138, 180, 28), 9f);
        pickedCell.Click += delegate
        {
            Player player = SelectedPlayer();
            if (player != null)
                cellText.Text = player.Cell;
        };
        var resetCell = LauncherForm.MakeButton("Reset the cell", new Rectangle(232, 138, 140, 28), 9.5f);
        resetCell.Click += delegate { OnResetCell(); };
        var startup = LauncherForm.MakeButton("Run the startup scripts again", new Rectangle(0, 230, 260, 28), 9.5f);
        startup.Click += delegate { Send("startup"); };
        pages[2].Controls.AddRange(new Control[] {
            PageLabel("Time of day (hour)", 0), hour, setHour,
            PageLabel("Reset a cell: everything in it back as at the start (like Seyda Neen, Census and Excise " +
                "Office)", 66, 40), cellText, pickedCell, resetCell,
            PageLabel("Morrowind's startup scripts put some actors and objects in place. They run by themselves " +
                "when the first player logs in; again after a cell is reset.", 182, 44), startup });

        // At the bottom: save, restart, stop
        var save = LauncherForm.MakeButton("Save now", new Rectangle(16, 592, 120, 32), 9.5f);
        save.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
        save.Click += delegate { Send("save"); };
        restartButton = LauncherForm.MakeButton("Restart", new Rectangle(686, 592, 110, 32), 9.5f);
        restartButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
        restartButton.Click += delegate { OnRestart(); };
        stopButton = LauncherForm.MakeButton("Stop", new Rectangle(808, 592, 116, 32), 9.5f);
        stopButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
        stopButton.Click += delegate { OnStop(); };
        Controls.AddRange(new Control[] { save, restartButton, stopButton });

        ShowPage(0);
        ShowSelectedPlayer();

        refresh = new System.Windows.Forms.Timer { Interval = 1000 };
        refresh.Tick += delegate { ReadStatus(); };
        refresh.Start();
        FormClosing += OnClosing;
        FormClosed += delegate { refresh.Stop(); refresh.Dispose(); };
    }

    public bool IsRunning { get { return process != null && !process.HasExited; } }

    // Starts the server; false (and why, in the log) if it can't be
    public bool StartServer()
    {
        try { File.Delete(Path.Combine(serverDir, CommandsFile)); } catch { }
        try { File.Delete(Path.Combine(serverDir, StatusFile)); } catch { }
        players = new List<Player>();
        lastStatusTime = 0;
        try
        {
            var info = new ProcessStartInfo(Path.Combine(engineDir, "tes3mp-server.exe"))
            {
                WorkingDirectory = serverDir,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8,
                StandardErrorEncoding = Encoding.UTF8,
            };
            process = new Process { StartInfo = info, EnableRaisingEvents = true };
            process.OutputDataReceived += delegate(object sender, DataReceivedEventArgs e) { AddLog(e.Data); };
            process.ErrorDataReceived += delegate(object sender, DataReceivedEventArgs e) { AddLog(e.Data); };
            process.Exited += delegate { BeginInvoke(new Action(OnExited)); };
            process.Start();
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();
        }
        catch (Exception e)
        {
            AddLog("Could not start the server: " + e.Message);
            process = null;
            return false;
        }
        AddLog("--- Server started on port " + port + ". To play on it yourself, press Join in Isowind (the address " +
            "is set to this computer). Friends join with this computer's internet address, which needs UDP port " +
            port + " forwarded to it, or a shared VPN (see README.txt).");
        UpdateSummary();
        return true;
    }

    void OnExited()
    {
        AddLog("--- The server has stopped" + (process != null ? " (exit code " + process.ExitCode + ")" : "") + ".");
        players = new List<Player>();
        FillPlayers();
        UpdateSummary();
        if (restarting)
        {
            restarting = false;
            StartServer();
        }
        else if (stopping)
            Close();
    }

    // A command for the server's scripts, one line in the commands file
    void Send(string verb, params string[] arguments)
    {
        if (!IsRunning)
        {
            AddLog("The server isn't running.");
            return;
        }
        var line = new StringBuilder();
        line.Append(nextCommandId++).Append('\t').Append(verb);
        foreach (string argument in arguments)
            line.Append('\t').Append(Regex.Replace(argument ?? "", @"[\t\r\n]", " "));
        line.Append('\n');
        try
        {
            File.AppendAllText(Path.Combine(serverDir, CommandsFile), line.ToString(), new UTF8Encoding(false));
        }
        catch (Exception e)
        {
            AddLog("Could not send that to the server: " + e.Message);
        }
    }

    void ReadStatus()
    {
        if (!IsRunning)
            return;
        string[] lines;
        try
        {
            using (var stream = new FileStream(Path.Combine(serverDir, StatusFile), FileMode.Open, FileAccess.Read,
                FileShare.ReadWrite | FileShare.Delete))
            using (var reader = new StreamReader(stream, Encoding.UTF8))
                lines = reader.ReadToEnd().Split('\n');
        }
        catch (IOException)
        {
            UpdateSummary();
            return;
        }
        catch (UnauthorizedAccessException)
        {
            return;
        }

        var newPlayers = new List<Player>();
        var names = new List<string>();
        var ips = new List<string>();
        long statusTime = 0;
        string uptime = null, worldHour = null;
        foreach (string raw in lines)
        {
            string[] f = raw.TrimEnd('\r').Split('\t');
            switch (f[0])
            {
                case "time":
                    long.TryParse(f[1], out statusTime);
                    break;
                case "uptime":
                    uptime = f[1];
                    break;
                case "world":
                    worldHour = f[1];
                    break;
                case "player":
                    if (f.Length >= 9)
                    {
                        var player = new Player { Name = f[2], Ip = f[5], Cell = f[6], Region = f[7], Level = f[8] };
                        int.TryParse(f[1], out player.Pid);
                        int.TryParse(f[3], out player.Rank);
                        int.TryParse(f[4], out player.Ping);
                        newPlayers.Add(player);
                    }
                    break;
                case "banname":
                    names.Add(f[1]);
                    break;
                case "banip":
                    ips.Add(f[1]);
                    break;
                case "result":
                    int id;
                    if (f.Length >= 4 && int.TryParse(f[1], out id) && id > lastResultShown)
                    {
                        lastResultShown = id;
                        AddLog((f[2] == "1" ? "Done: " : "Not done: ") + f[3]);
                    }
                    break;
            }
        }
        if (statusTime != lastStatusTime)
        {
            lastStatusTime = statusTime;
            lastStatusChange = DateTime.Now;
        }
        players = newPlayers;
        FillPlayers();
        FillList(bannedNames, names);
        FillList(bannedIps, ips);
        UpdateSummary(uptime, worldHour);
    }

    void UpdateSummary(string uptime = null, string worldHour = null)
    {
        string state;
        if (!IsRunning)
            state = "stopped";
        else if (DateTime.Now - lastStatusChange > TimeSpan.FromSeconds(5))
            state = "starting up";
        else
            state = "running";
        var text = new StringBuilder(serverName + "  -  port " + port + "  -  " + state);
        if (IsRunning && state == "running")
        {
            text.Append("  -  " + players.Count + " / " + maxPlayers + " players");
            int seconds;
            if (uptime != null && int.TryParse(uptime, out seconds))
                text.Append("  -  up " + TimeSpan.FromSeconds(seconds).ToString(@"h\:mm\:ss"));
            double gameHour;
            if (worldHour != null && double.TryParse(worldHour, System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture, out gameHour))
                text.Append(string.Format("  -  {0:00}:{1:00} in the game", Math.Floor(gameHour),
                    Math.Floor(gameHour % 1 * 60)));
        }
        summary.Text = text.ToString();
        stopButton.Text = IsRunning ? "Stop" : "Start";
        restartButton.Enabled = IsRunning;
    }

    // The player list, updated in place (a row per player, the one picked stays picked)
    void FillPlayers()
    {
        updatingSelection = true;
        playerList.BeginUpdate();
        for (int i = playerList.Items.Count - 1; i >= 0; i--)
        {
            var shown = (Player)playerList.Items[i].Tag;
            if (!players.Any(p => p.Pid == shown.Pid && p.Name == shown.Name))
                playerList.Items.RemoveAt(i);
        }
        foreach (Player player in players)
        {
            string[] texts = { player.Name, RankName(player.Rank), player.Ping.ToString(), player.Level, player.Cell };
            ListViewItem item = playerList.Items.Cast<ListViewItem>().FirstOrDefault(
                row => ((Player)row.Tag).Pid == player.Pid);
            if (item == null)
            {
                playerList.Items.Add(new ListViewItem(texts) { Tag = player });
                continue;
            }
            item.Tag = player;
            for (int column = 0; column < texts.Length; column++)
                if (item.SubItems[column].Text != texts[column])
                    item.SubItems[column].Text = texts[column];
        }
        playerList.EndUpdate();
        updatingSelection = false;
        ShowSelectedPlayer();
    }

    static void FillList(ListBox list, List<string> items)
    {
        if (list.Items.Cast<string>().SequenceEqual(items))
            return;
        object picked = list.SelectedItem;
        list.Items.Clear();
        list.Items.AddRange(items.ToArray());
        if (picked != null && items.Contains((string)picked))
            list.SelectedItem = picked;
    }

    Player SelectedPlayer()
    {
        return playerList.SelectedItems.Count > 0 ? (Player)playerList.SelectedItems[0].Tag : null;
    }

    // What the actions show for the player picked; left alone while a drop-down is open, and only changed when it
    // differs (the list refreshes every second)
    void ShowSelectedPlayer()
    {
        // (The actions stay enabled, as Windows draws a disabled drop-down white; without a player picked they do
        // nothing)
        Player player = SelectedPlayer();
        updatingSelection = true;
        int rankIndex = player != null ? Math.Max(0, Math.Min(3, player.Rank)) : -1;
        if (!rank.DroppedDown && rank.SelectedIndex != rankIndex)
            rank.SelectedIndex = rankIndex;
        var others = player == null ? new List<string>() :
            players.Where(p => p.Pid != player.Pid).Select(p => p.Name).ToList();
        if (!teleportTarget.DroppedDown && !teleportTarget.Items.Cast<string>().SequenceEqual(others))
        {
            object target = teleportTarget.SelectedItem;
            teleportTarget.Items.Clear();
            teleportTarget.Items.AddRange(others.ToArray());
            if (target != null && others.Contains((string)target))
                teleportTarget.SelectedItem = target;
        }
        updatingSelection = false;
    }

    static string RankName(int rank)
    {
        return rank >= 0 && rank < RankNames.Length ? RankNames[rank] : rank.ToString();
    }

    void OnPlayerAction(string verb, string question)
    {
        Player player = SelectedPlayer();
        if (player != null && MessageBox.Show(this, string.Format(question, player.Name), Text,
            MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            Send(verb, player.Pid.ToString());
    }

    void OnTell()
    {
        Player player = SelectedPlayer();
        if (player == null || tellText.Text.Trim().Length == 0)
            return;
        Send("tell", player.Pid.ToString(), tellText.Text.Trim());
        tellText.Clear();
    }

    void OnTeleport()
    {
        Player player = SelectedPlayer();
        if (player == null || teleportTarget.SelectedItem == null)
            return;
        Player target = players.FirstOrDefault(p => p.Name == (string)teleportTarget.SelectedItem);
        if (target != null)
            Send("teleport", player.Pid.ToString(), target.Pid.ToString());
    }

    void OnSay()
    {
        if (sayText.Text.Trim().Length == 0)
            return;
        Send("say", sayText.Text.Trim());
        sayText.Clear();
    }

    void OnBan(string verb)
    {
        string what = banText.Text.Trim();
        if (what.Length == 0)
            return;
        Send(verb, what);
        banText.Clear();
    }

    void OnResetCell()
    {
        string cell = cellText.Text.Trim();
        if (cell.Length > 0 && MessageBox.Show(this, "Reset " + cell + "? Everything in it goes back to how it " +
            "was at the start.", Text, MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            Send("resetcell", cell);
    }

    void OnRestart()
    {
        if (!IsRunning || !ConfirmWithPlayers("Restart the server?"))
            return;
        restarting = true;
        Send("stop");
    }

    void OnStop()
    {
        if (!IsRunning)
        {
            StartServer();
            return;
        }
        if (ConfirmWithPlayers("Stop the server?"))
            Send("stop");
    }

    bool ConfirmWithPlayers(string question)
    {
        if (players.Count == 0)
            return true;
        return MessageBox.Show(this, question + " " + players.Count + (players.Count == 1 ? " player is" :
            " players are") + " in the world.", Text, MessageBoxButtons.YesNo, MessageBoxIcon.Question) ==
            DialogResult.Yes;
    }

    // Closing stops the server, saved; it closes when the server has
    void OnClosing(object sender, FormClosingEventArgs e)
    {
        if (!IsRunning)
            return;
        if (stopping)
        {
            // Asked to stop and still running: the scripts didn't answer, so it goes
            if (DateTime.Now - lastStatusChange > TimeSpan.FromSeconds(10))
                try { process.Kill(); } catch { }
            else
                e.Cancel = true;
            return;
        }
        if (!ConfirmWithPlayers("Close the window and stop the server?"))
        {
            e.Cancel = true;
            return;
        }
        e.Cancel = true;
        stopping = true;
        Send("stop");
        AddLog("--- Stopping the server...");
        // If the scripts don't stop it, it is stopped anyway
        var force = new System.Windows.Forms.Timer { Interval = 15000 };
        force.Tick += delegate
        {
            force.Stop();
            if (IsRunning)
                try { process.Kill(); } catch { }
        };
        force.Start();
    }

    void AddLog(string line)
    {
        if (line == null)
            return;
        if (InvokeRequired)
        {
            try { BeginInvoke(new Action<string>(AddLog), line); } catch (InvalidOperationException) { }
            return;
        }
        // The log keeps its last 3000 lines
        if (log.Lines.Length > 3000)
            log.Lines = log.Lines.Skip(1000).ToArray();
        log.AppendText(line + Environment.NewLine);
    }

    void ShowPage(int page)
    {
        for (int i = 0; i < pages.Length; i++)
        {
            pages[i].Visible = i == page;
            pageButtons[i].ForeColor = i == page ? LauncherForm.Gold : LauncherForm.Dim;
        }
    }

    static Label ActionLabel(string text, int y)
    {
        return new Label { Text = text, ForeColor = LauncherForm.Gold, Bounds = new Rectangle(12, y, 104, 22) };
    }

    static Label PageLabel(string text, int y, int height = 20)
    {
        return new Label { Text = text, ForeColor = LauncherForm.Gold, Bounds = new Rectangle(0, y, 372, height) };
    }

    static TextBox MakeTextBox(Rectangle bounds)
    {
        return new TextBox { Bounds = bounds, BackColor = LauncherForm.Panel, ForeColor = LauncherForm.Sand,
            BorderStyle = BorderStyle.FixedSingle };
    }

    static ListBox MakeList(Rectangle bounds)
    {
        return new ListBox { Bounds = bounds, BackColor = LauncherForm.Panel, ForeColor = LauncherForm.Sand,
            BorderStyle = BorderStyle.FixedSingle, IntegralHeight = false };
    }

    static ComboBox HostFormChoice(string[] items)
    {
        return HostForm.MakeChoice(items);
    }
}

// Pixel art textures: every Morrowind texture at half size with at most 32 colours, as uncompressed .dds files that
// OpenMW prefers over the originals. UI art and water are left alone.
static class TextureBuilder
{

    static readonly Regex Skip = new Regex(
        @"^textures\\(levelup|birthsigns|magicitem|water|nvwater)\\" +
        @"|^textures\\(menu_|tx_menu|cursor|compass|mapfow|target|scroll|bookicon|tx_ui_)" +
        @"|_icon\.dds$",
        RegexOptions.IgnoreCase);

    const int TextureScale = 2;
    const int Colours = 32;
    const int MinSize = 16;

    class Entry
    {
        public string Archive;
        public string Name;
        public long Offset;
        public int Size;
    }

    // Builds the textures into <root>\pixelart\textures. Returns null when done, otherwise what went wrong.
    public static string Build(string root, Action<string, int, int> report)
    {
        string texturesDir = Path.Combine(root, "pixelart", "textures");
        try
        {
            report("Finding Morrowind's archives...", 0, 1);
            List<string> archives = FindArchives();
            if (archives.Count == 0)
                return "Couldn't find Morrowind.bsa. Check the Morrowind folder in the OpenMW launcher.";

            // Later archives override earlier ones, as in OpenMW.
            var entries = new Dictionary<string, Entry>(StringComparer.OrdinalIgnoreCase);
            foreach (string archive in archives)
            {
                foreach (Entry entry in ReadBsa(archive))
                {
                    if (entry.Name.StartsWith("textures\\", StringComparison.OrdinalIgnoreCase)
                        && entry.Name.EndsWith(".dds", StringComparison.OrdinalIgnoreCase) && !Skip.IsMatch(entry.Name))
                        entries[entry.Name] = entry;
                }
            }

            int total = entries.Count;
            int done = 0;
            foreach (var group in entries.Values.GroupBy(x => x.Archive))
            {
                using (var stream = new FileStream(group.Key, FileMode.Open, FileAccess.Read, FileShare.Read))
                {
                    // Read a batch in file order, then convert it on all cores.
                    foreach (var batch in Batches(group.OrderBy(x => x.Offset), 256))
                    {
                        var blobs = new List<KeyValuePair<Entry, byte[]>>();
                        foreach (Entry entry in batch)
                        {
                            var data = new byte[entry.Size];
                            stream.Seek(entry.Offset, SeekOrigin.Begin);
                            ReadExactly(stream, data);
                            blobs.Add(new KeyValuePair<Entry, byte[]>(entry, data));
                        }
                        Parallel.ForEach(blobs, blob =>
                        {
                            try { ConvertTexture(root, blob.Key.Name, blob.Value); }
                            catch { } // a few odd formats are not worth failing for; the original is used instead
                        });
                        done += blobs.Count;
                        report("Building pixel art textures from your Morrowind files... " + done + " / " + total,
                            done, total);
                    }
                }
            }

            Directory.CreateDirectory(texturesDir);
            File.WriteAllText(Path.Combine(texturesDir, "isowind-generated.txt"),
                "Built from " + string.Join(", ", archives) + "\r\n");
            report("Pixel art textures ready.", total, total);
            return null;
        }
        catch (Exception e)
        {
            return "Building the pixel art textures failed:\n" + e.Message;
        }
    }

    static IEnumerable<List<T>> Batches<T>(IEnumerable<T> items, int size)
    {
        var batch = new List<T>(size);
        foreach (T item in items)
        {
            batch.Add(item);
            if (batch.Count == size)
            {
                yield return batch;
                batch = new List<T>(size);
            }
        }
        if (batch.Count > 0)
            yield return batch;
    }

    static void ReadExactly(Stream stream, byte[] buffer)
    {
        int read = 0;
        while (read < buffer.Length)
        {
            int n = stream.Read(buffer, read, buffer.Length - read);
            if (n <= 0)
                throw new EndOfStreamException();
            read += n;
        }
    }

    // The BSAs named by fallback-archive= in the user's openmw.cfg, found in its data= folders (last one wins).
    static List<string> FindArchives()
    {
        var dataDirs = new List<string>();
        var archiveNames = new List<string>();
        string cfg = Path.Combine(LauncherForm.UserConfigDir, "openmw.cfg");
        foreach (string raw in File.ReadAllLines(cfg))
        {
            string line = raw.Trim();
            if (line.StartsWith("data=", StringComparison.OrdinalIgnoreCase))
                dataDirs.Add(Unquote(line.Substring(5)));
            else if (line.StartsWith("fallback-archive=", StringComparison.OrdinalIgnoreCase))
                archiveNames.Add(Unquote(line.Substring(17)));
        }
        if (archiveNames.Count == 0)
            archiveNames.AddRange(new[] { "Morrowind.bsa", "Tribunal.bsa", "Bloodmoon.bsa" });

        var found = new List<string>();
        foreach (string name in archiveNames)
        {
            for (int i = dataDirs.Count - 1; i >= 0; --i)
            {
                string candidate = Path.Combine(dataDirs[i], name);
                if (File.Exists(candidate))
                {
                    found.Add(candidate);
                    break;
                }
            }
        }
        return found;
    }

    // openmw.cfg paths may be quoted, with & escaping the next character.
    static string Unquote(string value)
    {
        value = value.Trim();
        if (!value.StartsWith("\""))
            return value;
        var result = new StringBuilder();
        for (int i = 1; i < value.Length; ++i)
        {
            char c = value[i];
            if (c == '&' && i + 1 < value.Length)
                result.Append(value[++i]);
            else if (c == '"')
                break;
            else
                result.Append(c);
        }
        return result.ToString();
    }

    // Morrowind (version 0x100) BSA: header, (size, offset) records, name offsets, names, hashes, then the data.
    static List<Entry> ReadBsa(string path)
    {
        var entries = new List<Entry>();
        using (var reader = new BinaryReader(File.OpenRead(path)))
        {
            uint version = reader.ReadUInt32();
            uint hashOffset = reader.ReadUInt32();
            int count = reader.ReadInt32();
            if (version != 0x100)
                return entries;
            var sizes = new int[count];
            var offsets = new uint[count];
            for (int i = 0; i < count; ++i)
            {
                sizes[i] = reader.ReadInt32();
                offsets[i] = reader.ReadUInt32();
            }
            var nameOffsets = new uint[count];
            for (int i = 0; i < count; ++i)
                nameOffsets[i] = reader.ReadUInt32();
            long namesStart = 12 + (long)count * 12;
            long dataStart = 12 + (long)hashOffset + (long)count * 8;
            int namesLength = (int)(dataStart - (long)count * 8 - namesStart);
            byte[] names = reader.ReadBytes(namesLength);
            Encoding latin1 = Encoding.GetEncoding(28591);
            for (int i = 0; i < count; ++i)
            {
                int start = (int)nameOffsets[i];
                int end = Array.IndexOf(names, (byte)0, start);
                entries.Add(new Entry
                {
                    Archive = path,
                    Name = latin1.GetString(names, start, end - start),
                    Offset = dataStart + offsets[i],
                    Size = sizes[i],
                });
            }
        }
        return entries;
    }

    static void ConvertTexture(string root, string name, byte[] dds)
    {
        int width, height;
        byte[] rgba = DecodeDds(dds, out width, out height);
        if (rgba == null)
            return;

        bool hasAlpha = false;
        for (int i = 3; i < rgba.Length; i += 4)
        {
            if (rgba[i] < 255) { hasAlpha = true; break; }
        }

        int factor = TextureScale;
        while (factor > 1 && (width / factor < MinSize || height / factor < MinSize))
            factor /= 2;
        if (factor > 1)
            rgba = BoxReduce(rgba, ref width, ref height, factor);

        MedianCut(rgba, Colours);
        if (!hasAlpha)
        {
            for (int i = 3; i < rgba.Length; i += 4)
                rgba[i] = 255;
        }

        string relative = name.Replace('\\', Path.DirectorySeparatorChar);
        string target = Path.Combine(root, "pixelart", relative);
        Directory.CreateDirectory(Path.GetDirectoryName(target));
        File.WriteAllBytes(target, EncodeDds(rgba, width, height));
    }

    // ---- DDS -------------------------------------------------------------------------------------------------------

    static byte[] DecodeDds(byte[] d, out int width, out int height)
    {
        width = height = 0;
        if (d.Length < 128 || d[0] != 'D' || d[1] != 'D' || d[2] != 'S' || d[3] != ' ')
            return null;
        height = BitConverter.ToInt32(d, 12);
        width = BitConverter.ToInt32(d, 16);
        uint pfFlags = BitConverter.ToUInt32(d, 80);
        string fourCC = Encoding.ASCII.GetString(d, 84, 4);
        int bits = BitConverter.ToInt32(d, 88);
        uint rMask = BitConverter.ToUInt32(d, 92), gMask = BitConverter.ToUInt32(d, 96);
        uint bMask = BitConverter.ToUInt32(d, 100), aMask = BitConverter.ToUInt32(d, 104);
        if (width <= 0 || height <= 0 || width > 8192 || height > 8192)
            return null;

        var rgba = new byte[width * height * 4];
        const int offset = 128;
        if ((pfFlags & 0x4) != 0) // compressed
        {
            int blockSize = fourCC == "DXT1" ? 8 : 16;
            if (fourCC != "DXT1" && fourCC != "DXT3" && fourCC != "DXT5")
                return null;
            int blocksX = (width + 3) / 4, blocksY = (height + 3) / 4;
            if (d.Length < offset + blocksX * blocksY * blockSize)
                return null;
            for (int by = 0; by < blocksY; ++by)
            {
                for (int bx = 0; bx < blocksX; ++bx)
                {
                    int block = offset + (by * blocksX + bx) * blockSize;
                    DecodeBlock(d, block, fourCC, rgba, width, height, bx * 4, by * 4);
                }
            }
            return rgba;
        }
        if ((pfFlags & 0x40) != 0 && (bits == 32 || bits == 24)) // uncompressed RGB(A)
        {
            int bytes = bits / 8;
            if (d.Length < offset + width * height * bytes)
                return null;
            bool alpha = (pfFlags & 0x1) != 0 && aMask != 0;
            for (int i = 0; i < width * height; ++i)
            {
                uint p = 0;
                for (int b = 0; b < bytes; ++b)
                    p |= (uint)d[offset + i * bytes + b] << (8 * b);
                rgba[i * 4] = Channel(p, rMask);
                rgba[i * 4 + 1] = Channel(p, gMask);
                rgba[i * 4 + 2] = Channel(p, bMask);
                rgba[i * 4 + 3] = alpha ? Channel(p, aMask) : (byte)255;
            }
            return rgba;
        }
        return null;
    }

    static byte Channel(uint pixel, uint mask)
    {
        if (mask == 0)
            return 0;
        int shift = 0;
        while (((mask >> shift) & 1) == 0)
            ++shift;
        uint max = mask >> shift;
        return (byte)(((pixel & mask) >> shift) * 255 / max);
    }

    static void DecodeBlock(byte[] d, int block, string fourCC, byte[] rgba, int width, int height, int x0, int y0)
    {
        int colorBlock = fourCC == "DXT1" ? block : block + 8;
        ushort c0 = BitConverter.ToUInt16(d, colorBlock), c1 = BitConverter.ToUInt16(d, colorBlock + 2);
        uint indices = BitConverter.ToUInt32(d, colorBlock + 4);
        var colors = new int[4, 4];
        Unpack565(c0, colors, 0);
        Unpack565(c1, colors, 1);
        bool fourColors = fourCC != "DXT1" || c0 > c1;
        for (int c = 0; c < 3; ++c)
        {
            if (fourColors)
            {
                colors[2, c] = (2 * colors[0, c] + colors[1, c]) / 3;
                colors[3, c] = (colors[0, c] + 2 * colors[1, c]) / 3;
            }
            else
            {
                colors[2, c] = (colors[0, c] + colors[1, c]) / 2;
                colors[3, c] = 0;
            }
        }
        colors[0, 3] = colors[1, 3] = colors[2, 3] = 255;
        colors[3, 3] = fourColors ? 255 : 0;

        // DXT5 interpolated alpha
        var alphas = new int[8];
        ulong alphaBits = 0;
        if (fourCC == "DXT5")
        {
            int a0 = d[block], a1 = d[block + 1];
            alphas[0] = a0;
            alphas[1] = a1;
            if (a0 > a1)
            {
                for (int i = 1; i < 7; ++i)
                    alphas[i + 1] = ((7 - i) * a0 + i * a1) / 7;
            }
            else
            {
                for (int i = 1; i < 5; ++i)
                    alphas[i + 1] = ((5 - i) * a0 + i * a1) / 5;
                alphas[6] = 0;
                alphas[7] = 255;
            }
            for (int i = 0; i < 6; ++i)
                alphaBits |= (ulong)d[block + 2 + i] << (8 * i);
        }

        for (int py = 0; py < 4; ++py)
        {
            for (int px = 0; px < 4; ++px)
            {
                int x = x0 + px, y = y0 + py;
                if (x >= width || y >= height)
                    continue;
                int texel = py * 4 + px;
                int index = (int)((indices >> (2 * texel)) & 3);
                int o = (y * width + x) * 4;
                rgba[o] = (byte)colors[index, 0];
                rgba[o + 1] = (byte)colors[index, 1];
                rgba[o + 2] = (byte)colors[index, 2];
                int alpha = colors[index, 3];
                if (fourCC == "DXT3")
                {
                    int nibble = (d[block + texel / 2] >> (4 * (texel % 2))) & 0xF;
                    alpha = nibble * 17;
                }
                else if (fourCC == "DXT5")
                {
                    alpha = alphas[(int)((alphaBits >> (3 * texel)) & 7)];
                }
                rgba[o + 3] = (byte)alpha;
            }
        }
    }

    static void Unpack565(ushort c, int[,] colors, int i)
    {
        int r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
        colors[i, 0] = (r << 3) | (r >> 2);
        colors[i, 1] = (g << 2) | (g >> 4);
        colors[i, 2] = (b << 3) | (b >> 2);
    }

    // Uncompressed 32-bit BGRA .dds without mipmaps (OpenMW generates them).
    static byte[] EncodeDds(byte[] rgba, int width, int height)
    {
        var output = new byte[128 + width * height * 4];
        Action<int, uint> put = (at, v) => BitConverter.GetBytes(v).CopyTo(output, at);
        output[0] = (byte)'D'; output[1] = (byte)'D'; output[2] = (byte)'S'; output[3] = (byte)' ';
        put(4, 124);
        put(8, 0x1 | 0x2 | 0x4 | 0x8 | 0x1000); // caps, height, width, pitch, pixel format
        put(12, (uint)height);
        put(16, (uint)width);
        put(20, (uint)(width * 4));
        put(76, 32);
        put(80, 0x40 | 0x1); // RGB, alpha pixels
        put(88, 32);
        put(92, 0x00FF0000);
        put(96, 0x0000FF00);
        put(100, 0x000000FF);
        put(104, 0xFF000000);
        put(108, 0x1000); // DDSCAPS_TEXTURE
        for (int i = 0; i < width * height; ++i)
        {
            output[128 + i * 4] = rgba[i * 4 + 2];
            output[128 + i * 4 + 1] = rgba[i * 4 + 1];
            output[128 + i * 4 + 2] = rgba[i * 4];
            output[128 + i * 4 + 3] = rgba[i * 4 + 3];
        }
        return output;
    }

    // ---- Image processing ------------------------------------------------------------------------------------------

    static byte[] BoxReduce(byte[] rgba, ref int width, ref int height, int factor)
    {
        int w = width / factor, h = height / factor;
        var output = new byte[w * h * 4];
        int area = factor * factor;
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                for (int c = 0; c < 4; ++c)
                {
                    int sum = 0;
                    for (int dy = 0; dy < factor; ++dy)
                    {
                        for (int dx = 0; dx < factor; ++dx)
                            sum += rgba[((y * factor + dy) * width + x * factor + dx) * 4 + c];
                    }
                    output[(y * w + x) * 4 + c] = (byte)(sum / area);
                }
            }
        }
        width = w;
        height = h;
        return output;
    }

    // Median cut: split the pixels into boxes along the widest colour channel at the median until there are `colours`
    // boxes, then give every pixel the average colour of its box. Alpha is kept as it is.
    static void MedianCut(byte[] rgba, int colours)
    {
        int count = rgba.Length / 4;
        var order = Enumerable.Range(0, count).ToArray();
        var boxes = new List<int[]> { new[] { 0, count } }; // [start, end) into order

        while (boxes.Count < colours)
        {
            int bestBox = -1, bestChannel = 0, bestRange = 0;
            for (int b = 0; b < boxes.Count; ++b)
            {
                int start = boxes[b][0], end = boxes[b][1];
                if (end - start < 2)
                    continue;
                for (int c = 0; c < 3; ++c)
                {
                    int lo = 255, hi = 0;
                    for (int i = start; i < end; ++i)
                    {
                        int v = rgba[order[i] * 4 + c];
                        if (v < lo) lo = v;
                        if (v > hi) hi = v;
                    }
                    if (hi - lo > bestRange)
                    {
                        bestRange = hi - lo;
                        bestBox = b;
                        bestChannel = c;
                    }
                }
            }
            if (bestBox < 0)
                break; // every box is a single colour

            int s = boxes[bestBox][0], e = boxes[bestBox][1];
            var keys = new int[e - s];
            var items = new int[e - s];
            for (int i = s; i < e; ++i)
            {
                keys[i - s] = rgba[order[i] * 4 + bestChannel];
                items[i - s] = order[i];
            }
            Array.Sort(keys, items);
            Array.Copy(items, 0, order, s, e - s);
            int middle = s + (e - s) / 2;
            boxes[bestBox] = new[] { s, middle };
            boxes.Add(new[] { middle, e });
        }

        foreach (int[] box in boxes)
        {
            int start = box[0], end = box[1];
            if (end <= start)
                continue;
            long r = 0, g = 0, bl = 0;
            for (int i = start; i < end; ++i)
            {
                int p = order[i] * 4;
                r += rgba[p];
                g += rgba[p + 1];
                bl += rgba[p + 2];
            }
            int n = end - start;
            byte rr = (byte)(r / n), gg = (byte)(g / n), bb = (byte)(bl / n);
            for (int i = start; i < end; ++i)
            {
                int p = order[i] * 4;
                rgba[p] = rr;
                rgba[p + 1] = gg;
                rgba[p + 2] = bb;
            }
        }
    }
}
