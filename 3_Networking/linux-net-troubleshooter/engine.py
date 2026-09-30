"""
Network Troubleshooter & Repair Engine
Performs deep diagnostics and safe network repairs across modern Linux distros.
Integrates with NetworkManager, systemd-resolved, iproute2, and socket-level probes.
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
        self.status = status  # 'PASS', 'FAIL', 'WARN', 'RUNNING', 'PENDING'
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
    # DIAGNOSTIC CHECKS
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

    def get_interfaces(self) -> List[Dict[str, Any]]:
        """Parses network interfaces and their states."""
        interfaces = []
        rc, out, _ = self.run_cmd(["ip", "-o", "link", "show"])
        if rc != 0:
            return interfaces

        for line in out.splitlines():
            # Format: 1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000\    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
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
        """Checks if at least one non-loopback network interface has link / carrier."""
        ifaces = self.get_interfaces()
        non_lo = [i for i in ifaces if not i["is_loopback"]]
        if not non_lo:
            return DiagnosticResult("link", "Network Interface & Link", "FAIL", "No physical or wireless network adapters detected on system", "", fix_action="check_drivers")

        active_ifaces = [i["name"] for i in non_lo if i["carrier"] or i["up"]]
        if active_ifaces:
            return DiagnosticResult("link", "Network Interface & Link", "PASS", f"Active link detected on: {', '.join(active_ifaces)}", str(non_lo))
        
        down_ifaces = [i["name"] for i in non_lo]
        return DiagnosticResult(
            "link", "Network Interface & Link", "FAIL",
            f"No carrier detected on adapters ({', '.join(down_ifaces)}). Cable unplugged or Wi-Fi disconnected.",
            str(non_lo),
            fix_action="bounce_interfaces"
        )

    def get_ip_addresses(self) -> List[Dict[str, str]]:
        """Gets assigned IPv4 and IPv6 addresses."""
        ips = []
        rc, out, _ = self.run_cmd(["ip", "-o", "-4", "addr", "show"])
        if rc == 0:
            for line in out.splitlines():
                # 2: eth0    inet 192.168.1.100/24 brd 192.168.1.255 scope global eth0
                parts = line.split()
                if len(parts) >= 4 and parts[1] != "lo":
                    iface = parts[1]
                    cidr = parts[3]
                    ip = cidr.split('/')[0]
                    ips.append({"iface": iface, "ip": ip, "cidr": cidr})
        return ips

    def check_ip_assignment(self) -> DiagnosticResult:
        """Verifies if an interface has a valid IP address (not APIPA 169.254.x.x)."""
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

    def get_default_gateway(self) -> Optional[str]:
        """Finds the default gateway IP address."""
        rc, out, _ = self.run_cmd(["ip", "route", "show", "default"])
        if rc == 0 and out:
            # default via 192.168.1.1 dev wlan0 proto dhcp metric 600
            m = re.search(r'default\s+via\s+([0-9a-fA-F:.]+)', out)
            if m:
                return m.group(1)
        return None

    def check_gateway(self) -> DiagnosticResult:
        """Tests whether the default route exists and gateway responds to ping/ARP."""
        gw = self.get_default_gateway()
        if not gw:
            return DiagnosticResult(
                "gateway", "Default Gateway & Routing", "FAIL",
                "No default gateway found in routing table. Traffic has no route out of local network.",
                "",
                fix_action="quick_reset"
            )

        # Ping gateway with timeout 2s
        rc, out, _ = self.run_cmd(["ping", "-c", "2", "-W", "2", gw])
        if rc == 0:
            return DiagnosticResult("gateway", "Default Gateway & Routing", "PASS", f"Default gateway ({gw}) is reachable and responding", out)
        
        # In case gateway blocks ICMP, check neighbor table (ARP)
        rc_arp, out_arp, _ = self.run_cmd(["ip", "neigh", "show", gw])
        if "REACHABLE" in out_arp or "DELAY" in out_arp or "STALE" in out_arp:
            return DiagnosticResult("gateway", "Default Gateway & Routing", "PASS", f"Gateway ({gw}) ARP confirmed active (ICMP ping blocked by router)", out_arp)

        return DiagnosticResult(
            "gateway", "Default Gateway & Routing", "FAIL",
            f"Cannot reach default gateway ({gw}). Local router is unresponsive or disconnected.",
            out + "\n" + out_arp,
            fix_action="quick_reset"
        )

    def check_internet_ip(self) -> DiagnosticResult:
        """Tests raw internet IP reachability (bypassing DNS entirely)."""
        targets = ["1.1.1.1", "8.8.8.8"]
        for target in targets:
            rc, out, _ = self.run_cmd(["ping", "-c", "2", "-W", "2", target])
            if rc == 0:
                return DiagnosticResult("internet_ip", "Internet IP Reachability (No DNS)", "PASS", f"Internet route confirmed active (replied by {target})", out)
            
            # Fallback to TCP socket connection on port 53 (DNS port) or 80
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

    def check_dns(self) -> DiagnosticResult:
        """Tests domain name resolution across multiple domains."""
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

        # Check resolver configuration
        rc_res, out_res, _ = self.run_cmd(["resolvectl", "status"])
        if rc_res != 0:
            rc_res, out_res, _ = self.run_cmd(["cat", "/etc/resolv.conf"])

        return DiagnosticResult(
            "dns", "DNS Name Resolution", "FAIL",
            "DNS lookup failed. Websites cannot be resolved to IP addresses.",
            out_res,
            fix_action="set_public_dns"
        )

    def check_http_connectivity(self) -> DiagnosticResult:
        """Checks for full HTTP web access and detects Captive Portals."""
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
                        # Captive portal redirect detected!
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

    def run_full_diagnostics(self) -> List[DiagnosticResult]:
        """Runs the entire diagnostic pipeline sequentially."""
        results = []
        checks = [
            self.check_rfkill,
            self.check_network_service,
            self.check_physical_link,
            self.check_ip_assignment,
            self.check_gateway,
            self.check_internet_ip,
            self.check_dns,
            self.check_http_connectivity
        ]

        for check_fn in checks:
            res = check_fn()
            results.append(res)
            # If critical low-level check fails completely, continue but flag
        return results

    # -------------------------------------------------------------
    # AUTOMATED REPAIRS & NETWORK RESETS
    # -------------------------------------------------------------

    def flush_dns(self) -> Tuple[bool, str]:
        """Flushes systemd-resolved and nscd DNS caches."""
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
        """Flushes neighbor / ARP table to remove stale gateway MAC entries."""
        rc, out, err = self.run_cmd(["ip", "neigh", "flush", "all"])
        if rc == 0:
            return True, "✔ Flushed ARP neighbor table"
        return False, f"✘ Failed to flush ARP: {err or out}"

    def unblock_rfkill(self) -> Tuple[bool, str]:
        """Unblocks all software switches (airplane mode)."""
        rc, out, err = self.run_cmd(["rfkill", "unblock", "all"])
        if rc == 0:
            return True, "✔ Unblocked all wireless radios"
        return False, f"✘ Could not unblock rfkill: {err or out}"

    def quick_reset(self) -> Tuple[bool, str]:
        """
        Soft reset:
        1. Flush DNS
        2. Flush ARP
        3. Toggle NetworkManager networking subsystem cleanly
        """
        logs = []
        f_dns, msg_dns = self.flush_dns()
        logs.append(msg_dns)

        f_arp, msg_arp = self.flush_arp()
        logs.append(msg_arp)

        # Toggle networking
        rc, out, err = self.run_cmd(["nmcli", "networking", "off"])
        time.sleep(0.5)
        rc2, out2, err2 = self.run_cmd(["nmcli", "networking", "on"])
        if rc2 == 0:
            logs.append("✔ Cycled NetworkManager networking subsystem (Off -> On)")
            return True, "\n".join(logs)
        else:
            logs.append(f"ℹ nmcli cycle returned: {err2 or out2}")
            return False, "\n".join(logs)

    def full_stack_reset(self) -> Tuple[bool, str]:
        """
        Hard reset (Windows Network Reset equivalent):
        Restarts NetworkManager and systemd-resolved services via pkexec if unprivileged.
        """
        logs = []
        f_dns, msg_dns = self.flush_dns()
        logs.append(msg_dns)
        f_arp, msg_arp = self.flush_arp()
        logs.append(msg_arp)

        script = "systemctl restart NetworkManager 2>/dev/null; systemctl restart systemd-resolved 2>/dev/null; resolvectl flush-caches 2>/dev/null"
        
        # Try direct or pkexec for graphical privilege elevation
        if os.geteuid() == 0:
            rc, out, err = self.run_cmd(["bash", "-c", script])
        else:
            # NOTE: script is a static constant, not user-controlled input.
            # Use pkexec so a standard graphical password dialog opens if needed
            rc, out, err = self.run_cmd(["pkexec", "bash", "-c", script])

        if rc == 0:
            logs.append("✔ Successfully restarted NetworkManager & systemd-resolved daemons")
            return True, "\n".join(logs)
        else:
            logs.append(f"✘ Error restarting services: {err or out}")
            return False, "\n".join(logs)

    def set_dns_servers(self, connection_name: str, dns_list: List[str]) -> Tuple[bool, str]:
        """Sets custom DNS servers on a connection profile or reverts to auto."""
        if not connection_name:
            # Detect active connection
            rc, out, _ = self.run_cmd(["nmcli", "-t", "-f", "NAME,DEVICE", "connection", "show", "--active"])
            if rc == 0 and out:
                connection_name = out.splitlines()[0].split(':')[0]
            else:
                return False, "No active connection profile found to modify."

        if not dns_list:
            # Revert to automatic DHCP DNS
            cmd = ["nmcli", "connection", "modify", connection_name, "ipv4.ignore-auto-dns", "no", "ipv4.dns", ""]
        else:
            dns_str = " ".join(dns_list)
            cmd = ["nmcli", "connection", "modify", connection_name, "ipv4.ignore-auto-dns", "yes", "ipv4.dns", dns_str]

        rc, out, err = self.run_cmd(cmd)
        if rc == 0:
            # Reapply connection
            self.run_cmd(["nmcli", "connection", "up", connection_name])
            self.flush_dns()
            dns_label = ", ".join(dns_list) if dns_list else "Automatic (DHCP)"
            return True, f"✔ Successfully set DNS to: {dns_label} for '{connection_name}'"
        else:
            return False, f"✘ Failed to set DNS: {err or out}"

    def renew_dhcp(self) -> Tuple[bool, str]:
        """Requests a fresh DHCP lease from the local router."""
        rc, out, _ = self.run_cmd(["nmcli", "-t", "-f", "DEVICE,STATE", "device", "status"])
        if rc == 0 and out:
            for line in out.splitlines():
                parts = line.split(':')
                if len(parts) >= 2 and parts[1] == "connected":
                    dev = parts[0]
                    rc_re, _, err_re = self.run_cmd(["nmcli", "device", "reapply", dev])
                    if rc_re == 0:
                        return True, f"✔ Renewed DHCP configuration on adapter '{dev}'"
        
        # Fallback to cycling networking
        return self.quick_reset()
