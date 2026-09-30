#!/usr/bin/env python3
"""
High-Availability Multi-Route Ethernet Overlay Daemon (127.0.0.1:8888)
----------------------------------------------------------------------
- Distributes outbound SOCKS5 / HTTP CONNECT traffic across multiple
  independent rotating circuit pools (127.0.0.1:9050, 9052, 9054, 9056).
- Continuously health-checks each route in the background.
- RESILIENT ROUTING: Tries healthy upstream circuits first. If all circuits
  are unavailable, falls back to direct Ethernet connection (with logging).
"""

import asyncio
import random
import socket
import struct
import time
import sys

LISTEN_HOST = "127.0.0.1"
LISTEN_PORT = 8888

# 4 independent SOCKS5 circuit endpoints (each isolated to a different route)
UPSTREAM_ROUTES = [
    ("127.0.0.1", 9050, "Route-Pool-Alpha"),
    ("127.0.0.1", 9052, "Route-Pool-Bravo"),
    ("127.0.0.1", 9054, "Route-Pool-Charlie"),
    ("127.0.0.1", 9056, "Route-Pool-Delta"),
]

# Track live health & latency of each route
ROUTE_HEALTH = {
    port: {"healthy": False, "latency_ms": 9999.0, "name": name}
    for _, port, name in UPSTREAM_ROUTES
}


async def check_route_health():
    """Background probe checking which SOCKS5 circuit ports are up and fast."""
    while True:
        for host, port, name in UPSTREAM_ROUTES:
            t0 = time.monotonic()
            try:
                reader, writer = await asyncio.wait_for(
                    asyncio.open_connection(host, port), timeout=1.2
                )
                writer.write(b"\x05\x01\x00")
                await writer.drain()
                resp = await asyncio.wait_for(reader.read(2), timeout=1.2)
                writer.close()
                await writer.wait_closed()
                if resp == b"\x05\x00":
                    ROUTE_HEALTH[port]["healthy"] = True
                    ROUTE_HEALTH[port]["latency_ms"] = round((time.monotonic() - t0) * 1000, 1)
                else:
                    ROUTE_HEALTH[port]["healthy"] = False
            except Exception:
                ROUTE_HEALTH[port]["healthy"] = False
                ROUTE_HEALTH[port]["latency_ms"] = 9999.0
        await asyncio.sleep(5)


def pick_healthy_routes():
    """Return healthy upstream routes in random load-balanced order."""
    healthy = [
        (host, port, name)
        for host, port, name in UPSTREAM_ROUTES
        if ROUTE_HEALTH[port]["healthy"]
    ]
    random.shuffle(healthy)
    return healthy


async def pipe_stream(reader, writer):
    try:
        while True:
            data = await reader.read(65536)
            if not data:
                break
            writer.write(data)
            await writer.drain()
    except Exception:
        pass
    finally:
        try:
            writer.close()
        except Exception:
            pass


async def socks5_connect(socks_host, socks_port, target_host, target_port):
    """Connect to target_host:target_port through a specific SOCKS5 circuit port."""
    reader, writer = await asyncio.wait_for(
        asyncio.open_connection(socks_host, socks_port), timeout=2.5
    )
    # Handshake: no-auth
    writer.write(b"\x05\x01\x00")
    await writer.drain()
    resp = await asyncio.wait_for(reader.read(2), timeout=2.0)
    if resp != b"\x05\x00":
        writer.close()
        raise ConnectionError("SOCKS5 auth failed")

    # Request CONNECT with domain name resolution on remote exit (prevents DNS leaks)
    host_bytes = target_host.encode("utf-8")
    req = b"\x05\x01\x00\x03" + struct.pack("B", len(host_bytes)) + host_bytes + struct.pack("!H", target_port)
    writer.write(req)
    await writer.drain()

    hdr = await asyncio.wait_for(reader.read(4), timeout=6.0)
    if len(hdr) < 4 or hdr[1] != 0x00:
        writer.close()
        raise ConnectionError("SOCKS5 connect rejected")

    atyp = hdr[3]
    if atyp == 1:
        await reader.read(4 + 2)
    elif atyp == 3:
        ln = (await reader.read(1))[0]
        await reader.read(ln + 2)
    elif atyp == 4:
        await reader.read(16 + 2)

    return reader, writer


ALLOW_DIRECT_FALLBACK = "--allow-direct-fallback" in sys.argv

