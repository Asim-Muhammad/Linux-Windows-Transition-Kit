#!/bin/bash
echo "Installing DNF Automatic..."
sudo dnf install -y dnf-automatic

echo "Configuring silent auto-installs..."
# Change apply_updates = no to apply_updates = yes
sudo sed -i 's/apply_updates = no/apply_updates = yes/' /etc/dnf/automatic.conf

echo "Enabling the silent background timer..."
# Disable the default 'download-only' timer just in case
sudo systemctl disable --now dnf-automatic.timer 2>/dev/null || true
# Enable the 'download and install' timer
sudo systemctl enable --now dnf-automatic-install.timer

echo ""
echo "Done! Your system will now silently update itself in the background."
