#!/usr/bin/env bash
# ==============================================================================
# install_timeshift_auto_prune.sh
# Installs and enables the automatic Timeshift single-snapshot retention system.
# ==============================================================================
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Please run this installer with sudo: sudo bash install_timeshift_auto_prune.sh" >&2
    exit 1
fi

SCRIPTPATH="$DIR/timeshift-auto-prune.sh"
TARGETBIN="/usr/local/bin/timeshift-auto-prune"

chmod +x "$SCRIPTPATH"
cp -f "$SCRIPTPATH" "$TARGETBIN"
chmod 755 "$TARGETBIN"
echo "[1/4] Installed script to $TARGETBIN"

# 2. Create systemd Service unit
cat > /etc/systemd/system/timeshift-auto-prune.service << 'EOF'
[Unit]
Description=Timeshift Auto-Prune (Retain Only Verified Latest Snapshot)
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/timeshift-auto-prune
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF
echo "[2/4] Created /etc/systemd/system/timeshift-auto-prune.service"

# 3. Create systemd Timer unit (Runs every 6 hours and 15 mins after boot)
cat > /etc/systemd/system/timeshift-auto-prune.timer << 'EOF'
[Unit]
Description=Run Timeshift Auto-Prune Periodically

[Timer]
OnBootSec=15min
OnUnitActiveSec=6h
Persistent=true

[Install]
WantedBy=timers.target
EOF
echo "[3/4] Created /etc/systemd/system/timeshift-auto-prune.timer"

# 4. Reload systemd, enable and start timer
systemctl daemon-reload
systemctl enable --now timeshift-auto-prune.timer
echo "[4/4] Enabled and activated timeshift-auto-prune.timer"

echo "=== Running initial prune now to free space ==="
"$TARGETBIN"
echo "=== Setup Complete! Your system will now automatically keep only the latest snapshot. ==="
