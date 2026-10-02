#!/bin/bash
# ==============================================================================
# Fast Bootloader Timeout Configuration
# Sets GRUB menu to instant / minimal timeout.
# ==============================================================================
set -euo pipefail

echo "================================================================="
echo "                GRUB Fast Boot Timeout Configuration             "
echo "================================================================="
echo "WARNING: Setting GRUB_TIMEOUT=0 skips the boot menu on startup."
echo "If you ever need to access recovery or dual-boot menus, hold ESC/SHIFT"
echo "during BIOS/UEFI POST to force GRUB to appear."
echo "================================================================="

read -p "Set timeout to 0 seconds (instant boot)? (y/N): " confirm
if [[ "${confirm,,}" == "y" ]]; then
    sudo sed -i 's/^GRUB_TIMEOUT=.*/GRUB_TIMEOUT=0/' /etc/default/grub 2>/dev/null || echo "GRUB_TIMEOUT=0" | sudo tee -a /etc/default/grub >/dev/null
    sudo grub2-mkconfig -o /boot/grub2/grub.cfg 2>/dev/null || sudo grub2-mkconfig -o /boot/efi/EFI/fedora/grub.cfg 2>/dev/null || true
    echo "Done! GRUB timeout set to 0 seconds."
else
    echo "Cancelled. GRUB timeout unchanged."
fi
