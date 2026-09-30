# Linux Network Troubleshooter & Reset (NetDoctor)

A native, modern graphical troubleshooting and network reset application for Linux.
Built directly on top of battle-tested Linux system engines: NetworkManager, systemd-resolved, iproute2, and socket probes.

---

## Features

### 1. 🔍 Automated 8-Layer Diagnostics
Runs a step-by-step diagnostic sequence through the entire networking stack:
1. **Wireless Radio State:** Detects software/hardware `rfkill` blocks (Airplane Mode).
2. **Network Service Daemon:** Checks if NetworkManager or systemd-networkd is running.
3. **Interface & Link Carrier:** Checks physical cable carrier and Wi-Fi link status.
4. **IP Configuration:** Detects DHCP lease status and self-assigned APIPA (`169.254.x.x`) failures.
5. **Default Gateway & Routing:** Verifies default route and tests gateway reachability via ICMP/ARP.
6. **Raw Internet Reachability:** Probes public IPs (1.1.1.1 / 8.8.8.8) to isolate routing from DNS.
7. **DNS Name Resolution:** Resolves domains through the system stub resolver.
8. **Web & Captive Portal Detection:** Probes HTTP status 204 endpoints to detect coffee shop / hotel captive portals.

### 2. 🔄 1-Click Fix & Reset (Windows Network Reset Equivalent)
- **⚡ Quick Soft Reset:** Clears DNS cache, flushes ARP table, and cycles NetworkManager cleanly.
- **🔄 Complete Network Reset:** Restarts NetworkManager & systemd-resolved daemons and rebuilds socket states.
- **🔓 Unblock Wireless:** Unblocks all rfkill software/hardware switches.
- **🌐 Renew DHCP Lease:** Requests a fresh IP address and routing information from the router.

### 3. 🌐 Fast DNS Profile Changer
- **Cloudflare DNS:** `1.1.1.1, 1.0.0.1` (Fast & private)
- **Google Public DNS:** `8.8.8.8, 8.8.4.4` (High reliability)
- **Quad9 Security DNS:** `9.9.9.9, 149.112.112.112` (Blocks malware & phishing)
- **Automatic (DHCP):** Restores default router DNS.
- Quick shortcut to open system adapter properties (`nm-connection-editor`).

### 4. 📊 Interface Inspector & Continuous Ping Tester
- Real-time display of all active network adapters, IPs, and default gateway.
- Interactive live ping and packet loss tester with real-time output.

---

## How to Launch

- **From Application Menu / Taskbar:** Search for **"Network Troubleshooter & Reset"**
- **From Terminal or KRunner (Alt+F2):** Run `linux-net-troubleshooter`
- **Headless / CLI Mode:** Run `linux-net-troubleshooter --cli`
