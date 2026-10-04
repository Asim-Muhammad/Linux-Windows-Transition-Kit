"""
GTK3 Modern Graphical Interface for Linux Network Troubleshooter & Reset.
"""

import sys
import threading
import time
import subprocess
import gi

gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango

from engine import NetworkEngine, DiagnosticResult

def esc(text) -> str:
    """Safely escapes text for use in Pango markup."""
    if text is None:
        return ""
    return GLib.markup_escape_text(str(text))

CSS = b"""
window {
    background-color: #f7f9fa;
}

.header-title {
    font-weight: bold;
    font-size: 15px;
}

.status-badge {
    padding: 6px 14px;
    border-radius: 16px;
    font-weight: bold;
    font-size: 13px;
}
.status-green {
    background-color: #d1e7dd;
    color: #0f5132;
    border: 1px solid #badbcc;
}
.status-red {
    background-color: #f8d7da;
    color: #842029;
    border: 1px solid #f5c2c7;
}
.status-yellow {
    background-color: #fff3cd;
    color: #664d03;
    border: 1px solid #ffecb5;
}
.status-blue {
    background-color: #cff4fc;
    color: #055160;
    border: 1px solid #b6effb;
}

.card {
    background-color: #ffffff;
    border-radius: 8px;
    border: 1px solid #e1e4e8;
    padding: 12px;
    margin: 6px 10px;
}

.btn-primary {
    background-color: #0d6efd;
    color: white;
    font-weight: bold;
    border-radius: 6px;
    padding: 8px 16px;
}
.btn-primary:hover {
    background-color: #0b5ed7;
}

.btn-danger {
    background-color: #dc3545;
    color: white;
    font-weight: bold;
    border-radius: 6px;
    padding: 8px 16px;
}
.btn-danger:hover {
    background-color: #bb2d3b;
}

.btn-action {
    background-color: #f8f9fa;
    border: 1px solid #ced4da;
    border-radius: 6px;
    padding: 8px 14px;
    font-weight: 500;
}
.btn-action:hover {
    background-color: #e9ecef;
}

.log-view {
    font-family: monospace;
    font-size: 11px;
    background-color: #1e1e1e;
    color: #d4d4d4;
    padding: 8px;
}
"""

