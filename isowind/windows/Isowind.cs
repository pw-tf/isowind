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
    readonly CheckBox controller;
    readonly Button play;
    readonly Button openmwLauncher;
    readonly TextBox serverAddress;
    readonly Button join;
    readonly Button host;
    readonly Label status;
    readonly ProgressBar progress;

    string EngineDir { get { return Path.Combine(root, "engine"); } }
    string TexturesDir { get { return Path.Combine(root, "pixelart", "textures"); } }
    string TexturesMarker { get { return Path.Combine(TexturesDir, "isowind-generated.txt"); } }
    string ModeFile { get { return Path.Combine(root, "profiles", "launcher-mode.txt"); } }
    string ServerFile { get { return Path.Combine(root, "profiles", "launcher-server.txt"); } }
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
        ClientSize = new Size(460, 558);
        BackColor = Back;
        ForeColor = Sand;
        Font = new Font("Segoe UI", 10f);
        try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

        var title = new Label
        {
            Text = "ISOWIND",
            Font = new Font("Georgia", 30f, FontStyle.Bold),
            ForeColor = Gold,
            AutoSize = false,
            TextAlign = ContentAlignment.MiddleCenter,
            Bounds = new Rectangle(0, 18, 460, 54),
        };
        var subtitle = new Label
        {
            Text = "Morrowind, seen from above",
            ForeColor = Dim,
            AutoSize = false,
            TextAlign = ContentAlignment.MiddleCenter,
            Bounds = new Rectangle(0, 72, 460, 22),
        };

        var look = new Panel { BackColor = Panel, Bounds = new Rectangle(30, 110, 400, 146) };
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
        // Menus made for a controller or Morrowind's own: ticked when one is plugged in (ControllerConnected), and
        // the player's to change, as in the Linux launcher window
        controller = new CheckBox
        {
            Text = "Controller  -  menus made for a controller rather than a mouse",
            Bounds = new Rectangle(14, 96, 380, 26),
            ForeColor = Sand,
        };
        var hint = new Label
        {
            Text = "The top-down camera works in both. The zoom (or F7) in game switches camera.",
            ForeColor = Dim,
            Font = new Font("Segoe UI", 8.5f),
            Bounds = new Rectangle(14, 122, 380, 18),
        };
        look.Controls.AddRange(new Control[] { lookTitle, pixelArt, normal, controller, hint });

        play = MakeButton("Play", new Rectangle(30, 272, 400, 52), 16f);
        play.Click += delegate { Launch(null); };

        // Multiplayer: join a server by its address, or host one on this computer
        var multiplayer = new Panel { BackColor = Panel, Bounds = new Rectangle(30, 336, 400, 122) };
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
            new Rectangle(30, 470, 300, 34), 9f);
        openmwLauncher.Click += delegate { StartOpenMWLauncher(); };
        var licenses = MakeButton("Licenses", new Rectangle(340, 470, 90, 34), 9.5f);
        licenses.Click += delegate { ShowLicenses(); };

        progress = new ProgressBar { Bounds = new Rectangle(30, 512, 400, 12), Visible = false };
        status = new Label { ForeColor = Dim, Bounds = new Rectangle(30, 528, 400, 22), Font = new Font("Segoe UI", 9f) };

        Controls.AddRange(new Control[] { title, subtitle, look, play, multiplayer, openmwLauncher, licenses, progress,
            status });

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
        controller.Checked = ControllerConnected();

        if (!File.Exists(Path.Combine(EngineDir, "openmw.exe")))
            status.Text = "engine\\openmw.exe is missing - unpack the whole Isowind folder.";
        else if (!MorrowindConfigured())
            status.Text = "First time? Press Play to set up your Morrowind installation.";
    }

    // Who made what Isowind is built from and ships with, and under which terms. The full texts are in licenses\.
    const string Credits =
        "Isowind is a modified OpenMW with a multiplayer port of TES3MP. It needs your own copy of The Elder Scrolls III: " +
        "Morrowind (Bethesda Softworks); none of the game's files are included.\r\n\r\n" +
        "ENGINE\r\n" +
        "OpenMW - openmw.org - by the OpenMW team and contributors (licenses\\OpenMW-AUTHORS.md). " +
        "GNU General Public License v3 (licenses\\GPL-3.0.txt). Isowind's changes are under the same license; " +
        "the source is in the source folder.\r\n\r\n" +
        "TES3MP - tes3mp.com - by David Cernat, Stanislav Zhukov and contributors. The multiplayer code in Isowind is " +
        "ported from it, and the server runs its CoreScripts. GNU General Public License v3.\r\n\r\n" +
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
        "(mods\\ExpansionDelay).\r\n\r\n" +
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
            Text = Credits,
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
        Launch("--connect=" + address);
    }

    void OnHost()
    {
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
        if (!Start(Path.Combine(EngineDir, "tes3mp-server.exe"), "", ServerDir, true))
            return;
        serverAddress.Text = port == DefaultPort ? "127.0.0.1" : "127.0.0.1:" + port;
        status.Text = "Server started (its window shows what happens). Press Join to play on it.";
        MessageBox.Show(this,
            "The server is running in its own window; close that window to stop it.\n\n" +
            "To play on it yourself, press Join (the address is set to this computer).\n\n" +
            "Friends join with this computer's internet address (search \"what is my IP\")" +
            (port == DefaultPort ? "" : ":" + port) + ", which needs UDP port " + port + " forwarded to this " +
            "computer in your router. Or all of you join the same VPN (e.g. Tailscale, ZeroTier, Radmin VPN) and " +
            "friends use this computer's VPN address.\n\n" +
            "Everyone needs Morrowind with Tribunal and Bloodmoon, and no other mods turned on.",
            "Isowind - hosting", MessageBoxButtons.OK, MessageBoxIcon.Information);
    }

    async void Launch(string connectArgument)
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
            play.Enabled = openmwLauncher.Enabled = pixelArt.Enabled = normal.Enabled = controller.Enabled = join.Enabled = false;
            progress.Visible = true;
            bool ok = await Task.Run(() => GenerateTextures());
            progress.Visible = false;
            play.Enabled = openmwLauncher.Enabled = pixelArt.Enabled = normal.Enabled = controller.Enabled = join.Enabled = true;
            if (!ok)
                return;
        }

        string profile = Path.Combine(root, "profiles", mode);
        SetControllerMenus(Path.Combine(profile, "settings.cfg"), controller.Checked);
        string arguments = "--config \"" + profile + "\"";
        if (connectArgument != null)
            arguments += " " + connectArgument;
        if (Start(Path.Combine(EngineDir, "openmw.exe"), arguments))
            Close();
    }

    // Menus made for a controller (bumpers switch windows, buttons pick items) or Morrowind's own (drag and drop):
    // the tick box, ticked to begin with when a controller is plugged in at launch.
    [System.Runtime.InteropServices.DllImport("xinput1_4.dll")]
    static extern int XInputGetState(int userIndex, byte[] state);
    [System.Runtime.InteropServices.DllImport("winmm.dll")]
    static extern int joyGetNumDevs();
    [System.Runtime.InteropServices.DllImport("winmm.dll")]
    static extern int joyGetPosEx(int id, int[] info);

    static bool ControllerConnected()
    {
        try
        {
            var state = new byte[16];
            for (int i = 0; i < 4; i++)
                if (XInputGetState(i, state) == 0)
                    return true;
        }
        catch { }
        try
        {
            // JOYINFOEX: 13 ints, the first its size, the second what to return (JOY_RETURNALL)
            for (int id = 0; id < joyGetNumDevs(); id++)
            {
                var info = new int[13];
                info[0] = 13 * 4;
                info[1] = 0xFF;
                if (joyGetPosEx(id, info) == 0)
                    return true;
            }
        }
        catch { }
        return false;
    }

    static void SetControllerMenus(string settingsFile, bool on)
    {
        try { SetConfigValue(settingsFile, "GUI", "controller menus", on ? "true" : "false"); } catch { }
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
        ClientSize = new Size(460, 450);
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

    static ComboBox MakeChoice(string[] items)
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
                    TextFormatFlags.VerticalCenter | TextFormatFlags.Left | TextFormatFlags.NoPrefix);
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
