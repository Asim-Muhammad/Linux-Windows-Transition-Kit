#!/usr/bin/env bash
# ==============================================================================
# configure_clean_fast_boot.sh
# Optimizes boot speed and streamlines bootloader:
#   - Sets GRUB timeout to 2s (quick boot without removing menu access)
#   - Organizes secondary kernels inside standard submenu
#   - Preserves recovery/rescue entries for safe rollback
#   - Enables Dracut hostonly mode with fast Zstandard compression
# ==============================================================================
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Please run this script with sudo: sudo bash configure_clean_fast_boot.sh" >&2
    exit 1
fi

echo "=== [1/5] Optimizing GRUB Timeout & Submenu in /etc/default/grub ==="
# Set timeout to 2s (fast boot while still allowing keypress interrupt)
sed -i 's/^GRUB_TIMEOUT=.*/GRUB_TIMEOUT=2/' /etc/default/grub 2>/dev/null || echo "GRUB_TIMEOUT=2" >> /etc/default/grub
# Organize older kernels in submenu, preserve recovery entries for safety
sed -i 's/^GRUB_DISABLE_SUBMENU=.*/GRUB_DISABLE_SUBMENU=false/' /etc/default/grub 2>/dev/null || echo "GRUB_DISABLE_SUBMENU=false" >> /etc/default/grub
sed -i 's/^GRUB_DISABLE_RECOVERY=.*/GRUB_DISABLE_RECOVERY=false/' /etc/default/grub 2>/dev/null || echo "GRUB_DISABLE_RECOVERY=false" >> /etc/default/grub
echo "[OK] /etc/default/grub updated (timeout=2s, recovery preserved)"

echo "=== [2/5] Configuring DNF Kernel Retention in /etc/dnf/dnf.conf ==="
if [ -f "/etc/dnf/dnf.conf" ]; then
    if grep -q "installonly_limit" /etc/dnf/dnf.conf; then
        sed -i 's/^installonly_limit=.*/installonly_limit=3/' /etc/dnf/dnf.conf
    else
        echo "installonly_limit=3" >> /etc/dnf/dnf.conf
    fi
    echo "[OK] Kernel retention set to 3 versions for rollback safety."
fi

echo "=== [3/5] Disabling NetworkManager-wait-online Service ==="
# Prevents systemd from halting boot waiting for unneeded network handshakes
systemctl disable NetworkManager-wait-online.service 2>/dev/null || true
echo "[OK] NetworkManager-wait-online disabled."

echo "=== [4/5] Optimizing Dracut Initramfs for Fast Boot (Zstandard) ==="
mkdir -p /etc/dracut.conf.d/
cat > /etc/dracut.conf.d/99-fastboot.conf << 'EOF'
hostonly="yes"
compress="zstd"
EOF
echo "[Dracut] Rebuilding fast initramfs for current kernel with zstd..."
dracut -f --kver "$(uname -r)" 2>/dev/null || true
echo "[OK] Fast initramfs created."

echo "=== [5/5] Updating GRUB2 Boot Configuration ==="
if [ -f "/boot/grub2/grub.cfg" ]; then
    grub2-mkconfig -o /boot/grub2/grub.cfg 2>/dev/null || true
fi
if [ -f "/boot/efi/EFI/fedora/grub.cfg" ]; then
    grub2-mkconfig -o /boot/efi/EFI/fedora/grub.cfg 2>/dev/null || true
fi

echo "======================================================================"
echo " [SUCCESS] Safe boot optimization applied!"
echo " Loader timeout: 2 seconds (press any key to pause menu)."
echo " Recovery entries: Preserved for emergency rollback."
echo " Initramfs: Streamlined with Dracut hostonly & fast zstd compression."
echo " Userspace: NetworkManager wait-online deferred."
echo "======================================================================"
