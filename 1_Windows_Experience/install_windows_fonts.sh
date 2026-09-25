#!/usr/bin/env bash
# ==============================================================================
# install_windows_fonts.sh
# Installs authentic Windows 10/11 Segoe UI & Consolas fonts and configures
# KDE Plasma 6 to use Segoe UI across the entire desktop environment.
# ==============================================================================
set -euo pipefail

echo "=== [1/4] Preparing Windows Fonts Directory ==="
FONT_DIR="$HOME/.local/share/fonts/WindowsFonts"
mkdir -p "$FONT_DIR"

FOUND_LOCAL=false

# 1. Check if Windows partition is already mounted locally
echo "[*] Checking for local Windows installation..."
for mnt in /run/media/*/* /mnt/* /mnt; do
    if [ -d "$mnt/Windows/Fonts" ]; then
        echo "  [+] Found Windows fonts at: $mnt/Windows/Fonts"
        cp -n "$mnt/Windows/Fonts"/[sS][eE][gG][oO][eE]*.ttf "$FONT_DIR/" 2>/dev/null || true
        cp -n "$mnt/Windows/Fonts"/[cC][oO][nN][sS][oO][lL][aA]*.ttf "$FONT_DIR/" 2>/dev/null || true
        cp -n "$mnt/Windows/Fonts"/[cC][aA][lL][iI][bB][rR][iI]*.ttf "$FONT_DIR/" 2>/dev/null || true
        FOUND_LOCAL=true
        break
    fi
done

# 2. If not found locally, download Segoe UI font package from GitHub
if [ "$FOUND_LOCAL" = false ] || [ ! -f "$FONT_DIR/segoeui.ttf" ]; then
    echo "[*] Downloading Segoe UI font package from GitHub repository..."
    TMP_DIR=$(mktemp -d /tmp/segoe-XXXXXX)
    if git clone --depth 1 https://github.com/mrbvrz/segoe-ui-linux.git "$TMP_DIR" 2>/dev/null; then
        cp -f "$TMP_DIR"/font/*.ttf "$FONT_DIR/" 2>/dev/null || true
        echo "  [OK] Segoe UI font family downloaded and installed."
    else
        # Fallback to direct curl download
        echo "  [*] Falling back to direct curl download..."
        for f in segoeui.ttf segoeuib.ttf segoeuii.ttf segoeuiz.ttf segoeuil.ttf seguisb.ttf seguiemj.ttf; do
            curl -fsSL "https://raw.githubusercontent.com/mrbvrz/segoe-ui-linux/master/font/$f" -o "$FONT_DIR/$f" 2>/dev/null || true
        done
    fi
    rm -rf "$TMP_DIR"
fi

# 3. Refresh font cache
echo "=== [2/4] Updating Font Cache ==="
fc-cache -f "$FONT_DIR"
echo "[OK] Font cache updated."

# Verify Segoe UI is recognized by Fontconfig
if fc-list : family | grep -qi "Segoe UI"; then
    echo "[VERIFIED] Segoe UI is successfully recognized by the system."
else
    echo "[WARNING] Segoe UI not yet listed in fontconfig, refreshing full cache..."
    fc-cache -fv "$HOME/.local/share/fonts"
fi

# 4. Configure KDE Plasma 6 to use Segoe UI (Clean 10pt Windows typography)
echo "=== [3/4] Applying Windows Font Settings to KDE Plasma 6 ==="
kwriteconfig6 --file kdeglobals --group General --key font "Segoe UI,10,-1,5,400,0,0,0,0,0,0,0,0,0,0,1"
kwriteconfig6 --file kdeglobals --group General --key fixed "Consolas,10,-1,5,400,0,0,0,0,0,0,0,0,0,0,1"
kwriteconfig6 --file kdeglobals --group General --key menuFont "Segoe UI,10,-1,5,400,0,0,0,0,0,0,0,0,0,0,1"
kwriteconfig6 --file kdeglobals --group General --key toolBarFont "Segoe UI,10,-1,5,400,0,0,0,0,0,0,0,0,0,0,1"
kwriteconfig6 --file kdeglobals --group General --key smallestReadableFont "Segoe UI,8,-1,5,400,0,0,0,0,0,0,0,0,0,0,1"
kwriteconfig6 --file kdeglobals --group WM --key font "Segoe UI,10,-1,5,600,0,0,0,0,0,0,0,0,0,0,1"

# 5. ClearType-like subpixel RGB rendering & slight hinting (Windows ClearType appearance)
kwriteconfig6 --file kcmfonts --group General --key antiAliasing --type bool true
kwriteconfig6 --file kcmfonts --group General --key hinting slight
kwriteconfig6 --file kcmfonts --group General --key subPixel rgb

# 6. Notify KWin & Desktop to reload fonts
echo "=== [4/4] Reloading Desktop Fonts ==="
qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null || true

echo "======================================================================"
echo " [SUCCESS] System font changed to authentic Windows Segoe UI!"
echo "   - General & UI Font:   Segoe UI (10 pt)"
echo "   - Window Titles:       Segoe UI SemiBold (10 pt)"
echo "   - Menus & Toolbars:    Segoe UI (10 pt)"
echo "   - Small Text:          Segoe UI (8 pt)"
echo "   - Terminal / Fixed:    Consolas (10 pt)"
echo "   - Smoothing:           ClearType Subpixel RGB + Slight Hinting"
echo "======================================================================"
