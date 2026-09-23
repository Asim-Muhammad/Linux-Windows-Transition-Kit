#!/bin/bash
# ==============================================================================
# Linux-Windows Transition Kit Installer
# Modular installer with per-module dependency checks and clean error handling.
# ==============================================================================
set -euo pipefail
IFS=$'\n\t'

echo "================================================================="
echo "             Linux-Windows Transition Kit Installer              "
echo "================================================================="
echo "Select the features you want to install by typing their numbers."
echo "Example: '1 2 4' to install options 1, 2, and 4."
echo "Leave blank to ABORT (no default install)."
echo ""
echo "[1]  Windows 10 Explorer UI Plugin (Dolphin C++ Hook & Theme)"
echo "[2]  Windows Start Menu Tweaks, Fonts & Titlebars"
echo "[3]  .EXE & .MSI Wine Double-Click Integration"
echo "[4]  Net Doctor Network Diagnostics GUI"
echo "[5]  Thermal Monitor Desktop Alert Guard (User Service)"
echo "[6]  Timeshift Auto-Pruning System (Retain Latest Snapshot)"
echo "[7]  Zstandard Dracut & Clean Fast Boot Optimization"
echo "[8]  Silent Background OS Updates (dnf-automatic)"
echo "[9]  Custom GRUB Bootloader Theme Installer"
echo "[10] Dynamic Boot-Time Antivirus Deferral (CPU Idle Wait)"
echo "[11] USBGuard USB Device Authorization"
echo "[12] 0-Second GRUB Instant Boot Menu"
echo "================================================================="
read -p "Enter choices separated by spaces: " choices

if [[ -z "$choices" ]]; then
    echo "No modules selected. Installation aborted."
    exit 0
fi

# Validate all choices before touching anything
for choice in $choices; do
    if [[ ! "$choice" =~ ^(1|2|3|4|5|6|7|8|9|10|11|12)$ ]]; then
        echo "ERROR: Invalid choice '$choice'. Valid options: 1 through 12." >&2
        exit 1
    fi
done

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CURRENT_USER="${SUDO_USER:-${USER}}"

for choice in $choices; do
    echo ""
    case $choice in
        1)
            echo "[1] Installing Windows 10 Explorer UI Plugin & Dolphin Wrapper..."
            command -v make >/dev/null 2>&1 || { echo >&2 "Installing make..."; sudo dnf install -y make; }
            command -v g++  >/dev/null 2>&1 || { echo >&2 "Installing gcc-c++..."; sudo dnf install -y gcc-c++; }
            pkg-config --exists Qt6Widgets 2>/dev/null || { echo >&2 "Installing qt6-qtbase-devel..."; sudo dnf install -y qt6-qtbase-devel; }
            bash "$DIR/1_Windows_Experience/fedora-win-explorer/setup-win-dolphin.sh"
            ;;
        2)
            echo "[2] Installing Windows Fonts, Titlebar Buttons, and Context Menus..."
            bash -euo pipefail "$DIR/1_Windows_Experience/install_windows_fonts.sh"
            bash -euo pipefail "$DIR/1_Windows_Experience/install_windows10_titlebar_buttons.sh"
            bash -euo pipefail "$DIR/1_Windows_Experience/install_windows_terminal_aliases.sh"
            bash -euo pipefail "$DIR/1_Windows_Experience/install_7zip_integration.sh"
            bash -euo pipefail "$DIR/1_Windows_Experience/install_right_click_menus.sh"
            python3 "$DIR/1_Windows_Experience/setup_windows_experience.py"
            ;;
        3)
            echo "[3] Installing Wine .EXE / .MSI Integration..."
            bash -euo pipefail "$DIR/1_Windows_Experience/setup_wine_integration.sh"
            ;;
        4)
            echo "[4] Installing Net Doctor Diagnostics GUI..."
            NETDOCTOR_DIR="$DIR/3_Networking/linux-net-troubleshooter"
            INSTALL_ROOT="/home/${CURRENT_USER}/.local/share/netdoctor"
            TARGET_BIN="/home/${CURRENT_USER}/.local/bin/netdoctor"
            ICON_DIR="/home/${CURRENT_USER}/.local/share/icons/hicolor/scalable/apps"
            APP_DIR="/home/${CURRENT_USER}/.local/share/applications"

            mkdir -p "$INSTALL_ROOT" "$(dirname "$TARGET_BIN")" "$ICON_DIR" "$APP_DIR"
            cp -rf "$NETDOCTOR_DIR/netdoctor" "$INSTALL_ROOT/"
            cp -rf "$NETDOCTOR_DIR/bin" "$INSTALL_ROOT/"
            chmod +x "$INSTALL_ROOT/bin/netdoctor"
            ln -sf "$INSTALL_ROOT/bin/netdoctor" "$TARGET_BIN"
            cp -f "$NETDOCTOR_DIR/data/netdoctor.svg" "$ICON_DIR/netdoctor.svg" 2>/dev/null || true

            cat > "$APP_DIR/netdoctor.desktop" << DEOF
