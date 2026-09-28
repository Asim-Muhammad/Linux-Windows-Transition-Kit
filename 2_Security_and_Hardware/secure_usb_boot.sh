#!/bin/bash
echo "Installing Boot-Time USB Security Lock..."

sudo tee /usr/local/bin/usb_boot_guard.sh > /dev/null << 'INNER_EOF'
#!/bin/bash
UPTIME=$(cut -d. -f1 /proc/uptime)
if [ "$UPTIME" -lt 65 ]; then
    # If a USB is plugged in within 65 seconds of boot, kill the connection
    echo "$DEVPATH" > /sys/bus/usb/drivers/usb/unbind 2>/dev/null
    # Send an alert to the desktop (using sudo -u $SUDO_USER to reach the desktop session)
    sudo -u $SUDO_USER DISPLAY=:0 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u $SUDO_USER)/bus notify-send -u critical "USB Security Lock" "USB Blocked! Antivirus is still booting. Please wait 60 seconds, then re-plug the drive."
fi
INNER_EOF

sudo chmod +x /usr/local/bin/usb_boot_guard.sh

sudo tee /etc/udev/rules.d/99-usb-boot-guard.rules > /dev/null << 'INNER_EOF'
ACTION=="add", SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", RUN+="/usr/local/bin/usb_boot_guard.sh"
INNER_EOF

sudo udevadm control --reload-rules
echo "Done! The USB hardware ports are now locked for the first 60 seconds of boot."
