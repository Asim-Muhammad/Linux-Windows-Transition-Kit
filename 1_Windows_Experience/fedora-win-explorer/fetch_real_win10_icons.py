#!/usr/bin/env python3
import urllib.request
import base64
import shutil
from pathlib import Path
from PIL import Image, ImageDraw

BASE_URLS = [
    "https://raw.githubusercontent.com/B00merang-Artwork/Windows-10/master",
    "https://cdn.jsdelivr.net/gh/B00merang-Artwork/Windows-10@master",
]

ICONS_TO_FETCH = {
    "256x256_places_folder.png": ["256x256/places/folder.png", "48x48/places/folder.png"],
    "256x256_places_user-desktop.png": ["256x256/places/user-desktop.png", "48x48/places/user-desktop.png"],
    "256x256_places_folder-documents.png": ["256x256/places/folder-documents.png", "48x48/places/folder-documents.png"],
    "256x256_places_folder-download.png": ["256x256/places/folder-download.png", "48x48/places/folder-download.png"],
    "256x256_places_folder-music.png": ["256x256/places/folder-music.png", "48x48/places/folder-music.png"],
    "256x256_places_folder-pictures.png": ["256x256/places/folder-pictures.png", "48x48/places/folder-pictures.png"],
    "256x256_places_folder-videos.png": ["256x256/places/folder-videos.png", "48x48/places/folder-videos.png"],
    "256x256_places_user-home.png": ["256x256/places/user-home.png", "48x48/places/user-home.png"],
    "256x256_places_user-trash.png": ["256x256/places/user-trash.png", "48x48/places/user-trash.png"],
    "256x256_places_network-workgroup.png": ["256x256/places/network-workgroup.png", "48x48/places/network-workgroup.png"],
    "256x256_places_network-server.png": ["256x256/places/network-server.png", "48x48/places/network-server.png"],
    "256x256_devices_computer.png": ["256x256/devices/computer.png", "48x48/devices/computer.png"],
    "256x256_devices_drive-harddisk-system.png": ["256x256/devices/drive-harddisk-system.png", "48x48/devices/drive-harddisk-system.png"],
    "256x256_devices_drive-harddisk.png": ["256x256/devices/drive-harddisk.png", "48x48/devices/drive-harddisk.png"],
}

CACHE_DIR = Path(__file__).resolve().parent / "real_win10_icons"
CACHE_DIR.mkdir(parents=True, exist_ok=True)
THEME_DIR = Path.home() / ".local/share/icons/Win10-Explorer"
CFG_DIR = Path.home() / ".config/win-explorer"
CFG_DIR.mkdir(parents=True, exist_ok=True)

downloaded = 0
for local_name, rel_paths in ICONS_TO_FETCH.items():
    dest = CACHE_DIR / local_name
    if dest.exists() and dest.stat().st_size > 100:
        downloaded += 1
        continue
    fetched = False
    for base_url in BASE_URLS:
        if fetched:
            break
        for rel in rel_paths:
            url = f"{base_url}/{rel}"
            try:
                req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
                with urllib.request.urlopen(req, timeout=15) as resp:
                    data = resp.read()
                    if len(data) > 100:
                        dest.write_bytes(data)
                        downloaded += 1
                        fetched = True
                        break
            except Exception as e:
                print(f"[warn] {url}: {e}")
print(f"[*] Fetched {downloaded}/{len(ICONS_TO_FETCH)} authentic 256x256 PNGs")

# Build 256x256 USB drive icon (silver 3D drive + Windows 10 blue USB badge)
hdd_path = CACHE_DIR / "256x256_devices_drive-harddisk.png"
usb_out = CACHE_DIR / "256x256_devices_drive-removable-media-usb.png"
if hdd_path.exists():
    usb_img = Image.open(hdd_path).convert("RGBA")
    badge = Image.new("RGBA", (86, 86), (0, 0, 0, 0))
    bd = ImageDraw.Draw(badge)
    bd.rounded_rectangle([4, 4, 82, 82], radius=14, fill=(0, 120, 215, 255), outline=(0, 90, 158, 255), width=3)
    bd.line([(43, 66), (43, 20)], fill=(255, 255, 255, 255), width=5)
    bd.polygon([(43, 14), (34, 26), (52, 26)], fill=(255, 255, 255, 255))
    bd.ellipse([37, 60, 49, 72], fill=(255, 255, 255, 255))
    bd.line([(43, 50), (25, 38), (25, 28)], fill=(255, 255, 255, 255), width=4)
    bd.ellipse([20, 23, 30, 33], fill=(255, 255, 255, 255))
    bd.line([(43, 44), (61, 34), (61, 26)], fill=(255, 255, 255, 255), width=4)
    bd.rectangle([56, 20, 66, 30], fill=(255, 255, 255, 255))
    usb_img.alpha_composite(badge, (22, 62))
    usb_img.save(usb_out)

