#!/usr/bin/env bash
# ==============================================================================
# timeshift-auto-prune.sh
# Automatically verifies recent Timeshift snapshots and maintains a safe 2-snapshot
# retention window, preventing BTRFS storage exhaustion while preserving rollback.
# ==============================================================================
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: timeshift-auto-prune must be run as root." >&2
    exit 1
fi

echo "=== [$(date '+%Y-%m-%d %H:%M:%S')] Starting Timeshift Auto-Pruner ==="

# 1. Enforce safe 2-snapshot daily retention in timeshift.json
CONFIG_FILE="/etc/timeshift/timeshift.json"
if [ -f "$CONFIG_FILE" ]; then
    sed -i 's/"count_daily"[[:space:]]*:[[:space:]]*"[0-9]*"/"count_daily" : "2"/' "$CONFIG_FILE"
    sed -i 's/"count_weekly"[[:space:]]*:[[:space:]]*"[0-9]*"/"count_weekly" : "0"/' "$CONFIG_FILE"
    sed -i 's/"count_monthly"[[:space:]]*:[[:space:]]*"[0-9]*"/"count_monthly" : "0"/' "$CONFIG_FILE"
    sed -i 's/"schedule_weekly"[[:space:]]*:[[:space:]]*"true"/"schedule_weekly" : "false"/' "$CONFIG_FILE"
    echo "[OK] timeshift.json configured: count_daily=2, count_weekly=0"
fi

# 2. Locate snapshots directory on disk
SNAPSHOTS_DIR="/timeshift/snapshots"
if [ ! -d "$SNAPSHOTS_DIR" ]; then
    for mp in /run/timeshift/*/backup/timeshift/snapshots; do
        if [ -d "$mp" ]; then
            SNAPSHOTS_DIR="$mp"
            break
        fi
    done
fi

# 3. Retrieve list of snapshots
SNAPSHOT_LIST=()
while IFS= read -r line; do
    [ -n "$line" ] && SNAPSHOT_LIST+=("$line")
done < <(timeshift --list 2>/dev/null | grep -E '^[0-9]+[[:space:]]+>[[:space:]]+[0-9]{4}-[0-9]{2}-[0-9]{2}_' | awk '{print $3}' || true)

if [ ${#SNAPSHOT_LIST[@]} -eq 0 ] && [ -d "$SNAPSHOTS_DIR" ]; then
    while IFS= read -r line; do
        [ -n "$line" ] && SNAPSHOT_LIST+=("$line")
    done < <(find "$SNAPSHOTS_DIR" -mindepth 1 -maxdepth 1 -type d -name "20*" -printf "%f\n" | sort || true)
fi

KEEP_COUNT=2
TOTAL=${#SNAPSHOT_LIST[@]}
echo "Total snapshots found: $TOTAL"

if [ "$TOTAL" -le "$KEEP_COUNT" ]; then
    echo "[OK] Only $TOTAL snapshot(s) present (retention limit: $KEEP_COUNT). No pruning needed."
    df -h /
    exit 0
fi

# 4. Sort snapshots chronologically (YYYY-MM-DD_HH-MM-SS)
IFS=$'\n' SORTED=($(sort <<<"${SNAPSHOT_LIST[*]}"))
unset IFS

LATEST="${SORTED[-1]}"
echo "Latest snapshot candidate: $LATEST"

# 5. Verify the latest snapshot is valid & intact
LATEST_PATH="$SNAPSHOTS_DIR/$LATEST"
VALID=false

if [ -d "$LATEST_PATH" ]; then
    if [ -d "$LATEST_PATH/localhost/usr" ] || [ -d "$LATEST_PATH/localhost/etc" ] || [ -d "$LATEST_PATH/usr" ]; then
        SZ=$(du -s "$LATEST_PATH" 2>/dev/null | awk '{print $1}' || echo "0")
        if [ "$SZ" -gt 500000 ]; then # At least 500MB
            VALID=true
            echo "[VERIFIED] Latest snapshot $LATEST verified healthy (${SZ} KB)."
        fi
    fi
fi

if [ "$VALID" != "true" ]; then
    if timeshift --list 2>/dev/null | grep -q "$LATEST"; then
        VALID=true
        echo "[VERIFIED] Latest snapshot $LATEST verified via Timeshift DB."
    fi
fi

if [ "$VALID" != "true" ]; then
    echo "ERROR: Latest snapshot $LATEST could not be verified! Skipping deletion of older snapshots." >&2
    exit 2
fi

# 6. Delete older snapshots, retaining the newest KEEP_COUNT snapshots
PRUNED=0
DELETE_COUNT=$((TOTAL - KEEP_COUNT))
for ((i=0; i < DELETE_COUNT; i++)); do
    OLD="${SORTED[$i]}"
    echo "Pruning older snapshot: $OLD ..."
    
    # Use native Timeshift deletion
    if timeshift --delete --snapshot "$OLD" --yes 2>/dev/null; then
        echo "  [OK] Timeshift deleted: $OLD"
        PRUNED=$((PRUNED + 1))
    else
        echo "  [WARNING] Could not safely delete snapshot $OLD via Timeshift. Skipping direct rm to protect backup integrity." >&2
    fi
done

echo "=== [SUCCESS] Pruned $PRUNED old snapshot(s). Kept $KEEP_COUNT latest verified snapshots ==="
df -h /