[Desktop Entry]
Version=1.0
Type=Application
Name=NetDoctor
GenericName=Network Troubleshooter & Reset
Comment=Automated network diagnostics, DNS fixer, and safe network stack reset
Exec=${TARGET_BIN}
Icon=${ICON_DIR}/netdoctor.svg
Terminal=false
Categories=Network;System;
Keywords=network;internet;troubleshoot;diagnose;reset;dns;wifi;ethernet;
StartupNotify=true
DEOF
            echo "Net Doctor GUI installed successfully to ${TARGET_BIN}"
            ;;
        5)
            echo "[5] Installing Thermal Monitor Guard (User Service)..."
            TARGET_BIN="/home/${CURRENT_USER}/.local/bin/thermal_monitor.py"
            USER_SYSTEMD="/home/${CURRENT_USER}/.config/systemd/user"
            mkdir -p "$(dirname "$TARGET_BIN")" "$USER_SYSTEMD"
            cp -f "$DIR/2_Security_and_Hardware/thermal_monitor.py" "$TARGET_BIN"
            chmod +x "$TARGET_BIN"

            cat > "$USER_SYSTEMD/thermal-monitor.service" << SRV
[Unit]
Description=Thermal Guard Monitor (Desktop Alerts)
After=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 ${TARGET_BIN}
Restart=on-failure

[Install]
WantedBy=default.target
SRV
            systemctl --user daemon-reload 2>/dev/null || true
            systemctl --user enable --now thermal-monitor.service 2>/dev/null || true
            echo "Thermal Monitor user service installed and started."
            ;;
        6)
            echo "[6] Installing Timeshift Single-Snapshot Auto-Pruning System..."
            sudo bash "$DIR/4_System_and_Boot/install_timeshift_auto_prune.sh"
            ;;
        7)
            echo "[7] Applying Zstandard Dracut & Clean Fast Boot Optimization..."
            sudo bash "$DIR/4_System_and_Boot/configure_clean_fast_boot.sh"
            ;;
        8)
            echo "[8] Setting up Silent Background OS Updates..."
            sudo bash "$DIR/4_System_and_Boot/setup_auto_updates.sh"
            ;;
        9)
            echo "[9] Launching Custom GRUB Theme Installer..."
            sudo bash "$DIR/4_System_and_Boot/install_custom_grub_theme.sh"
            ;;
        10)
            echo "[10] Applying Dynamic Boot-Time Antivirus Deferral (CPU Idle Wait)..."
            sudo bash "$DIR/2_Security_and_Hardware/delay_antivirus.sh"
            ;;
        11)
            echo "[11] Installing USBGuard USB Device Authorization..."
            sudo bash "$DIR/2_Security_and_Hardware/install_usbguard.sh"
            ;;
        12)
            echo "[12] Configuring 0-Second GRUB Instant Boot Menu..."
            bash "$DIR/4_System_and_Boot/fast_boot_0_sec.sh"
            ;;
        *)
            echo "Invalid choice: $choice"
            ;;
    esac
done

echo ""
echo "================================================================="
echo "                     INSTALLATION COMPLETE!                      "
echo "================================================================="