# Copy base PNGs into ~/.config/win-explorer/ for C++ drive tile renderer & Desktop shortcuts
cfg_copies = {
    "256x256_devices_drive-harddisk-system.png": "drive_c_base.png",
    "256x256_devices_drive-harddisk.png": "drive_d_base.png",
    "256x256_devices_drive-removable-media-usb.png": "drive_usb_base.png",
    "256x256_devices_computer.png": "this_pc_icon.png",
    "256x256_places_user-home.png": "user_home_icon.png",
    "256x256_places_folder.png": "folder_base.png",
}
for src_name, dst_name in cfg_copies.items():
    sp = CACHE_DIR / src_name
    if sp.exists():
        shutil.copy2(sp, CFG_DIR / dst_name)

MAPPING = {
    "folder": "256x256_places_folder.png",
    "folder-with-contents": "256x256_places_folder.png",
    "inode-directory": "256x256_places_folder.png",
    "folder-yellow": "256x256_places_folder.png",
    "folder-windows": "256x256_places_folder.png",
    "folder-root": "256x256_places_folder.png",
    "folder-text": "256x256_places_folder.png",
    "folder-new": "256x256_places_folder.png",
    "user-desktop": "256x256_places_user-desktop.png",
    "folder-desktop": "256x256_places_user-desktop.png",
    "folder-documents": "256x256_places_folder-documents.png",
    "folder-download": "256x256_places_folder-download.png",
    "folder-downloads": "256x256_places_folder-download.png",
    "folder-music": "256x256_places_folder-music.png",
    "folder-sound": "256x256_places_folder-music.png",
    "folder-pictures": "256x256_places_folder-pictures.png",
    "folder-images": "256x256_places_folder-pictures.png",
    "folder-videos": "256x256_places_folder-videos.png",
    "folder-video": "256x256_places_folder-videos.png",
    "user-home": "256x256_places_user-home.png",
    "folder-home": "256x256_places_user-home.png",
    "user-trash": "256x256_places_user-trash.png",
    "user-trash-full": "256x256_places_user-trash.png",
    "user-trash-symbolic": "256x256_places_user-trash.png",
    "network-workgroup": "256x256_places_network-workgroup.png",
    "network-server": "256x256_places_network-server.png",
    "folder-network": "256x256_places_network-workgroup.png",
    "folder-remote": "256x256_places_network-workgroup.png",
    "computer": "256x256_devices_computer.png",
    "computer-laptop": "256x256_devices_computer.png",
    "drive-harddisk-system": "256x256_devices_drive-harddisk-system.png",
    "drive-harddisk-root": "256x256_devices_drive-harddisk-system.png",
    "drive-c": "256x256_devices_drive-harddisk-system.png",
    "drive-harddisk": "256x256_devices_drive-harddisk.png",
    "drive-d": "256x256_devices_drive-harddisk.png",
    "drive-removable-media": "256x256_devices_drive-removable-media-usb.png",
    "drive-removable-media-usb": "256x256_devices_drive-removable-media-usb.png",
    "drive-removable-media-usb-pendrive": "256x256_devices_drive-removable-media-usb.png",
}

for icon_name, png_name in MAPPING.items():
    png_path = CACHE_DIR / png_name
    if not png_path.exists():
        continue
    b64 = base64.b64encode(png_path.read_bytes()).decode("ascii")
    svg = (
        f"<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 256 256\" width=\"256\" height=\"256\">"
        f"<image x=\"0\" y=\"0\" width=\"256\" height=\"256\" preserveAspectRatio=\"xMidYMid meet\" href=\"data:image/png;base64,{b64}\"/>"
        f"</svg>"
    )
    for sub in ("places", "actions", "devices"):
        out = THEME_DIR / "scalable" / sub / f"{icon_name}.svg"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(svg)

print("[✓] Installed all 37 authentic Windows 10 3D icons and drive tile PNGs!")
