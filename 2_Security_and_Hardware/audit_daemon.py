#!/usr/bin/env python3
"""
Audit Log Management and Inspection CLI
Provides real-time inspection, decryption of AES-256 payloads,
daemon lifecycle management, and cryptographic SHA-256 ledger verification.
"""

import os
import sys
import time
import json
import sqlite3
import argparse
import subprocess
from pathlib import Path

AUDIT_DIR = Path(os.path.expanduser("~/.audit"))
sys.path.insert(0, str(AUDIT_DIR))
import crypto_engine

DB_FILE = AUDIT_DIR / "audit.db"
PID_FILE = AUDIT_DIR / "audit.pid"
SOCKET_FILE = AUDIT_DIR / "audit.sock"
DAEMON_SCRIPT = AUDIT_DIR / "audit_daemon.py"

C_RESET = "\033[0m"
C_BOLD = "\033[1m"
C_GREEN = "\033[32m"
C_RED = "\033[31m"
C_YELLOW = "\033[33m"
C_BLUE = "\033[34m"
C_CYAN = "\033[36m"
C_MAGENTA = "\033[35m"


def is_daemon_running() -> tuple[bool, int]:
    if not PID_FILE.exists():
        return False, 0
    try:
        with open(PID_FILE, "r") as f:
            pid = int(f.read().strip())
        os.kill(pid, 0)
        return True, pid
    except (OSError, ValueError):
        return False, 0


def cmd_status(args):
    running, pid = is_daemon_running()
    print(f"{C_BOLD}=== Audit Subsystem Status ==={C_RESET}")
    if running:
        print(f"Daemon Status    : {C_GREEN}RUNNING{C_RESET} (PID: {pid})")
    else:
        print(f"Daemon Status    : {C_RED}STOPPED{C_RESET}")

    sock_ok = SOCKET_FILE.exists()
    print(f"Unix Socket      : {C_GREEN}ACTIVE{C_RESET} ({SOCKET_FILE})" if sock_ok else f"Unix Socket      : {C_YELLOW}OFFLINE{C_RESET}")

    key_exists = (AUDIT_DIR / ".audit_master.key").exists()
    print(f"AES-256 Key      : {C_GREEN}INITIALIZED{C_RESET}" if key_exists else f"AES-256 Key      : {C_RED}MISSING{C_RESET}")

    if DB_FILE.exists():
        size_kb = DB_FILE.stat().st_size / 1024
        try:
            with sqlite3.connect(str(DB_FILE)) as conn:
                cur = conn.cursor()
                cur.execute("SELECT count(*), max(timestamp) FROM audit_log;")
                count, max_ts = cur.fetchone()
                print(f"Database Entries : {C_BOLD}{count or 0}{C_RESET} records ({size_kb:.1f} KB)")
                print(f"Latest Event     : {max_ts or 'None'}")
        except Exception as e:
            print(f"Database Error   : {e}")
    else:
        print("Database Entries : 0 (Database not yet created)")


def cmd_start(args):
    running, pid = is_daemon_running()
    if running:
        print(f"Daemon is already running (PID {pid}).")
        return
    subprocess.Popen([sys.executable, str(DAEMON_SCRIPT), "--daemon"])
    time.sleep(0.5)
    running, pid = is_daemon_running()
    if running:
        print(f"{C_GREEN}Audit daemon started successfully (PID {pid}).{C_RESET}")
    else:
        print(f"{C_RED}Failed to start audit daemon.{C_RESET}", file=sys.stderr)


def cmd_stop(args):
    running, pid = is_daemon_running()
    if not running:
        print("Audit daemon is not running.")
        return
    subprocess.run([sys.executable, str(DAEMON_SCRIPT), "--stop"])
    print(f"{C_YELLOW}Audit daemon stopped.{C_RESET}")


def cmd_restart(args):
    cmd_stop(args)
    time.sleep(0.5)
    cmd_start(args)


