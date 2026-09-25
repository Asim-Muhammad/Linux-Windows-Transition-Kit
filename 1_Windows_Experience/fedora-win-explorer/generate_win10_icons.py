#!/usr/bin/env python3
"""
Generates the authentic Windows 10 Upright 3D Folder & Drive SVG Icon Theme
(Win10-Explorer) for KDE Dolphin.
- Upright 3D perspective golden folders (left spine, back amber cover, angled front flap,
  open right slot where sideways-peeking images or white paper sheets appear).
- Local Disk (C:), Local Disk (D:), and USB Removable Drive (E:) icons.
- Full set of 24 Windows 10 Ribbon & Navigation toolbar icons.
"""
from pathlib import Path

HOME = Path.home()
ICON_DIR = HOME / ".local/share/icons/Win10-Explorer"
PLACES_DIR = ICON_DIR / "scalable/places"
ACTIONS_DIR = ICON_DIR / "scalable/actions"
DEVICES_DIR = ICON_DIR / "scalable/devices"
APPS_DIR = ICON_DIR / "scalable/apps"
MIMETYPES_DIR = ICON_DIR / "scalable/mimetypes"
EMBLEMS_DIR = ICON_DIR / "scalable/emblems"

for d in [PLACES_DIR, ACTIONS_DIR, DEVICES_DIR, APPS_DIR, MIMETYPES_DIR, EMBLEMS_DIR]:
    d.mkdir(parents=True, exist_ok=True)

# Remove any stale icon-theme.cache so Qt loads dynamic sideways-peek folder SVGs immediately
for cache_file in ICON_DIR.glob("**/icon-theme.cache"):
    cache_file.unlink(missing_ok=True)

INDEX_THEME = """[Icon Theme]
Name=Win10-Explorer
Comment=Windows 10 File Explorer Upright 3D Icon Theme for Dolphin
Inherits=breeze-dark,breeze,Adwaita,hicolor
Example=folder
Directories=scalable/places,scalable/actions,scalable/devices,scalable/apps,scalable/mimetypes,scalable/emblems

[scalable/places]
Size=48
Context=Places
Type=Scalable
MinSize=16
MaxSize=256

[scalable/actions]
Size=32
Context=Actions
Type=Scalable
MinSize=16
MaxSize=128

[scalable/devices]
Size=48
Context=Devices
Type=Scalable
MinSize=16
MaxSize=256

[scalable/apps]
Size=48
Context=Applications
Type=Scalable
MinSize=16
MaxSize=256

[scalable/mimetypes]
Size=48
Context=MimeTypes
Type=Scalable
MinSize=16
MaxSize=256

[scalable/emblems]
Size=16
Context=Emblems
Type=Scalable
MinSize=8
MaxSize=64
"""
(ICON_DIR / "index.theme").write_text(INDEX_THEME)

# Transparent symlink emblem so symlinks never show ugly chain badges
(EMBLEMS_DIR / "emblem-symbolic-link.svg").write_text(
    '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"/>'
)


