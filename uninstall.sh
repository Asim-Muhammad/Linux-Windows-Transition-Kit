#!/bin/bash
# ==============================================================================
# Linux-Windows Transition Kit Uninstaller
# Thoroughly reverts installed UI hooks, user/system services, and configurations.
# ==============================================================================
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "================================================================="
echo "             Linux-Windows Transition Kit Uninstaller            "
echo "================================================================="
echo "This will revert UI customizations, stop background services,"
echo "and clean up installed scripts, shortcuts, and configurations."
echo "================================================================="
read -p "Are you sure you want to proceed? (y/N): " confirm
if [[ "${confirm,,}" != "y" ]]; then
    echo "Uninstall cancelled."
    exit 0
fi

echo "[1/7] Restoring stock Dolphin and Windows Explorer UI..."
if [ -f "$DIR/1_Windows_Experience/fedora-win-explorer/setup-win-dolphin.sh" ]; then
    bash "$DIR/1_Windows_Experience/fedora-win-explorer/setup-win-dolphin.sh" --restore 2>/dev/null || true
fi
make -C "$DIR/1_Windows_Experience/fedora-win-explorer" uninstall 2>/dev/null || true
rm -f "$HOME/Desktop/this-pc.desktop" "$HOME/Desktop/user.desktop" 2>/dev/null || true

echo "[2/7] Reverting Windows titlebar, fonts, and right-click context menus..."
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key library org.kde.breeze 2>/dev/null || true
kwriteconfig6 --file kwinrc --group org.kde.kdecoration2 --key theme Breeze 2>/dev/null || true
qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null || true
rm -f ~/.local/share/kio/servicemenus/refresh.desktop ~/.local/share/kio/servicemenus/run_as_admin.desktop 2>/dev/null || true
rm -f ~/.bashrc.d/windows_diagnostics.sh ~/.bashrc.d/case_insensitive.sh 2>/dev/null || true

echo "[3/7] Removing Net Doctor GUI and desktop shortcuts..."
rm -f ~/.local/bin/netdoctor
rm -f ~/.local/share/applications/netdoctor.desktop
rm -f ~/.local/share/icons/hicolor/scalable/apps/netdoctor.svg
rm -rf ~/.local/share/netdoctor 2>/dev/null || true

echo "[4/7] Stopping and disabling custom daemons..."
systemctl --user disable --now thermal-monitor.service 2>/dev/null || true
rm -f ~/.config/systemd/user/thermal-monitor.service 2>/dev/null || true
systemctl --user daemon-reload 2>/dev/null || true

sudo systemctl disable --now timeshift-auto-prune.timer 2>/dev/null || true
sudo rm -f /etc/systemd/system/timeshift-auto-prune.service /etc/systemd/system/timeshift-auto-prune.timer
sudo rm -f /usr/local/bin/timeshift-auto-prune
if [ -f "/etc/timeshift/timeshift.json" ]; then
    sudo sed -i 's/"count_daily"[[:space:]]*:[[:space:]]*"[0-9]*"/"count_daily" : "5"/' /etc/timeshift/timeshift.json 2>/dev/null || true
fi

sudo systemctl disable --now usbguard.service 2>/dev/null || true
echo "Notice: /etc/usbguard/rules.conf was preserved to prevent device lockouts."
sudo sed -i 's/^InsertedDevicePolicy=.*/InsertedDevicePolicy=apply-policy/' /etc/usbguard/usbguard-daemon.conf 2>/dev/null || true

echo "[5/7] Safely removing systemd drop-ins and sysctl rules..."
sudo rm -f /etc/udev/rules.d/99-usb-boot-guard.rules
sudo rm -f /etc/systemd/system/clamd@scan.service.d/delay.conf
sudo rm -f /usr/local/bin/clamav-wait-idle.sh
sudo rmdir --ignore-fail-on-non-empty /etc/systemd/system/clamd@scan.service.d/ 2>/dev/null || true

sudo rm -f /etc/sysctl.d/99-security-hardening.conf
sudo sysctl --system 2>/dev/null || true
sudo systemctl daemon-reload

echo "[6/7] Reverting Dracut fastboot drop-ins..."
if [ -f "/etc/dracut.conf.d/99-fastboot.conf" ]; then
    sudo rm -f /etc/dracut.conf.d/99-fastboot.conf
    sudo systemctl enable NetworkManager-wait-online.service 2>/dev/null || true
    echo "Rebuilding standard initramfs..."
    sudo dracut -f --kver "$(uname -r)" 2>/dev/null || true
fi

echo "[7/7] Cleaning up Wine desktop association overrides..."
rm -f ~/.local/share/applications/wine.desktop 2>/dev/null || true
update-mime-database ~/.local/share/mime 2>/dev/null || true

echo "================================================================="
echo "  UNINSTALL COMPLETE! Customizations reverted successfully. "
echo "================================================================="
