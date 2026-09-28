#!/bin/bash
# ==============================================================================
# Kernel Security Hardening Configuration
# Applies sysctl network, filesystem, and kernel pointer restrictions.
# ==============================================================================
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Please run this script with sudo: sudo bash install_kernel_hardening.sh" >&2
    exit 1
fi

echo "Applying Kernel Security Hardening sysctl parameters..."

cat << 'EOF' > /etc/sysctl.d/99-security-hardening.conf
# Filesystem protection against link attacks
fs.protected_hardlinks = 1
fs.protected_symlinks = 1
fs.protected_fifos = 2
fs.protected_regular = 2
fs.suid_dumpable = 0

# Kernel pointer and dmesg restriction
kernel.dmesg_restrict = 1
kernel.kptr_restrict = 2
kernel.unprivileged_bpf_disabled = 1

# Network spoofing and redirect protection
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.default.rp_filter = 1
net.ipv4.tcp_syncookies = 1
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.default.accept_redirects = 0
net.ipv6.conf.all.accept_redirects = 0
net.ipv6.conf.default.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0
net.ipv4.conf.default.send_redirects = 0
EOF

sysctl --system >/dev/null 2>&1 || true
echo "Done! Kernel security parameters applied to /etc/sysctl.d/99-security-hardening.conf"