def win10_upright_folder_svg(middle_slot_svg="", front_badge_svg=""):
    """
    Authentic Windows 10 Upright 3D Perspective Folder (48x48 viewBox):
    - Back Cover: Vertical golden-amber plate (x=7..38, y=5..40)
    - Middle Slot: Sideways-peeking content (photos or white paper sheets at x=20..43)
    - Front Flap: Angled 3D golden cover (x=7..33, y=10..43) overlapping the left half
      of the middle slot so contents peek out sideways on the right (x=33..43)!
    """
    return f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48" width="48" height="48">
  <defs>
    <linearGradient id="w10back" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0%" stop-color="#F0AD28"/>
      <stop offset="100%" stop-color="#C98010"/>
    </linearGradient>
    <linearGradient id="w10front" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0%" stop-color="#FFED9E"/>
      <stop offset="45%" stop-color="#FCD452"/>
      <stop offset="100%" stop-color="#E8AA1E"/>
    </linearGradient>
  </defs>
  <!-- 1. Back Cover (Windows 10 Upright Golden Back Plate) -->
  <path d="M7 6 a1.5 1.5 0 0 1 1.5 -1.5 h10 l3 3 H37.5 a1.5 1.5 0 0 1 1.5 1.5 v30 a1.5 1.5 0 0 1 -1.5 1.5 H8.5 A1.5 1.5 0 0 1 7 39 V6 z"
        fill="url(#w10back)" stroke="#B8730A" stroke-width="0.8"/>
  <!-- 2. Middle Slot (Sideways-Peeking Image or Document Sheets) -->
  {middle_slot_svg}
  <!-- 3. Front Flap (3D Angled Golden Cover hinged on left spine x=7, ending at x=33.5) -->
  <path d="M7 10.5 l10 0.8 l2.5 2.2 l13 1.2 a1.2 1.2 0 0 1 1 1.2 v26.2 a1.2 1.2 0 0 1 -1.3 1.2 L8.2 41 A1.2 1.2 0 0 1 7 39.8 V10.5 z"
        fill="url(#w10front)" stroke="#D19214" stroke-width="0.8"/>
  <!-- Top-left highlight on Front Flap -->
  <path d="M7.8 11.2 l9.2 0.7 l2.5 2.2 l13 1.2" fill="none" stroke="#FFF8D6" stroke-width="1"/>
  <!-- 4. Optional Special Folder Emblem on Front Flap -->
  {front_badge_svg}
</svg>"""


PAPER_PEEK_SVG = """
  <!-- White paper sheets peeking out sideways from the right side of the folder -->
  <g transform="rotate(4 32 25)">
    <rect x="20" y="10" width="20" height="27" rx="0.8" fill="#F1F5F9" stroke="#94A3B8" stroke-width="0.9"/>
    <rect x="22" y="12" width="20" height="27" rx="0.8" fill="#FFFFFF" stroke="#64748B" stroke-width="0.9"/>
    <line x1="34" y1="16" x2="39" y2="16" stroke="#94A3B8" stroke-width="1.2"/>
    <line x1="34" y1="20" x2="39" y2="20" stroke="#94A3B8" stroke-width="1.2"/>
    <line x1="34" y1="24" x2="39" y2="24" stroke="#94A3B8" stroke-width="1.2"/>
    <line x1="34" y1="28" x2="38" y2="28" stroke="#94A3B8" stroke-width="1.2"/>
  </g>
"""


def win10_drive_svg(fill_width=16, is_system=False, is_usb=False):
    """
    Authentic Windows 10 Hard Drive & USB Removable Drive icon (48x48).
    """
    badge = ""
    if is_system:
        badge = """
  <!-- Windows 10 Blue OS Badge on Local Disk (C:) -->
  <g transform="translate(8, 10)">
    <rect x="0" y="0" width="5.5" height="5.5" fill="#0078D7"/>
    <rect x="6.5" y="0" width="5.5" height="5.5" fill="#0078D7"/>
    <rect x="0" y="6.5" width="5.5" height="5.5" fill="#0078D7"/>
    <rect x="6.5" y="6.5" width="5.5" height="5.5" fill="#0078D7"/>
  </g>"""
    elif is_usb:
        badge = """
  <!-- Windows 10 USB Removable Drive Badge -->
  <g transform="translate(8, 9)">
    <rect x="0" y="0" width="13" height="12" rx="1.5" fill="#0078D7"/>
    <line x1="6.5" y1="2" x2="6.5" y2="10" stroke="#FFFFFF" stroke-width="1.5"/>
    <circle cx="6.5" cy="2.5" r="1.2" fill="#FFFFFF"/>
    <polyline points="3.5,6.5 6.5,4.5 9.5,6.5" fill="none" stroke="#FFFFFF" stroke-width="1.2"/>
  </g>"""
    bar_rect = f'<rect x="9" y="29" width="{fill_width}" height="5" fill="#0078D7"/>' if fill_width > 0 else ""
    return f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48" width="48" height="48">
  <path d="M5 23 l4.5 -9 h29 l4.5 9 v14 a2 2 0 0 1 -2 2 H7 a2 2 0 0 1 -2 -2 z" fill="#EBEBEB" stroke="#8E8E8E" stroke-width="1.2"/>
  <rect x="5" y="24" width="38" height="15" rx="1.5" fill="#D6D6D6" stroke="#8E8E8E" stroke-width="1.2"/>
  <rect x="9" y="29" width="24" height="5" fill="#FFFFFF" stroke="#8A8A8A" stroke-width="1"/>
  {bar_rect}
  <circle cx="38" cy="31.5" r="2" fill="#0078D7"/>
  {badge}
</svg>"""


