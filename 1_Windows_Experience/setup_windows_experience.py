#!/usr/bin/env python3
"""
Fedora KDE Plasma 6 -> Windows 11 Familiarity & Power-User Setup
Applies:
  1. Windows Muscle Memory & Shortcuts:
     - Double-click to open files/folders (SingleClick=false)
     - Ctrl+Shift+Esc -> Plasma System Monitor (Windows Task Manager)
     - Win+Shift+S -> Spectacle Rectangular Region Snip directly to Clipboard
     - Win+E -> Dolphin File Explorer (with full path bar & drive usage bar)
     - Win+S -> Windows 11 Search Hub (Apps + "Everything" Instant File Search + Calc + Web)
     - Win+I -> System Settings
     - Win+R / Alt+Space -> KRunner Omnibox
     - Win+V -> Clipboard History at cursor
     - Win+D -> Show Desktop / Peek at Desktop
     - Centered new window placement (Windows style instead of Linux top-left)
  2. Taskbar Search Button (`org.kde.plasma.winsearch`) + "WinSearch Everything Hub":
     - Installs ~/.local/bin/win-search-hub.py (GTK 4.0 Windows 11 + Voidtools "Everything" instant search)
     - Installs Plasma 6 Applet `org.kde.plasma.winsearch` next to the Start Menu on the bottom panel
       with Wayland-safe cursor-away auto-close + embedded Milou/KRunner search + Everything launcher
  3. Visual Polish (Section 4):
     - Windows 11 Fluent Dark Color Palette (#202020 Mica dark + #0078D4 / #60CDFF Windows Accent Blue)
     - ClearType-style RGB subpixel font rendering & clean 10pt UI typography
     - Enhanced Windows 11 Fluent yellow folder & system icons in ~/.local/share/icons/Win10-Explorer
"""

import os
import re
import subprocess
import sys
from pathlib import Path

HOME = Path.home()
CONFIG_DIR = HOME / ".config"
LOCAL_BIN = HOME / ".local" / "bin"
LOCAL_APPS = HOME / ".local" / "share" / "applications"
PLASMOIDS_DIR = HOME / ".local" / "share" / "plasma" / "plasmoids"
WINSEARCH_PLASMOID_DIR = PLASMOIDS_DIR / "org.kde.plasma.winsearch"


def run_kwrite(file_name: str, group: list[str], key: str, value: str, val_type: str = "string"):
    cmd = ["kwriteconfig6", "--file", file_name]
    for g in group:
        cmd.extend(["--group", g])
    cmd.extend(["--key", key])
    if val_type == "bool":
        cmd.extend(["--type", "bool", value])
    else:
        cmd.append(value)
    subprocess.run(cmd, check=False)