class TroubleshooterWindow(Gtk.Window):
    def __init__(self):
        super().__init__(title="Linux Network Troubleshooter & Reset")
        self.set_default_size(820, 620)
        self.set_position(Gtk.WindowPosition.CENTER)
        self.engine = NetworkEngine()
        self.ping_process = None

        self._apply_css()
        self._build_ui()

        # Start initial quick check on launch
        GLib.idle_add(self.on_run_diagnostics)

    def _apply_css(self):
        style_provider = Gtk.CssProvider()
        style_provider.load_from_data(CSS)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(),
            style_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

    def _build_ui(self):
        # Main layout container
        main_vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.add(main_vbox)

        # Header Bar
        header = Gtk.HeaderBar()
        header.set_show_close_button(True)
        header.props.title = "Network Troubleshooter & Reset"
        header.props.subtitle = "Automated Diagnostic Engine & Safe Network Stack Reset"
        self.set_titlebar(header)

        # Top Banner / Status Overview
        banner_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=16)
        banner_box.set_margin_top(12)
        banner_box.set_margin_bottom(12)
        banner_box.set_margin_start(16)
        banner_box.set_margin_end(16)

        banner_text_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        title_label = Gtk.Label(xalign=0)
        title_label.set_markup("<b>System Network Status</b>")
        self.status_detail_label = Gtk.Label(label="Initializing diagnostics...", xalign=0)
        self.status_detail_label.set_ellipsize(Pango.EllipsizeMode.END)
        banner_text_box.pack_start(title_label, False, False, 0)
        banner_text_box.pack_start(self.status_detail_label, False, False, 0)

        self.status_badge = Gtk.Label(label="Checking...")
        self.status_badge.get_style_context().add_class("status-badge")
        self.status_badge.get_style_context().add_class("status-blue")

        banner_box.pack_start(banner_text_box, True, True, 0)
        banner_box.pack_end(self.status_badge, False, False, 0)
        main_vbox.pack_start(banner_box, False, False, 0)

        # Separator
        main_vbox.pack_start(Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL), False, False, 0)

        # Stack & Stack Switcher (Tabs)
        self.stack = Gtk.Stack()
        self.stack.set_transition_type(Gtk.StackTransitionType.SLIDE_LEFT_RIGHT)
        self.stack.set_transition_duration(200)

        stack_switcher = Gtk.StackSwitcher()
        stack_switcher.set_stack(self.stack)
        stack_switcher.set_halign(Gtk.Align.CENTER)
        stack_switcher.set_margin_top(8)
        stack_switcher.set_margin_bottom(8)
        main_vbox.pack_start(stack_switcher, False, False, 0)

        # Build Tabs
        self.tab_diag = self._build_diagnostics_tab()
        self.tab_reset = self._build_reset_tab()
        self.tab_dns = self._build_dns_tab()
        self.tab_tools = self._build_tools_tab()

        self.stack.add_titled(self.tab_diag, "diag", "🔍 Diagnostics")
        self.stack.add_titled(self.tab_reset, "reset", "🔄 1-Click Fix & Reset")
        self.stack.add_titled(self.tab_dns, "dns", "🌐 Fast DNS Changer")
        self.stack.add_titled(self.tab_tools, "tools", "📊 Adapters & Ping")

        main_vbox.pack_start(self.stack, True, True, 0)

    # -------------------------------------------------------------
    # TAB 1: DIAGNOSTICS
    # -------------------------------------------------------------
    def _build_diagnostics_tab(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        vbox.set_margin_start(16)
        vbox.set_margin_end(16)
        vbox.set_margin_bottom(12)

        # Action bar with "Run Diagnostics" button and progress spinner
        action_bar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.btn_run_diag = Gtk.Button(label="  Run Full Diagnostics  ")
        self.btn_run_diag.get_style_context().add_class("btn-primary")
        self.btn_run_diag.connect("clicked", lambda b: self.on_run_diagnostics())

        self.spinner = Gtk.Spinner()
        self.diag_status_label = Gtk.Label(label="Ready to run diagnostics", xalign=0)

        action_bar.pack_start(self.btn_run_diag, False, False, 0)
        action_bar.pack_start(self.spinner, False, False, 0)
        action_bar.pack_start(self.diag_status_label, True, True, 0)
        vbox.pack_start(action_bar, False, False, 4)

        # Scrolled List of Diagnostic Steps
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        scrolled.set_vexpand(True)

        self.results_listbox = Gtk.ListBox()
        self.results_listbox.set_selection_mode(Gtk.SelectionMode.NONE)
        scrolled.add(self.results_listbox)
        vbox.pack_start(scrolled, True, True, 0)

        return vbox

    def on_run_diagnostics(self):
        try:
            self.btn_run_diag.set_sensitive(False)
            self.spinner.start()
            self.diag_status_label.set_text("Analyzing network layers (Physical ➜ IP ➜ Gateway ➜ DNS ➜ Web)...")
            self.update_status_badge("Checking...", "status-blue", "Running diagnostics...")

            # Clear previous items
            for child in self.results_listbox.get_children():
                self.results_listbox.remove(child)

            def worker():
                try:
                    results = self.engine.run_full_diagnostics()
                    GLib.idle_add(self._on_diagnostics_complete, results)
                except Exception as e:
                    print(f"Diagnostics worker exception: {e}", file=sys.stderr)
                    GLib.idle_add(lambda: self.diag_status_label.set_text(f"Diagnostic error: {e}"))
                    GLib.idle_add(lambda: self.spinner.stop())
                    GLib.idle_add(lambda: self.btn_run_diag.set_sensitive(True))

            threading.Thread(target=worker, daemon=True).start()
        except Exception as e:
            print(f"Error starting diagnostics: {e}", file=sys.stderr)
        return False

    def _on_diagnostics_complete(self, results):
        try:
            self.btn_run_diag.set_sensitive(True)
            self.spinner.stop()

            has_fail = any(r.status == "FAIL" for r in results)
            has_warn = any(r.status == "WARN" for r in results)

            if not has_fail and not has_warn:
                self.update_status_badge("🟢 Fully Operational", "status-green", "All layers passed! Full internet connectivity verified.")
                self.diag_status_label.set_text("✔ All 12 network checks passed successfully!")
            elif not has_fail and has_warn:
                self.update_status_badge("🟡 Attention Required", "status-yellow", "Connected, but warning detected (e.g. Wi-Fi PowerSave).")
                self.diag_status_label.set_text("⚠ Network active with warnings.")
            else:
                first_fail = next((r for r in results if r.status == "FAIL"), None)
                if first_fail:
                    self.update_status_badge("🔴 Network Issue", "status-red", f"Failure at: {first_fail.title}")
                    self.diag_status_label.set_text(f"✘ Issue detected: {first_fail.message}")

            for res in results:
                row = self._create_diag_row(res)
                self.results_listbox.add(row)

            self.results_listbox.show_all()
        except Exception as e:
            print(f"Error rendering diagnostics results: {e}", file=sys.stderr)
        return False

    def _create_diag_row(self, res: DiagnosticResult):
        row = Gtk.ListBoxRow()
        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
        card.get_style_context().add_class("card")

        top_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        # Icon
        if res.status == "PASS":
            icon = Gtk.Image.new_from_icon_name("emblem-ok-symbolic", Gtk.IconSize.BUTTON)
        elif res.status == "WARN":
            icon = Gtk.Image.new_from_icon_name("dialog-warning-symbolic", Gtk.IconSize.BUTTON)
        else:
            icon = Gtk.Image.new_from_icon_name("process-stop-symbolic", Gtk.IconSize.BUTTON)

        title = Gtk.Label(xalign=0)
        title.set_markup(f"<b>{esc(res.title)}</b>")
        
        status_lbl = Gtk.Label(label=f"[{res.status}]", xalign=1)
        if res.status == "PASS":
            status_lbl.get_style_context().add_class("status-green")
        elif res.status == "WARN":
            status_lbl.get_style_context().add_class("status-yellow")
        else:
            status_lbl.get_style_context().add_class("status-red")
        status_lbl.get_style_context().add_class("status-badge")

        top_box.pack_start(icon, False, False, 0)
        top_box.pack_start(title, True, True, 0)
        top_box.pack_end(status_lbl, False, False, 0)
        card.pack_start(top_box, False, False, 0)

        # Message
        msg_lbl = Gtk.Label(label=res.message, xalign=0)
        msg_lbl.set_line_wrap(True)
        msg_lbl.set_margin_start(28)
        card.pack_start(msg_lbl, False, False, 0)

        # Fix button if applicable
        if res.status in ("FAIL", "WARN") and res.fix_action:
            action_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
            action_box.set_margin_start(28)
            action_box.set_margin_top(4)

            btn_fix = Gtk.Button(label="Auto-Fix This Issue")
            btn_fix.get_style_context().add_class("btn-action")
            btn_fix.connect("clicked", lambda b, act=res.fix_action: self._handle_fix_action(act))
            action_box.pack_start(btn_fix, False, False, 0)
            card.pack_start(action_box, False, False, 0)

        row.add(card)
        return row

    def _handle_fix_action(self, fix_action: str):
        if fix_action == "unblock_rfkill":
            self.on_unblock_radio()
        elif fix_action == "quick_reset":
            self.on_quick_reset()
        elif fix_action == "restart_network_service":
            self.on_full_reset()
        elif fix_action == "set_public_dns":
            self.stack.set_visible_child_name("dns")
        elif fix_action == "renew_dhcp":
            self.on_renew_dhcp()
        elif fix_action == "open_browser":
            subprocess.Popen(["xdg-open", "http://connectivity-check.ubuntu.com"])
        elif fix_action == "bounce_interfaces":
            self.on_quick_reset()

    # -------------------------------------------------------------
    # TAB 2: 1-CLICK FIX & RESET (Windows Network Reset Equivalent)
    # -------------------------------------------------------------
    def _build_reset_tab(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        vbox.set_margin_start(16)
        vbox.set_margin_end(16)
        vbox.set_margin_bottom(12)

        desc = Gtk.Label(xalign=0)
        desc.set_markup(
            "<b>Safe Network Stack Reset</b>\n"
            "Just like the Windows 'Network Reset' feature, these actions restore connections, "
            "clear DNS/ARP routing deadlocks, and restart daemons <i>without rebooting or harming your files</i>."
        )
        desc.set_line_wrap(True)
        vbox.pack_start(desc, False, False, 4)

        grid = Gtk.Grid()
        grid.set_column_spacing(12)
        grid.set_row_spacing(10)
        grid.set_column_homogeneous(True)

        # Card 1: Quick Soft Reset
        c1 = self._create_action_card(
            title="⚡ Quick Soft Reset (Recommended)",
            description="Flushes DNS resolver cache, flushes ARP table, and cycles NetworkManager cleanly. Solves 90% of sudden connection freezes.",
            button_label="Run Quick Reset",
            callback=self.on_quick_reset,
            is_danger=False
        )
        grid.attach(c1, 0, 0, 1, 1)

        # Card 2: Full Stack Reset
        c2 = self._create_action_card(
            title="🔄 Complete Network Reset",
            description="Restarts NetworkManager and systemd-resolved services from scratch, clearing all internal socket state. (Windows equivalent).",
            button_label="Full Stack Reset",
            callback=self.on_full_reset,
            is_danger=True
        )
        grid.attach(c2, 1, 0, 1, 1)

        # Card 3: Unblock Radio
        c3 = self._create_action_card(
            title="🔓 Unblock Wireless (Airplane Mode)",
            description="Removes any rfkill software or hardware blocks on Wi-Fi and Bluetooth cards.",
            button_label="Unblock Radios",
            callback=self.on_unblock_radio,
            is_danger=False
        )
        grid.attach(c3, 0, 1, 1, 1)

        # Card 4: Renew DHCP
        c4 = self._create_action_card(
            title="🌐 Renew DHCP Lease",
            description="Releases existing IP address and requests a clean configuration and gateway from the local router.",
            button_label="Renew DHCP Lease",
            callback=self.on_renew_dhcp,
            is_danger=False
        )
        grid.attach(c4, 1, 1, 1, 1)

        vbox.pack_start(grid, False, False, 4)

        # Live Execution Log
        log_label = Gtk.Label(xalign=0)
        log_label.set_markup("<b>Live Repair Execution Log:</b>")
        vbox.pack_start(log_label, False, False, 2)

        scrolled_log = Gtk.ScrolledWindow()
        scrolled_log.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled_log.set_vexpand(True)

        self.log_buffer = Gtk.TextBuffer()
        self.log_view = Gtk.TextView(buffer=self.log_buffer)
        self.log_view.set_editable(False)
        self.log_view.set_cursor_visible(False)
        self.log_view.get_style_context().add_class("log-view")
        scrolled_log.add(self.log_view)
        vbox.pack_start(scrolled_log, True, True, 0)

        return vbox

    def _create_action_card(self, title, description, button_label, callback, is_danger=False):
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        box.get_style_context().add_class("card")

        t_lbl = Gtk.Label(xalign=0)
        t_lbl.set_markup(f"<b>{esc(title)}</b>")
        box.pack_start(t_lbl, False, False, 0)

        d_lbl = Gtk.Label(label=description, xalign=0)
        d_lbl.set_line_wrap(True)
        box.pack_start(d_lbl, True, True, 0)

        btn = Gtk.Button(label=button_label)
        if is_danger:
            btn.get_style_context().add_class("btn-danger")
        else:
            btn.get_style_context().add_class("btn-primary")
        btn.connect("clicked", lambda b: callback())
        box.pack_end(btn, False, False, 4)

        return box

    def log_message(self, text: str):
        timestamp = time.strftime("[%H:%M:%S] ")
        end_iter = self.log_buffer.get_end_iter()
        self.log_buffer.insert(end_iter, timestamp + text + "\n")
        self.log_view.scroll_to_iter(self.log_buffer.get_end_iter(), 0.0, False, 0.0, 0.0)

    def on_quick_reset(self):
        self.log_message("Starting Quick Soft Reset...")
        def worker():
            ok, msg = self.engine.quick_reset()
            GLib.idle_add(lambda: self.log_message(msg))
            GLib.idle_add(self.on_run_diagnostics)
        threading.Thread(target=worker, daemon=True).start()

    def on_full_reset(self):
        self.log_message("Initiating Full Network Stack Reset (restarting daemons)...")
        def worker():
            ok, msg = self.engine.full_stack_reset()
            GLib.idle_add(lambda: self.log_message(msg))
            GLib.idle_add(self.on_run_diagnostics)
        threading.Thread(target=worker, daemon=True).start()

    def on_unblock_radio(self):
        self.log_message("Unblocking wireless radios via rfkill...")
        def worker():
            ok, msg = self.engine.unblock_rfkill()
            GLib.idle_add(lambda: self.log_message(msg))
            GLib.idle_add(self.on_run_diagnostics)
        threading.Thread(target=worker, daemon=True).start()

    def on_renew_dhcp(self):
        self.log_message("Requesting DHCP lease renewal from router...")
        def worker():
            ok, msg = self.engine.renew_dhcp()
            GLib.idle_add(lambda: self.log_message(msg))
            GLib.idle_add(self.on_run_diagnostics)
        threading.Thread(target=worker, daemon=True).start()

    # -------------------------------------------------------------
    # TAB 3: FAST DNS CHANGER
    # -------------------------------------------------------------
    def _build_dns_tab(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        vbox.set_margin_start(16)
        vbox.set_margin_end(16)
        vbox.set_margin_bottom(12)

        desc = Gtk.Label(xalign=0)
        desc.set_markup(
            "<b>1-Click DNS Profile Switcher</b>\n"
            "90% of 'connected but no internet' problems are caused by slow or unresponsive ISP DNS servers. "
            "Switch to a fast, reliable public DNS provider below with one click:"
        )
        desc.set_line_wrap(True)
        vbox.pack_start(desc, False, False, 4)

        grid = Gtk.Grid()
        grid.set_column_spacing(12)
        grid.set_row_spacing(10)
        grid.set_column_homogeneous(True)

        presets = [
            ("⚡ Cloudflare DNS", "1.1.1.1, 1.0.0.1", "Extremely fast, private, does not log IP addresses.", ["1.1.1.1", "1.0.0.1"]),
            ("🔍 Google Public DNS", "8.8.8.8, 8.8.4.4", "Highly reliable worldwide resolution backed by Google.", ["8.8.8.8", "8.8.4.4"]),
            ("🛡️ Quad9 Security DNS", "9.9.9.9, 149.112.112.112", "Blocks malware, phishing, and spyware domains automatically.", ["9.9.9.9", "149.112.112.112"]),
            ("🔄 Automatic (DHCP)", "Router Default", "Reverts DNS back to your local home/office router default.", [])
        ]

        for idx, (name, ips, desc_txt, dns_arr) in enumerate(presets):
            c = self._create_dns_card(name, ips, desc_txt, dns_arr)
            row = idx // 2
            col = idx % 2
            grid.attach(c, col, row, 1, 1)

        vbox.pack_start(grid, False, False, 4)

        # External Utilities Shortcuts
        ext_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        ext_box.set_margin_top(12)

        btn_nm_editor = Gtk.Button(label="Open System Adapter Properties (nm-connection-editor)")
        btn_nm_editor.get_style_context().add_class("btn-action")
        btn_nm_editor.connect("clicked", lambda b: subprocess.Popen(["nm-connection-editor"]))
        ext_box.pack_start(btn_nm_editor, True, True, 0)

        vbox.pack_start(ext_box, False, False, 0)
        return vbox

    def _create_dns_card(self, title, ips, description, dns_list):
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        box.get_style_context().add_class("card")

        t_lbl = Gtk.Label(xalign=0)
        t_lbl.set_markup(f"<b>{esc(title)}</b>  <span color='#6c757d'>({esc(ips)})</span>")
        box.pack_start(t_lbl, False, False, 0)

        d_lbl = Gtk.Label(label=description, xalign=0)
        d_lbl.set_line_wrap(True)
        box.pack_start(d_lbl, True, True, 0)

        btn = Gtk.Button(label="Apply This DNS")
        btn.get_style_context().add_class("btn-action")
        btn.connect("clicked", lambda b, d=dns_list: self.on_apply_dns(d))
        box.pack_end(btn, False, False, 4)

        return box

    def on_apply_dns(self, dns_list):
        self.stack.set_visible_child_name("reset")
        label = ", ".join(dns_list) if dns_list else "Automatic (DHCP)"
        self.log_message(f"Applying DNS configuration ({label})...")
        def worker():
            ok, msg = self.engine.set_dns_servers("", dns_list)
            GLib.idle_add(lambda: self.log_message(msg))
            GLib.idle_add(self.on_run_diagnostics)
        threading.Thread(target=worker, daemon=True).start()

    # -------------------------------------------------------------
    # TAB 4: ADAPTERS & LIVE PING TOOL
    # -------------------------------------------------------------
    def _build_tools_tab(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        vbox.set_margin_start(18)
        vbox.set_margin_end(18)
        vbox.set_margin_bottom(14)

        ad_title = Gtk.Label(xalign=0)
        ad_title.get_style_context().add_class("nd-text-primary")
        ad_title.set_markup("<b>Detected Network Adapters &amp; IP Routing:</b>")
        vbox.pack_start(ad_title, False, False, 2)

        self.ad_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        
        scrolled_ad = Gtk.ScrolledWindow()
        scrolled_ad.set_min_content_height(160)
        scrolled_ad.add(self.ad_container)
        vbox.pack_start(scrolled_ad, False, False, 0)

        btn_refresh_ad = Gtk.Button(label="Refresh Adapters Info")
        btn_refresh_ad.connect("clicked", lambda b: self.refresh_adapters_info())
        vbox.pack_start(btn_refresh_ad, False, False, 0)

        ping_title = Gtk.Label(xalign=0)
        ping_title.get_style_context().add_class("nd-text-primary")
        ping_title.set_markup("<b>Continuous Live Ping Monitor:</b>")
        vbox.pack_start(ping_title, False, False, 2)

        ping_bar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        self.ping_entry = Gtk.Entry()
        self.ping_entry.set_text("1.1.1.1")
        self.btn_ping = Gtk.Button(label="Start Ping")
        self.btn_ping.get_style_context().add_class("suggested-action")
        self.btn_ping.connect("clicked", self.toggle_ping)

        btn_ping_gw = Gtk.Button(label="Ping Gateway")
        btn_ping_gw.connect("clicked", self._on_quick_ping_gw)

        btn_ping_net = Gtk.Button(label="Ping 1.1.1.1")
        btn_ping_net.connect("clicked", lambda b: self._set_ping_target("1.1.1.1"))

        ping_bar.pack_start(Gtk.Label(label="Target:"), False, False, 0)
        ping_bar.pack_start(self.ping_entry, True, True, 0)
        ping_bar.pack_start(self.btn_ping, False, False, 0)
        ping_bar.pack_start(btn_ping_gw, False, False, 0)
        ping_bar.pack_start(btn_ping_net, False, False, 0)
        vbox.pack_start(ping_bar, False, False, 0)

        # Dashboard for ping
        dash_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=20)
        self.ping_latency_label = Gtk.Label()
        self.ping_latency_label.set_markup("<span size='xx-large' weight='bold' color='#0078D4'>-- ms</span>")
        dash_box.pack_start(self.ping_latency_label, False, False, 10)
        vbox.pack_start(dash_box, False, False, 0)

        self.ping_buffer = Gtk.TextBuffer()
        self.ping_view = Gtk.TextView(buffer=self.ping_buffer)
        self.ping_view.set_editable(False)
        self.ping_view.get_style_context().add_class("nd-console")

        scrolled_ping = Gtk.ScrolledWindow()
        scrolled_ping.set_vexpand(True)
        scrolled_ping.add(self.ping_view)
        vbox.pack_start(scrolled_ping, True, True, 0)

        self.refresh_adapters_info()
        return vbox

    def _on_quick_ping_gw(self, button):
        gw = self.engine.get_default_gateway()
        if gw:
            self._set_ping_target(gw)
        else:
            self.ping_buffer.set_text("No default gateway detected in routing table.\n")

    def _set_ping_target(self, host):
        self.ping_entry.set_text(host)
        if self.ping_process:
            self.toggle_ping(self.btn_ping)
        self.toggle_ping(self.btn_ping)

    def refresh_adapters_info(self):
        for child in self.ad_container.get_children():
            self.ad_container.remove(child)

        rc, out, _ = self.engine.run_cmd(["ip", "-br", "addr", "show"])
        gw = self.engine.get_default_gateway() or "None"

        gw_lbl = Gtk.Label(xalign=0)
        gw_lbl.set_markup(f"<b>Default Gateway:</b> <span color='#0078D4'>{esc(gw)}</span>")
        self.ad_container.pack_start(gw_lbl, False, False, 5)

        if rc == 0:
            for line in out.strip().splitlines():
                parts = line.split()
                if len(parts) >= 2:
                    iface = parts[0]
                    state = parts[1]
                    ips = " ".join(parts[2:]) if len(parts) > 2 else "No IP"
                    
                    card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
                    card.get_style_context().add_class("card")
                    
                    header = Gtk.Label(xalign=0)
                    header.set_markup(f"<span size='large' weight='bold'>{esc(iface)}</span>   <span color='#6c757d'>({esc(state)})</span>")
                    card.pack_start(header, False, False, 0)
                    
                    ip_lbl = Gtk.Label(label=f"IP Addresses: {ips}", xalign=0)
                    card.pack_start(ip_lbl, False, False, 0)
                    
                    self.ad_container.pack_start(card, False, False, 2)
        else:
            err = Gtk.Label(label="Could not query network adapters.", xalign=0)
            self.ad_container.pack_start(err, False, False, 0)
            
        self.ad_container.show_all()

    def toggle_ping(self, button):
        if self.ping_process:
            self.ping_process.terminate()
            self.ping_process = None
            self.btn_ping.set_label("Start Ping")
            self.btn_ping.get_style_context().remove_class("destructive-action")
            self.btn_ping.get_style_context().add_class("suggested-action")
            self.ping_latency_label.set_markup("<span size='xx-large' weight='bold' color='#0078D4'>-- ms</span>")
        else:
            host = self.ping_entry.get_text().strip() or "1.1.1.1"
            self.ping_buffer.set_text(f"Pinging {host}...\n")
            self.btn_ping.set_label("Stop Ping")
            self.btn_ping.get_style_context().remove_class("suggested-action")
            self.btn_ping.get_style_context().add_class("destructive-action")

            def ping_thread():
                try:
                    self.ping_process = subprocess.Popen(
                        ["ping", "-i", "1", host],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT,
                        text=True
                    )
                    for line in iter(self.ping_process.stdout.readline, ''):
                        if not self.ping_process:
                            break
                        GLib.idle_add(self._append_ping_line, line)
                except Exception as e:
                    GLib.idle_add(self._append_ping_line, f"Error: {e}\n")
                finally:
                    GLib.idle_add(self._ping_stopped)

            threading.Thread(target=ping_thread, daemon=True).start()

    def _append_ping_line(self, line):
        import re
        m = re.search(r"time=([\d.]+)\s*ms", line)
        if m:
            ms = m.group(1)
            self.ping_latency_label.set_markup(f"<span size='xx-large' weight='bold' color='#0078D4'>{ms} ms</span>")
            
        end = self.ping_buffer.get_end_iter()
        self.ping_buffer.insert(end, line)
        self.ping_view.scroll_to_iter(self.ping_buffer.get_end_iter(), 0.0, False, 0.0, 0.0)

    def _ping_stopped(self):
        self.btn_ping.set_label("Start Ping")
        self.btn_ping.get_style_context().remove_class("btn-danger")
        self.btn_ping.get_style_context().add_class("btn-primary")
        self.ping_process = None

    def update_status_badge(self, text, style_class, detail):
        self.status_badge.set_text(text)
        ctx = self.status_badge.get_style_context()
        for c in ["status-green", "status-red", "status-yellow", "status-blue"]:
            ctx.remove_class(c)
        ctx.add_class(style_class)
        self.status_detail_label.set_text(detail)


def main():
    app = TroubleshooterWindow()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()

if __name__ == "__main__":
    main()
