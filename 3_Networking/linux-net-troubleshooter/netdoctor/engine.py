"""
Network Troubleshooter & Repair Engine
Performs 12-layer deep diagnostics and safe network repairs across modern Linux distros.
Integrates with NetworkManager, systemd-resolved, iproute2, kernel drivers, and socket probes.
"""

import os
import re
import socket
import subprocess
import urllib.request
import time
from typing import Dict, List, Tuple, Optional, Any


class DiagnosticResult:
    def __init__(self, step_id: str, title: str, status: str, message: str, details: str = "", fix_action: Optional[str] = None):
        self.step_id = step_id
        self.title = title
        self.status = status  # 'PASS', 'FAIL', 'WARN'
        self.message = message
        self.details = details
        self.fix_action = fix_action

    def to_dict(self) -> Dict[str, Any]:
        return {
            "step_id": self.step_id,
            "title": self.title,
            "status": self.status,
            "message": self.message,
            "details": self.details,
            "fix_action": self.fix_action
        }


class NetworkEngine:
    def __init__(self):
        pass

    def run_cmd(self, cmd: List[str], timeout: int = 4) -> Tuple[int, str, str]:
        """Safely executes a shell command with a timeout."""
        try:
            p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout)
            return p.returncode, p.stdout.strip(), p.stderr.strip()
        except subprocess.TimeoutExpired:
            return -1, "", "Command timed out"
        except FileNotFoundError:
            return 127, "", f"Binary '{cmd[0]}' not found"
        except Exception as e:
            return 1, "", str(e)

    # -------------------------------------------------------------
    # 1. HARDWARE & KERNEL DRIVER DETECTION (PCI & USB)
    # -------------------------------------------------------------
    def check_hardware_drivers(self) -> DiagnosticResult:
        """Checks if physical network cards exist and have kernel drivers bound."""
        detected_cards = []
        missing_driver_cards = []

        # 1. Scan PCI devices (Intel, Realtek, Broadcom, Atheros, etc.)
        rc, out, _ = self.run_cmd(["lspci", "-nnk"])
        if rc == 0:
            devices = []
            current_dev = []
            for line in out.splitlines():
                if re.match(r'^[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F]', line):
                    if current_dev:
                        devices.append(current_dev)
                    current_dev = [line]
                elif current_dev:
                    current_dev.append(line)
            if current_dev:
                devices.append(current_dev)

            for dev_lines in devices:
                first_line = dev_lines[0]
                first_line_l = first_line.lower()
                if any(k in first_line_l for k in ["ethernet controller", "network controller", "wireless"]):
                    m = re.match(r'^[0-9a-fA-F:.]+\s+([^:]+):\s+(.*)', first_line)
                    dev_model = m.group(2).strip() if m else first_line
                    driver = None
                    for l in dev_lines:
                        if "kernel driver in use:" in l.lower():
                            driver = l.split(":")[-1].strip()

                    if driver:
                        detected_cards.append(f"{dev_model} [Driver: {driver}]")
                    else:
                        missing_driver_cards.append(dev_model)

        # 2. Scan USB devices (Wi-Fi and Ethernet dongles)
        rc_usb, out_usb, _ = self.run_cmd(["lsusb"])
        if rc_usb == 0:
            for line in out_usb.splitlines():
                line_l = line.lower()
                if any(k in line_l for k in ["wireless", "802.11", "ethernet", "realtek", "mediatek", "ralink"]) and "hub" not in line_l:
                    detected_cards.append(f"USB: {line}")

        if missing_driver_cards:
            return DiagnosticResult(
                "hw_driver", "Hardware Controller & Driver", "FAIL",
                f"Missing Driver! Hardware detected with no driver loaded: {missing_driver_cards[0]}",
                "\n".join(missing_driver_cards),
                fix_action="open_driver_manager"
            )

        if detected_cards:
            summary = ", ".join([c.split(' [')[0].split(' (')[0] for c in detected_cards])
            return DiagnosticResult(
                "hw_driver", "Hardware Controller & Driver", "PASS",
                f"Hardware verified: {summary}",
                "\n".join(detected_cards)
            )

        return DiagnosticResult(
            "hw_driver", "Hardware Controller & Driver", "WARN",
            "No physical network hardware identified on PCI or USB bus (Virtual machine or container environment)",
            out
        )

    # -------------------------------------------------------------
    # 2. WIRELESS RADIO (AIRPLANE MODE)
    # -------------------------------------------------------------
    def check_rfkill(self) -> DiagnosticResult:
        """Check if Wi-Fi or Bluetooth is soft or hard blocked."""
        rc, out, _ = self.run_cmd(["rfkill", "list"])
        if rc != 0:
            return DiagnosticResult("rfkill", "Wireless Radio State", "PASS", "Radio check skipped (no rfkill device)", "")
        
        blocked = []
        for line in out.splitlines():
            line_l = line.lower()
            if "soft blocked: yes" in line_l or "hard blocked: yes" in line_l:
                blocked.append(line.strip())

        if blocked:
            return DiagnosticResult(
                "rfkill", "Wireless Radio State", "FAIL",
                "Wireless hardware or software is disabled (Airplane mode active)",
                "\n".join(blocked),
                fix_action="unblock_rfkill"
            )
        return DiagnosticResult("rfkill", "Wireless Radio State", "PASS", "Wireless radios are active and unblocked", out)

    # -------------------------------------------------------------
    # 3. WI-FI AGGRESSIVE POWER SAVING (DROPOUT BUG)
    # -------------------------------------------------------------
    def check_wifi_powersave(self) -> DiagnosticResult:
        """Detects if Wi-Fi power saving is enabled (causes random disconnects on Linux)."""
        # Check NetworkManager config
        rc, out, _ = self.run_cmd(["cat", "/etc/NetworkManager/conf.d/default-wifi-powersave-on.conf"])
        is_aggressive = "wifi.powersave = 3" in out or "wifi.powersave=3" in out

        # Check disable override file
        rc2, out2, _ = self.run_cmd(["cat", "/etc/NetworkManager/conf.d/disable-wifi-powersave.conf"])
        if "wifi.powersave = 2" in out2 or "wifi.powersave=2" in out2:
            return DiagnosticResult(
                "wifi_powersave", "Wi-Fi Power Saving", "PASS",
                "Wi-Fi aggressive power-saving is disabled (Stable connection profile)",
                out2
            )

        if is_aggressive:
            return DiagnosticResult(
                "wifi_powersave", "Wi-Fi Power Saving", "WARN",
                "Wi-Fi aggressive power saving is ACTIVE (known cause of random 5-10 minute drops on Linux)",
                out,
                fix_action="disable_wifi_powersave"
            )

        return DiagnosticResult(
            "wifi_powersave", "Wi-Fi Power Saving", "PASS",
            "Wi-Fi power saving configuration is normal",
            ""
        )

    # -------------------------------------------------------------
    # 4. NETWORK SERVICE DAEMONS
    # -------------------------------------------------------------
    def check_network_service(self) -> DiagnosticResult:
        """Check if NetworkManager or systemd-networkd is alive."""
        rc_nm, out_nm, _ = self.run_cmd(["systemctl", "is-active", "NetworkManager"])
        rc_netd, out_netd, _ = self.run_cmd(["systemctl", "is-active", "systemd-networkd"])

        if out_nm == "active":
            return DiagnosticResult("service", "Network Service Daemon", "PASS", "NetworkManager service is active and running", f"Status: {out_nm}")
        elif out_netd == "active":
            return DiagnosticResult("service", "Network Service Daemon", "PASS", "systemd-networkd service is active and running", f"Status: {out_netd}")
        else:
            return DiagnosticResult(
                "service", "Network Service Daemon", "FAIL",
                "Network management service is stopped or inactive",
                f"NetworkManager: {out_nm} | systemd-networkd: {out_netd}",
                fix_action="restart_network_service"
            )

    # -------------------------------------------------------------
    # 5. PHYSICAL / WIRELESS LINK & CARRIER
    # -------------------------------------------------------------
    def get_interfaces(self) -> List[Dict[str, Any]]:
        interfaces = []
        rc, out, _ = self.run_cmd(["ip", "-o", "link", "show"])
        if rc != 0:
            return interfaces

        for line in out.splitlines():
            m = re.match(r'^\d+:\s+([^:@]+)[^:]*:\s+<([^>]+)>\s+.*state\s+(\w+)', line)
            if m:
                iface = m.group(1).strip()
                flags = m.group(2).split(',')
                state = m.group(3).strip()
                is_up = "UP" in flags or state == "UP"
                is_carrier = "LOWER_UP" in flags
                interfaces.append({
                    "name": iface,
                    "state": state,
                    "up": is_up,
                    "carrier": is_carrier,
                    "is_loopback": iface == "lo"
                })
        return interfaces

    def check_physical_link(self) -> DiagnosticResult:
        ifaces = self.get_interfaces()
        non_lo = [i for i in ifaces if not i["is_loopback"]]
        if not non_lo:
            return DiagnosticResult("link", "Network Interface & Link", "FAIL", "No physical or wireless network adapters detected on system", "", fix_action="open_driver_manager")

        active_ifaces = [i["name"] for i in non_lo if i["carrier"] or i["up"]]
        if active_ifaces:
            return DiagnosticResult("link", "Network Interface & Link", "PASS", f"Active link detected on: {', '.join(active_ifaces)}", str(non_lo))
        
        down_ifaces = [i["name"] for i in non_lo]
        return DiagnosticResult(
            "link", "Network Interface & Link", "FAIL",
            f"No carrier detected on adapters ({', '.join(down_ifaces)}). Cable unplugged or Wi-Fi disconnected.",
            str(non_lo),
            fix_action="quick_reset"
        )

    # -------------------------------------------------------------
    # 6. IP ADDRESS & DHCP
    # -------------------------------------------------------------
    def get_ip_addresses(self) -> List[Dict[str, str]]:
        ips = []
        rc, out, _ = self.run_cmd(["ip", "-o", "-4", "addr", "show"])
        if rc == 0:
            for line in out.splitlines():
                parts = line.split()
                if len(parts) >= 4 and parts[1] != "lo":
                    iface = parts[1]
                    cidr = parts[3]
                    ip = cidr.split('/')[0]
                    ips.append({"iface": iface, "ip": ip, "cidr": cidr})
        return ips

    def check_ip_assignment(self) -> DiagnosticResult:
        ips = self.get_ip_addresses()
        if not ips:
            return DiagnosticResult(
                "ip", "IP Address Assignment", "FAIL",
                "No IPv4 address assigned by DHCP or static configuration",
                "No active leases found",
                fix_action="renew_dhcp"
            )

        apipa = [item for item in ips if item["ip"].startswith("169.254.")]
        if apipa and len(apipa) == len(ips):
            return DiagnosticResult(
                "ip", "IP Address Assignment", "FAIL",
                "DHCP Failed: Adapter received self-assigned APIPA address (169.254.x.x)",
                f"Addresses: {apipa}",
                fix_action="renew_dhcp"
            )

        valid = [f"{item['iface']}: {item['cidr']}" for item in ips if not item["ip"].startswith("169.254.")]
        return DiagnosticResult("ip", "IP Address Assignment", "PASS", f"Valid IP assigned: {', '.join(valid)}", str(ips))

    # -------------------------------------------------------------
    # 7. DEFAULT GATEWAY & LOCAL ROUTING
    # -------------------------------------------------------------
    def get_default_gateway(self) -> Optional[str]:
        rc, out, _ = self.run_cmd(["ip", "route", "show", "default"])
        if rc == 0 and out:
            m = re.search(r'default\s+via\s+([0-9a-fA-F:.]+)', out)
            if m:
                return m.group(1)
        return None

    def check_gateway(self) -> DiagnosticResult:
        gw = self.get_default_gateway()
        if not gw:
            return DiagnosticResult(
                "gateway", "Default Gateway & Routing", "FAIL",
                "No default gateway found in routing table. Traffic has no route out of local network.",
                "",
                fix_action="quick_reset"
            )

        rc, out, _ = self.run_cmd(["ping", "-c", "2", "-W", "2", gw])
        if rc == 0:
            return DiagnosticResult("gateway", "Default Gateway & Routing", "PASS", f"Default gateway ({gw}) is reachable and responding", out)
        
        rc_arp, out_arp, _ = self.run_cmd(["ip", "neigh", "show", gw])
        if any(k in out_arp for k in ["REACHABLE", "DELAY", "STALE"]):
            return DiagnosticResult("gateway", "Default Gateway & Routing", "PASS", f"Gateway ({gw}) ARP confirmed active (ICMP ping blocked by router)", out_arp)

        return DiagnosticResult(
            "gateway", "Default Gateway & Routing", "FAIL",
            f"Cannot reach default gateway ({gw}). Local router is unresponsive or disconnected.",
            out + "\n" + out_arp,
            fix_action="quick_reset"
        )

    # -------------------------------------------------------------
    # 8. STALE VPN & DEAD VIRTUAL TUNNEL ROUTES
    # -------------------------------------------------------------
    def check_vpn_routes(self) -> DiagnosticResult:
        """Detects if dead VPN tunnels (tun, tap, wg, tailscale) are hijacking the default route."""
        rc, out, _ = self.run_cmd(["ip", "route", "show"])
        if rc != 0:
            return DiagnosticResult("vpn_routes", "VPN & Virtual Tunnel Routes", "PASS", "Routing check passed", "")

        dead_vpn_traps = []
        ifaces = {i["name"]: i for i in self.get_interfaces()}

        for line in out.splitlines():
            # Check default route or /0 routes pointing to tunnel interfaces
            if "default via" in line or "0.0.0.0/1" in line or "128.0.0.0/1" in line:
                for vpn_prefix in ["tun", "tap", "wg", "tailscale", "ppp", "nordlynx"]:
                    m = re.search(rf'dev\s+({vpn_prefix}\w*)', line)
                    if m:
                        dev = m.group(1)
                        dev_info = ifaces.get(dev)
                        if not dev_info or not dev_info.get("carrier", False) or not dev_info.get("up", False):
                            dead_vpn_traps.append(f"{dev} ({line.strip()})")

        if dead_vpn_traps:
            return DiagnosticResult(
                "vpn_routes", "VPN & Virtual Tunnel Routes", "FAIL",
                f"Stale VPN Route Trap! Inactive tunnel is capturing default internet traffic: {dead_vpn_traps[0].split()[0]}",
                "\n".join(dead_vpn_traps),
                fix_action="clear_stale_vpn_routes"
            )

        return DiagnosticResult(
            "vpn_routes", "VPN & Virtual Tunnel Routes", "PASS",
            "No stale or dead VPN routes hijacking network traffic",
            ""
        )

    # -------------------------------------------------------------
    # 9. RAW INTERNET IP REACHABILITY (NO DNS)
    # -------------------------------------------------------------
    def check_internet_ip(self) -> DiagnosticResult:
        targets = ["1.1.1.1", "8.8.8.8"]
        for target in targets:
            rc, out, _ = self.run_cmd(["ping", "-c", "2", "-W", "2", target])
            if rc == 0:
                return DiagnosticResult("internet_ip", "Internet IP Reachability (No DNS)", "PASS", f"Internet route confirmed active (replied by {target})", out)
            
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(2.0)
                s.connect((target, 53))
                s.close()
                return DiagnosticResult("internet_ip", "Internet IP Reachability (No DNS)", "PASS", f"Direct TCP handshake confirmed to {target}:53", "")
            except Exception:
                pass

        return DiagnosticResult(
            "internet_ip", "Internet IP Reachability (No DNS)", "FAIL",
            "Cannot reach public internet IP addresses (1.1.1.1, 8.8.8.8). ISP or WAN link down.",
            "Packets are being dropped at or past the gateway.",
            fix_action="quick_reset"
        )

    # -------------------------------------------------------------
    # 10. RESOLV.CONF RESOLVER INTEGRITY
    # -------------------------------------------------------------
    def check_resolv_symlink(self) -> DiagnosticResult:
        """Verifies if /etc/resolv.conf is properly configured and synchronized with systemd-resolved."""
        if not os.path.exists("/etc/resolv.conf"):
            return DiagnosticResult(
                "resolv_conf", "DNS Resolver Integrity (/etc/resolv.conf)", "FAIL",
                "/etc/resolv.conf does not exist! DNS lookups will fail system-wide.",
                "",
                fix_action="repair_resolv_conf"
            )

        # Check if systemd-resolved is active
        rc, out, _ = self.run_cmd(["systemctl", "is-active", "systemd-resolved"])
        if out == "active":
            is_link = os.path.islink("/etc/resolv.conf")
            if is_link:
                target = os.readlink("/etc/resolv.conf")
                if "stub-resolv.conf" in target or "resolv.conf" in target:
                    return DiagnosticResult("resolv_conf", "DNS Resolver Integrity (/etc/resolv.conf)", "PASS", "Resolver configuration is healthy and synchronized with systemd-resolved", target)

            # Check if it has 127.0.0.53
            try:
                with open("/etc/resolv.conf", "r") as f:
                    content = f.read()
                    if "127.0.0.53" in content or "nameserver" in content:
                        return DiagnosticResult("resolv_conf", "DNS Resolver Integrity (/etc/resolv.conf)", "PASS", "Active nameserver found in /etc/resolv.conf", content)
            except Exception:
                pass

            return DiagnosticResult(
                "resolv_conf", "DNS Resolver Integrity (/etc/resolv.conf)", "WARN",
                "/etc/resolv.conf is desynchronized from systemd-resolved",
                "Static or unmanaged file detected while systemd-resolved is active",
                fix_action="repair_resolv_conf"
            )

        return DiagnosticResult("resolv_conf", "DNS Resolver Integrity (/etc/resolv.conf)", "PASS", "DNS configuration file present", "")

    # -------------------------------------------------------------
    # 11. DNS NAME RESOLUTION
    # -------------------------------------------------------------
    def check_dns(self) -> DiagnosticResult:
        domains = ["google.com", "cloudflare.com", "kernel.org"]
        resolved = []
        for domain in domains:
            try:
                ip = socket.gethostbyname(domain)
                resolved.append(f"{domain} -> {ip}")
            except Exception:
                pass

        if len(resolved) >= 1:
            return DiagnosticResult("dns", "DNS Name Resolution", "PASS", f"DNS working normally ({resolved[0]})", "\n".join(resolved))

        rc_res, out_res, _ = self.run_cmd(["resolvectl", "status"])
        if rc_res != 0:
            rc_res, out_res, _ = self.run_cmd(["cat", "/etc/resolv.conf"])

        return DiagnosticResult(
            "dns", "DNS Name Resolution", "FAIL",
            "DNS lookup failed. Websites cannot be resolved to IP addresses.",
            out_res,
            fix_action="set_public_dns"
        )

    # -------------------------------------------------------------
    # 12. WEB & CAPTIVE PORTAL PROBE
    # -------------------------------------------------------------
    def check_http_connectivity(self) -> DiagnosticResult:
        urls = [
            ("http://connectivity-check.ubuntu.com", 204),
            ("http://www.google.com/generate_204", 204),
            ("http://www.msftconnecttest.com/connecttest.txt", 200)
        ]
        
        for url, expected_code in urls:
            try:
                req = urllib.request.Request(url, headers={'User-Agent': 'NetworkDiagnostic/1.0'})
                with urllib.request.urlopen(req, timeout=3) as resp:
                    status = resp.getcode()
                    if status == expected_code:
                        return DiagnosticResult("http", "Web & Captive Portal Check", "PASS", "Full unrestricted Web / HTTP access confirmed", f"HTTP {status} from {url}")
                    elif status in (301, 302, 307):
                        redirect_url = resp.getheader('Location', '')
                        return DiagnosticResult(
                            "http", "Web & Captive Portal Check", "WARN",
                            f"Captive Portal detected! Login required at: {redirect_url}",
                            f"Redirected from {url} to {redirect_url}",
                            fix_action="open_browser"
                        )
            except Exception:
                continue

        return DiagnosticResult(
            "http", "Web & Captive Portal Check", "FAIL",
            "HTTP web probe failed or connection timed out.",
            "",
            fix_action="quick_reset"
        )

    # -------------------------------------------------------------
    # RUN ALL 12 DIAGNOSTIC CHECKS
    # -------------------------------------------------------------
    def run_full_diagnostics(self) -> List[DiagnosticResult]:
        checks = [
            self.check_hardware_drivers,
            self.check_rfkill,
            self.check_wifi_powersave,
            self.check_network_service,
            self.check_physical_link,
            self.check_ip_assignment,
            self.check_gateway,
            self.check_vpn_routes,
            self.check_internet_ip,
            self.check_resolv_symlink,
            self.check_dns,
            self.check_http_connectivity
        ]

        results = []
        for fn in checks:
            results.append(fn())
        return results

    # -------------------------------------------------------------
    # SAFE AUTOMATED REPAIRS & FIXES
    # -------------------------------------------------------------
    def flush_dns(self) -> Tuple[bool, str]:
        cmds = [
            ["resolvectl", "flush-caches"],
            ["systemd-resolve", "--flush-caches"]
        ]
        logs = []
        success = False
        for cmd in cmds:
            rc, out, err = self.run_cmd(cmd)
            if rc == 0:
                logs.append(f"✔ Executed: {' '.join(cmd)}")
                success = True
                break
            else:
                logs.append(f"ℹ {' '.join(cmd)}: {err or out}")
        return success, "\n".join(logs)

    def flush_arp(self) -> Tuple[bool, str]:
        rc, out, err = self.run_cmd(["ip", "neigh", "flush", "all"])
        if rc == 0:
            return True, "✔ Flushed ARP neighbor table"
        return False, f"✘ Failed to flush ARP: {err or out}"

    def unblock_rfkill(self) -> Tuple[bool, str]:
        rc, out, err = self.run_cmd(["rfkill", "unblock", "all"])
        if rc == 0:
            return True, "✔ Unblocked all wireless radios"
        return False, f"✘ Could not unblock rfkill: {err or out}"

    def disable_wifi_powersave(self) -> Tuple[bool, str]:
        """Creates NetworkManager config to permanently disable aggressive Wi-Fi powersave."""
        config_content = "[connection]\nwifi.powersave = 2\n"
        script = f'mkdir -p /etc/NetworkManager/conf.d && printf "{config_content}" > /etc/NetworkManager/conf.d/disable-wifi-powersave.conf && systemctl reload NetworkManager 2>/dev/null'
        
        # NOTE: script is a static string built from safe constants, not from user input.
        cmd = ["bash", "-c", script] if os.geteuid() == 0 else ["pkexec", "bash", "-c", script]
        rc, out, err = self.run_cmd(cmd)
        if rc == 0:
            return True, "✔ Wi-Fi power saving disabled (wifi.powersave = 2). Wi-Fi stability restored."
        return False, f"✘ Failed to disable Wi-Fi powersave: {err or out}"

    def clear_stale_vpn_routes(self) -> Tuple[bool, str]:
        """Removes dead tunnel interfaces and restores physical default routes."""
        script = 'for dev in $(ip link show | grep -oE "(tun|tap|wg|tailscale|nordlynx)[0-9a-zA-Z]*"); do ip link set $dev down 2>/dev/null; done; nmcli networking off && nmcli networking on'
        # NOTE: script is a static string built from safe constants, not from user input.
        cmd = ["bash", "-c", script] if os.geteuid() == 0 else ["pkexec", "bash", "-c", script]
        rc, out, err = self.run_cmd(cmd)
        if rc == 0:
            return True, "✔ Cleared stale VPN tunnel routes and restored physical default route."
        return False, f"✘ Failed to clear VPN routes: {err or out}"

    def repair_resolv_conf(self) -> Tuple[bool, str]:
        """Re-links /etc/resolv.conf to systemd-resolved and restarts resolver."""
        script = 'ln -sf /run/systemd/resolve/stub-resolv.conf /etc/resolv.conf && systemctl restart systemd-resolved'
        # NOTE: script is a static string built from safe constants, not from user input.
        cmd = ["bash", "-c", script] if os.geteuid() == 0 else ["pkexec", "bash", "-c", script]
        rc, out, err = self.run_cmd(cmd)
        if rc == 0:
            return True, "✔ Repaired /etc/resolv.conf symlink and restarted systemd-resolved."
        return False, f"✘ Failed to repair resolv.conf: {err or out}"

    def open_driver_manager(self) -> Tuple[bool, str]:
        """Launches the system's native hardware driver GUI."""
        guis = [
            "software-properties-gtk --open-tab=4",  # Ubuntu Additional Drivers
            "driver-manager",                       # Linux Mint Driver Manager
            "hardinfo",
            "gnome-control-center info-overview"
        ]
        for gui in guis:
            parts = gui.split()
            rc, _, _ = self.run_cmd(["which", parts[0]])
            if rc == 0:
                subprocess.Popen(parts)
                return True, f"✔ Opened system Driver Manager: {parts[0]}"
        return False, "Driver Manager application not found. Use system settings to install proprietary drivers."

    def quick_reset(self) -> Tuple[bool, str]:
        logs = []
        _, msg_dns = self.flush_dns()
        logs.append(msg_dns)

        _, msg_arp = self.flush_arp()
        logs.append(msg_arp)

        self.run_cmd(["nmcli", "networking", "off"])
        time.sleep(0.5)
        rc2, out2, err2 = self.run_cmd(["nmcli", "networking", "on"])
        if rc2 == 0:
            logs.append("✔ Cycled NetworkManager networking subsystem (Off -> On)")
            return True, "\n".join(logs)
        else:
            logs.append(f"ℹ nmcli cycle returned: {err2 or out2}")
            return False, "\n".join(logs)

    def full_stack_reset(self) -> Tuple[bool, str]:
        logs = []
        _, msg_dns = self.flush_dns()
        logs.append(msg_dns)
        _, msg_arp = self.flush_arp()
        logs.append(msg_arp)

        script = "systemctl restart NetworkManager 2>/dev/null; systemctl restart systemd-resolved 2>/dev/null; resolvectl flush-caches 2>/dev/null"
        # NOTE: script is a static string built from safe constants, not from user input.
        cmd = ["bash", "-c", script] if os.geteuid() == 0 else ["pkexec", "bash", "-c", script]

        rc, out, err = self.run_cmd(cmd)
        if rc == 0:
            logs.append("✔ Successfully restarted NetworkManager & systemd-resolved daemons")
            return True, "\n".join(logs)
        else:
            logs.append(f"✘ Error restarting services: {err or out}")
            return False, "\n".join(logs)

    @staticmethod
    def probe_dns_server(ip: str, timeout: float = 1.0) -> bool:
        """Sends a standard DNS wire query for google.com to test if the DNS server is reachable and responding."""
        query = (
            b"\xaa\xbb"  # ID
            b"\x01\x00"  # Flags: Standard query, recursion desired
            b"\x00\x01"  # QDCOUNT: 1 question
            b"\x00\x00"  # ANCOUNT: 0
            b"\x00\x00"  # NSCOUNT: 0
            b"\x00\x00"  # ARCOUNT: 0
            b"\x06google\x03com\x00"  # QNAME: google.com
            b"\x00\x01"  # QTYPE: A
            b"\x00\x01"  # QCLASS: IN
        )
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(timeout)
        try:
            sock.sendto(query, (ip, 53))
            data, _ = sock.recvfrom(512)
            return len(data) > 12 and data[:2] == b"\xaa\xbb"
        except Exception:
            return False
        finally:
            sock.close()

    def set_dns_servers(self, connection_name: str, dns_list: List[str]) -> Tuple[bool, str]:
        if not connection_name:
            rc, out, _ = self.run_cmd(["nmcli", "-t", "-f", "NAME,DEVICE", "connection", "show", "--active"])
            if rc == 0 and out:
                connection_name = out.splitlines()[0].split(':')[0]
            else:
                return False, "No active connection profile found to modify."

        if not dns_list:
            cmd = ["nmcli", "connection", "modify", connection_name, "ipv4.ignore-auto-dns", "no", "ipv4.dns", ""]
            msg_suffix = ""
        else:
            # Pre-flight check: probe servers so dead/ISP-blocked IPs (e.g. 1.1.1.1 on BSNL) don't hang network
            responsive = []
            unresponsive = []
            for ip in dns_list:
                if self.probe_dns_server(ip, timeout=1.0):
                    responsive.append(ip)
                else:
                    unresponsive.append(ip)

            if not responsive and unresponsive:
                return False, (
                    f"✘ Aborted: None of the specified DNS servers ({', '.join(unresponsive)}) responded on UDP port 53. "
                    "Your ISP or router may be blocking these IPs. Network was kept untouched to prevent downtime."
                )

            # Order responsive IPs first, followed by unverified/fallback IPs
            final_list = responsive + unresponsive
            dns_str = " ".join(final_list)
            cmd = ["nmcli", "connection", "modify", connection_name, "ipv4.ignore-auto-dns", "yes", "ipv4.dns", dns_str]

            if unresponsive:
                msg_suffix = f"\n⚠ Note: {', '.join(unresponsive)} did not respond (likely blocked by ISP); placed working {', '.join(responsive)} first."
            else:
                msg_suffix = ""

        rc, out, err = self.run_cmd(cmd)
        if rc == 0:
            self.run_cmd(["nmcli", "connection", "up", connection_name])
            self.flush_dns()
            dns_label = ", ".join(dns_list) if dns_list else "Automatic (DHCP)"
            return True, f"✔ Successfully set DNS to: {dns_label} for '{connection_name}'{msg_suffix}"
        else:
            return False, f"✘ Failed to set DNS: {err or out}"

    def renew_dhcp(self) -> Tuple[bool, str]:
        rc, out, _ = self.run_cmd(["nmcli", "-t", "-f", "DEVICE,STATE", "device", "status"])
        if rc == 0 and out:
            for line in out.splitlines():
                parts = line.split(':')
                if len(parts) >= 2 and parts[1] == "connected":
                    dev = parts[0]
                    rc_re, _, _ = self.run_cmd(["nmcli", "device", "reapply", dev])
                    if rc_re == 0:
                        return True, f"✔ Renewed DHCP configuration on adapter '{dev}'"
        return self.quick_reset()