def format_event(record_id, timestamp, event_type, priv_level, is_sudo, payload) -> str:
    ts_str = f"{C_CYAN}[{timestamp}]{C_RESET}"
    if is_sudo or priv_level == "SUDO":
        type_str = f"{C_RED}{C_BOLD}[SUDO_EXEC]{C_RESET}"
    elif event_type in ("COMMAND_EXEC",):
        type_str = f"{C_GREEN}[COMMAND]{C_RESET}"
    elif event_type in ("FILE_RENAME",):
        type_str = f"{C_YELLOW}{C_BOLD}[FILE_RENAME]{C_RESET}"
    elif event_type in ("FILE_MOVE", "FILE_MOVE_OUT", "FILE_MOVE_IN"):
        type_str = f"{C_BLUE}{C_BOLD}[FILE_MOVE]{C_RESET}"
    elif event_type in ("FILE_CREATE", "DIR_CREATE"):
        type_str = f"{C_CYAN}[CREATE]{C_RESET}"
    elif event_type in ("FILE_DELETE", "DIR_DELETE"):
        type_str = f"{C_RED}[DELETE]{C_RESET}"
    else:
        type_str = f"{C_MAGENTA}[{event_type}]{C_RESET}"

    detail = ""
    if "command" in payload:
        cmd = payload.get("command", "")
        pwd = payload.get("working_dir", "")
        code = payload.get("exit_code", 0)
        code_str = f"{C_GREEN}0{C_RESET}" if code == 0 else f"{C_RED}{code}{C_RESET}"
        detail = f"(exit: {code_str}) in {C_BOLD}{pwd}{C_RESET} $ {cmd}"
    elif "old_path" in payload and "new_path" in payload:
        detail = f"{payload['old_path']} -> {C_BOLD}{payload['new_path']}{C_RESET}"
    elif "path" in payload:
        detail = f"{payload['path']}"
    elif "message" in payload:
        detail = f"{payload['message']}"
    else:
        detail = json.dumps(payload)

    return f"#{record_id:<4} {ts_str} {type_str} {detail}"


def cmd_tail(args):
    if not DB_FILE.exists():
        print("No audit database found.")
        return

    master_key = crypto_engine.get_or_create_master_key()
    limit = args.lines

    with sqlite3.connect(str(DB_FILE)) as conn:
        cur = conn.cursor()
        cur.execute("""
            SELECT id, timestamp, event_type, privilege_level, is_sudo, iv_hex, ciphertext
            FROM audit_log
            ORDER BY id DESC LIMIT ?;
        """, (limit,))
        rows = cur.fetchall()

    for row in reversed(rows):
        rec_id, ts, ev_type, priv, is_sudo, iv, ct = row
        try:
            payload = crypto_engine.decrypt_payload(iv, ct, master_key)
        except Exception:
            payload = {"error": "[DECRYPTION FAILED]"}
        print(format_event(rec_id, ts, ev_type, priv, is_sudo, payload))

    if args.follow:
        last_id = rows[0][0] if rows else 0
        try:
            while True:
                time.sleep(0.5)
                with sqlite3.connect(str(DB_FILE)) as conn:
                    cur = conn.cursor()
                    cur.execute("""
                        SELECT id, timestamp, event_type, privilege_level, is_sudo, iv_hex, ciphertext
                        FROM audit_log
                        WHERE id > ?
                        ORDER BY id ASC;
                    """, (last_id,))
                    new_rows = cur.fetchall()
                for row in new_rows:
                    rec_id, ts, ev_type, priv, is_sudo, iv, ct = row
                    last_id = rec_id
                    try:
                        payload = crypto_engine.decrypt_payload(iv, ct, master_key)
                    except Exception:
                        payload = {"error": "[DECRYPTION FAILED]"}
                    print(format_event(rec_id, ts, ev_type, priv, is_sudo, payload))
        except KeyboardInterrupt:
            pass


def cmd_query(args):
    if not DB_FILE.exists():
        print("No audit database found.")
        return

    master_key = crypto_engine.get_or_create_master_key()
    query = "SELECT id, timestamp, event_type, privilege_level, is_sudo, iv_hex, ciphertext FROM audit_log WHERE 1=1"
    params = []

    if args.type:
        types = [t.strip().upper() for t in args.type.split(",")]
        placeholders = ",".join("?" for _ in types)
        query += f" AND event_type IN ({placeholders})"
        params.extend(types)

    if args.sudo:
        query += " AND is_sudo = 1"

    if args.since:
        query += " AND timestamp >= ?"
        params.append(args.since)

    query += " ORDER BY id ASC LIMIT ?"
    params.append(args.limit)

    with sqlite3.connect(str(DB_FILE)) as conn:
        cur = conn.cursor()
        cur.execute(query, params)
        rows = cur.fetchall()

    for row in rows:
        rec_id, ts, ev_type, priv, is_sudo, iv, ct = row
        try:
            payload = crypto_engine.decrypt_payload(iv, ct, master_key)
        except Exception:
            payload = {"error": "[DECRYPTION FAILED]"}

        if args.search:
            payload_str = json.dumps(payload).lower()
            if args.search.lower() not in payload_str:
                continue

        print(format_event(rec_id, ts, ev_type, priv, is_sudo, payload))


