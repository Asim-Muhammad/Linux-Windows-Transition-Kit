#!/bin/bash
# ==============================================================================
# Windows-Style Right-Click Context Menus for Dolphin / KDE Plasma
# ==============================================================================
set -euo pipefail

echo "Installing Windows-style Right Click Context Menus..."

mkdir -p ~/.local/share/kio/servicemenus/

# 1. Add "Refresh" to Right-Click Menu
cat << 'INNER_EOF' > ~/.local/share/kio/servicemenus/refresh.desktop
[Desktop Entry]
Type=Service
Icon=view-refresh
Actions=Refresh
MimeType=all/allfiles;inode/directory;
X-KDE-Priority=TopLevel
X-KDE-StartupNotify=false

[Desktop Action Refresh]
Name=Refresh
Icon=view-refresh
Exec=dbus-send --session --dest=org.kde.plasmashell --type=method_call /org/kde/plasmashell org.kde.plasmashell.evaluateScript string:"workspace.refresh()"
INNER_EOF

# 2. Add "Run as Administrator" using standard Polkit (pkexec)
cat << 'INNER_EOF' > ~/.local/share/kio/servicemenus/run_as_admin.desktop
[Desktop Entry]
Type=Service
Icon=security-high
Actions=RunAsAdmin
MimeType=application/x-executable;application/x-shellscript;
X-KDE-Priority=TopLevel

[Desktop Action RunAsAdmin]
Name=Run as Administrator
Icon=security-high
Exec=pkexec "%f"
INNER_EOF

echo "Done! Right-click 'Refresh' and 'Run as Administrator' have been added."