async def open_resilient_connection(target_host, target_port):
    """
    Try healthy multi-route circuits first. If all circuits are down,
    fail closed by default to prevent traffic leaks, unless --allow-direct-fallback is passed.
    """
    for socks_host, socks_port, route_name in pick_healthy_routes():
        try:
            result = await socks5_connect(socks_host, socks_port, target_host, target_port)
            return result, route_name
        except Exception:
            continue

    if not ALLOW_DIRECT_FALLBACK:
        raise ConnectionError(
            "All SOCKS5 proxy routes unavailable. Failing closed to prevent unencrypted traffic leaks. "
            "(Run with --allow-direct-fallback to permit direct Ethernet fallback)."
        )

    # Direct Ethernet Fallback (Opt-in only)
    try:
        reader, writer = await asyncio.wait_for(
            asyncio.open_connection(target_host, target_port), timeout=5.0
        )
        return (reader, writer), "Direct-Ethernet-Fallback"
    except Exception as e:
        raise ConnectionError(f"All routes and direct fallback failed: {e}")


async def handle_client(client_reader, client_writer):
    try:
        first_byte = await asyncio.wait_for(client_reader.read(1), timeout=5.0)
        if not first_byte:
            client_writer.close()
            return

        # Case 1: Incoming SOCKS5 client
        if first_byte == b"\x05":
            nmethods = (await client_reader.read(1))[0]
            await client_reader.read(nmethods)
            client_writer.write(b"\x05\x00")
            await client_writer.drain()

            req_hdr = await client_reader.read(4)
            if len(req_hdr) < 4 or req_hdr[1] != 1:
                client_writer.close()
                return
            atyp = req_hdr[3]
            if atyp == 1:
                ip_bytes = await client_reader.read(4)
                target_host = socket.inet_ntoa(ip_bytes)
            elif atyp == 3:
                ln = (await client_reader.read(1))[0]
                target_host = (await client_reader.read(ln)).decode("utf-8", errors="ignore")
            elif atyp == 4:
                ip_bytes = await client_reader.read(16)
                target_host = socket.inet_ntop(socket.AF_INET6, ip_bytes)
            else:
                client_writer.close()
                return
            target_port = struct.unpack("!H", await client_reader.read(2))[0]

            try:
                (rem_reader, rem_writer), route_used = await open_resilient_connection(target_host, target_port)
            except Exception:
                client_writer.write(b"\x05\x01\x00\x01\x00\x00\x00\x00\x00\x00")
                await client_writer.drain()
                client_writer.close()
                return

            client_writer.write(b"\x05\x00\x00\x01\x00\x00\x00\x00\x00\x00")
            await client_writer.drain()

            await asyncio.gather(
                pipe_stream(client_reader, rem_writer),
                pipe_stream(rem_reader, client_writer),
            )

        # Case 2: Incoming HTTP CONNECT / proxy client
        else:
            line = first_byte + await client_reader.readline()
            parts = line.decode("utf-8", errors="ignore").strip().split()
            if len(parts) < 2:
                client_writer.close()
                return
            method, target = parts[0], parts[1]
            # Consume headers
            while True:
                h = await client_reader.readline()
                if h in (b"\r\n", b"\n", b""):
                    break

            if method.upper() == "CONNECT":
                host_part, _, port_part = target.partition(":")
                target_port = int(port_part) if port_part else 443
                try:
                    (rem_reader, rem_writer), route_used = await open_resilient_connection(host_part, target_port)
                except Exception:
                    client_writer.write(b"HTTP/1.1 502 Bad Gateway\r\n\r\n")
                    await client_writer.drain()
                    client_writer.close()
                    return

                client_writer.write(
                    f"HTTP/1.1 200 Connection Established\r\nX-Route-Used: {route_used}\r\n\r\n".encode("utf-8")
                )
                await client_writer.drain()
                await asyncio.gather(
                    pipe_stream(client_reader, rem_writer),
                    pipe_stream(rem_reader, client_writer),
                )
            else:
                client_writer.close()
    except Exception:
        try:
            client_writer.close()
        except Exception:
            pass


async def main():
    asyncio.create_task(check_route_health())
    server = await asyncio.start_server(handle_client, LISTEN_HOST, LISTEN_PORT)
    print(f"[+] HA Multi-Route Load-Balancer listening on {LISTEN_HOST}:{LISTEN_PORT}", flush=True)
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        sys.exit(0)
