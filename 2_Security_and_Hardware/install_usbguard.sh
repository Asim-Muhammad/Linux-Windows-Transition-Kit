#!/bin/bash
set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Please run with sudo." >&2
    exit 1
fi

echo "Installing USBGuard..."
dnf install -y usbguard

echo "WARNING: Generating baseline policy. ALL CURRENTLY CONNECTED USB DEVICES WILL BE PERMANENTLY AUTHORIZED."
echo "Please unplug any untrusted USB devices now."
sleep 3

usbguard generate-policy > /etc/usbguard/rules.conf
sed -i 's/^InsertedDevicePolicy=.*/InsertedDevicePolicy=block/' /etc/usbguard/usbguard-daemon.conf
systemctl enable --now usbguard.service

echo "SUCCESS: USBGuard is configured to block newly inserted USB devices by default."
