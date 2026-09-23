# Linux-Windows Transition Kit

**A modular systems engineering toolkit designed to adapt Fedora Linux into an optimized, Windows-familiar desktop environment.**

---

## 🎯 Project Scope & Architecture
This toolkit provides a curated collection of desktop UI customizations, diagnostic tools, and system configurations. It bridges the gap between Linux's underlying architecture and the muscle memory of Windows power users, without claiming unverified enterprise or military-grade guarantees.

Modules are independently selectable during installation via `install.sh`.

---

## 📦 Core Installer Modules (Options 1–12 in `install.sh`)

| Option | Module | Description |
| :---: | :--- | :--- |
| **1** | **Windows 10 Explorer UI Plugin** | C++ dynamic library (`libwin10_thispc.so`) hooking KDE Dolphin to provide a "This PC" view with capacity bars and folder/drive grouping. |
| **2** | **Desktop Polish, Fonts & Titlebars** | Segoe UI typography, authentic Windows 10 titlebar buttons, right-click context menus, and Windows terminal aliases (`taskmgr`, `dxdiag`, `dir`). |
| **3** | **Wine .EXE / .MSI Integration** | Registers FreeDesktop MIME associations for Windows executables and unmaps Wine's default `Z:` drive (filesystem exposure reduction, not container isolation). |
| **4** | **Net Doctor Diagnostic GUI** | Standalone GTK3 network diagnostic and repair utility (`netdoctor`) covering 8 layers of network health, Wi-Fi power-save, DNS latency, and routing resets. |
| **5** | **Thermal Monitor Guard** | User-level systemd service alerting via desktop notifications (`notify-send`) when CPU temperature thresholds are exceeded. |
| **6** | **Timeshift Auto-Pruning System** | Automated systemd timer maintaining a safe 2-snapshot retention window to prevent BTRFS/disk exhaustion while preserving recovery points. |
| **7** | **Zstandard Dracut Boot Tuning** | Streamlines initramfs generation using Dracut `hostonly` and Zstandard (`zstd`) compression while preserving rescue kernel entries. |
| **8** | **Silent Background OS Updates** | Configures `dnf-automatic` systemd timers to automatically install security and package updates. |
| **9** | **Custom GRUB Theme Installer** | Theme installer that parses `theme.txt` and copies theme assets safely without executing arbitrary archive scripts. |
| **10** | **Dynamic Antivirus Deferral** | Defers ClamAV (`clamd@scan`) startup until 1-minute CPU load average drops below 1.0 (max 40s timeout) to prevent login congestion. |
| **11** | **USBGuard Device Authorization** | Enforces post-boot `InsertedDevicePolicy=block` against unauthorized USB device classes (BadUSB / Rubber Ducky). |
| **12** | **0-Second GRUB Instant Boot** | Bypasses bootloader delay for instant boot, with interactive warning on recovery menu access. |

---

## 🧪 Standalone / Supplementary Utilities (In Repository)
The following supplementary tools are included in the repository for standalone or specialized use:
*   **`windows-security-center.py`:** Standalone Qt6 system security tray monitor and dashboard (SELinux, firewall, ClamAV status).
*   **`audit_daemon.py` & `crypto_engine.py`:** Standalone CLI inspection tool and AES-256-CBC encrypted local ledger engine with SHA-256 tamper-evident hash chaining.
*   **`ha_multiroute_daemon.py`:** Standalone local SOCKS5 proxy daemon with health probing across upstream endpoints and fail-closed security (prevents traffic leaks unless `--allow-direct-fallback` is passed).
*   **`secure_usb_boot.sh`:** Experimental udev early-boot USB device unbind script (USBGuard in Module 11 is the recommended standard).

---

## ⚙️ Installation & Usage

1. Open your terminal in the repository directory.
2. Launch the interactive installer:
```bash
chmod +x install.sh
./install.sh
```
3. Enter the space-separated numbers of the specific modules you wish to deploy (e.g. `1 2 4` to install only the Explorer, UI tweaks, and Net Doctor), or leave blank to cancel.

---

## 🔄 Uninstallation
A dedicated `uninstall.sh` script is included to revert UI customizations, remove application shortcuts, restore stock Dolphin configurations, disable background services, and clean up sysctl/drop-in configurations:
```bash
chmod +x uninstall.sh
./uninstall.sh
```
*Note: Bootloader timeout and package updates are preserved on uninstall to ensure uninterrupted boot recovery.*