SVGS = {
    # 1. Empty Upright Folder
    (PLACES_DIR, ("folder", "inode-directory", "folder-yellow")): win10_upright_folder_svg(),

    # 2. Upright Folder with Non-Image Files Inside (Sideways-peeking white paper sheets)
    (PLACES_DIR, ("folder-with-contents",)): win10_upright_folder_svg(middle_slot_svg=PAPER_PEEK_SVG),

    # 3. Desktop Folder (Upright Folder + Blue Desktop Monitor on Front Flap)
    (PLACES_DIR, ("user-desktop", "folder-desktop")): win10_upright_folder_svg(front_badge_svg="""
  <rect x="13" y="21" width="14" height="10" rx="0.8" fill="#0078D7" stroke="#004E8C" stroke-width="1"/>
  <rect x="14.3" y="22.3" width="11.4" height="7.4" fill="#40A9FF"/>
  <rect x="18" y="31" width="4" height="2" fill="#666666"/>
  <rect x="15.5" y="33" width="9" height="1.4" fill="#555555"/>
"""),

    # 4. Downloads Folder (Upright Folder + Blue Downward Arrow on Front Flap)
    (PLACES_DIR, ("folder-download", "folder-downloads")): win10_upright_folder_svg(front_badge_svg="""
  <path d="M18 20 h4.5 v6 h4 l-6.25 6.5 l-6.25 -6.5 h4 z" fill="#0078D7"/>
  <rect x="14" y="34" width="12.5" height="1.8" fill="#0078D7"/>
"""),

    # 5. Documents Folder (Upright Folder + White Document Page on Front Flap)
    (PLACES_DIR, ("folder-documents", "folder-text")): win10_upright_folder_svg(front_badge_svg="""
  <rect x="14.5" y="20" width="11.5" height="14.5" fill="#FFFFFF" stroke="#0078D7" stroke-width="1.2"/>
  <rect x="16.5" y="22.2" width="7.5" height="1.8" fill="#0078D7"/>
  <line x1="16.5" y1="26.2" x2="24" y2="26.2" stroke="#0078D7" stroke-width="1.2"/>
  <line x1="16.5" y1="29" x2="24" y2="29" stroke="#0078D7" stroke-width="1.2"/>
  <line x1="16.5" y1="31.8" x2="22" y2="31.8" stroke="#0078D7" stroke-width="1.2"/>
"""),

    # 6. Pictures Folder (Upright Folder + Blue Mountain Landscape on Front Flap)
    (PLACES_DIR, ("folder-pictures", "folder-images")): win10_upright_folder_svg(front_badge_svg="""
  <rect x="13.5" y="20.5" width="13.5" height="13" fill="#0078D7" stroke="#004E8C" stroke-width="1.1"/>
  <circle cx="17" cy="24" r="1.6" fill="#FFF176"/>
  <polygon points="14.5,32.5 18.5,27.5 21,30 23.8,26.5 26,32.5" fill="#FFFFFF"/>
"""),

    # 7. Music Folder (Upright Folder + Red Double Note on Front Flap)
    (PLACES_DIR, ("folder-music", "folder-sound")): win10_upright_folder_svg(front_badge_svg="""
  <path d="M18.5 31.5 V22 l7.5 -1.6 v9.5" fill="none" stroke="#D81B60" stroke-width="1.9"/>
  <circle cx="17" cy="31.8" r="2.3" fill="#D81B60"/>
  <circle cx="24.5" cy="29.8" r="2.3" fill="#D81B60"/>
"""),

    # 8. Videos Folder (Upright Folder + Purple Play Badge on Front Flap)
    (PLACES_DIR, ("folder-videos", "folder-video")): win10_upright_folder_svg(front_badge_svg="""
  <rect x="13.5" y="21" width="13.5" height="12" rx="1" fill="#7B1FA2" stroke="#4A148C" stroke-width="1.1"/>
  <polygon points="18.5,24 23.5,27 18.5,30" fill="#FFFFFF"/>
"""),

    # 9. User Home Folder
    (PLACES_DIR, ("user-home", "folder-home")): win10_upright_folder_svg(front_badge_svg="""
  <path d="M20.2 20.5 l-6.8 6 h2 v7.2 h9.6 v-7.2 h2 z" fill="#0078D7"/>
"""),

    # 10. This PC / Computer
    (PLACES_DIR, ("computer", "computer-laptop", "folder-root")): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48" width="48" height="48">
  <rect x="5" y="10" width="28" height="20" rx="1" fill="#0078D7" stroke="#004E8C" stroke-width="1.4"/>
  <rect x="7" y="12" width="24" height="16" fill="#40A9FF"/>
  <rect x="15" y="30" width="8" height="4" fill="#8E8E8E"/>
  <rect x="10" y="34" width="18" height="2.2" fill="#707070"/>
  <rect x="31" y="14" width="12" height="23" rx="1" fill="#5A5A5A" stroke="#383838" stroke-width="1.2"/>
  <rect x="33.5" y="17.5" width="7" height="2" fill="#A6A6A6"/>
  <circle cx="37" cy="31" r="1.8" fill="#40A9FF"/>
</svg>""",

    # 11. Local Disk (C:) - System Drive
    (DEVICES_DIR, ("drive-harddisk-system", "drive-harddisk-root", "drive-c")): win10_drive_svg(fill_width=16, is_system=True),

    # 12. Local Disk (D:) - Empty Data Drive
    (DEVICES_DIR, ("drive-harddisk", "drive-d")): win10_drive_svg(fill_width=2, is_system=False),

    # 13. USB / Removable Drive (E:, F:, ...)
    (DEVICES_DIR, ("drive-removable-media", "drive-removable-media-usb", "drive-removable-media-usb-pendrive", "multimedia-player", "phone")): win10_drive_svg(fill_width=11, is_usb=True),

    # 14. Network
    (PLACES_DIR, ("network-workgroup", "network-server", "folder-remote", "folder-network")): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48" width="48" height="48">
  <circle cx="24" cy="22" r="12" fill="#2997FF" stroke="#005A9E" stroke-width="1.4"/>
  <ellipse cx="24" cy="22" rx="5.5" ry="12" fill="none" stroke="#FFFFFF" stroke-width="1.2"/>
  <line x1="12" y1="22" x2="36" y2="22" stroke="#FFFFFF" stroke-width="1.2"/>
  <rect x="10" y="36" width="28" height="2.5" fill="#777777"/>
  <rect x="22" y="33" width="4" height="4" fill="#777777"/>
</svg>""",

    # 15. Recycle Bin
    (PLACES_DIR, ("user-trash", "user-trash-full")): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48" width="48" height="48">
  <path d="M13 14 h22 l-2 25 H15 z" fill="#DCEAF7" stroke="#5C7C99" stroke-width="1.4"/>
  <rect x="11" y="10" width="26" height="4" rx="1" fill="#9BB8D3" stroke="#5C7C99" stroke-width="1.2"/>
  <line x1="19" y1="19" x2="20" y2="34" stroke="#5C7C99" stroke-width="1.4"/>
  <line x1="24" y1="19" x2="24" y2="34" stroke="#5C7C99" stroke-width="1.4"/>
  <line x1="29" y1="19" x2="28" y2="34" stroke="#5C7C99" stroke-width="1.4"/>
</svg>""",

    # ================= RIBBON & NAVIGATION BAR ICONS (32x32 uniform) =================
    (ACTIONS_DIR, ("edit-paste",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="4" y="5" width="15" height="21" rx="1" fill="#D7A15C" stroke="#9B6B2F" stroke-width="1.2"/>
  <rect x="8" y="3" width="7" height="4" rx="1" fill="#A6A6A6" stroke="#666666" stroke-width="1"/>
  <rect x="12" y="10" width="15" height="18" fill="#FFFFFF" stroke="#0078D7" stroke-width="1.3"/>
  <line x1="15" y1="15" x2="24" y2="15" stroke="#0078D7" stroke-width="1.3"/>
  <line x1="15" y1="19" x2="24" y2="19" stroke="#0078D7" stroke-width="1.3"/>
  <line x1="15" y1="23" x2="21" y2="23" stroke="#0078D7" stroke-width="1.3"/>
</svg>""",

    (ACTIONS_DIR, ("edit-cut",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <circle cx="10" cy="23" r="4" fill="none" stroke="#0078D7" stroke-width="2"/>
  <circle cx="22" cy="23" r="4" fill="none" stroke="#0078D7" stroke-width="2"/>
  <line x1="13" y1="20" x2="24" y2="5" stroke="#444444" stroke-width="2.2"/>
  <line x1="19" y1="20" x2="8" y2="5" stroke="#444444" stroke-width="2.2"/>
</svg>""",

    (ACTIONS_DIR, ("edit-copy",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="5" y="4" width="15" height="19" fill="#FFFFFF" stroke="#777777" stroke-width="1.3"/>
  <rect x="11" y="9" width="15" height="19" fill="#FFFFFF" stroke="#0078D7" stroke-width="1.4"/>
  <line x1="14" y1="14" x2="23" y2="14" stroke="#0078D7" stroke-width="1.3"/>
  <line x1="14" y1="18" x2="23" y2="18" stroke="#0078D7" stroke-width="1.3"/>
  <line x1="14" y1="22" x2="20" y2="22" stroke="#0078D7" stroke-width="1.3"/>
</svg>""",

    (ACTIONS_DIR, ("edit-copy-path",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="6" y="4" width="20" height="24" fill="#FFFFFF" stroke="#0078D7" stroke-width="1.4"/>
  <path d="M11 21 l10 -10" stroke="#0078D7" stroke-width="2"/>
  <circle cx="11" cy="21" r="2" fill="#0078D7"/>
  <circle cx="21" cy="11" r="2" fill="#0078D7"/>
</svg>""",

    (ACTIONS_DIR, ("edit-delete", "user-trash-symbolic")): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <line x1="7" y1="7" x2="25" y2="25" stroke="#E81123" stroke-width="3.2" stroke-linecap="square"/>
  <line x1="25" y1="7" x2="7" y2="25" stroke="#E81123" stroke-width="3.2" stroke-linecap="square"/>
</svg>""",

    (ACTIONS_DIR, ("edit-rename",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="3" y="10" width="26" height="12" fill="#FFFFFF" stroke="#0078D7" stroke-width="1.4"/>
  <rect x="6" y="13" width="12" height="6" fill="#0078D7"/>
  <line x1="22" y1="7" x2="22" y2="25" stroke="#222222" stroke-width="1.8"/>
  <line x1="19" y1="7" x2="25" y2="7" stroke="#222222" stroke-width="1.8"/>
  <line x1="19" y1="25" x2="25" y2="25" stroke="#222222" stroke-width="1.8"/>
</svg>""",

    (ACTIONS_DIR, ("folder-new",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <path d="M5 5 h8 l2 2 H25 v20 H5 z" fill="#F0AD28" stroke="#B8730A" stroke-width="1"/>
  <path d="M5 9 l17 1.5 v16.5 L5 25.5 z" fill="#FCD452" stroke="#D19214" stroke-width="1"/>
  <circle cx="24" cy="10" r="5.5" fill="#FFF176" stroke="#E68A00" stroke-width="1.2"/>
  <line x1="24" y1="6.5" x2="24" y2="13.5" stroke="#D87000" stroke-width="1.8"/>
  <line x1="20.5" y1="10" x2="27.5" y2="10" stroke="#D87000" stroke-width="1.8"/>
</svg>""",

    (ACTIONS_DIR, ("document-new",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="6" y="4" width="18" height="24" fill="#FFFFFF" stroke="#0078D7" stroke-width="1.4"/>
  <circle cx="23" cy="10" r="5.5" fill="#E5F3FF" stroke="#0078D7" stroke-width="1.2"/>
  <line x1="23" y1="6.5" x2="23" y2="13.5" stroke="#0078D7" stroke-width="1.8"/>
  <line x1="19.5" y1="10" x2="26.5" y2="10" stroke="#0078D7" stroke-width="1.8"/>
</svg>""",

    (ACTIONS_DIR, ("document-properties",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="5" y="4" width="21" height="24" fill="#FFFFFF" stroke="#555555" stroke-width="1.4"/>
  <rect x="8" y="8" width="5" height="5" fill="#0078D7"/>
  <line x1="15" y1="10.5" x2="23" y2="10.5" stroke="#444444" stroke-width="1.5"/>
  <line x1="8" y1="17" x2="23" y2="17" stroke="#444444" stroke-width="1.5"/>
  <line x1="8" y1="22" x2="19" y2="22" stroke="#444444" stroke-width="1.5"/>
</svg>""",

    (ACTIONS_DIR, ("utilities-terminal",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="3" y="5" width="26" height="22" rx="1" fill="#012456" stroke="#0078D7" stroke-width="1.4"/>
  <polyline points="8,11 14,16 8,21" fill="none" stroke="#FFFFFF" stroke-width="2.2"/>
  <line x1="16" y1="21" x2="23" y2="21" stroke="#FFFFFF" stroke-width="2.2"/>
</svg>""",

    (ACTIONS_DIR, ("view-list-icons",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="5" y="5" width="9" height="9" fill="#FDD862" stroke="#D79B21" stroke-width="1.2"/>
  <rect x="18" y="5" width="9" height="9" fill="#FDD862" stroke="#D79B21" stroke-width="1.2"/>
  <rect x="5" y="18" width="9" height="9" fill="#0078D7" stroke="#005A9E" stroke-width="1.2"/>
  <rect x="18" y="18" width="9" height="9" fill="#0078D7" stroke="#005A9E" stroke-width="1.2"/>
</svg>""",

    (ACTIONS_DIR, ("view-list-details",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="4" y="6" width="5" height="4" fill="#0078D7"/>
  <line x1="12" y1="8" x2="27" y2="8" stroke="#333333" stroke-width="2"/>
  <rect x="4" y="14" width="5" height="4" fill="#FDD862" stroke="#D79B21" stroke-width="0.8"/>
  <line x1="12" y1="16" x2="27" y2="16" stroke="#333333" stroke-width="2"/>
  <rect x="4" y="22" width="5" height="4" fill="#0078D7"/>
  <line x1="12" y1="24" x2="27" y2="24" stroke="#333333" stroke-width="2"/>
</svg>""",

    (ACTIONS_DIR, ("view-list-tree", "view-list-compact")): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="4" y="7" width="4" height="4" fill="#FDD862" stroke="#D79B21" stroke-width="0.8"/>
  <line x1="10" y1="9" x2="15" y2="9" stroke="#333333" stroke-width="2"/>
  <rect x="18" y="7" width="4" height="4" fill="#0078D7"/>
  <line x1="24" y1="9" x2="29" y2="9" stroke="#333333" stroke-width="2"/>
  <rect x="4" y="16" width="4" height="4" fill="#0078D7"/>
  <line x1="10" y1="18" x2="15" y2="18" stroke="#333333" stroke-width="2"/>
  <rect x="18" y="16" width="4" height="4" fill="#FDD862" stroke="#D79B21" stroke-width="0.8"/>
  <line x1="24" y1="18" x2="29" y2="18" stroke="#333333" stroke-width="2"/>
</svg>""",

    (ACTIONS_DIR, ("view-sort",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <path d="M10 6 v18 m-4 -4 l4 4 l4 -4" fill="none" stroke="#0078D7" stroke-width="2"/>
  <path d="M22 26 V8 m-4 4 l4 -4 l4 4" fill="none" stroke="#444444" stroke-width="2"/>
</svg>""",

    (ACTIONS_DIR, ("view-group",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <line x1="4" y1="7" x2="28" y2="7" stroke="#0078D7" stroke-width="2"/>
  <rect x="6" y="10" width="6" height="5" fill="#FDD862" stroke="#D79B21" stroke-width="0.8"/>
  <rect x="15" y="10" width="6" height="5" fill="#FDD862" stroke="#D79B21" stroke-width="0.8"/>
  <line x1="4" y1="19" x2="28" y2="19" stroke="#0078D7" stroke-width="2"/>
  <rect x="6" y="22" width="6" height="5" fill="#0078D7"/>
  <rect x="15" y="22" width="6" height="5" fill="#0078D7"/>
</svg>""",

    (ACTIONS_DIR, ("view-visible",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <path d="M3 16 Q16 6 29 16 Q16 26 3 16 Z" fill="#E5F3FF" stroke="#0078D7" stroke-width="1.6"/>
  <circle cx="16" cy="16" r="4" fill="#0078D7"/>
</svg>""",

    (ACTIONS_DIR, ("view-right-new",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="4" y="5" width="24" height="22" fill="#FFFFFF" stroke="#555555" stroke-width="1.4"/>
  <rect x="19" y="6" width="8" height="20" fill="#0078D7"/>
</svg>""",

    (ACTIONS_DIR, ("edit-select-all", "edit-select-invert")): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <rect x="5" y="5" width="22" height="22" fill="#E5F3FF" stroke="#0078D7" stroke-width="1.5" stroke-dasharray="3,2"/>
  <rect x="9" y="9" width="6" height="6" fill="#0078D7"/>
  <rect x="17" y="9" width="6" height="6" fill="#0078D7"/>
  <rect x="9" y="17" width="6" height="6" fill="#0078D7"/>
  <rect x="17" y="17" width="6" height="6" fill="#0078D7"/>
</svg>""",

    (ACTIONS_DIR, ("go-previous",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <line x1="26" y1="16" x2="7" y2="16" stroke="#333333" stroke-width="2.2"/>
  <polyline points="15,8 7,16 15,24" fill="none" stroke="#333333" stroke-width="2.2"/>
</svg>""",

    (ACTIONS_DIR, ("go-next",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <line x1="6" y1="16" x2="25" y2="16" stroke="#333333" stroke-width="2.2"/>
  <polyline points="17,8 25,16 17,24" fill="none" stroke="#333333" stroke-width="2.2"/>
</svg>""",

    (ACTIONS_DIR, ("go-up",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <line x1="16" y1="26" x2="16" y2="7" stroke="#333333" stroke-width="2.2"/>
  <polyline points="8,15 16,7 24,15" fill="none" stroke="#333333" stroke-width="2.2"/>
</svg>""",

    (ACTIONS_DIR, ("view-refresh",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <path d="M24 11 A9 9 0 1 0 25 18" fill="none" stroke="#333333" stroke-width="2.2"/>
  <polyline points="20,11 25,11 25,6" fill="none" stroke="#333333" stroke-width="2.2"/>
</svg>""",

    (ACTIONS_DIR, ("edit-find",)): """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="32" height="32">
  <circle cx="14" cy="14" r="7" fill="none" stroke="#333333" stroke-width="2.2"/>
  <line x1="19" y1="19" x2="26" y2="26" stroke="#0078D7" stroke-width="2.5"/>
</svg>""",
}

for (target_dir, names), svg_content in SVGS.items():
    for name in names:
        for d in (target_dir, PLACES_DIR, ACTIONS_DIR, DEVICES_DIR):
            (d / f"{name}.svg").write_text(svg_content)

print(f"[✓] Generated Windows 10 Upright 3D SVG icon theme at {ICON_DIR}")
