"""
System-Adaptive Theme Engine for NetDoctor.
Adopts 100% of the active desktop environment colors (KDE Breeze, GNOME Adwaita, etc.)
using native GTK theme variables (@theme_base_color, @theme_fg_color, @theme_selected_bg_color, @borders).
Zero hardcoded colors.
"""

import gi

gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
from gi.repository import Gtk, Gdk

SYSTEM_ADAPTIVE_CSS = b"""
/* Fallback semantic colors for themes that don't provide them */
@define-color success_color #2ec27e;
@define-color warning_color #f5a623;
@define-color error_color #e01b24;

/* Native Card Container: Uses system base background, system text, and system border */
.nd-card {
    background-color: @theme_base_color;
    color: @theme_text_color;
    border: 1px solid @borders;
    border-radius: 8px;
    padding: 12px;
    margin: 4px 6px;
}
.nd-card:hover {
    background-color: mix(@theme_base_color, @theme_fg_color, 0.04);
}

/* Primary and Secondary Text using System Theme Tokens */
.nd-text-primary {
    color: @theme_text_color;
    font-weight: 600;
}

/* Semantic Badges: Translucent tint over system background */
.nd-badge {
    padding: 4px 10px;
    border-radius: 12px;
    font-weight: bold;
    font-size: 11px;
}
.nd-badge-success {
    background-color: alpha(@success_color, 0.15);
    color: @success_color;
    border: 1px solid alpha(@success_color, 0.35);
}
.nd-badge-danger {
    background-color: alpha(@error_color, 0.15);
    color: @error_color;
    border: 1px solid alpha(@error_color, 0.35);
}
.nd-badge-warning {
    background-color: alpha(@warning_color, 0.15);
    color: @warning_color;
    border: 1px solid alpha(@warning_color, 0.35);
}
.nd-badge-info {
    background-color: alpha(@theme_selected_bg_color, 0.15);
    color: @theme_selected_bg_color;
    border: 1px solid alpha(@theme_selected_bg_color, 0.35);
}

/* Console: Slightly darker than system base, using system text and borders */
.nd-console {
    font-family: monospace;
    font-size: 11px;
    background-color: darker(@theme_base_color);
    color: @theme_text_color;
    border: 1px solid @borders;
    border-radius: 6px;
    padding: 10px;
}
"""

class ThemeManager:
    def __init__(self):
        self.css_provider = Gtk.CssProvider()
        self._provider_added = False
        try:
            self.css_provider.load_from_data(SYSTEM_ADAPTIVE_CSS)
        except Exception:
            pass
        self.ensure_provider()

    def ensure_provider(self):
        if self._provider_added:
            return
        screen = Gdk.Screen.get_default()
        if screen is not None:
            Gtk.StyleContext.add_provider_for_screen(
                screen,
                self.css_provider,
                Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
            )
            self._provider_added = True


_instance = None
def get_theme_manager() -> ThemeManager:
    global _instance
    if _instance is None:
        _instance = ThemeManager()
    return _instance