def cmd_verify(args):
    if not DB_FILE.exists():
        print("No audit database found.")
        return

    print(f"{C_BOLD}Verifying Cryptographic Ledger Integrity...{C_RESET}")
    with sqlite3.connect(str(DB_FILE)) as conn:
        cur = conn.cursor()
        cur.execute("""
            SELECT id, timestamp, epoch_ns, event_type, iv_hex, ciphertext, prev_hash, curr_hash
            FROM audit_log
            ORDER BY id ASC;
        """)
        rows = cur.fetchall()

    if not rows:
        print(f"{C_YELLOW}Ledger is empty.{C_RESET}")
        return

    expected_prev = crypto_engine.GENESIS_HASH
    errors = 0

    for row in rows:
        rec_id, ts, epoch_ns, ev_type, iv_hex, ciphertext, prev_hash, curr_hash = row
        if prev_hash != expected_prev:
            print(f"{C_RED}[TAMPER DETECTED] Row #{rec_id}: prev_hash does not match previous record!{C_RESET}")
            errors += 1

        recomputed = crypto_engine.compute_chain_hash(prev_hash, epoch_ns, ev_type, iv_hex, ciphertext)
        if recomputed != curr_hash:
            print(f"{C_RED}[TAMPER DETECTED] Row #{rec_id}: curr_hash signature mismatch!{C_RESET}")
            errors += 1

        expected_prev = curr_hash

    if errors == 0:
        print(f"{C_GREEN}{C_BOLD}[INTEGRITY VERIFIED] 100% Valid!{C_RESET}")
        print(f"Verified {len(rows)} entries from Genesis ({crypto_engine.GENESIS_HASH[:8]}...) to Head ({expected_prev[:8]}...).")
        print(f"Zero deletions, modifications, or reordering detected.")
    else:
        print(f"{C_RED}{C_BOLD}[INTEGRITY CHECK FAILED] Found {errors} discrepancies!{C_RESET}", file=sys.stderr)
        sys.exit(1)


def cmd_dump_encrypted(args):
    if not DB_FILE.exists():
        print("No audit database found.")
        return

    with sqlite3.connect(str(DB_FILE)) as conn:
        cur = conn.cursor()
        cur.execute("""
            SELECT id, timestamp, event_type, iv_hex, ciphertext, curr_hash
            FROM audit_log
            ORDER BY id DESC LIMIT ?;
        """, (args.lines,))
        rows = cur.fetchall()

    print(f"{C_BOLD}Raw Encrypted Storage on Disk (Zero Plaintext Exposed):{C_RESET}\n")
    for row in reversed(rows):
        rec_id, ts, ev_type, iv, ct, ch = row
        print(f"Record #{rec_id} [{ts}] {ev_type}")
        print(f"  IV         : {iv}")
        print(f"  Ciphertext : {ct[:64]}... ({len(ct)} chars b64)")
        print(f"  SHA256 Hash: {ch}\n")


def main():
    parser = argparse.ArgumentParser(description="Audit System CLI")
    subparsers = parser.add_subparsers(dest="subcommand", required=True)

    p_status = subparsers.add_parser("status")
    p_status.set_defaults(func=cmd_status)

    p_start = subparsers.add_parser("start")
    p_start.set_defaults(func=cmd_start)
    p_stop = subparsers.add_parser("stop")
    p_stop.set_defaults(func=cmd_stop)
    p_restart = subparsers.add_parser("restart")
    p_restart.set_defaults(func=cmd_restart)

    p_tail = subparsers.add_parser("tail")
    p_tail.add_argument("-n", "--lines", type=int, default=20)
    p_tail.add_argument("-f", "--follow", action="store_true")
    p_tail.set_defaults(func=cmd_tail)

    p_query = subparsers.add_parser("query")
    p_query.add_argument("--type")
    p_query.add_argument("--sudo", action="store_true")
    p_query.add_argument("--search")
    p_query.add_argument("--since")
    p_query.add_argument("-l", "--limit", type=int, default=50)
    p_query.set_defaults(func=cmd_query)

    p_verify = subparsers.add_parser("verify")
    p_verify.set_defaults(func=cmd_verify)

    p_dump = subparsers.add_parser("dump-encrypted")
    p_dump.add_argument("-n", "--lines", type=int, default=5)
    p_dump.set_defaults(func=cmd_dump_encrypted)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
