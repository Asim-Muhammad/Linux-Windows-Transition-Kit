#!/bin/bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Please run this installer with sudo." >&2
    exit 1
fi

sudo mkdir -p /etc/systemd/system/clamd@scan.service.d/

cat << 'SCRIPT' | sudo tee /usr/local/bin/clamav-wait-idle.sh > /dev/null
#!/bin/bash
MAX_WAIT=40
SECONDS_WAITED=0

# Wait for CPU 1-minute load average to drop below 1.0 (indicating desktop is idle)
while [ $(awk '{print int($1)}' /proc/loadavg) -ge 1 ]; do
    if [ $SECONDS_WAITED -ge $MAX_WAIT ]; then
        exit 0
    fi
    sleep 2
    SECONDS_WAITED=$((SECONDS_WAITED + 2))
done
SCRIPT
sudo chmod +x /usr/local/bin/clamav-wait-idle.sh

cat << 'CONF' | sudo tee /etc/systemd/system/clamd@scan.service.d/delay.conf > /dev/null
[Service]
ExecStartPre=
ExecStartPre=/usr/local/bin/clamav-wait-idle.sh
CONF
sudo systemctl daemon-reload
