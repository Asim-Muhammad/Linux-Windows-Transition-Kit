#!/bin/bash
# ==============================================================================
# Wine Integration & .EXE / .MSI Double-Click Setup
# ==============================================================================
set -euo pipefail

echo "Installing Wine (Windows Translation Layer)..."
sudo dnf install -y wine wine-core wine-desktop 2>/dev/null || true

echo "Registering .EXE and .MSI file associations with Wine..."
mkdir -p ~/.local/share/applications
cat << 'EOF' > ~/.local/share/applications/wine.desktop
[Desktop Entry]
Type=Application
Name=Wine Windows Program Loader
Exec=wine start /unix %f
MimeType=application/x-ms-dos-executable;application/x-msdownload;application/x-msi;
Icon=wine
NoDisplay=true
EOF

xdg-mime default wine.desktop application/x-ms-dos-executable 2>/dev/null || true
xdg-mime default wine.desktop application/x-msdownload 2>/dev/null || true
xdg-mime default wine.desktop application/x-msi 2>/dev/null || true
update-mime-database ~/.local/share/mime 2>/dev/null || true

echo "Done! You can now double-click Windows .exe and .msi files to run them natively!"

echo "Reducing Wine Filesystem Exposure (Removing Z: Drive Mapping to Linux Root)..."
mkdir -p ~/.wine/dosdevices/
rm -f ~/.wine/dosdevices/z:
echo "Wine Z: drive removed. This reduces filesystem visibility for Windows applications (does not provide full container/namespace isolation)."
