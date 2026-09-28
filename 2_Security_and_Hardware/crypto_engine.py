"""
Cryptographic Engine for Audit Logging Subsystem
Provides AES-256-CBC payload encryption at rest,
master key management (0400 permissions), and SHA-256 tamper-evident hash chaining.
"""

import os
import sys
import json
import base64
import hashlib
import subprocess
from pathlib import Path

AUDIT_DIR = Path(os.path.expanduser("~/.audit"))
KEY_FILE = AUDIT_DIR / ".audit_master.key"
GENESIS_HASH = "0000000000000000000000000000000000000000000000000000000000000000"


def ensure_audit_dir():
    AUDIT_DIR.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(str(AUDIT_DIR), 0o700)


def get_or_create_master_key() -> str:
    """Retrieve or generate 256-bit master encryption key."""
    ensure_audit_dir()
    if KEY_FILE.exists():
        try:
            with open(KEY_FILE, "r") as f:
                key = f.read().strip()
                if len(key) == 64:
                    return key
        except Exception as e:
            print(f"[CryptoEngine] Error reading master key: {e}", file=sys.stderr)

    key = os.urandom(32).hex()
    flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC
    fd = os.open(str(KEY_FILE), flags, 0o400)
    with os.fdopen(fd, "w") as f:
        f.write(key + "\n")
    os.chmod(str(KEY_FILE), 0o400)
    return key


def encrypt_payload(payload: dict, master_key_hex: str = None) -> tuple[str, str]:
    if master_key_hex is None:
        master_key_hex = get_or_create_master_key()

    plaintext = json.dumps(payload, separators=(',', ':'), ensure_ascii=False).encode('utf-8')
    iv_bytes = os.urandom(16)
    iv_hex = iv_bytes.hex()

    proc = subprocess.run(
        [
            "openssl", "enc", "-aes-256-cbc",
            "-K", master_key_hex,
            "-iv", iv_hex,
            "-base64", "-A"
        ],
        input=plaintext,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True
    )
    ciphertext_b64 = proc.stdout.decode('ascii').strip()
    return iv_hex, ciphertext_b64


def decrypt_payload(iv_hex: str, ciphertext_b64: str, master_key_hex: str = None) -> dict:
    if master_key_hex is None:
        master_key_hex = get_or_create_master_key()

    proc = subprocess.run(
        [
            "openssl", "enc", "-d", "-aes-256-cbc",
            "-K", master_key_hex,
            "-iv", iv_hex,
            "-base64", "-A"
        ],
        input=ciphertext_b64.encode('ascii'),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True
    )
    decrypted_bytes = proc.stdout
    return json.loads(decrypted_bytes.decode('utf-8'))


def compute_chain_hash(prev_hash: str, epoch_ns: int, event_type: str, iv_hex: str, ciphertext: str) -> str:
    message = f"{prev_hash}:{epoch_ns}:{event_type}:{iv_hex}:{ciphertext}".encode('utf-8')
    return hashlib.sha256(message).hexdigest()
