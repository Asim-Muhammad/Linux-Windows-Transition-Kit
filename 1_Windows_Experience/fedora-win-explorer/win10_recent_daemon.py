#!/usr/bin/env python3
import os
import sys
import time
import struct
import select
import ctypes

recent_dir = os.path.expanduser("~/.win-explorer/RecentFiles")
os.makedirs(recent_dir, exist_ok=True)

folders = [
    os.path.expanduser("~/Desktop"),
    os.path.expanduser("~/Documents"),
    os.path.expanduser("~/Downloads"),
    os.path.expanduser("~/Pictures"),
    os.path.expanduser("~/Music"),
    os.path.expanduser("~/Videos")
]
folders = [f for f in folders if os.path.exists(f)]

recent_paths = []

def initial_scan():
    global recent_paths
    files_data = []
    for folder in folders:
        for root, dirs, files in os.walk(folder):
            if root[len(folder):].count(os.sep) > 4:
                dirs.clear()
                continue
            for f in files:
                if f.startswith('.'):
                    continue
                path = os.path.join(root, f)
                try:
                    files_data.append((os.path.getmtime(path), path))
                except Exception:
                    pass
    files_data.sort(key=lambda x: x[0], reverse=True)
    recent_paths = [p for _, p in files_data[:40]]

def rebuild_symlinks():
    global recent_paths
    # Incremental sync to prevent Dolphin flicker
    existing = []
    try:
        existing = os.listdir(recent_dir)
    except Exception:
        pass

    # Find valid symlinks we want to keep
    valid_links = {}
    for f in existing:
        sym_path = os.path.join(recent_dir, f)
        if os.path.islink(sym_path):
            try:
                target = os.readlink(sym_path)
                valid_links[target] = f
            except Exception:
                pass

    # Remove symlinks that point to targets no longer in recent_paths
    for f in existing:
        sym_path = os.path.join(recent_dir, f)
        if os.path.islink(sym_path):
            try:
                target = os.readlink(sym_path)
                if target not in recent_paths or not os.path.exists(target):
                    os.remove(sym_path)
            except Exception:
                pass
        else:
            try:
                os.remove(sym_path)
            except Exception:
                pass

    # Create missing symlinks
    for path in recent_paths:
        if not os.path.exists(path):
            continue
        if path not in valid_links:
            name = os.path.basename(path)
            sym = os.path.join(recent_dir, name)
            c = 1
            while os.path.exists(sym) or os.path.islink(sym):
                sym = os.path.join(recent_dir, f"{c}_{name}")
                c += 1
            try:
                os.symlink(path, sym)
            except Exception:
                pass

initial_scan()
rebuild_symlinks()

# Inotify constants
IN_CLOSE_WRITE = 0x00000008
IN_MOVED_FROM  = 0x00000040
IN_MOVED_TO    = 0x00000080
IN_CREATE      = 0x00000100
IN_DELETE      = 0x00000200
IN_ISDIR       = 0x40000000
WATCH_MASK     = IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE | IN_DELETE

try:
    libc = ctypes.CDLL('libc.so.6', use_errno=True)
    inotify_init1 = libc.inotify_init1
    inotify_init1.argtypes = [ctypes.c_int]
    inotify_init1.restype = ctypes.c_int

    inotify_add_watch = libc.inotify_add_watch
    inotify_add_watch.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_uint32]
    inotify_add_watch.restype = ctypes.c_int

    fd = inotify_init1(0)
    has_inotify = (fd >= 0)
except Exception:
    has_inotify = False

wd_to_dir = {}

def add_watches(base_folder=None):
    global wd_to_dir
    if not has_inotify:
        return
    targets = [base_folder] if base_folder else folders
    for folder in targets:
        root_parent = folder
        for f in folders:
            if folder.startswith(f):
                root_parent = f
                break
        
        for root, dirs, _ in os.walk(folder):
            if root[len(root_parent):].count(os.sep) > 4:
                dirs.clear()
                continue
            try:
                b_path = root.encode('utf-8')
                wd = inotify_add_watch(fd, b_path, WATCH_MASK)
                if wd >= 0:
                    wd_to_dir[wd] = root
            except Exception:
                pass

if has_inotify:
    add_watches()

EVENT_FMT = 'iIII'
EVENT_SIZE = struct.calcsize(EVENT_FMT)

last_periodic_check = time.time()

while True:
    try:
        if has_inotify:
            r, _, _ = select.select([fd], [], [], 2.0)
            if r:
                data = os.read(fd, 4096)
                offset = 0
                changed = False
                while offset + EVENT_SIZE <= len(data):
                    wd, mask, cookie, length = struct.unpack_from(EVENT_FMT, data, offset)
                    offset += EVENT_SIZE
                    name = b""
                    if length > 0:
                        name = data[offset:offset+length].split(b'\0', 1)[0]
                        offset += length
                    
                    dir_path = wd_to_dir.get(wd)
                    if dir_path and name:
                        try:
                            full_name = name.decode('utf-8', errors='ignore')
                            if not full_name.startswith('.'):
                                full_path = os.path.join(dir_path, full_name)
                                
                                if (mask & IN_ISDIR) and (mask & (IN_CREATE | IN_MOVED_TO)):
                                    add_watches(full_path)
                                
                                if mask & (IN_DELETE | IN_MOVED_FROM):
                                    if full_path in recent_paths:
                                        recent_paths.remove(full_path)
                                        changed = True
                                elif mask & (IN_CREATE | IN_MOVED_TO | IN_CLOSE_WRITE):
                                    if os.path.isfile(full_path):
                                        if full_path in recent_paths:
                                            recent_paths.remove(full_path)
                                        recent_paths.insert(0, full_path)
                                        recent_paths = recent_paths[:40]
                                        changed = True
                        except Exception:
                            pass
                if changed:
                    rebuild_symlinks()
        else:
            time.sleep(2.0)

        now = time.time()
        if now - last_periodic_check > 30.0:
            last_periodic_check = now
            if has_inotify:
                add_watches() # refresh watches
            else:
                initial_scan()
                rebuild_symlinks()

    except Exception:
        time.sleep(1.0)
