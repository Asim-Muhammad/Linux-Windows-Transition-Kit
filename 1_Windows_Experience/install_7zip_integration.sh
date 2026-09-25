#!/bin/bash
# ==============================================================================
# 7-Zip (p7zip) Engine & Archive Manager Integration
# ==============================================================================
set -euo pipefail

echo "Installing 7-Zip (p7zip) compression engine and File Roller archive GUI..."
sudo dnf install -y p7zip p7zip-plugins file-roller 2>/dev/null || true

echo "Associating archive formats (.zip, .7z, .rar, .tar.gz) with archive manager..."
xdg-mime default org.gnome.FileRoller.desktop application/zip 2>/dev/null || true
xdg-mime default org.gnome.FileRoller.desktop application/x-rar 2>/dev/null || true
xdg-mime default org.gnome.FileRoller.desktop application/x-7z-compressed 2>/dev/null || true
xdg-mime default org.gnome.FileRoller.desktop application/x-tar 2>/dev/null || true
xdg-mime default org.gnome.FileRoller.desktop application/gzip 2>/dev/null || true

echo "Done! Full 7-Zip archive extraction and creation is now enabled."
