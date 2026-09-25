#!/usr/bin/env bash
# ==============================================================================
# WINDOWS 10 FILE EXPLORER FOR FEDORA KDE — 1-CLICK COMPLETE INSTALLER
# Restores the exact final iteration (This PC, Local Disk C: & D:, 3D Windows 10
# imageres.dll icons, bright taskbar icons, USB hotplug, C:\Users\user dot-folders,
# image/video thumbnails, and auto-rebuild hook) in < 10 seconds!
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "[*] Installing required packages (if missing)..."
if command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y dolphin gcc-c++ qt6-qtbase-devel ffmpegthumbs kdegraphics-thumbnailers 2>/dev/null || true
fi

echo "[*] Running main Windows 10 File Explorer setup..."
if [ -f "${SCRIPT_DIR}/fedora-win-explorer/setup-win-dolphin.sh" ]; then
    bash "${SCRIPT_DIR}/fedora-win-explorer/setup-win-dolphin.sh"
elif [ -f "${SCRIPT_DIR}/setup-win-dolphin.sh" ]; then
    bash "${SCRIPT_DIR}/setup-win-dolphin.sh"
fi

echo "[*] Restoring pre-built authentic Windows 10 3D icon theme & configs..."
if [ -d "${SCRIPT_DIR}/config_and_themes/Win10-Explorer-icons" ]; then
    mkdir -p "${HOME}/.local/share/icons"
    rm -rf "${HOME}/.local/share/icons/Win10-Explorer"
    cp -r "${SCRIPT_DIR}/config_and_themes/Win10-Explorer-icons" "${HOME}/.local/share/icons/Win10-Explorer"
fi

if [ -d "${SCRIPT_DIR}/config_and_themes/win-explorer-config" ]; then
    mkdir -p "${HOME}/.config/win-explorer"
    cp -r "${SCRIPT_DIR}/config_and_themes/win-explorer-config/"* "${HOME}/.config/win-explorer/"
fi

echo "[*] Compiling libwin10_thispc.so..."
g++ -shared -fPIC -O2 -I/usr/include/qt6 -I/usr/include/qt6/QtCore -I/usr/include/qt6/QtGui -I/usr/include/qt6/QtWidgets \
    "${HOME}/.config/win-explorer/win10_thispc_helper.cpp" \
    -o "${HOME}/.config/win-explorer/libwin10_thispc.so" \
    -lQt6Widgets -lQt6Gui -lQt6Core -ldl

mkdir -p "${HOME}/Desktop"
if [ -d "${SCRIPT_DIR}/desktop_shortcuts" ]; then
    cp "${SCRIPT_DIR}/desktop_shortcuts/"*.desktop "${HOME}/Desktop/" 2>/dev/null || true
    chmod +x "${HOME}/Desktop/this-pc.desktop" "${HOME}/Desktop/user.desktop" 2>/dev/null || true
fi

rm -f "${HOME}/.cache/icon-cache.kcache" "${HOME}/.cache/ksycoca6"* 2>/dev/null || true
command -v plasma-apply-icontheme >/dev/null 2>&1 && plasma-apply-icontheme Win10-Explorer >/dev/null 2>&1 || true
command -v kbuildsycoca6 >/dev/null 2>&1 && kbuildsycoca6 --noincremental >/dev/null 2>&1 || true

killall -9 dolphin 2>/dev/null || true
setsid "${HOME}/.local/bin/win-explorer" </dev/null >/dev/null 2>&1 &
echo "[✓] Windows 10 File Explorer completely installed and launched!"
