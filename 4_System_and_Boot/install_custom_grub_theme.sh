#!/bin/bash
# ==============================================================================
# Custom GRUB Theme Installer
# Safely extracts and applies a GRUB theme without running untrusted scripts.
# ==============================================================================
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Please run this installer with sudo: sudo bash install_custom_grub_theme.sh" >&2
    exit 1
fi

read -p "Enter the direct URL to the GRUB theme .tar.gz (or press Enter to skip): " THEME_URL
if [ -z "$THEME_URL" ]; then
    echo "No URL provided. Exiting."
    exit 0
fi

echo "Downloading theme archive..."
TMP_DIR=$(mktemp -d /tmp/grub-theme-XXXXXX)
if ! curl -fsSL "$THEME_URL" -o "$TMP_DIR/theme.tar.gz"; then
    echo "ERROR: Failed to download theme from '$THEME_URL'." >&2
    rm -rf "$TMP_DIR"
    exit 1
fi

echo "Extracting archive safely..."
tar -xzf "$TMP_DIR/theme.tar.gz" -C "$TMP_DIR"

# Locate the folder containing theme.txt
THEME_TXT_PATH=$(find "$TMP_DIR" -name "theme.txt" -print -quit 2>/dev/null || true)
if [ -z "$THEME_TXT_PATH" ]; then
    echo "ERROR: No 'theme.txt' found inside the archive. Aborting." >&2
    rm -rf "$TMP_DIR"
    exit 1
fi

THEME_FOLDER=$(dirname "$THEME_TXT_PATH")
THEME_NAME=$(basename "$THEME_FOLDER")

echo "Installing theme assets to /boot/grub2/themes/$THEME_NAME..."
mkdir -p "/boot/grub2/themes/"
cp -rf "$THEME_FOLDER" "/boot/grub2/themes/$THEME_NAME"

echo "Updating GRUB configuration..."
if grep -q "^GRUB_THEME=" /etc/default/grub; then
    sed -i "s|^GRUB_THEME=.*|GRUB_THEME=\"/boot/grub2/themes/$THEME_NAME/theme.txt\"|" /etc/default/grub
else
    echo "GRUB_THEME=\"/boot/grub2/themes/$THEME_NAME/theme.txt\"" >> /etc/default/grub
fi

if [ -f "/boot/grub2/grub.cfg" ]; then
    grub2-mkconfig -o /boot/grub2/grub.cfg 2>/dev/null || true
fi
if [ -f "/boot/efi/EFI/fedora/grub.cfg" ]; then
    grub2-mkconfig -o /boot/efi/EFI/fedora/grub.cfg 2>/dev/null || true
fi

rm -rf "$TMP_DIR"
echo "[OK] Custom GRUB theme '$THEME_NAME' applied successfully!"