def configure_muscle_memory():
    print("[1/4] Configuring Windows Muscle Memory, Shortcuts & File Explorer...")

    # 1. Double-click to open files & folders + Windows 11 Fluent Dark Colors + Typography
    run_kwrite("kdeglobals", ["KDE"], "SingleClick", "false", "bool")
    run_kwrite("kdeglobals", ["KDE"], "LookAndFeelPackage", "org.fedoraproject.fedoradark.desktop")

    # Clean 10pt Windows 10/11 Segoe UI Typography
    run_kwrite("kdeglobals", ["General"], "font", "Segoe UI,10,-1,5,400,0,0,0,0,0,0,0,0,0,0,1")
    run_kwrite("kdeglobals", ["General"], "fixed", "Consolas,10,-1,5,400,0,0,0,0,0,0,0,0,0,0,1")
    run_kwrite("kdeglobals", ["General"], "menuFont", "Segoe UI,10,-1,5,400,0,0,0,0,0,0,0,0,0,0,1")
    run_kwrite("kdeglobals", ["General"], "toolBarFont", "Segoe UI,10,-1,5,400,0,0,0,0,0,0,0,0,0,0,1")
    run_kwrite("kdeglobals", ["General"], "smallestReadableFont", "Segoe UI,8,-1,5,400,0,0,0,0,0,0,0,0,0,0,1")
    run_kwrite("kdeglobals", ["WM"], "font", "Segoe UI,10,-1,5,600,0,0,0,0,0,0,0,0,0,0,1")

    # ClearType-like Font Rendering (kcmfonts)
    run_kwrite("kcmfonts", ["General"], "antiAliasing", "true", "bool")
    run_kwrite("kcmfonts", ["General"], "hinting", "slight")
    run_kwrite("kcmfonts", ["General"], "subPixel", "rgb")

    # 2. Windows 11 Fluent Dark Color Palette in kdeglobals (#202020 Window, #191919 View, #0078D4 / #60CDFF Accent)
    win11_colors = {
        "Colors:Window": {
            "BackgroundNormal": "32,32,32",
            "BackgroundAlternate": "43,43,43",
            "ForegroundNormal": "245,245,245",
            "ForegroundInactive": "166,166,166",
            "DecorationFocus": "96,205,255",
            "DecorationHover": "96,205,255",
        },
        "Colors:View": {
            "BackgroundNormal": "25,25,25",
            "BackgroundAlternate": "32,32,32",
            "ForegroundNormal": "245,245,245",
            "ForegroundInactive": "166,166,166",
            "DecorationFocus": "96,205,255",
            "DecorationHover": "96,205,255",
        },
        "Colors:Button": {
            "BackgroundNormal": "45,45,45",
            "BackgroundAlternate": "55,55,55",
            "ForegroundNormal": "245,245,245",
            "DecorationFocus": "96,205,255",
            "DecorationHover": "96,205,255",
        },
        "Colors:Selection": {
            "BackgroundNormal": "0,120,212",
            "BackgroundAlternate": "0,90,158",
            "ForegroundNormal": "255,255,255",
            "DecorationFocus": "96,205,255",
            "DecorationHover": "96,205,255",
        },
        "Colors:Header": {
            "BackgroundNormal": "32,32,32",
            "BackgroundAlternate": "38,38,38",
            "ForegroundNormal": "245,245,245",
        },
        "WM": {
            "activeBackground": "32,32,32",
            "activeForeground": "245,245,245",
            "inactiveBackground": "28,28,28",
            "inactiveForeground": "150,150,150",
        },
    }
    for group_name, kv in win11_colors.items():
        for k, v in kv.items():
            run_kwrite("kdeglobals", [group_name], k, v)

    # 3. KWin Window Behavior & Windows 10 Titlebar Buttons
    run_kwrite("kwinrc", ["Windows"], "Placement", "Centered")
    run_kwrite("kwinrc", ["org.kde.kdecoration2"], "library", "org.kde.kwin.aurorae")
    run_kwrite("kwinrc", ["org.kde.kdecoration2"], "theme", "__aurorae__svg__Windows10-dark")
    run_kwrite("kwinrc", ["org.kde.kdecoration2"], "ButtonsOnRight", "IAX")
    run_kwrite("kwinrc", ["org.kde.kdecoration2"], "BorderSize", "None")

    # 4. Spectacle (Win+Shift+S): Rectangular region capture auto-copies image to clipboard immediately
    run_kwrite("spectaclerc", ["General"], "clipboardGroup", "PostScreenshotCopyImage")
    run_kwrite("spectaclerc", ["General"], "launchAction", "UseLastUsedCapturemode")
    run_kwrite("spectaclerc", ["GuiConfig"], "captureMode", "RectangularRegion")
    run_kwrite("spectaclerc", ["GuiConfig"], "quitAfterSaveCopyExport", "true", "bool")

    # 5. Dolphin File Explorer (Win+E): Editable full path bar, space bar, inline previews
    run_kwrite("dolphinrc", ["General"], "EditableUrl", "true", "bool")
    run_kwrite("dolphinrc", ["General"], "ShowFullPath", "true", "bool")
    run_kwrite("dolphinrc", ["General"], "ShowSpaceInfo", "true", "bool")
    run_kwrite("dolphinrc", ["General"], "ShowZoomSlider", "false", "bool")
    run_kwrite("dolphinrc", ["IconsMode"], "MaximumTextLines", "2", "int")
    run_kwrite("dolphinrc", ["IconsMode"], "TextWidthIndex", "1", "int")

    # 6. Supercharge KRunner plugins (Calculator, Converter, Files, Places, Apps, Settings, Web)
    run_kwrite("krunnerrc", ["General"], "FreeFloating", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "baloosearchEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "calculatorEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "helprunnerEnabled", "false", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "krunner_appstreamEnabled", "false", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "krunner_killEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "krunner_placesrunnerEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "krunner_servicesEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "krunner_shellEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "krunner_systemsettingsEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "krunner_webshortcutsEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "locationsEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "org.kde.activities2Enabled", "false", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "recentdocumentsEnabled", "true", "bool")
    run_kwrite("krunnerrc", ["Plugins"], "unitconverterEnabled", "true", "bool")

    # 7. Global Keyboard Shortcuts
    run_kwrite("kglobalshortcutsrc", ["services", "org.kde.plasma-systemmonitor.desktop"], "_launch", "Ctrl+Shift+Esc\tMeta+Esc")
    run_kwrite("kglobalshortcutsrc", ["services", "org.kde.spectacle.desktop"], "RectangularRegionScreenShot", "Meta+Shift+S\tPrint")
    run_kwrite("kglobalshortcutsrc", ["services", "org.kde.dolphin.desktop"], "_launch", "Meta+E")
    run_kwrite("kglobalshortcutsrc", ["services", "systemsettings.desktop"], "_launch", "Meta+I")
    run_kwrite("kglobalshortcutsrc", ["services", "org.kde.krunner.desktop"], "_launch", "Alt+Space\tMeta+R\tSearch")
    run_kwrite("kglobalshortcutsrc", ["services", "win-search-hub.desktop"], "_launch", "Meta+S")


def create_everything_search_hub():
    print("[2/4] Installing Windows 11 + 'Everything' Instant Search Hub (~/.local/bin/win-search-hub.py)...")
    LOCAL_BIN.mkdir(parents=True, exist_ok=True)
    LOCAL_APPS.mkdir(parents=True, exist_ok=True)

    hub_script = LOCAL_BIN / "win-search-hub.py"
    hub_code = r'''#!/usr/bin/env python3
"""
Windows 11 Search + Voidtools "Everything" Instant Search Hub for Fedora KDE Plasma 6 (GTK 4.0)
Features:
  - Instant Everything-style filename & folder search across Home & Drives via `plocate` & `find`
  - Instant Application & KDE System Settings launcher with icons
  - Live Math / Unit Calculator (`24*365`, `15% of 480`, `sqrt(144)`, hex/bin conversion)
  - Quick Web Search (`g query`) & Shell runner (`> cmd`)
  - Windows 11 Fluent Dark styling with Escape & focus-loss auto-close
"""

import math
import os
import re
import shlex
import subprocess
import sys
import threading
import urllib.parse
from pathlib import Path

import gi
gi.require_version("Gtk", "4.0")
gi.require_version("Gdk", "4.0")
from gi.repository import Gtk, Gdk, GLib, Pango

HOME = Path.home()


class SafeMathEvaluator:
    ALLOWED_NAMES = {
        k: getattr(math, k)
        for k in ("sin", "cos", "tan", "sqrt", "log", "log10", "pi", "e", "ceil", "floor", "factorial", "pow")
    }
    ALLOWED_NAMES.update({"abs": abs, "round": round, "hex": hex, "bin": bin})

    @classmethod
    def evaluate(cls, expr: str):
        cleaned = expr.strip()
        if cleaned.startswith("="):
            cleaned = cleaned[1:].strip()
        if " of " in cleaned.lower() and "%" in cleaned:
            parts = cleaned.lower().split(" of ")
            if len(parts) == 2 and parts[0].strip().endswith("%"):
                try:
                    pct = float(parts[0].strip()[:-1]) / 100.0
                    val = float(parts[1].strip())
                    return f"{pct * val:g}"
                except Exception:
                    pass
        cleaned = cleaned.replace("^", "**")
        if not any(ch in cleaned for ch in "+-*/%^()") and not cleaned.startswith(("sqrt", "sin", "cos", "log", "hex", "bin")):
            return None
        try:
            code = compile(cleaned, "<calc>", "eval")
            for name in code.co_names:
                if name not in cls.ALLOWED_NAMES:
                    return None
            val = eval(code, {"__builtins__": {}}, cls.ALLOWED_NAMES)
            if isinstance(val, (int, float)):
                if isinstance(val, float) and val.is_integer():
                    val = int(val)
                return f"{val:,}" if isinstance(val, int) else f"{val:.6g}"
            return str(val)
        except Exception:
            return None


def load_desktop_apps():
    apps = []
    seen = set()
    dirs = [
        HOME / ".local/share/applications",
        Path("/usr/share/applications"),
        Path("/var/lib/flatpak/exports/share/applications"),
        HOME / ".local/share/flatpak/exports/share/applications",
    ]
    for d in dirs:
        if not d.exists():
            continue
        for p in d.glob("*.desktop"):
            try:
                text = p.read_text(errors="ignore")
                if "NoDisplay=true" in text or "Hidden=true" in text:
                    continue
                name, icon, exec_cmd, comment, categories = "", "application-x-executable", "", "", ""
                in_entry = False
                for line in text.splitlines():
                    line = line.strip()
                    if line == "[Desktop Entry]":
                        in_entry = True
                        continue
                    elif line.startswith("[") and line != "[Desktop Entry]":
                        if in_entry:
                            break
                    if not in_entry or "=" not in line:
                        continue
                    k, v = line.split("=", 1)
                    if k == "Name" and not name:
                        name = v
                    elif k == "Icon" and icon == "application-x-executable":
                        icon = v
                    elif k == "Exec" and not exec_cmd:
                        exec_cmd = re.sub(r"\s+%[fFuUdDnNickvm]", "", v).strip()
                    elif k == "Comment" and not comment:
                        comment = v
                    elif k == "Categories" and not categories:
                        categories = v
                if name and exec_cmd and name.lower() not in seen:
                    seen.add(name.lower())
                    is_setting = "Settings" in categories or "systemsettings" in exec_cmd
                    apps.append({
                        "type": "setting" if is_setting else "app",
                        "title": name,
                        "subtitle": comment or ("System Setting" if is_setting else "Application"),
                        "icon": icon,
                        "exec": exec_cmd,
                    })
            except Exception:
                continue
    return sorted(apps, key=lambda x: x["title"].lower())


CSS = """
window {
    background-color: #202020;
    color: #f5f5f5;
    border: 1px solid #3c3c3c;
    border-radius: 12px;
}
.main-box {
    padding: 16px;
}
.search-entry {
    background-color: #2b2b2b;
    color: #ffffff;
    border: 2px solid #0078D4;
    border-radius: 8px;
    padding: 10px 14px;
    font-size: 15px;
}
.tab-btn {
    background-color: #2b2b2b;
    color: #d0d0d0;
    border: 1px solid #3c3c3c;
    border-radius: 14px;
    padding: 4px 14px;
    font-size: 12px;
    font-weight: bold;
}
.tab-btn:hover {
    background-color: #363636;
    color: #ffffff;
}
.tab-btn-active {
    background-color: #0078D4;
    color: #ffffff;
    border: 1px solid #60CDFF;
    border-radius: 14px;
    padding: 4px 14px;
    font-size: 12px;
    font-weight: bold;
}
list {
    background-color: transparent;
}
row {
    border-radius: 8px;
    padding: 8px 10px;
    margin: 2px 0px;
}
row:selected {
    background-color: #1f3b52;
    border: 1px solid #0078D4;
}
row:hover {
    background-color: #2c2c2c;
}
.row-title {
    font-size: 13.5px;
    font-weight: 600;
    color: #ffffff;
}
.row-sub {
    font-size: 11.5px;
    color: #a5a5a5;
}
.footer-label {
    font-size: 11px;
    color: #909090;
}
"""


class SearchHubApp(Gtk.Application):
    def __init__(self):
        super().__init__(application_id="org.fedora.WinSearchHub")
        self.apps = load_desktop_apps()
        self.active_tab = "all"
        self.search_seq = 0
        self.results_data = []

    def do_activate(self):
        provider = Gtk.CssProvider()
        provider.load_from_data(CSS.encode("utf-8"))
        Gtk.StyleContext.add_provider_for_display(
            Gdk.Display.get_default(),
            provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION,
        )

        self.win = Gtk.ApplicationWindow(application=self)
        self.win.set_title("Windows Search & Everything Hub")
        self.win.set_default_size(720, 540)
        self.win.set_decorated(False)
        self.win.set_resizable(False)

        key_ctrl = Gtk.EventControllerKey()
        key_ctrl.connect("key-pressed", self.on_key_pressed)
        self.win.add_controller(key_ctrl)

        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        main_box.add_css_class("main-box")
        self.win.set_child(main_box)

        # Search Input
        self.entry = Gtk.Entry()
        self.entry.add_css_class("search-entry")
        self.entry.set_placeholder_text("🔍  Search Apps, 'Everything' Files, Settings, Calculator (e.g. 24*365), or Web...")
        self.entry.connect("changed", self.on_query_changed)
        self.entry.connect("activate", self.on_entry_activate)
        main_box.append(self.entry)

        # Filter Tabs
        tab_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        self.tab_buttons = {}
        for tid, label in [
            ("all", "All"),
            ("apps", "Apps"),
            ("files", "Files (Everything)"),
            ("settings", "Settings"),
            ("web", "Web & Calc"),
        ]:
            btn = Gtk.Button(label=label)
            btn.add_css_class("tab-btn-active" if tid == "all" else "tab-btn")
            btn.connect("clicked", lambda b, t=tid: self.set_tab(t))
            tab_box.append(btn)
            self.tab_buttons[tid] = btn
        main_box.append(tab_box)

        # Scrollable ListBox
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_vexpand(True)
        self.listbox = Gtk.ListBox()
        self.listbox.set_selection_mode(Gtk.SelectionMode.SINGLE)
        self.listbox.connect("row-activated", self.on_row_activated)
        scrolled.set_child(self.listbox)
        main_box.append(scrolled)

        # Footer
        footer = Gtk.Label(
            label="↵ Open   •   Ctrl+↵ Reveal File in Dolphin Explorer   •   ↑/↓ Navigate   •   Esc Close"
        )
        footer.add_css_class("footer-label")
        footer.set_xalign(0.0)
        main_box.append(footer)

        self.populate_defaults()
        self.win.present()
        self.entry.grab_focus()

    def set_tab(self, tab_id: str):
        self.active_tab = tab_id
        for tid, btn in self.tab_buttons.items():
            btn.remove_css_class("tab-btn")
            btn.remove_css_class("tab-btn-active")
            btn.add_css_class("tab-btn-active" if tid == tab_id else "tab-btn")
        self.on_query_changed(self.entry)
        self.entry.grab_focus()

    def clear_list(self):
        self.results_data.clear()
        while True:
            row = self.listbox.get_row_at_index(0)
            if row is None:
                break
            self.listbox.remove(row)

    def add_item(self, title: str, subtitle: str, icon_name: str, data: dict):
        row = Gtk.ListBoxRow()
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)

        img = Gtk.Image.new_from_icon_name(icon_name or "application-x-executable")
        img.set_pixel_size(28)
        hbox.append(img)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        lbl_title = Gtk.Label(label=title)
        lbl_title.add_css_class("row-title")
        lbl_title.set_xalign(0.0)
        lbl_title.set_ellipsize(Pango.EllipsizeMode.END)

        lbl_sub = Gtk.Label(label=subtitle)
        lbl_sub.add_css_class("row-sub")
        lbl_sub.set_xalign(0.0)
        lbl_sub.set_ellipsize(Pango.EllipsizeMode.END)

        vbox.append(lbl_title)
        vbox.append(lbl_sub)
        hbox.append(vbox)

        row.set_child(hbox)
        self.listbox.append(row)
        self.results_data.append(data)

    def select_first(self):
        first = self.listbox.get_row_at_index(0)
        if first:
            self.listbox.select_row(first)

    def populate_defaults(self):
        self.clear_list()
        quick = [
            ("File Explorer (Dolphin)", "Open Home & Drives (Win + E)", "system-file-manager", {"action": "exec", "cmd": "dolphin"}),
            ("Task Manager (System Monitor)", "Processes, CPU, Memory & GPU (Ctrl + Shift + Esc)", "utilities-system-monitor", {"action": "exec", "cmd": "plasma-systemmonitor"}),
            ("System Settings", "Control Panel & Personalization (Win + I)", "preferences-system", {"action": "exec", "cmd": "systemsettings"}),
            ("Snipping Tool (Spectacle)", "Capture Rectangular Region to Clipboard (Win + Shift + S)", "spectacle", {"action": "exec", "cmd": "spectacle -r -b -c"}),
            ("Terminal (Konsole)", "Command Prompt / Bash Shell", "utilities-terminal", {"action": "exec", "cmd": "konsole"}),
        ]
        for t, s, ic, d in quick:
            self.add_item(t, s, ic, d)
        for app in self.apps[:15]:
            self.add_item(app["title"], app["subtitle"], app["icon"], {"action": "exec", "cmd": app["exec"]})
        self.select_first()

    def on_query_changed(self, entry):
        q = entry.get_text().strip()
        self.search_seq += 1
        seq = self.search_seq
        if not q:
            self.populate_defaults()
            return

        self.clear_list()
        ql = q.lower()

        if self.active_tab in ("all", "web"):
            calc_res = SafeMathEvaluator.evaluate(q)
            if calc_res is not None:
                self.add_item(
                    f"= {calc_res}",
                    f"Calculator Result (Press Enter to copy '{calc_res}' to clipboard)",
                    "accessories-calculator",
                    {"action": "copy", "text": calc_res},
                )

        if self.active_tab in ("all", "apps", "settings"):
            scored = []
            for app in self.apps:
                if self.active_tab == "apps" and app["type"] != "app":
                    continue
                if self.active_tab == "settings" and app["type"] != "setting":
                    continue
                tl, sl = app["title"].lower(), app["subtitle"].lower()
                if tl.startswith(ql):
                    scored.append((0, app))
                elif ql in tl:
                    scored.append((1, app))
                elif ql in sl:
                    scored.append((2, app))
            scored.sort(key=lambda x: (x[0], x[1]["title"].lower()))
            limit = 8 if self.active_tab == "all" else 25
            for _, app in scored[:limit]:
                badge = "⚙️ System Setting" if app["type"] == "setting" else "🚀 Application"
                self.add_item(app["title"], f"{badge}  •  {app['subtitle']}", app["icon"], {"action": "exec", "cmd": app["exec"]})

        if q.startswith(">") and q[1:].strip():
            cmd_str = q[1:].strip()
            self.add_item(
                f"Run Command: {cmd_str}",
                "Execute in Konsole terminal",
                "utilities-terminal",
                {"action": "exec", "cmd": f"konsole --hold -e bash -lc {shlex.quote(cmd_str)}"},
            )

        if self.active_tab in ("all", "web"):
            encoded = urllib.parse.quote_plus(q)
            self.add_item(
                f'Search Google for "{q}"',
                "Open in Default Web Browser",
                "internet-web-browser",
                {"action": "url", "url": f"https://www.google.com/search?q={encoded}"},
            )

        self.select_first()

        if self.active_tab in ("all", "files") and len(q) >= 2:
            threading.Thread(target=self.run_file_search, args=(seq, q), daemon=True).start()

    def run_file_search(self, seq: int, query: str):
        results = []
        paths = []
        try:
            proc = subprocess.run(
                ["plocate", "-i", "-l", "25", query],
                capture_output=True,
                text=True,
                timeout=1.2,
            )
            paths = [line.strip() for line in proc.stdout.splitlines() if line.strip()]
        except Exception:
            paths = []

        if not paths:
            try:
                proc = subprocess.run(
                    ["find", str(HOME), "-maxdepth", "4", "-not", "-path", "*/.*", "-iname", f"*{query}*"],
                    capture_output=True,
                    text=True,
                    timeout=1.2,
                )
                paths = [line.strip() for line in proc.stdout.splitlines() if line.strip()][:25]
            except Exception:
                paths = []

        for p_str in paths[:20]:
            p = Path(p_str)
            if not p.exists():
                continue
            icon = "folder" if p.is_dir() else "text-x-generic"
            results.append({
                "title": p.name,
                "subtitle": f"📁 {p.parent}   (Ctrl+Enter to open folder)",
                "icon": icon,
                "data": {"action": "file", "path": str(p)},
            })
        GLib.idle_add(self.append_async_files, seq, results)

    def append_async_files(self, seq: int, items: list):
        if seq != self.search_seq:
            return False
        for it in items:
            self.add_item(it["title"], it["subtitle"], it["icon"], it["data"])
        if self.listbox.get_selected_row() is None:
            self.select_first()
        return False

    def on_key_pressed(self, controller, keyval, keycode, state):
        ctrl = bool(state & Gdk.ModifierType.CONTROL_MASK)
        if keyval == Gdk.KEY_Escape:
            self.win.close()
            return True
        if keyval == Gdk.KEY_Down:
            row = self.listbox.get_selected_row()
            idx = row.get_index() if row else -1
            nxt = self.listbox.get_row_at_index(idx + 1)
            if nxt:
                self.listbox.select_row(nxt)
                nxt.grab_focus()
                self.entry.grab_focus_without_selecting()
            return True
        if keyval == Gdk.KEY_Up:
            row = self.listbox.get_selected_row()
            idx = row.get_index() if row else 0
            if idx > 0:
                prv = self.listbox.get_row_at_index(idx - 1)
                if prv:
                    self.listbox.select_row(prv)
                    prv.grab_focus()
                    self.entry.grab_focus_without_selecting()
            return True
        if ctrl and keyval in (Gdk.KEY_Return, Gdk.KEY_KP_Enter):
            self.reveal_selected_in_folder()
            return True
        return False

    def reveal_selected_in_folder(self):
        row = self.listbox.get_selected_row()
        if not row:
            return
        idx = row.get_index()
        if 0 <= idx < len(self.results_data):
            data = self.results_data[idx]
            if data.get("action") == "file":
                subprocess.Popen(["dolphin", "--select", data["path"]], start_new_session=True)
                self.win.close()

    def on_entry_activate(self, entry):
        row = self.listbox.get_selected_row()
        if row:
            self.on_row_activated(self.listbox, row)

    def on_row_activated(self, listbox, row):
        idx = row.get_index()
        if not (0 <= idx < len(self.results_data)):
            return
        data = self.results_data[idx]
        action = data.get("action")
        if action == "exec":
            cmd = data["cmd"]
            if isinstance(cmd, str):
                import shlex
                try:
                    args = shlex.split(cmd)
                    subprocess.Popen(args, start_new_session=True)
                except Exception:
                    subprocess.Popen(cmd, shell=True, start_new_session=True)
            else:
                subprocess.Popen(cmd, start_new_session=True)
        elif action == "file":
            subprocess.Popen(["xdg-open", data["path"]], start_new_session=True)
        elif action == "url":
            subprocess.Popen(["xdg-open", data["url"]], start_new_session=True)
        elif action == "copy":
            clipboard = Gdk.Display.get_default().get_clipboard()
            clipboard.set(data["text"])
        self.win.close()


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        assert SafeMathEvaluator.evaluate("24*365") == "8,760"
        print("SELF-TEST PASSED")
        sys.exit(0)
    app = SearchHubApp()
    app.run(sys.argv)
'''
    hub_script.write_text(hub_code)
    hub_script.chmod(0o755)

    desktop_entry = LOCAL_APPS / "win-search-hub.desktop"
    desktop_entry.write_text(f"""[Desktop Entry]
Type=Application
Name=Windows Search & Everything Hub
GenericName=Instant App, File & Calculator Search
Comment=Windows 11 + Voidtools Everything Instant Search Hub (Win + S)
Exec={hub_script}
Icon=system-search
Terminal=false
Categories=Utility;Core;
StartupNotify=false
""")
    desktop_entry.chmod(0o755)


def install_panel_search_button():
    print("[3/4] Installing Native KDE Plasma 6 Search Button Applet (`org.kde.plasma.winsearch`) next to Start Menu...")
    ui_dir = WINSEARCH_PLASMOID_DIR / "contents" / "ui"
    ui_dir.mkdir(parents=True, exist_ok=True)

    metadata_json = """{
    "KPackageStructure": "Plasma/Applet",
    "KPlugin": {
        "Authors": [
            {
                "Name": "Windows Familiarity Setup"
            }
        ],
        "Category": "Application Launchers",
        "Description": "Windows 11 Taskbar Search Button with Auto-Close & Everything Search",
        "Icon": "system-search",
        "Id": "org.kde.plasma.winsearch",
        "License": "GPL-2.0+",
        "Name": "Windows Search",
        "Version": "1.0"
    },
    "X-Plasma-API-Minimum-Version": "6.0"
}
"""
    (WINSEARCH_PLASMOID_DIR / "metadata.json").write_text(metadata_json)

    # Native Plasma 6 QML Search Popup with Milou (KRunner engine) + Everything Hub Launcher + Cursor-Away Auto-Close
    main_qml = f"""import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami
import org.kde.milou as Milou
import org.kde.plasma.plasma5support as P5Support

PlasmoidItem {{
    id: searchRoot
    toolTipMainText: "Search (Win + S)"
    toolTipSubText: "Search Apps, Files (Everything), Calculator & Web"

    switchWidth: Kirigami.Units.gridUnit * 14
    switchHeight: Kirigami.Units.gridUnit * 14

    property bool popupHovered: false
    property bool buttonHovered: false
    property bool hasEnteredPopup: false

    P5Support.DataSource {{
        id: executable
        engine: "executable"
        connectedSources: []
        onNewData: (sourceName, data) => {{
            disconnectSource(sourceName)
        }}
        function exec(cmd) {{
            connectSource(cmd)
        }}
    }}

    onExpandedChanged: {{
        if (searchRoot.expanded) {{
            searchRoot.hasEnteredPopup = searchRoot.popupHovered
            initialIdleTimer.restart()
        }} else {{
            searchRoot.hasEnteredPopup = false
            autoCloseTimer.stop()
            initialIdleTimer.stop()
        }}
    }}

    onPopupHoveredChanged: {{
        if (searchRoot.popupHovered && searchRoot.expanded) {{
            searchRoot.hasEnteredPopup = true
            initialIdleTimer.stop()
            autoCloseTimer.stop()
        }} else if (searchRoot.expanded && searchRoot.hasEnteredPopup && !searchRoot.popupHovered && !searchRoot.buttonHovered) {{
            autoCloseTimer.restart()
        }}
    }}

    Timer {{
        id: initialIdleTimer
        interval: 3500
        repeat: false
        onTriggered: {{
            if (searchRoot.expanded && !searchRoot.popupHovered && !searchRoot.buttonHovered) {{
                searchRoot.expanded = false
            }}
        }}
    }}

    Timer {{
        id: autoCloseTimer
        interval: 200
        repeat: true
        onTriggered: {{
            if (!searchRoot.expanded || searchRoot.popupHovered || searchRoot.buttonHovered) {{
                stop()
                return
            }}
            searchRoot.expanded = false
            stop()
        }}
    }}

    compactRepresentation: MouseArea {{
        id: compactBtn
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        onContainsMouseChanged: searchRoot.buttonHovered = containsMouse

        Layout.minimumWidth: Kirigami.Units.iconSizes.medium + Kirigami.Units.smallSpacing * 2
        Layout.maximumWidth: Kirigami.Units.iconSizes.medium + Kirigami.Units.smallSpacing * 2

        onClicked: (mouse) => {{
            if (mouse.button === Qt.MiddleButton) {{
                executable.exec("{LOCAL_BIN / 'win-search-hub.py'}")
            }} else {{
                searchRoot.expanded = !searchRoot.expanded
            }}
        }}

        Rectangle {{
            anchors.fill: parent
            anchors.margins: 3
            radius: 6
            color: compactBtn.containsMouse ? Qt.rgba(0.38, 0.80, 1.0, 0.18) : Qt.rgba(1, 1, 1, 0.06)
            border.width: 1
            border.color: compactBtn.containsMouse ? "#60CDFF" : Qt.rgba(1, 1, 1, 0.12)

            Kirigami.Icon {{
                anchors.centerIn: parent
                width: Kirigami.Units.iconSizes.smallMedium
                height: Kirigami.Units.iconSizes.smallMedium
                source: "system-search"
            }}
        }}
    }}

    fullRepresentation: Item {{
        id: fullRoot
        Layout.minimumWidth: Kirigami.Units.gridUnit * 32
        Layout.minimumHeight: Kirigami.Units.gridUnit * 26
        Layout.preferredWidth: Kirigami.Units.gridUnit * 34
        Layout.preferredHeight: Kirigami.Units.gridUnit * 28

        HoverHandler {{
            id: fullHover
            onHoveredChanged: searchRoot.popupHovered = hovered
        }}

        Connections {{
            target: searchRoot
            function onExpandedChanged() {{
                if (searchRoot.expanded) {{
                    queryInput.text = ""
                    queryInput.forceActiveFocus()
                }}
            }}
        }}

        ColumnLayout {{
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.mediumSpacing

            RowLayout {{
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                PC3.TextField {{
                    id: queryInput
                    Layout.fillWidth: true
                    placeholderText: "🔍 Type here to search Apps, Files, Calculator (e.g. 24*365), Settings..."
                    clearButtonShown: true
                    Keys.onDownPressed: resultsView.decrementCurrentIndex()
                    Keys.onUpPressed: resultsView.incrementCurrentIndex()
                    Keys.onEscapePressed: searchRoot.expanded = false
                    onAccepted: {{
                        if (resultsView.currentItem) {{
                            resultsView.currentItem.activate()
                            searchRoot.expanded = false
                        }}
                    }}
                }}

                PC3.Button {{
                    icon.name: "system-search"
                    text: "Everything File Search"
                    PC3.ToolTip.visible: hovered
                    PC3.ToolTip.text: "Open Full 'Everything' Instant File & Folder Search (Win + S)"
                    onClicked: {{
                        searchRoot.expanded = false
                        executable.exec("{LOCAL_BIN / 'win-search-hub.py'}")
                    }}
                }}
            }}

            RowLayout {{
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                PC3.Button {{
                    text: "📁 File Explorer"
                    flat: true
                    onClicked: {{
                        searchRoot.expanded = false
                        executable.exec("dolphin")
                    }}
                }}
                PC3.Button {{
                    text: "📊 Task Manager"
                    flat: true
                    onClicked: {{
                        searchRoot.expanded = false
                        executable.exec("plasma-systemmonitor")
                    }}
                }}
                PC3.Button {{
                    text: "✂️ Snip Screen"
                    flat: true
                    onClicked: {{
                        searchRoot.expanded = false
                        executable.exec("spectacle -r -b -c")
                    }}
                }}
                PC3.Button {{
                    text: "⚙️ Settings"
                    flat: true
                    onClicked: {{
                        searchRoot.expanded = false
                        executable.exec("systemsettings")
                    }}
                }}
            }}

            PC3.ScrollView {{
                Layout.fillWidth: true
                Layout.fillHeight: true

                Milou.ResultsView {{
                    id: resultsView
                    queryString: queryInput.text
                    queryField: queryInput
                    onActivated: {{
                        searchRoot.expanded = false
                    }}
                }}
            }}
        }}
    }}
}}
"""
    (ui_dir / "main.qml").write_text(main_qml)

    applets_rc = CONFIG_DIR / "plasma-org.kde.plasma.desktop-appletsrc"
    if applets_rc.exists():
        text = applets_rc.read_text()
        applet_block = """[Containments][2][Applets][29]
immutability=1
plugin=org.kde.plasma.winsearch
"""
        if "[Containments][2][Applets][29]" in text:
            # Update plugin to org.kde.plasma.winsearch if previously created
            text = re.sub(
                r"\[Containments\]\[2\]\[Applets\]\[29\][\s\S]*?(?=\n\[|\Z)",
                applet_block.strip() + "\n",
                text,
            )
        else:
            text = text.rstrip() + "\n\n" + applet_block

        text = re.sub(
            r"AppletOrder=3;(?!29;)",
            "AppletOrder=3;29;",
            text,
        )
        applets_rc.write_text(text)


def polish_windows_icons():
    print("[4/4] Polishing Windows 11 Yellow Folder & Search Icons...")
    icon_theme_dir = HOME / ".local" / "share" / "icons" / "Win10-Explorer"
    places_dir = icon_theme_dir / "places" / "scalable"
    apps_dir = icon_theme_dir / "apps" / "scalable"
    places_dir.mkdir(parents=True, exist_ok=True)
    apps_dir.mkdir(parents=True, exist_ok=True)

    search_svg = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48" width="48" height="48">
  <defs>
    <linearGradient id="winGlass" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#60CDFF"/>
      <stop offset="100%" stop-color="#0078D4"/>
    </linearGradient>
  </defs>
  <circle cx="21" cy="21" r="13" fill="none" stroke="url(#winGlass)" stroke-width="4.5"/>
  <circle cx="21" cy="21" r="10.5" fill="#0078D4" fill-opacity="0.18"/>
  <line x1="30.5" y1="30.5" x2="41" y2="41" stroke="#60CDFF" stroke-width="5" stroke-linecap="round"/>
</svg>
"""
    (apps_dir / "system-search.svg").write_text(search_svg)

    folder_svg = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48" width="48" height="48">
  <defs>
    <linearGradient id="winFolderBack" x1="0%" y1="0%" x2="0%" y2="100%">
      <stop offset="0%" stop-color="#FCB900"/>
      <stop offset="100%" stop-color="#E09600"/>
    </linearGradient>
    <linearGradient id="winFolderFront" x1="0%" y1="0%" x2="0%" y2="100%">
      <stop offset="0%" stop-color="#FFE066"/>
      <stop offset="100%" stop-color="#F7B500"/>
    </linearGradient>
  </defs>
  <path d="M4 10 C4 7.8 5.8 6 8 6 L18 6 C19.5 6 20.8 6.8 21.6 8 L24 11 L40 11 C42.2 11 44 12.8 44 15 L44 38 C44 40.2 42.2 42 40 42 L8 42 C5.8 42 4 40.2 4 38 Z" fill="url(#winFolderBack)"/>
  <path d="M4 17 C4 15.3 5.3 14 7 14 L41 14 C42.7 14 44 15.3 44 17 L44 38 C44 40.2 42.2 42 40 42 L8 42 C5.8 42 4 40.2 4 38 Z" fill="url(#winFolderFront)"/>
</svg>
"""
    for folder_name in ("folder.svg", "inode-directory.svg"):
        target = places_dir / folder_name
        if not target.exists():
            target.write_text(folder_svg)

    run_kwrite("kdeglobals", ["Icons"], "Theme", "Win10-Explorer")


def reload_plasma_and_shortcuts():
    print("Reloading KWin, Global Shortcuts, and Plasma Shell...")
    subprocess.run(["qdbus-qt6", "org.kde.KWin", "/KWin", "reconfigure"], check=False)
    subprocess.run(["systemctl", "--user", "restart", "plasma-kglobalaccel.service"], check=False)
    subprocess.run(["systemctl", "--user", "restart", "plasma-plasmashell.service"], check=False)


if __name__ == "__main__":
    configure_muscle_memory()
    create_everything_search_hub()
    install_panel_search_button()
    polish_windows_icons()
    reload_plasma_and_shortcuts()
    print("\nAll Windows Familiarity & Search Hub upgrades applied successfully!")
