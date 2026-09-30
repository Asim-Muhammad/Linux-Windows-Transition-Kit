#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== Installing NetDoctor (Linux Network Troubleshooter & Reset) ==="

# 1. Create standard local directories
mkdir -p "$HOME/.local/bin"
mkdir -p "$HOME/.local/share/applications"
mkdir -p "$HOME/.local/share/icons/hicolor/scalable/apps"

# 2. Symlink executable launchers
ln -sf "$DIR/bin/netdoctor" "$HOME/.local/bin/netdoctor"
ln -sf "$DIR/bin/netdoctor" "$HOME/.local/bin/linux-net-troubleshooter"
chmod +x "$DIR/bin/netdoctor"
chmod +x "$HOME/.local/bin/netdoctor"
chmod +x "$HOME/.local/bin/linux-net-troubleshooter"

# 3. Install Icon
cp "$DIR/data/netdoctor.svg" "$HOME/.local/share/icons/hicolor/scalable/apps/netdoctor.svg"
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
fi

# 4. Install Desktop File
cp "$DIR/data/netdoctor.desktop" "$HOME/.local/share/applications/"
chmod +x "$HOME/.local/share/applications/netdoctor.desktop"

# 5. Refresh Desktop Database so Application Menu & Taskbar immediately see it
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
fi

if command -v kbuildsycoca5 >/dev/null 2>&1; then
    kbuildsycoca5 2>/dev/null || true
elif command -v kbuildsycoca6 >/dev/null 2>&1; then
    kbuildsycoca6 2>/dev/null || true
fi

echo "✔ NetDoctor installed successfully!"
echo "✔ Commands available: 'netdoctor' and 'linux-net-troubleshooter'"
echo "✔ Desktop launcher: $HOME/.local/share/applications/netdoctor.desktop"
echo "✔ Icon: $HOME/.local/share/icons/hicolor/scalable/apps/netdoctor.svg"
