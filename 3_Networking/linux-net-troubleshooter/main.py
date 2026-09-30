#!/usr/bin/env python3
"""
Linux Network Troubleshooter & Reset (NetDoctor)
Main executable entry point.
"""

import sys
import os

# Ensure current directory is in sys.path
APP_DIR = os.path.dirname(os.path.abspath(__file__))
if APP_DIR not in sys.path:
    sys.path.insert(0, APP_DIR)

from engine import NetworkEngine

def run_cli_diagnostics():
    print("=" * 60)
    print(" Linux Network Troubleshooter - Diagnostic Report")
    print("=" * 60)
    engine = NetworkEngine()
    results = engine.run_full_diagnostics()
    for res in results:
        status_symbol = "✔" if res.status == "PASS" else ("⚠" if res.status == "WARN" else "✘")
        print(f"[{status_symbol}] {res.title:<32} : {res.status}")
        print(f"    Message: {res.message}")
        if res.status != "PASS" and res.details:
            first_detail = res.details.splitlines()[0] if res.details else ""
            if first_detail:
                print(f"    Detail : {first_detail}")
        print("-" * 60)

def main():
    if "--cli" in sys.argv or "--diagnose" in sys.argv or not (os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")):
        run_cli_diagnostics()
        return

    # Launch GTK GUI
    try:
        import gi
        gi.require_version('Gtk', '3.0')
        from gi.repository import Gtk, GdkPixbuf
        from gui import TroubleshooterWindow

        # Set window icon
        icon_path = os.path.join(APP_DIR, "netdoctor.svg")
        if os.path.exists(icon_path):
            try:
                pixbuf = GdkPixbuf.Pixbuf.new_from_file(icon_path)
                Gtk.Window.set_default_icon(pixbuf)
            except Exception:
                pass

        app = TroubleshooterWindow()
        app.set_wmclass("linux-net-troubleshooter", "Network Troubleshooter")
        app.connect("destroy", Gtk.main_quit)
        app.show_all()
        Gtk.main()
    except Exception as e:
        print(f"Error launching GUI: {e}", file=sys.stderr)
        print("Falling back to command-line diagnostics:\n", file=sys.stderr)
        run_cli_diagnostics()

if __name__ == "__main__":
    main()
