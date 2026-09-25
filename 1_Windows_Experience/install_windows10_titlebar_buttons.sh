#!/usr/bin/env bash
# ==============================================================================
# install_windows10_titlebar_buttons.sh
# Replaces KDE default window control buttons (chevron, diamond, cross)
# with authentic Windows 10 titlebar buttons:
#   - Minimize:  Clean bottom dash/line (―)
#   - Maximize:  Square outline (□) / Restore (❐)
#   - Close:     Cross (✕) with full red highlight on hover
# ==============================================================================
set -euo pipefail

echo "=== [1/3] Preparing Aurorae Themes Directory ==="
AURORAE_DIR="$HOME/.local/share/aurorae/themes"
mkdir -p "$AURORAE_DIR"

TMP_DIR=$(mktemp -d /tmp/win10-deco-XXXXXX)

echo "=== [2/3] Downloading Windows 10 Window Decoration Theme ==="
if git clone --depth 1 https://github.com/SourLemonJuice/windows10-kde-decoration.git "$TMP_DIR" 2>/dev/null; then
    cp -rf "$TMP_DIR"/aurorae/themes/Windows10-* "$AURORAE_DIR/"
    echo "[OK] Installed Windows10-dark and Windows10-light themes to $AURORAE_DIR"
else
    echo "[!] Git clone failed. Falling back to direct archive download..."
    curl -fsSL https://github.com/SourLemonJuice/windows10-kde-decoration/archive/refs/heads/master.tar.gz | tar -xz -C "$TMP_DIR"
    cp -rf "$TMP_DIR"/windows10-kde-decoration-master/aurorae/themes/Windows10-* "$AURORAE_DIR/"
    echo "[OK] Downloaded and installed Windows 10 themes."
fi
rm -rf "$TMP_DIR"

echo "=== [3/3] Applying Windows 10 Titlebar Theme to KWin ==="
# Set KWin window decoration engine to Aurorae with Windows 10 Dark theme
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key library org.kde.kwin.aurorae
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key theme __aurorae__svg__Windows10-dark
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key ButtonsOnRight IAX
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key BorderSize None

# Reload KWin window manager to apply immediately across all open windows
qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null || true

echo "======================================================================"
echo " [SUCCESS] Titlebar buttons changed system-wide to Windows 10!"
echo "   - Minimize:  ― (Bottom line)"
echo "   - Maximize:  □ (Square) / ❐ (Restore)"
echo "   - Close:     ✕ (Turns red on hover)"
echo "======================================================================"
