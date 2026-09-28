#!/usr/bin/env python3
import subprocess
import re

def show_notification(virus, filename):
    subprocess.run([
        "notify-send", "-u", "normal", "-t", "10000", "-a", "Device Security", "-i", "security-high",
        "🚨 Threat Blocked & Quarantined!",
        f"Malware: {virus}\nFile: {filename}"
    ])

def main():
    # Tail the system journal specifically for our antivirus alerts
    process = subprocess.Popen(
        ["journalctl", "-f", "-t", "linux-defender", "-o", "cat"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )
    
    # ALERT: Threat 'Eicar-Signature' detected in '/opt/Downloads/eicar.com'
    alert_regex = re.compile(r"ALERT: Threat '(.*?)' detected in '(.*?)'")

    print("Native Desktop Notifier listening for threats...")
    for line in iter(process.stdout.readline, ''):
        match = alert_regex.search(line)
        if match:
            virus = match.group(1)
            filename = match.group(2)
            show_notification(virus, filename)

if __name__ == "__main__":
    main()
