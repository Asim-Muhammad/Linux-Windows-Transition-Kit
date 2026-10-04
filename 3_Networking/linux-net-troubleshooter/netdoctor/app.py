"""
NetDoctor Gtk.Application implementation.
Provides single-instance lifecycle and system desktop integration.
"""

import sys
import os
import gi

gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
from gi.repository import Gtk, Gio, GdkPixbuf

from netdoctor.window import NetDoctorWindow
from netdoctor.engine import NetworkEngine


class NetDoctorApp(Gtk.Application):
    def __init__(self):
        super().__init__(
            application_id="org.transitionkit.netdoctor",
            flags=Gio.ApplicationFlags.FLAGS_NONE
        )
        self.window = None

    def do_activate(self):
        if not self.window:
            icon_candidates = [
                os.path.expanduser("~/.local/share/icons/hicolor/scalable/apps/netdoctor.svg"),
                os.path.join(os.path.dirname(os.path.dirname(__file__)), "data", "netdoctor.svg"),
                os.path.join(os.path.dirname(os.path.dirname(__file__)), "netdoctor.svg")
            ]
            for p in icon_candidates:
                if os.path.exists(p):
                    try:
                        pix = GdkPixbuf.Pixbuf.new_from_file(p)
                        Gtk.Window.set_default_icon(pix)
                        break
                    except Exception:
                        pass

            self.window = NetDoctorWindow(application=self)
            try:
                self.window.set_wmclass("netdoctor", "NetDoctor")
            except Exception:
                pass
            self.window.connect("destroy", lambda w: setattr(self, "window", None))

        self.window.show_all()
        self.window.present()


def run_cli():
    print("=" * 64)
    print(" NetDoctor — Linux Network Diagnostics & Repair Engine")
    print("=" * 64)
    engine = NetworkEngine()
    results = engine.run_full_diagnostics()
    for res in results:
        status_symbol = "✔" if res.status == "PASS" else ("⚠" if res.status == "WARN" else "✘")
        print(f"[{status_symbol}] {res.title:<34} : {res.status}")
        print(f"    Message: {res.message}")
        if res.status != "PASS" and res.details:
            first_line = res.details.splitlines()[0] if res.details else ""
            if first_line:
                print(f"    Detail : {first_line}")
        print("-" * 64)


def ensure_mime_database():
    """Ensures SVG MIME type is recognized by GIO/GdkPixbuf to prevent GTK icon helper assertions."""
    try:
        ctype, _ = Gio.content_type_guess("test.svg", None)
        if ctype != "image/svg+xml":
            local_mime = os.path.expanduser("~/.local/share/mime/packages")
            os.makedirs(local_mime, exist_ok=True)
            import subprocess
            subprocess.run(
                "cp /usr/share/mime/packages/*.xml ~/.local/share/mime/packages/ 2>/dev/null; update-mime-database ~/.local/share/mime 2>/dev/null",
                shell=True, timeout=5
            )
    except Exception:
        pass


def main():
    ensure_mime_database()
    if "--cli" in sys.argv or "--diagnose" in sys.argv:
        run_cli()
        return 0

    try:
        app = NetDoctorApp()
        return app.run(sys.argv)
    except Exception as e:
        print(f"Notice: NetDoctor GUI fallback: {e}", file=sys.stderr)
        run_cli()
        return 0


if __name__ == "__main__":
    sys.exit(main())
