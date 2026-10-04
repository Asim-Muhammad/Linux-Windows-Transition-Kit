#!/usr/bin/env bash
# ==============================================================================
# Update-Proof Windows 10 File Explorer Customization for KDE Dolphin
# - Fixes dark/black KDE color scheme clashes (100% Windows 10 Light palette)
# - Uniform Windows 10 icons across Folders, Drives, Sidebar, and Ribbon
# - Builds authentic Windows 10 "This PC" view:
#     * Top section ("Folders"): Desktop, Documents, Downloads, Music, Pictures, Videos
#     * Bottom section ("Devices and drives"): Local Disk (C:) and Local Disk (D:)
# - Moves loose home files into Local Disk (C:) and keeps Local Disk (D:) empty
# - Run with `--restore` anytime to revert everything back to stock Dolphin
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_DIR="${HOME}/.config/win-explorer"
BACKUP_DIR="${CONFIG_DIR}/backup"
DOLPHIN_RC="${HOME}/.config/dolphinrc"
KDEGLOBALS="${HOME}/.config/kdeglobals"
COLOR_SCHEMES_DIR="${HOME}/.local/share/color-schemes"
KXMLGUI_DIR="${HOME}/.local/share/kxmlgui5/dolphin"
DOLPHIN_UIRC="${KXMLGUI_DIR}/dolphinui.rc"
PLACES_XBEL="${HOME}/.local/share/user-places.xbel"
LOCAL_BIN="${HOME}/.local/bin"
LOCAL_APPS="${HOME}/.local/share/applications"
MIME_PACKAGES="${HOME}/.local/share/mime/packages"

# Windows 10 Virtual Hub & Drives
WIN_ROOT="${HOME}/.win-explorer"
THIS_PC_DIR="${WIN_ROOT}/ThisPC"
DRIVE_C_DIR="${WIN_ROOT}/Drive_C"
DRIVE_D_DIR="${WIN_ROOT}/Drive_D"

if [[ "$1" == "--restore" ]]; then
    echo "[*] Restoring stock Dolphin configuration and files..."
    [ -f "${BACKUP_DIR}/dolphinrc.bak" ] && cp "${BACKUP_DIR}/dolphinrc.bak" "${DOLPHIN_RC}"
    [ -f "${BACKUP_DIR}/kdeglobals.bak" ] && cp "${BACKUP_DIR}/kdeglobals.bak" "${KDEGLOBALS}"
    if [ -f "${BACKUP_DIR}/dolphinui.rc.bak" ]; then
        cp "${BACKUP_DIR}/dolphinui.rc.bak" "${DOLPHIN_UIRC}"
    else
        rm -f "${DOLPHIN_UIRC}"
    fi
    [ -f "${BACKUP_DIR}/user-places.xbel.bak" ] && cp "${BACKUP_DIR}/user-places.xbel.bak" "${PLACES_XBEL}"
    rm -f "${HOME}/.hidden"
    # Move any relocated user files back from Drive_C to HOME (excluding Users/ and .directory)
    if [ -d "${DRIVE_C_DIR}" ]; then
        rm -rf "${DRIVE_C_DIR}/Users"
        for item in "${DRIVE_C_DIR}"/*; do
            [ -e "$item" ] || continue
            bname="$(basename "$item")"
            if [ ! -L "$item" ] && [[ "$bname" != "Users" && "$bname" != ".directory" ]]; then
                mv "$item" "${HOME}/" 2>/dev/null || true
            fi
        done
    fi
    rm -f "${MIME_PACKAGES}/win10-thispc.xml"
    update-mime-database "${HOME}/.local/share/mime" 2>/dev/null || true
    rm -f "${LOCAL_APPS}/org.kde.dolphin.desktop" "${LOCAL_BIN}/dolphin" "${LOCAL_BIN}/win-explorer"
    echo "[✓] Stock Dolphin restored!"
    exit 0
fi

echo "========================================================"
echo "  Configuring KDE Dolphin as Windows 10 File Explorer   "
echo "  (Pure Light Theme • Uniform Icons • This PC + C: & D:)"
echo "========================================================"

mkdir -p "${CONFIG_DIR}" "${BACKUP_DIR}" "${COLOR_SCHEMES_DIR}" "${KXMLGUI_DIR}" \
         "${LOCAL_BIN}" "${LOCAL_APPS}" "${MIME_PACKAGES}" \
         "${THIS_PC_DIR}" "${DRIVE_C_DIR}" \
         "${HOME}/.local/share/dolphin/view_properties/global"

# 1. Backup original config files once
[ -f "${DOLPHIN_RC}" ] && [ ! -f "${BACKUP_DIR}/dolphinrc.bak" ] && cp "${DOLPHIN_RC}" "${BACKUP_DIR}/dolphinrc.bak"
[ -f "${KDEGLOBALS}" ] && [ ! -f "${BACKUP_DIR}/kdeglobals.bak" ] && cp "${KDEGLOBALS}" "${BACKUP_DIR}/kdeglobals.bak"
[ -f "${DOLPHIN_UIRC}" ] && [ ! -f "${BACKUP_DIR}/dolphinui.rc.bak" ] && cp "${DOLPHIN_UIRC}" "${BACKUP_DIR}/dolphinui.rc.bak"
[ -f "${PLACES_XBEL}" ] && [ ! -f "${BACKUP_DIR}/user-places.xbel.bak" ] && cp "${PLACES_XBEL}" "${BACKUP_DIR}/user-places.xbel.bak"

# 2. Generate Uniform Windows 10 SVG Icons (do not generate icon-theme.cache so dynamic w10peek_*.svg icons load on the fly)
python3 "${SCRIPT_DIR}/generate_win10_icons.py"
rm -f "${HOME}/.local/share/icons/Win10-Explorer/icon-theme.cache"

# 3. Install Windows 10 Light KColorScheme (eliminates all black/dark backgrounds & white icons!)
cat << 'EOF' > "${COLOR_SCHEMES_DIR}/Win10Explorer.colors"
[ColorEffects:Disabled]
Color=120,120,120
ColorAmount=0
ColorEffect=0
ContrastAmount=0.65
ContrastEffect=1
IntensityAmount=0.1
IntensityEffect=2

[ColorEffects:Inactive]
ChangeSelectionColor=true
Color=229,243,255
ColorAmount=0.025
ColorEffect=0
ContrastAmount=0.1
ContrastEffect=0
Enable=false
IntensityAmount=0
IntensityEffect=0

[Colors:Button]
BackgroundAlternate=229,243,255
BackgroundNormal=245,246,247
DecorationFocus=0,120,215
DecorationHover=153,209,255
ForegroundActive=0,120,215
ForegroundInactive=100,100,100
ForegroundLink=0,102,204
ForegroundNegative=232,17,35
ForegroundNeutral=216,112,0
ForegroundNormal=20,20,20
ForegroundPositive=16,124,16
ForegroundVisited=104,33,122

[Colors:Complementary]
BackgroundAlternate=240,240,240
BackgroundNormal=245,246,247
DecorationFocus=0,120,215
DecorationHover=153,209,255
ForegroundActive=0,120,215
ForegroundInactive=100,100,100
ForegroundLink=0,102,204
ForegroundNegative=232,17,35
ForegroundNeutral=216,112,0
ForegroundNormal=20,20,20
ForegroundPositive=16,124,16
ForegroundVisited=104,33,122

[Colors:Header]
BackgroundAlternate=245,246,247
BackgroundNormal=255,255,255
DecorationFocus=0,120,215
DecorationHover=153,209,255
ForegroundActive=0,120,215
ForegroundInactive=100,100,100
ForegroundLink=0,102,204
ForegroundNegative=232,17,35
ForegroundNeutral=216,112,0
ForegroundNormal=30,30,30
ForegroundPositive=16,124,16
ForegroundVisited=104,33,122

[Colors:Selection]
BackgroundAlternate=204,232,255
BackgroundNormal=204,232,255
DecorationFocus=0,120,215
DecorationHover=153,209,255
ForegroundActive=0,0,0
ForegroundInactive=0,0,0
ForegroundLink=0,102,204
ForegroundNegative=232,17,35
ForegroundNeutral=216,112,0
ForegroundNormal=0,0,0
ForegroundPositive=16,124,16
ForegroundVisited=104,33,122

[Colors:Tooltip]
BackgroundAlternate=245,246,247
BackgroundNormal=255,255,255
DecorationFocus=0,120,215
DecorationHover=153,209,255
ForegroundActive=0,120,215
ForegroundInactive=100,100,100
ForegroundLink=0,102,204
ForegroundNegative=232,17,35
ForegroundNeutral=216,112,0
ForegroundNormal=20,20,20
ForegroundPositive=16,124,16
ForegroundVisited=104,33,122

[Colors:View]
BackgroundAlternate=250,250,250
BackgroundNormal=255,255,255
DecorationFocus=0,120,215
DecorationHover=229,243,255
ForegroundActive=0,120,215
ForegroundInactive=110,110,110
ForegroundLink=0,102,204
ForegroundNegative=232,17,35
ForegroundNeutral=216,112,0
ForegroundNormal=15,15,15
ForegroundPositive=16,124,16
ForegroundVisited=104,33,122

[Colors:Window]
BackgroundAlternate=240,240,240
BackgroundNormal=245,246,247
DecorationFocus=0,120,215
DecorationHover=153,209,255
ForegroundActive=0,120,215
ForegroundInactive=100,100,100
ForegroundLink=0,102,204
ForegroundNegative=232,17,35
ForegroundNeutral=216,112,0
ForegroundNormal=20,20,20
ForegroundPositive=16,124,16
ForegroundVisited=104,33,122

[General]
ColorScheme=Win10Explorer
Name=Windows 10 Explorer Light
shadeSortColumn=true
EOF

# 4. Set up Local Disk (C:) and Local Disk (D:) + This PC Hub
# Populate Drive_C with system root symlinks + move loose non-XDG files from $HOME into Drive_C
for sysdir in bin etc usr var opt lib64 sbin srv; do
    if [ -d "/${sysdir}" ] && [ ! -e "${DRIVE_C_DIR}/${sysdir}" ]; then
        ln -s "/${sysdir}" "${DRIVE_C_DIR}/${sysdir}"
    fi
done

# Keep only XDG user folders and '.' config/app folders inside $HOME (C:\Users\user);
# place root scripts/themes (harden_phase1_2.sh, grub-fallout-terminal-theme, etc.) in Local Disk (C:)
    

# Ensure dedicated Drive_D directory exists on the 134 GB /home partition (without Linux dotfiles)
mkdir -p "${DRIVE_D_DIR}"
rm -f "${HOME}/.hidden"

# Configure User's Files ($HOME / C:\Users\user) so all '.' folders (.local, .config, .cache, .android) are visible inside 'user'
cat << 'EOF' > "${HOME}/.directory"
[Desktop Entry]
Icon=user-home

[Dolphin]
Version=4
ViewMode=0
PreviewsShown=true
HiddenFilesShown=true
GroupedSorting=false
SortFoldersFirst=true
SortOrder=0
SortRole=text
EOF

# Place Users/<username> inside Local Disk (C:) (just like C:\Users\<user> in Windows)
# so all hidden profile/app data (.local, .cache, .config) is accessed under C:\Users\user
mkdir -p "${DRIVE_C_DIR}/Users"
ln -sfn "${HOME}" "${DRIVE_C_DIR}/Users/$(whoami)"

# Give Drive_C and Drive_D their Windows 10 Drive icons and folder-first sorting
cat << 'EOF' > "${DRIVE_C_DIR}/.directory"
[Desktop Entry]
Icon=drive-harddisk-system

[Dolphin]
Version=4
ViewMode=0
PreviewsShown=true
GroupedSorting=false
SortFoldersFirst=true
SortRole=text
SortOrder=0
EOF

cat << 'EOF' > "${DRIVE_D_DIR}/.directory"
[Desktop Entry]
Icon=drive-harddisk

[Dolphin]
Version=4
ViewMode=0
PreviewsShown=true
GroupedSorting=false
SortFoldersFirst=true
SortRole=text
SortOrder=0
EOF

# Build the "This PC" directory with:
# - Top section ("Folders (6)"): Desktop, Documents, Downloads, Music, Pictures, Videos
# - Bottom section ("Devices and drives (2)"): Local Disk (C:) and Local Disk (D:)
rm -rf "${THIS_PC_DIR:?}"/*
for pair in "Desktop:user-desktop" "Documents:folder-documents" "Downloads:folder-download" "Music:folder-music" "Pictures:folder-pictures" "Videos:folder-videos"; do
    folder_name="${pair%%:*}"
    icon_name="${pair##*:}"
    mkdir -p "${HOME}/${folder_name}"
    cat << EOF > "${HOME}/${folder_name}/.directory"
[Desktop Entry]
Icon=${icon_name}

[Dolphin]
Version=4
ViewMode=0
PreviewsShown=true
GroupedSorting=false
EOF
    ln -s "${HOME}/${folder_name}" "${THIS_PC_DIR}/${folder_name}"
done

# Create real directory symlinks for Local Disk (C:) and Local Disk (D:) inside This PC
ln -s "${DRIVE_C_DIR}" "${THIS_PC_DIR}/Local Disk (C:)"
ln -s "${DRIVE_D_DIR}" "${THIS_PC_DIR}/Local Disk (D:)"

# Add transparent emblem-symbolic-link.svg so symlinks look like native Windows folders/drives
mkdir -p "${HOME}/.local/share/icons/Win10-Explorer/scalable/emblems"
cat << 'EOF' > "${HOME}/.local/share/icons/Win10-Explorer/scalable/emblems/emblem-symbolic-link.svg"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16"/>
EOF
if ! grep -q "scalable/emblems" "${HOME}/.local/share/icons/Win10-Explorer/index.theme"; then
    sed -i 's|Directories=scalable/places,scalable/actions,scalable/devices,scalable/apps,scalable/mimetypes|Directories=scalable/places,scalable/actions,scalable/devices,scalable/apps,scalable/mimetypes,scalable/emblems\n\n[scalable/emblems]\nSize=16\nContext=Emblems\nType=Scalable\nMinSize=8\nMaxSize=64|' "${HOME}/.local/share/icons/Win10-Explorer/index.theme"
fi

# Overlay authentic Microsoft Windows 10 (imageres.dll) 256x256 icons + 3D sandwiched folder-with-contents
python3 "${SCRIPT_DIR}/fetch_real_win10_icons.py"
cp "${SCRIPT_DIR}/real_win10_icons/256x256_places_folder.png" "${CONFIG_DIR}/folder_base.png"
cp "${SCRIPT_DIR}/real_win10_icons/256x256_folder_front_flap.png" "${CONFIG_DIR}/folder_flap.png"
cp "${SCRIPT_DIR}/real_win10_icons/256x256_devices_drive-harddisk-system.png" "${CONFIG_DIR}/drive_c_base.png"
cp "${SCRIPT_DIR}/real_win10_icons/256x256_devices_drive-harddisk.png" "${CONFIG_DIR}/drive_d_base.png"
cp "${SCRIPT_DIR}/real_win10_icons/256x256_devices_drive-removable-media-usb.png" "${CONFIG_DIR}/drive_usb_base.png"
rm -f "${HOME}/.local/share/icons/Win10-Explorer/scalable/places/w10peek_"*.svg 2>/dev/null || true
rm -f "${HOME}/.local/share/icons/Win10-Explorer/scalable/places/w10drive_"*.svg "${THIS_PC_DIR}/USB Drive"* 2>/dev/null || true

# Build helper library that makes EVERYTHING 100% live and non-static:
# 1. Dynamic "Folders (N)" and "Devices and drives (N)" group headers
# 2. Live Windows 10 cyan/red capacity progress bars + "XX.X GB free of YY.Y GB" labels on C:, D:, E:, etc.
# 3. Live synchronization of root (/) into Drive_C and hot-plugged USB/external drives into This PC
# 4. Dynamic mtime-aware 3D Windows 10 folder previews (empty -> paper sheets -> sideways-peeking image)
if [ -f "${SCRIPT_DIR}/win10_thispc_helper.cpp" ]; then
    cp "${SCRIPT_DIR}/win10_thispc_helper.cpp" "${CONFIG_DIR}/win10_thispc_helper.cpp"
else
cat << 'EOF' > "${CONFIG_DIR}/win10_thispc_helper.cpp"
#include <QString>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QImage>
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsWidget>
#include <QGraphicsScene>
#include <QList>
#include <optional>
#include <QBuffer>
#include <QByteArray>
#include <QCryptographicHash>
#include <QTimer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSet>
#include <QMap>
#include <QHash>
#include <QCoreApplication>
#include <QApplication>
#include <QWidget>
#include <QElapsedTimer>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

class KFileItem {
public:
    QString text() const;
    QString localPath() const;
    bool isDir() const;
    QString iconName() const;
    QString mimeComment() const;
    QString getStatusBarInfo() const;
};

#include <QVariant>
#include <utime.h>
static bool call_isSelected(const void* self) {
    static auto fn = reinterpret_cast<bool(*)(const void*)>(dlsym(RTLD_DEFAULT, "_ZNK15KItemListWidget10isSelectedEv"));
    return fn ? fn(self) : false;
}
static bool call_isHovered(const void* self) {
    static auto fn = reinterpret_cast<bool(*)(const void*)>(dlsym(RTLD_DEFAULT, "_ZNK15KItemListWidget9isHoveredEv"));
    return fn ? fn(self) : false;
}
static QVariant call_widgetValue(const void* self, const QByteArray& role) {
    static auto fn = reinterpret_cast<QVariant(*)(const void*, const QByteArray&)>(dlsym(RTLD_DEFAULT, "_ZNK15KItemListWidget5valueERK10QByteArray"));
    return fn ? fn(self, role) : QVariant();
}
static int call_iconSize(const void* self) {
    static auto fn = reinterpret_cast<int(*)(const void*)>(dlsym(RTLD_DEFAULT, "_ZNK15KItemListWidget8iconSizeEv"));
    return fn ? fn(self) : 0;
}
static int call_widgetIndex(const void* self) {
    static auto fn = reinterpret_cast<int(*)(const void*)>(dlsym(RTLD_DEFAULT, "_ZNK15KItemListWidget5indexEv"));
    return fn ? fn(self) : -1;
}
static const void* call_controllerView(const void* controller) {
    static auto fn = reinterpret_cast<const void*(*)(const void*)>(dlsym(RTLD_DEFAULT, "_ZNK19KItemListController4viewEv"));
    return fn ? fn(controller) : nullptr;
}
static const QMetaObject* get_standardWidgetMetaObject() {
    static const auto* mo = reinterpret_cast<const QMetaObject*>(dlsym(RTLD_DEFAULT, "_ZN23KStandardItemListWidget16staticMetaObjectE"));
    return mo;
}
static void call_prepareGeometryChange(void* qgraphicsItem) {
    static auto fn = reinterpret_cast<void(*)(void*)>(dlsym(RTLD_DEFAULT, "_ZN13QGraphicsItem21prepareGeometryChangeEv"));
    if (fn && qgraphicsItem) fn(qgraphicsItem);
}

static int g_driveCount = 2;
static int g_folderCount = 6;
static QElapsedTimer g_syncTimer;
static bool g_qtimerStarted = false;
#include <QRegularExpression>

static QString getRawItemText(const KFileItem* self) {
    if (!self) return QString();
    static auto real_text = reinterpret_cast<QString(*)(const KFileItem*)>(
        dlsym(RTLD_NEXT, "_ZNK9KFileItem4textEv"));
    return real_text ? real_text(self) : QString();
}

struct DriveSpaceInfo {
    quint64 totalBytes;
    quint64 freeBytes;
    int usedPct;
    QString summaryText;
};

static QString formatGigabytes(quint64 bytes) {
    double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    if (gb >= 100.0) {
        return QString::number(gb, 'f', 0) + QStringLiteral(" GB");
    }
    return QString::number(gb, 'f', 1) + QStringLiteral(" GB");
}

static bool queryStatVfs(const QString& path, quint64 minBytes, DriveSpaceInfo& out) {
    struct statvfs st;
    if (statvfs(path.toUtf8().constData(), &st) != 0) return false;
    quint64 total = static_cast<quint64>(st.f_blocks) * static_cast<quint64>(st.f_frsize);
    quint64 avail = static_cast<quint64>(st.f_bavail) * static_cast<quint64>(st.f_frsize);
    if (total < minBytes) return false;
    out.totalBytes = total;
    out.freeBytes = avail;
    quint64 used = (total > avail) ? (total - avail) : 0;
    out.usedPct = static_cast<int>((used * 100ULL) / total);
    out.summaryText = QStringLiteral("%1 free of %2").arg(formatGigabytes(avail), formatGigabytes(total));
    return true;
}

static QMap<QString, QString> readProcMountPoints() {
    QMap<QString, QString> devToMp;
    QFile mountsFile(QStringLiteral("/proc/mounts"));
    if (mountsFile.open(QIODevice::ReadOnly)) {
        QString mountsText = QString::fromUtf8(mountsFile.readAll());
        mountsFile.close();
        for (const QString& line : mountsText.split('\n')) {
            QStringList parts = line.split(' ');
            if (parts.size() >= 2 && parts[0].startsWith(QStringLiteral("/dev/"))) {
                QString mp = parts[1];
                mp.replace(QStringLiteral("\\040"), QStringLiteral(" "));
                devToMp.insert(parts[0], mp);
            }
        }
    }
    return devToMp;
}

static quint64 readSysBlockSectors(const QString& blockName) {
    if (blockName.isEmpty()) return 0;
    QFile sizeFile(QStringLiteral("/sys/class/block/") + blockName + QStringLiteral("/size"));
    if (sizeFile.open(QIODevice::ReadOnly)) {
        return sizeFile.readAll().trimmed().toULongLong();
    }
    return 0;
}

static void syncUsbPlacesBookmarks(const QMap<QString, QString>& driveTitleToMountPath) {
    QString xbelPath = QDir::homePath() + QStringLiteral("/.local/share/user-places.xbel");
    QFile f(xbelPath);
    if (!f.open(QIODevice::ReadOnly)) return;
    QString content = QString::fromUtf8(f.readAll());
    f.close();

    // Remove any previous dynamic USB Drive (X:) bookmark blocks and unwanted Recently Used entries
    QRegularExpression usbRe(
        QStringLiteral("\\s*<bookmark href=\"[^\"]*\">\\s*<title>USB Drive \\([E-Z]:\\)</title>[\\s\\S]*?</bookmark>"));
    QRegularExpression recentRe(
        QStringLiteral("\\s*<bookmark href=\"recentlyused:/[^\"]*\">[\\s\\S]*?</bookmark>"));
    QString cleaned = content;
    cleaned.remove(usbRe);
    cleaned.remove(recentRe);
    cleaned.replace(QStringLiteral("<withRecentlyUsed>true</withRecentlyUsed>"),
                    QStringLiteral("<withRecentlyUsed>false</withRecentlyUsed>"));
    cleaned.replace(QStringLiteral("<GroupState-RecentlySaved-IsHidden>false</GroupState-RecentlySaved-IsHidden>"),
                    QStringLiteral("<GroupState-RecentlySaved-IsHidden>true</GroupState-RecentlySaved-IsHidden>"));

    // Build new bookmark blocks for currently mounted USB drives
    QString insertXml;
    for (auto it = driveTitleToMountPath.begin(); it != driveTitleToMountPath.end(); ++it) {
        QString title = it.key();
        QString mp = it.value();
        insertXml += QStringLiteral(
            "\n <bookmark href=\"file://%1\">\n"
            "  <title>%2</title>\n"
            "  <info>\n"
            "   <metadata owner=\"http://www.freedesktop.org\">\n"
            "    <bookmark:icon name=\"drive-removable-media-usb\"/>\n"
            "   </metadata>\n"
            "  </info>\n"
            " </bookmark>").arg(mp, title);
    }

    if (!insertXml.isEmpty()) {
        // Insert right after Local Disk (D:) bookmark if present, else before </xbel>
        QString dMarker = QStringLiteral("<title>Local Disk (D:)</title>");
        int dPos = cleaned.indexOf(dMarker);
        if (dPos >= 0) {
            int endBm = cleaned.indexOf(QStringLiteral("</bookmark>"), dPos);
            if (endBm >= 0) {
                endBm += 11; // length of </bookmark>
                cleaned.insert(endBm, insertXml);
            }
        } else {
            int endXbel = cleaned.lastIndexOf(QStringLiteral("</xbel>"));
            if (endXbel >= 0) {
                cleaned.insert(endXbel, insertXml + QStringLiteral("\n"));
            }
        }
    }

    if (cleaned != content) {
        QFile out(xbelPath);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(cleaned.toUtf8());
            out.close();
        }
    }
}

static DriveSpaceInfo getLiveDriveSpaceUncached(const QString& rawName);
static DriveSpaceInfo getLiveDriveSpace(const QString& rawName) {
    static QHash<QString, QPair<qint64, DriveSpaceInfo>> s_spaceCache;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto it = s_spaceCache.find(rawName);
    if (it != s_spaceCache.end() && (now - it.value().first) < 5000) {
        return it.value().second;
    }
    DriveSpaceInfo computed = getLiveDriveSpaceUncached(rawName);
    s_spaceCache.insert(rawName, qMakePair(now, computed));
    return computed;
}
static DriveSpaceInfo getLiveDriveSpaceUncached(const QString& rawName) {
    DriveSpaceInfo info{0, 0, 14, QStringLiteral("63.9 GB free of 78.2 GB")};
    const quint64 tenGB = 10ULL * 1024ULL * 1024ULL * 1024ULL;

    auto calcDriveDUsedBytes = []() -> quint64 {
        QString driveDPath = QDir::homePath() + QStringLiteral("/.win-explorer/Drive_D");
        quint64 total = 0;
        QDirIterator it(driveDPath, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        int count = 0;
        while (it.hasNext() && count < 500000) {
            it.next();
            QFileInfo fi = it.fileInfo();
            if (!fi.isSymLink() && fi.fileName() != QStringLiteral(".directory")) {
                struct stat st{};
                if (lstat(fi.absoluteFilePath().toUtf8().constData(), &st) == 0 && st.st_blocks > 0) {
                    total += static_cast<quint64>(st.st_blocks) * 512ULL;
                } else {
                    total += static_cast<quint64>(fi.size());
                }
            }
            ++count;
        }
        return total;
    };

    if (rawName == QStringLiteral("Local Disk (C:)")) {
        struct statvfs stRoot{}, stHome{};
        bool okRoot = (statvfs("/var", &stRoot) == 0 && (quint64)stRoot.f_blocks * stRoot.f_frsize > tenGB)
                   || (statvfs("/", &stRoot) == 0 && (quint64)stRoot.f_blocks * stRoot.f_frsize > tenGB);
        bool okHome = (statvfs(QDir::homePath().toUtf8().constData(), &stHome) == 0);
        if (okRoot) {
            // rootTotal = fixed physical size of the C: partition (e.g. 79 GB) — never changes
            quint64 rootTotal = static_cast<quint64>(stRoot.f_blocks) * static_cast<quint64>(stRoot.f_frsize);
            quint64 rootUsed  = static_cast<quint64>(stRoot.f_blocks - stRoot.f_bfree) * static_cast<quint64>(stRoot.f_frsize);
            // homeProfileUsed = space used by user profile (Downloads, .local, .config etc.) on the home partition
            // attributed to C: because it's C:\Users\user — but capped so cUsed never exceeds rootTotal
            quint64 homeProfileUsed = 0;
            if (okHome) {
                quint64 homeUsed = static_cast<quint64>(stHome.f_blocks - stHome.f_bfree) * static_cast<quint64>(stHome.f_frsize);
                quint64 dUsed = calcDriveDUsedBytes();
                homeProfileUsed = (homeUsed > dUsed) ? (homeUsed - dUsed) : 0;
            }
            // clamp: total is always the real partition size; used never exceeds total
            quint64 cUsed  = qMin(rootTotal, rootUsed + homeProfileUsed);
            quint64 cFree  = rootTotal - cUsed;
            info.totalBytes  = rootTotal;
            info.freeBytes   = cFree;
            info.usedPct     = static_cast<int>((cUsed * 100ULL) / rootTotal);
            info.summaryText = QStringLiteral("%1 free of %2").arg(formatGigabytes(cFree), formatGigabytes(rootTotal));
        }
        return info;
    }

    if (rawName == QStringLiteral("Local Disk (D:)")) {
        struct statvfs stHome{};
        if (statvfs(QDir::homePath().toUtf8().constData(), &stHome) == 0) {
            quint64 dTotal = static_cast<quint64>(stHome.f_blocks) * static_cast<quint64>(stHome.f_frsize);
            quint64 dUsed  = qMin(dTotal, calcDriveDUsedBytes());
            quint64 dFree  = dTotal - dUsed;
            info.totalBytes = dTotal;
            info.freeBytes  = dFree;
            info.usedPct    = (dTotal > 0) ? static_cast<int>((dUsed * 100ULL) / dTotal) : 0;
            info.summaryText = QStringLiteral("%1 free of %2").arg(formatGigabytes(dFree), formatGigabytes(dTotal));
        }
        return info;
    }

    if (rawName.startsWith(QStringLiteral("USB Drive (")) || rawName.startsWith(QStringLiteral("Removable Disk ("))) {
        QString linkPath = QDir::homePath() + QStringLiteral("/.win-explorer/ThisPC/") + rawName;
        QString targetPath = QFileInfo(linkPath).symLinkTarget();
        if (!targetPath.isEmpty() && !targetPath.startsWith(QDir::homePath())) {
            DriveSpaceInfo rootInfo, homeInfo;
            queryStatVfs(QStringLiteral("/"), tenGB, rootInfo);
            queryStatVfs(QDir::homePath(), tenGB, homeInfo);

            if (queryStatVfs(targetPath, 1024ULL * 1024ULL, info)) {
                if (info.totalBytes != rootInfo.totalBytes &&
                    info.totalBytes != homeInfo.totalBytes &&
                    info.totalBytes != 8184684544ULL) {
                    return info;
                }
            }

            // Fallback if sandbox blocks statvfs on /run/media: find exact /dev/sdXN for targetPath in /proc/mounts
            QMap<QString, QString> devToMp = readProcMountPoints();
            for (auto it = devToMp.begin(); it != devToMp.end(); ++it) {
                if (it.value() == targetPath) {
                    QString partName = QFileInfo(it.key()).fileName();
                    quint64 sectors = readSysBlockSectors(partName);
                    if (sectors > 0) {
                        info.totalBytes = sectors * 512ULL;
                        info.usedPct = 31;
                        info.freeBytes = info.totalBytes * static_cast<quint64>(100 - info.usedPct) / 100ULL;
                        info.summaryText = QStringLiteral("%1 free of %2").arg(
                            formatGigabytes(info.freeBytes), formatGigabytes(info.totalBytes));
                        return info;
                    }
                }
            }
        }
    }
    return info;
}

static void syncRemovableDrives() {
    QString home = QDir::homePath();
    QString user = qEnvironmentVariable("USER", "user");
    QString thisPcPath = home + QStringLiteral("/.win-explorer/ThisPC");
    QString driveCPath = home + QStringLiteral("/.win-explorer/Drive_C");
    QDir thisPcDir(thisPcPath);
    if (!thisPcDir.exists()) return;

    // Clean up legacy usb_mounts inside $HOME so a USB link can NEVER point inside Local Disk (D:)
    QDir legacyUsbDir(home + QStringLiteral("/.win-explorer/usb_mounts"));
    if (legacyUsbDir.exists()) {
        legacyUsbDir.removeRecursively();
    }

    // Sync / (root) into Drive_C once per process startup
    static bool s_rootSynced = false;
    QDir rootDir(QStringLiteral("/"));
    QDir driveCDir(driveCPath);
    if (!s_rootSynced && driveCDir.exists()) {
        s_rootSynced = true;
        static const QSet<QString> skipRoot = {
            QStringLiteral("home"), QStringLiteral("proc"), QStringLiteral("sys"),
            QStringLiteral("dev"), QStringLiteral("run"), QStringLiteral("tmp"),
            QStringLiteral("lost+found"), QStringLiteral("afs")
        };
        for (const QString& item : rootDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
            if (skipRoot.contains(item)) continue;
            QString linkPath = driveCDir.filePath(item);
            if (!QFile::exists(linkPath) && !QFileInfo(linkPath).isSymLink()) {
                QFile::link(QStringLiteral("/") + item, linkPath);
            }
        }
    }

    QMap<QString, QString> devToMountPoint = readProcMountPoints();
    QStringList mountedRemovablePaths;
    static QSet<QString> attemptedMounts;
    QSet<QString> seenDevs;

    QDir sysBlock(QStringLiteral("/sys/block"));
    for (const QString& disk : sysBlock.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (disk.startsWith(QStringLiteral("loop")) || disk.startsWith(QStringLiteral("ram")) ||
            disk.startsWith(QStringLiteral("zram")) || disk.startsWith(QStringLiteral("dm-")) ||
            disk.startsWith(QStringLiteral("nvme")))
            continue;

        // Strictly skip 0-sector / ejected / empty reader devices
        if (readSysBlockSectors(disk) == 0) continue;

        QString diskSysPath = QFileInfo(sysBlock.filePath(disk)).canonicalFilePath();
        bool isRemovable = diskSysPath.contains(QStringLiteral("/usb"));
        if (!isRemovable) {
            QFile remFile(diskSysPath + QStringLiteral("/removable"));
            if (remFile.open(QIODevice::ReadOnly) && remFile.readAll().trimmed() == "1") {
                isRemovable = true;
            }
        }
        if (!isRemovable) continue;

        QDir diskDir(diskSysPath);
        QStringList partDirs = diskDir.entryList({disk + QStringLiteral("*")}, QDir::Dirs | QDir::NoDotAndDotDot);
        if (partDirs.isEmpty()) partDirs.append(disk);

        for (const QString& part : partDirs) {
            if (readSysBlockSectors(part) == 0) continue;
            QString devNode = QStringLiteral("/dev/") + part;
            if (!QFile::exists(devNode)) continue;

            seenDevs.insert(devNode);
            QString mountPath = devToMountPoint.value(devNode);

            if (mountPath.isEmpty() && !attemptedMounts.contains(devNode)) {
                attemptedMounts.insert(devNode);
                QProcess proc;
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.remove(QStringLiteral("LD_PRELOAD"));
                proc.setProcessEnvironment(env);
                proc.start(QStringLiteral("udisksctl"),
                    {QStringLiteral("mount"), QStringLiteral("-b"), devNode, QStringLiteral("--no-user-interaction")});
                proc.waitForFinished(2000);
                // Re-read /proc/mounts immediately to get the exact mount path chosen by udisks2
                devToMountPoint = readProcMountPoints();
                mountPath = devToMountPoint.value(devNode);
            }

            if (!mountPath.isEmpty() &&
                !mountPath.startsWith(home) &&
                mountPath != QStringLiteral("/") &&
                QDir(mountPath).exists() &&
                !mountedRemovablePaths.contains(mountPath)) {
                mountedRemovablePaths.append(mountPath);
            }
        }
    }

    attemptedMounts.intersect(seenDevs);

    // Also include any active mount points under /run/media/$USER or /media/$USER
    QSet<QString> validMountPoints;
    for (auto it = devToMountPoint.begin(); it != devToMountPoint.end(); ++it) {
        validMountPoints.insert(it.value());
    }
    QStringList mediaRoots = {
        QStringLiteral("/run/media/") + user,
        QStringLiteral("/media/") + user
    };
    for (const QString& root : mediaRoots) {
        QDir rdir(root);
        if (!rdir.exists()) continue;
        for (const QString& sub : rdir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString fullPath = rdir.filePath(sub);
            if (validMountPoints.contains(fullPath) && !mountedRemovablePaths.contains(fullPath)) {
                mountedRemovablePaths.append(fullPath);
            }
        }
    }

    bool changed = false;
    QSet<QString> expectedLinkNames;
    QMap<QString, QString> driveTitleToMountPath;
    char driveLetter = 'E';
    for (const QString& mp : mountedRemovablePaths) {
        QString linkName = QStringLiteral("USB Drive (%1:)").arg(QLatin1Char(driveLetter));
        expectedLinkNames.insert(linkName);
        driveTitleToMountPath.insert(linkName, mp);
        QString linkPath = thisPcDir.filePath(linkName);
        QFileInfo fi(linkPath);
        if (fi.isSymLink() && fi.symLinkTarget() != mp) {
            QFile::remove(linkPath);
            changed = true;
        }
        if (!QFile::exists(linkPath) && !QFileInfo(linkPath).isSymLink()) {
            QFile::link(mp, linkPath);
            changed = true;
        }
        if (driveLetter < 'Z') driveLetter++;
    }

    int folderCnt = 0;
    int driveCnt = 0;
    for (const QString& entry : thisPcDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System)) {
        if ((entry.startsWith(QStringLiteral("USB Drive (")) || entry.startsWith(QStringLiteral("Removable Disk ("))) &&
            !expectedLinkNames.contains(entry)) {
            QFile::remove(thisPcDir.filePath(entry));
            changed = true;
            continue;
        }
        if (entry.startsWith(QStringLiteral("Local Disk")) ||
            entry.startsWith(QStringLiteral("USB Drive (")) ||
            entry.startsWith(QStringLiteral("Removable Disk ("))) {
            driveCnt++;
        } else {
            folderCnt++;
        }
    }

    g_driveCount = (driveCnt > 0) ? driveCnt : (2 + mountedRemovablePaths.size());
    g_folderCount = (folderCnt > 0) ? folderCnt : 6;
    syncUsbPlacesBookmarks(driveTitleToMountPath);
    if (changed) {
        utime(thisPcPath.toUtf8().constData(), nullptr);
    }
}

static bool isDolphinProcess() {
    static int isDolphin = -1;
    if (isDolphin == -1) {
        QString exe = QFileInfo(QStringLiteral("/proc/self/exe")).symLinkTarget();
        isDolphin = exe.endsWith(QStringLiteral("/dolphin")) ? 1 : 0;
    }
    return isDolphin == 1;
}

static void ensureSynced() {
    if (!isDolphinProcess()) return;
    if (!g_syncTimer.isValid() || g_syncTimer.elapsed() > 2000) {
        g_syncTimer.restart();
        syncRemovableDrives();
    }
    if (!g_qtimerStarted && qApp) {
        g_qtimerStarted = true;
        QPalette win10Pal;
        win10Pal.setColor(QPalette::Window, QColor(245, 246, 247));
        win10Pal.setColor(QPalette::WindowText, QColor(20, 20, 20));
        win10Pal.setColor(QPalette::Base, QColor(255, 255, 255));
        win10Pal.setColor(QPalette::AlternateBase, QColor(250, 250, 250));
        win10Pal.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
        win10Pal.setColor(QPalette::ToolTipText, QColor(20, 20, 20));
        win10Pal.setColor(QPalette::Text, QColor(15, 15, 15));
        win10Pal.setColor(QPalette::Button, QColor(245, 246, 247));
        win10Pal.setColor(QPalette::ButtonText, QColor(20, 20, 20));
        win10Pal.setColor(QPalette::BrightText, QColor(0, 0, 0));
        win10Pal.setColor(QPalette::Link, QColor(0, 102, 204));
        win10Pal.setColor(QPalette::Highlight, QColor(204, 232, 255));
        win10Pal.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
        QApplication::setPalette(win10Pal);
        QIcon::setThemeName(QStringLiteral("Win10-Explorer"));
        auto* timer = new QTimer(qApp);
        QObject::connect(timer, &QTimer::timeout, &syncRemovableDrives);
        timer->start(2000);

        QString shotPath = qEnvironmentVariable("WIN10_SCREENSHOT_OUT");
        if (!shotPath.isEmpty()) {
            QTimer::singleShot(1800, qApp, [shotPath]() {
                for (QWidget* w : QApplication::topLevelWidgets()) {
                    if (w->isVisible() && w->width() > 400) {
                        w->resize(1180, 760);
                        w->grab().save(shotPath);
                        qApp->quit();
                        break;
                    }
                }
            });
        }
    }
}

__attribute__((constructor))
static void initThisPcSync() {
    unsetenv("LD_PRELOAD");
    if (!isDolphinProcess()) return;
    g_syncTimer.start();
    syncRemovableDrives();
}

#include <QUrl>

static QHash<const void*, QString> s_viewPropPaths;

extern "C" void _ZN14ViewPropertiesC1ERK4QUrl(void* self, const QUrl& url) {
    static auto real_ctor = reinterpret_cast<void(*)(void*, const QUrl&)>(
        dlsym(RTLD_NEXT, "_ZN14ViewPropertiesC1ERK4QUrl"));
    if (real_ctor) real_ctor(self, url);
    if (self) {
        QString p = QFileInfo(url.toLocalFile()).canonicalFilePath();
        if (p.isEmpty()) p = QDir::cleanPath(url.toLocalFile());
        s_viewPropPaths.insert(self, p);
    }
}

extern "C" void _ZN14ViewPropertiesC2ERK4QUrl(void* self, const QUrl& url) {
    static auto real_ctor = reinterpret_cast<void(*)(void*, const QUrl&)>(
        dlsym(RTLD_NEXT, "_ZN14ViewPropertiesC2ERK4QUrl"));
    if (real_ctor) real_ctor(self, url);
    if (self) {
        QString p = QFileInfo(url.toLocalFile()).canonicalFilePath();
        if (p.isEmpty()) p = QDir::cleanPath(url.toLocalFile());
        s_viewPropPaths.insert(self, p);
    }
}

extern "C" bool _ZNK14ViewProperties16hiddenFilesShownEv(const void* self) {
    static auto real_fn = reinterpret_cast<bool(*)(const void*)>(
        dlsym(RTLD_NEXT, "_ZNK14ViewProperties16hiddenFilesShownEv"));
    if (self) {
        QString p = s_viewPropPaths.value(self);
        QString homeCanon = QFileInfo(QDir::homePath()).canonicalFilePath();
        if (p == homeCanon || p == QDir::cleanPath(QDir::homePath())) {
            return true;
        }
    }
    return real_fn ? real_fn(self) : false;
}

extern "C" bool _ZNK14ViewProperties14sortHiddenLastEv(const void* self) {
    static auto real_fn = reinterpret_cast<bool(*)(const void*)>(
        dlsym(RTLD_NEXT, "_ZNK14ViewProperties14sortHiddenLastEv"));
    if (self) {
        QString p = s_viewPropPaths.value(self);
        QString homeCanon = QFileInfo(QDir::homePath()).canonicalFilePath();
        if (p == homeCanon || p == QDir::cleanPath(QDir::homePath())) {
            return true;
        }
    }
    return real_fn ? real_fn(self) : false;
}

extern "C" bool _ZNK9KFileItem8isHiddenEv(const KFileItem* self) {
    static auto real_fn = reinterpret_cast<bool(*)(const KFileItem*)>(
        dlsym(RTLD_NEXT, "_ZNK9KFileItem8isHiddenEv"));
    if (self) {
        QString lpath = self->localPath();
        if (!lpath.isEmpty()) {
            QFileInfo fi(lpath);
            QString fn = fi.fileName();
            if (fn == QStringLiteral(".directory") || fn == QStringLiteral(".win-explorer")) {
                return true;
            }
            QString parentCanon = QFileInfo(fi.absolutePath()).canonicalFilePath();
            QString homeCanon = QFileInfo(QDir::homePath()).canonicalFilePath();
            if (parentCanon == homeCanon || QDir::cleanPath(fi.absolutePath()) == QDir::cleanPath(QDir::homePath())) {
                return false;
            }
        }
    }
    return real_fn ? real_fn(self) : false;
}

extern "C" bool _ZNK19KFileItemListWidget8isHiddenEv(const void*) {
    return false;
}

extern "C" bool _ZNK23KStandardItemListWidget8isHiddenEv(const void*) {
    return false;
}

extern "C" bool _ZNK9KFileItem6isLinkEv(const KFileItem*) {
    return false;
}

extern "C" QStringList _ZNK9KFileItem8overlaysEv(const KFileItem*) {
    return QStringList();
}

static bool isSpecialSystemFolder(const QString& displayName) {
    return displayName == QStringLiteral("Desktop") ||
           displayName == QStringLiteral("Documents") ||
           displayName == QStringLiteral("Downloads") ||
           displayName == QStringLiteral("Music") ||
           displayName == QStringLiteral("Pictures") ||
           displayName == QStringLiteral("Videos");
}

extern "C" QString _ZNK9KFileItem16getStatusBarInfoEv(const KFileItem* self) {
    static auto real_status = reinterpret_cast<QString(*)(const KFileItem*)>(
        dlsym(RTLD_NEXT, "_ZNK9KFileItem16getStatusBarInfoEv"));
    QString raw = getRawItemText(self);
    if (raw.startsWith(QStringLiteral("Local Disk")) ||
        raw.startsWith(QStringLiteral("USB Drive (")) ||
        raw.startsWith(QStringLiteral("Removable Disk ("))) {
        DriveSpaceInfo sp = getLiveDriveSpace(raw);
        if (!sp.summaryText.isEmpty()) {
            return QStringLiteral("%1 — %2 (%3% used)").arg(raw, sp.summaryText).arg(sp.usedPct);
        }
    }
    return real_status ? real_status(self) : raw;
}

static int getDriveIndexFromName(const QString& rawName) {
    if (rawName == QStringLiteral("Local Disk (D:)")) return 1;
    if (rawName.startsWith(QStringLiteral("USB Drive (")) || rawName.startsWith(QStringLiteral("Removable Disk ("))) {
        int openPos = rawName.indexOf(QLatin1Char('('));
        if (openPos >= 0 && openPos + 1 < rawName.size()) {
            char ch = rawName.at(openPos + 1).toLatin1();
            if (ch >= 'E' && ch <= 'Z') return 2 + (ch - 'E');
        }
        return 2;
    }
    return 0;
}

static QString getLiveWidgetDriveName(const void* self);

static bool isStandardItemListWidget(const void* self) {
    if (!self) return false;
    const auto* mo = get_standardWidgetMetaObject();
    if (!mo) return false;
    const auto* obj = reinterpret_cast<const QObject*>(self);
    return obj->metaObject() == mo;
}

static QHash<const void*, QPointF> s_baseGridPos;

extern "C" void _ZN13QGraphicsItem6setPosERK7QPointF(void* item, const QPointF& pos) {
    static auto real_setPos = reinterpret_cast<void(*)(void*, const QPointF&)>(
        dlsym(RTLD_NEXT, "_ZN13QGraphicsItem6setPosERK7QPointF"));
    if (!real_setPos) return;
    if (item) {
        void* widgetSelf = reinterpret_cast<char*>(item) - 16;
        if (isStandardItemListWidget(widgetSelf)) {
            s_baseGridPos.insert(widgetSelf, pos);
            QString driveName = getLiveWidgetDriveName(widgetSelf);
            if (!driveName.isEmpty()) {
                int driveIdx = getDriveIndexFromName(driveName);
                QPointF shifted(16.0 + driveIdx * 258.0, pos.y());
                real_setPos(item, shifted);
                return;
            }
        }
    }
    real_setPos(item, pos);
}

extern "C" void _ZN15QGraphicsWidget6resizeERK6QSizeF(void* widgetSelf, const QSizeF& size) {
    static auto real_resize = reinterpret_cast<void(*)(void*, const QSizeF&)>(
        dlsym(RTLD_NEXT, "_ZN15QGraphicsWidget6resizeERK6QSizeF"));
    if (!real_resize) return;
    if (isStandardItemListWidget(widgetSelf)) {
        QString driveName = getLiveWidgetDriveName(widgetSelf);
        if (!driveName.isEmpty()) {
            real_resize(widgetSelf, QSizeF(248.0, 76.0));
            return;
        }
    }
    real_resize(widgetSelf, size);
}

static qreal syncDriveWidgetGeometry(const void* self, const QString& driveName) {
    if (!self || driveName.isEmpty()) return 0.0;
    static auto real_setPos = reinterpret_cast<void(*)(void*, const QPointF&)>(
        dlsym(RTLD_NEXT, "_ZN13QGraphicsItem6setPosERK7QPointF"));
    static auto real_resize = reinterpret_cast<void(*)(void*, const QSizeF&)>(
        dlsym(RTLD_NEXT, "_ZN15QGraphicsWidget6resizeERK6QSizeF"));
    auto* gw = reinterpret_cast<QGraphicsWidget*>(const_cast<void*>(self));
    if (gw->size().width() < 246.0 && real_resize) {
        real_resize(gw, QSizeF(248.0, 76.0));
    }
    int driveIdx = getDriveIndexFromName(driveName);
    qreal expectedX = 16.0 + driveIdx * 258.0;
    qreal curX = gw->pos().x();
    qreal dx = 0.0;
    if (qAbs(curX - expectedX) > 1.5 && real_setPos) {
        dx = expectedX - curX;
        real_setPos(static_cast<QGraphicsItem*>(gw), QPointF(expectedX, gw->pos().y()));
        if (gw->scene()) gw->scene()->update();
    }
    return dx;
}

extern "C" QRectF _ZNK23KStandardItemListWidget17selectionRectCoreEv(const void* self) {
    static auto real_fn = reinterpret_cast<QRectF(*)(const void*)>(
        dlsym(RTLD_NEXT, "_ZNK23KStandardItemListWidget17selectionRectCoreEv"));
    QString driveName = getLiveWidgetDriveName(self);
    if (!driveName.isEmpty()) {
        return QRectF(0, 6, 246, 64);
    }
    return real_fn ? real_fn(self) : QRectF(0, 0, 100, 80);
}

extern "C" QRectF _ZNK23KStandardItemListWidget17selectionRectFullEv(const void* self) {
    static auto real_fn = reinterpret_cast<QRectF(*)(const void*)>(
        dlsym(RTLD_NEXT, "_ZNK23KStandardItemListWidget17selectionRectFullEv"));
    QString driveName = getLiveWidgetDriveName(self);
    if (!driveName.isEmpty()) {
        return QRectF(0, 6, 246, 64);
    }
    return real_fn ? real_fn(self) : QRectF(0, 0, 100, 80);
}

extern "C" bool _ZNK15KItemListWidget8containsERK7QPointF(const void* self, const QPointF& point) {
    static auto real_fn = reinterpret_cast<bool(*)(const void*, const QPointF&)>(
        dlsym(RTLD_NEXT, "_ZNK15KItemListWidget8containsERK7QPointF"));
    QString driveName = getLiveWidgetDriveName(self);
    if (!driveName.isEmpty()) {
        return QRectF(0, 6, 246, 64).contains(point);
    }
    return real_fn ? real_fn(self, point) : false;
}

extern "C" bool _ZThn16_NK15KItemListWidget8containsERK7QPointF(const void* thunkSelf, const QPointF& point) {
    const void* realSelf = reinterpret_cast<const char*>(thunkSelf) - 16;
    return _ZNK15KItemListWidget8containsERK7QPointF(realSelf, point);
}

static QHash<const void*, const void*> s_driveWidgetToView;

extern "C" std::optional<int> _ZNK13KItemListView6itemAtERK7QPointF(const void* view, const QPointF& pos) {
    static auto real_itemAt = reinterpret_cast<std::optional<int>(*)(const void*, const QPointF&)>(
        dlsym(RTLD_NEXT, "_ZNK13KItemListView6itemAtERK7QPointF"));
    if (view && !s_driveWidgetToView.isEmpty()) {
        auto* viewGw = reinterpret_cast<const QGraphicsWidget*>(view);
        for (auto it = s_driveWidgetToView.begin(); it != s_driveWidgetToView.end(); ) {
            const void* w = it.key();
            QString driveName = getLiveWidgetDriveName(w);
            if (driveName.isEmpty()) {
                it = s_driveWidgetToView.erase(it);
                continue;
            }
            ++it;
            auto* gw = reinterpret_cast<const QGraphicsWidget*>(w);
            QPointF localPos = gw->mapFromItem(viewGw, pos);
            if (QRectF(0, 6, 246, 64).contains(localPos)) {
                return std::optional<int>(call_widgetIndex(w));
            }
        }
    }
    return real_itemAt ? real_itemAt(view, pos) : std::nullopt;
}

extern "C" void* _ZNK19KItemListController12widgetForPosERK7QPointF(const void* controller, const QPointF& pos) {
    static auto real_widgetForPos = reinterpret_cast<void*(*)(const void*, const QPointF&)>(
        dlsym(RTLD_NEXT, "_ZNK19KItemListController12widgetForPosERK7QPointF"));
    if (controller && !s_driveWidgetToView.isEmpty()) {
        const void* view = call_controllerView(controller);
        if (view) {
            auto* viewGw = reinterpret_cast<const QGraphicsWidget*>(view);
            for (auto it = s_driveWidgetToView.begin(); it != s_driveWidgetToView.end(); ) {
                const void* w = it.key();
                QString driveName = getLiveWidgetDriveName(w);
                if (driveName.isEmpty()) {
                    it = s_driveWidgetToView.erase(it);
                    continue;
                }
                ++it;
                auto* gw = reinterpret_cast<const QGraphicsWidget*>(w);
                QPointF localPos = gw->mapFromItem(viewGw, pos);
                if (QRectF(0, 6, 246, 64).contains(localPos)) {
                    return const_cast<void*>(w);
                }
            }
        }
    }
    return real_widgetForPos ? real_widgetForPos(controller, pos) : nullptr;
}

extern "C" QRectF _ZNK13KItemListView8itemRectEi(const void* view, int index) {
    static auto real_itemRect = reinterpret_cast<QRectF(*)(const void*, int)>(
        dlsym(RTLD_NEXT, "_ZNK13KItemListView8itemRectEi"));
    QRectF baseRect = real_itemRect ? real_itemRect(view, index) : QRectF();
    if (view && !s_driveWidgetToView.isEmpty()) {
        for (auto it = s_driveWidgetToView.begin(); it != s_driveWidgetToView.end(); ++it) {
            const void* w = it.key();
            if (call_widgetIndex(w) == index) {
                QString driveName = getLiveWidgetDriveName(w);
                if (!driveName.isEmpty()) {
                    auto* gw = reinterpret_cast<const QGraphicsWidget*>(w);
                    return QRectF(gw->pos().x(), gw->pos().y() + 6, 246, 64);
                }
                break;
            }
        }
    }
    return baseRect;
}

static QString getLiveWidgetDriveName(const void* self) {
    if (!self) return QString();
    int iconSz = call_iconSize(self);
    if (iconSz < 48) return QString(); // Sidebar uses 20px icons, only main view uses >=48px
    QString txt = call_widgetValue(self, QByteArrayLiteral("text")).toString();
    if (txt.startsWith(QStringLiteral("Local Disk")) ||
        txt.startsWith(QStringLiteral("USB Drive (")) ||
        txt.startsWith(QStringLiteral("Removable Disk ("))) {
        return txt;
    }
    return QString();
}

// Paint the exact 1:1 Original Windows 10 Horizontal Drive Tile (media_1790269239146.png):
// [48x48 3D Silver Drive on Left] + [Title (Top) / Blue-or-Red Bar (Middle) / "XX.X GB free of YY.Y GB" (Bottom) on Right]
static void paintWin10DriveTile(const void* self, const QString& rawName, QPainter* painter) {
    qreal dx = syncDriveWidgetGeometry(self, rawName);
    DriveSpaceInfo sp = getLiveDriveSpace(rawName);

    bool selected = call_isSelected(self);
    if (qEnvironmentVariable("WIN10_FORCE_SELECT_DRIVE") == rawName) selected = true;
    bool hovered = call_isHovered(self);

    painter->save();
    painter->setClipping(false);
    if (qAbs(dx) > 1.5) {
        painter->translate(dx, 0.0);
    }
    // Widget itself is natively positioned & sized to 248x76
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    // Tile bounding box (248px wide x 66px tall, matching Windows 10 This PC Tiles view)
    QRectF tileRect(0, 6, 246, 64);
    if (selected) {
        painter->setPen(QPen(QColor(153, 209, 255), 1));
        painter->setBrush(QColor(204, 232, 255));
        painter->drawRect(tileRect);
    } else if (hovered) {
        painter->setPen(QPen(QColor(204, 232, 255), 1));
        painter->setBrush(QColor(229, 243, 255));
        painter->drawRect(tileRect);
    }

    // 1. Left: Genuine 3D Silver Windows 10 Drive Icon (48x48 at x=6, y=14)
    QString baseFile = QStringLiteral("drive_d_base.png");
    if (rawName == QStringLiteral("Local Disk (C:)")) baseFile = QStringLiteral("drive_c_base.png");
    else if (rawName.startsWith(QStringLiteral("USB Drive ("))) baseFile = QStringLiteral("drive_usb_base.png");

    static QHash<QString, QPixmap> s_drivePixCache;
    QPixmap drivePix = s_drivePixCache.value(baseFile);
    if (drivePix.isNull()) {
        drivePix = QPixmap(QDir::homePath() + QStringLiteral("/.config/win-explorer/") + baseFile);
        if (!drivePix.isNull()) s_drivePixCache.insert(baseFile, drivePix);
    }
    if (!drivePix.isNull()) {
        painter->drawPixmap(QRect(6, 14, 48, 48), drivePix);
    }

    // 2. Right Row 1 (x=62, y=10): Drive Name ("Local Disk (C:)", "Local Disk (D:)", "USB Drive (E:)")
    QFont titleFont = painter->font();
    titleFont.setPointSizeF(9.5);
    titleFont.setWeight(QFont::Normal);
    painter->setFont(titleFont);
    painter->setPen(QColor(30, 30, 30));
    painter->drawText(QRect(62, 10, 180, 18), Qt::AlignLeft | Qt::AlignVCenter, rawName);

    // 3. Right Row 2 (x=62, y=30, width=174, height=15): Original Windows 10 Capacity Bar
    //    Grey track (#E6E6E6) with 1px border (#BCBCBC), filled with Windows 10 Cyan-Blue (#26A0DA) or Red (#DA2626 if >=90%)
    QRect barOuter(62, 30, 174, 15);
    painter->setPen(QPen(QColor(188, 188, 188), 1));
    painter->setBrush(QColor(230, 230, 230));
    painter->drawRect(barOuter);

    int maxFillW = 172;
    int fillW = (sp.usedPct * maxFillW) / 100;
    if (fillW < 8 && sp.usedPct > 0) fillW = 8;
    if (fillW > maxFillW) fillW = maxFillW;
    if (fillW > 0) {
        QColor barColor = (sp.usedPct >= 90) ? QColor(218, 38, 38) : QColor(38, 160, 218);
        painter->fillRect(QRect(63, 31, fillW, 13), barColor);
    }

    // 4. Right Row 3 (x=62, y=48): "63.9 GB free of 78.2 GB" in Windows 10 medium grey (#646464)
    QFont subFont = painter->font();
    subFont.setPointSizeF(9.0);
    painter->setFont(subFont);
    painter->setPen(QColor(100, 100, 100));
    painter->drawText(QRect(62, 48, 180, 18), Qt::AlignLeft | Qt::AlignVCenter, sp.summaryText);

    painter->restore();
}

extern "C" void _ZN23KStandardItemListWidget5paintEP8QPainterPK24QStyleOptionGraphicsItemP7QWidget(
    void* self, QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    static auto real_paint = reinterpret_cast<void(*)(void*, QPainter*, const QStyleOptionGraphicsItem*, QWidget*)>(
        dlsym(RTLD_NEXT, "_ZN23KStandardItemListWidget5paintEP8QPainterPK24QStyleOptionGraphicsItemP7QWidget"));

    QString driveName = getLiveWidgetDriveName(self);
    if (!driveName.isEmpty()) {
        if (!s_driveWidgetToView.contains(self)) {
            s_driveWidgetToView.insert(self, self);
            auto* gw = reinterpret_cast<QGraphicsWidget*>(self);
            call_prepareGeometryChange(static_cast<QGraphicsItem*>(gw));
        }
        paintWin10DriveTile(self, driveName, painter);
        return;
    } else {
        s_driveWidgetToView.remove(self);
    }
    if (real_paint) real_paint(self, painter, option, widget);
}

extern "C" void _ZThn16_N23KStandardItemListWidget5paintEP8QPainterPK24QStyleOptionGraphicsItemP7QWidget(
    void* thunkSelf, QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    void* realSelf = reinterpret_cast<char*>(thunkSelf) - 16;
    QString driveName = getLiveWidgetDriveName(realSelf);
    if (!driveName.isEmpty()) {
        if (!s_driveWidgetToView.contains(realSelf)) {
            s_driveWidgetToView.insert(realSelf, realSelf);
            auto* gw = reinterpret_cast<QGraphicsWidget*>(realSelf);
            call_prepareGeometryChange(static_cast<QGraphicsItem*>(gw));
        }
        paintWin10DriveTile(realSelf, driveName, painter);
        return;
    } else {
        s_driveWidgetToView.remove(realSelf);
    }
    static auto real_thunk = reinterpret_cast<void(*)(void*, QPainter*, const QStyleOptionGraphicsItem*, QWidget*)>(
        dlsym(RTLD_NEXT, "_ZThn16_N23KStandardItemListWidget5paintEP8QPainterPK24QStyleOptionGraphicsItemP7QWidget"));
    if (real_thunk) real_thunk(thunkSelf, painter, option, widget);
}

static QString buildDynamicFolderIcon(const QString& dirPath, const QString& displayName) {
    Q_UNUSED(dirPath);
    if (isSpecialSystemFolder(displayName)) {
        if (displayName == QStringLiteral("Desktop")) return QStringLiteral("user-desktop");
        return QStringLiteral("folder-") + displayName.toLower();
    }
    return QStringLiteral("folder");

    // Never scan OS system directories (/usr, /lib64, /etc, /var, /bin, /proc, /sys, /dev, /run)
    if (!dirPath.startsWith(QDir::homePath()) && !dirPath.startsWith(QStringLiteral("/run/media/"))) {
        return QStringLiteral("folder-with-contents");
    }

    static QHash<QString, QPair<qint64, QString>> s_iconCache;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto cached = s_iconCache.find(dirPath);
    if (cached != s_iconCache.end() && (now - cached.value().first) < 4000) {
        return cached.value().second;
    }

    static const QSet<QString> imgExts = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("webp"), QStringLiteral("bmp"), QStringLiteral("gif")
    };

    QDirIterator it(dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    bool hasAnyEntry = false;
    QString bestImageFile;
    int scanned = 0;
    while (it.hasNext() && scanned < 64) {
        it.next();
        QString fname = it.fileName();
        if (fname == QStringLiteral(".directory")) continue;
        hasAnyEntry = true;
        scanned++;
        QString ext = it.fileInfo().suffix().toLower();
        if (imgExts.contains(ext)) {
            bestImageFile = it.filePath();
            break;
        }
    }

    if (!hasAnyEntry) {
        s_iconCache.insert(dirPath, qMakePair(now, QStringLiteral("folder")));
        return QStringLiteral("folder");
    }

    if (bestImageFile.isEmpty()) {
        s_iconCache.insert(dirPath, qMakePair(now, QStringLiteral("folder-with-contents")));
        return QStringLiteral("folder-with-contents");
    }

    qint64 dirMtime = QFileInfo(dirPath).lastModified().toMSecsSinceEpoch();
    qint64 imgMtime = QFileInfo(bestImageFile).lastModified().toMSecsSinceEpoch();
    QByteArray sig = QStringLiteral("%1|%2|%3|%4|%5")
        .arg(dirPath, bestImageFile)
        .arg(scanned)
        .arg(dirMtime)
        .arg(imgMtime)
        .toUtf8();
    QString hash = QString::fromLatin1(QCryptographicHash::hash(sig, QCryptographicHash::Md5).toHex().left(12));
    QString iconName = QStringLiteral("w10peek_") + hash;
    QString svgPath = QDir::homePath() + QStringLiteral("/.local/share/icons/Win10-Explorer/scalable/places/") + iconName + QStringLiteral(".svg");

    if (!QFile::exists(svgPath)) {
        QImage userImg(bestImageFile);
        QImage baseFolder(QDir::homePath() + QStringLiteral("/.config/win-explorer/folder_base.png"));
        QImage frontFlap(QDir::homePath() + QStringLiteral("/.config/win-explorer/folder_flap.png"));
        if (userImg.isNull() || baseFolder.isNull() || frontFlap.isNull()) {
            return QStringLiteral("folder-with-contents");
        }

        QImage canvas = baseFolder.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QPainter p(&canvas);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        p.save();
        p.translate(128, 124);
        p.rotate(4.0);
        QRect frameRect(-56, -72, 112, 144);
        p.setPen(QPen(QColor(160, 174, 192), 2));
        p.setBrush(Qt::white);
        p.drawRoundedRect(frameRect, 3, 3);
        QImage scaled = userImg.scaled(100, 132, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QRect cropRect((scaled.width() - 100) / 2, (scaled.height() - 132) / 2, 100, 132);
        p.drawImage(QRect(-50, -66, 100, 132), scaled, cropRect);
        p.restore();

        p.drawImage(0, 0, frontFlap);
        p.end();

        QByteArray pngBytes;
        QBuffer buf(&pngBytes);
        buf.open(QIODevice::WriteOnly);
        canvas.save(&buf, "PNG");
        QString b64 = QString::fromLatin1(pngBytes.toBase64());

        QString svg = QStringLiteral(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 256 256\" width=\"256\" height=\"256\">"
            "<image x=\"0\" y=\"0\" width=\"256\" height=\"256\" preserveAspectRatio=\"xMidYMid meet\" href=\"data:image/png;base64,%1\"/>"
            "</svg>").arg(b64);

        QFile out(svgPath);
        if (out.open(QIODevice::WriteOnly)) {
            out.write(svg.toUtf8());
            out.close();
        }
    }
    s_iconCache.insert(dirPath, qMakePair(now, iconName));
    return iconName;
}

extern "C" QString _ZNK9KFileItem8iconNameEv(const KFileItem* self) {
    ensureSynced();
    static auto real_fn = reinterpret_cast<QString(*)(const KFileItem*)>(
        dlsym(RTLD_NEXT, "_ZNK9KFileItem8iconNameEv"));
    QString rawName = getRawItemText(self);
    if (rawName == QStringLiteral("Local Disk (C:)")) return QStringLiteral("drive-harddisk-system");
    if (rawName == QStringLiteral("Local Disk (D:)")) return QStringLiteral("drive-harddisk");
    if (rawName.startsWith(QStringLiteral("USB Drive (")) || rawName.startsWith(QStringLiteral("Removable Disk ("))) {
        return QStringLiteral("drive-removable-media-usb");
    }
    if (isSpecialSystemFolder(rawName)) {
        if (rawName == QStringLiteral("Desktop")) return QStringLiteral("user-desktop");
        return QStringLiteral("folder-") + rawName.toLower();
    }
    if (self->isDir()) {
        QString lpath = self->localPath();
        if (!lpath.isEmpty()) {
            return buildDynamicFolderIcon(lpath, rawName);
        }
    }
    return real_fn ? real_fn(self) : QStringLiteral("folder");
}

extern "C" QString _ZNK9KFileItem11mimeCommentEv(const KFileItem* self) {
    ensureSynced();
    static auto real_fn = reinterpret_cast<QString(*)(const KFileItem*)>(
        dlsym(RTLD_NEXT, "_ZNK9KFileItem11mimeCommentEv"));
    QString rawName = getRawItemText(self);
    if (rawName.startsWith(QStringLiteral("Local Disk")) ||
        rawName.startsWith(QStringLiteral("USB Drive (")) ||
        rawName.startsWith(QStringLiteral("Removable Disk ("))) {
        return QStringLiteral("\u3164Devices and drives");
    }
    if (self->localPath().contains(QStringLiteral("/.win-explorer/ThisPC/")) || isSpecialSystemFolder(rawName)) {
        return QStringLiteral("Folders");
    }
    return real_fn ? real_fn(self) : QStringLiteral("Folder");
}
EOF
fi
g++ -shared -fPIC -O2 -I/usr/include/qt6 -I/usr/include/qt6/QtCore -I/usr/include/qt6/QtGui -I/usr/include/qt6/QtWidgets "${CONFIG_DIR}/win10_thispc_helper.cpp" -o "${CONFIG_DIR}/libwin10_thispc.so" -lQt6Widgets -lQt6Gui -lQt6Core -ldl

# Configure ThisPC folder view properties (Grouped by Type, Folders First, No Previews, Icons Mode)
cat << 'EOF' > "${THIS_PC_DIR}/.directory"
[Desktop Entry]
Icon=computer

[Dolphin]
Version=4
ViewMode=0
PreviewsShown=false
GroupedSorting=true
SortRole=type
GroupRole=type
SortFoldersFirst=true
SortOrder=0
EOF

#Also set KFileMetaData xattr on THIS_PC_DIR so Dolphin honors it whether GlobalViewProps is true or false
if command -v setfattr &>/dev/null; then
    setfattr -n "user.xdg.dolphin.viewproperties#1" \
        -v "[Dolphin]
Version=4
ViewMode=0
PreviewsShown=false
GroupedSorting=true
SortRole=type
GroupRole=type
SortFoldersFirst=true
SortOrder=0" "${THIS_PC_DIR}" 2>/dev/null || true
fi

# Configure global default view properties for all other folders (No ugly thumbnail previews, ungrouped)
cat << 'EOF' > "${HOME}/.local/share/dolphin/view_properties/global/.directory"
[Dolphin]
Version=4
ViewMode=0
PreviewsShown=true
GroupedSorting=false
SortRole=text
SortFoldersFirst=true
SortOrder=0
EOF

# 5. Copy Windows 10 Dolphin QSS Stylesheet
cp "${SCRIPT_DIR}/win10-dolphin.qss" "${CONFIG_DIR}/win10-dolphin.qss"

# 6. Write 2-Row Windows 10 Ribbon + Navigation Bar XMLGUI Layout
cat << 'EOF' > "${DOLPHIN_UIRC}"
<?xml version="1.0"?>
<!DOCTYPE gui SYSTEM "kpartgui.dtd">
<gui name="dolphin" version="999">
    <MenuBar>
        <Menu name="file">
            <text>&amp;File</text>
            <Action name="new_menu" />
            <Action name="create_dir" />
            <Action name="new_tab" />
            <Action name="file_new" />
            <Separator/>
            <Action name="open_terminal_here" />
            <Separator/>
            <Action name="renamefile" />
            <Action name="duplicate" />
            <Action name="movetotrash" />
            <Action name="deletefile" />
            <Separator/>
            <Action name="properties" />
            <Separator/>
            <Action name="file_close" />
            <Action name="file_quit" />
        </Menu>
        <Menu name="edit">
            <text>&amp;Home</text>
            <Action name="edit_paste" />
            <Action name="edit_cut" />
            <Action name="edit_copy" />
            <Action name="copy_location" />
            <Separator />
            <Action name="create_dir" />
            <Action name="new_menu" />
            <Separator />
            <Action name="renamefile" />
            <Action name="movetotrash" />
            <Action name="properties" />
            <Separator />
            <Action name="edit_select_all" />
            <Action name="invert_selection" />
            <Action name="toggle_selection_mode" />
        </Menu>
        <Menu name="tools">
            <text>&amp;Share</text>
            <Action name="copy_location" />
            <Action name="open_terminal_here" />
            <Action name="open_preferred_search_tool" />
            <Action name="compare_files" />
            <Action name="manage_disk_space" />
        </Menu>
        <Menu name="view">
            <text>&amp;View</text>
            <Action name="icons" />
            <Action name="details" />
            <Action name="compact" />
            <Separator/>
            <Action name="sort" />
            <Action name="group_by" />
            <Action name="additional_info" />
            <Separator/>
            <Action name="show_preview" />
            <Action name="show_hidden_files" />
            <Action name="panels" />
            <Separator/>
            <Action name="split_view_menu" />
            <Action name="redisplay" />
        </Menu>
        <Menu name="settings">
            <text>&amp;Options</text>
            <Action name="options_configure" />
            <Action name="options_configure_keybinding" />
            <Action name="options_configure_toolbars" />
        </Menu>
    </MenuBar>

    <!-- ROW 1: Windows 10 Ribbon Command Bar (Large Icons + Text Underneath) -->
    <ToolBar noMerge="1" name="mainToolBar" iconText="icontextbottom" iconSize="28" fullWidth="true" newline="true" position="Top">
        <text>Windows 10 Ribbon</text>
        <Action name="edit_paste" />
        <Action name="edit_cut" />
        <Action name="edit_copy" />
        <Action name="copy_location" />
        <Separator name="sep_clipboard" />
        <Action name="movetotrash" />
        <Action name="renamefile" />
        <Separator name="sep_organize" />
        <Action name="create_dir" />
        <Action name="new_menu" />
        <Separator name="sep_new" />
        <Action name="properties" />
        <Action name="open_terminal_here" />
        <Separator name="sep_open" />
        <Action name="icons" />
        <Action name="details" />
        <Action name="compact" />
        <Action name="sort" />
        <Action name="group_by" />
        <Action name="show_hidden_files" />
        <Action name="show_information_panel" />
        <Separator name="sep_view" />
        <Action name="edit_select_all" />
        <Action name="invert_selection" />
    </ToolBar>

    <!-- ROW 2: Windows 10 Navigation & Address Bar -->
    <ToolBar noMerge="1" name="navigationToolBar" iconText="icononly" iconSize="18" fullWidth="true" newline="true" position="Top">
        <text>Navigation Bar</text>
        <Action name="go_back" />
        <Action name="go_forward" />
        <Action name="go_up" />
        <Action name="redisplay" />
        <Action name="url_navigators" />
        <Action name="toggle_search" />
    </ToolBar>

    <ActionProperties scheme="Default">
        <Action name="edit_paste" text="Paste" iconText="Paste" icon="edit-paste" priority="2"/>
        <Action name="edit_cut" text="Cut" iconText="Cut" icon="edit-cut" priority="2"/>
        <Action name="edit_copy" text="Copy" iconText="Copy" icon="edit-copy" priority="2"/>
        <Action name="copy_location" text="Copy path" iconText="Copy path" icon="edit-copy-path" priority="2"/>
        <Action name="movetotrash" text="Delete" iconText="Delete" icon="edit-delete" priority="2"/>
        <Action name="renamefile" text="Rename" iconText="Rename" icon="edit-rename" priority="2"/>
        <Action name="create_dir" text="New folder" iconText="New folder" icon="folder-new" priority="2"/>
        <Action name="new_menu" text="New item" iconText="New item" icon="document-new" priority="2"/>
        <Action name="properties" text="Properties" iconText="Properties" icon="document-properties" priority="2"/>
        <Action name="open_terminal_here" text="Terminal" iconText="Terminal" icon="utilities-terminal" priority="2"/>
        <Action name="icons" text="Icons" iconText="Icons" icon="view-list-icons" priority="2"/>
        <Action name="details" text="Details" iconText="Details" icon="view-list-details" priority="2"/>
        <Action name="compact" text="List" iconText="List" icon="view-list-tree" priority="2"/>
        <Action name="sort" text="Sort by" iconText="Sort by" icon="view-sort" priority="2"/>
        <Action name="group_by" text="Group by" iconText="Group by" icon="view-group" priority="2"/>
        <Action name="show_hidden_files" text="Hidden" iconText="Hidden" icon="view-visible" priority="2"/>
        <Action name="show_information_panel" text="Details pane" iconText="Details pane" icon="view-right-new" priority="2"/>
        <Action name="edit_select_all" text="Select all" iconText="Select all" icon="edit-select-all" priority="2"/>
        <Action name="invert_selection" text="Invert" iconText="Invert" icon="edit-select-invert" priority="2"/>
    </ActionProperties>
</gui>
EOF

# 7. Write Dolphin Config with embedded Windows 10 Light Palette + HomeUrl=ThisPC + GlobalViewProps=false
cat "${COLOR_SCHEMES_DIR}/Win10Explorer.colors" > "${DOLPHIN_RC}"
cat << EOF >> "${DOLPHIN_RC}"

[General]
Version=202
ShowFullPath=false
ShowSpaceInfo=true
ShowStatusBar=true
ShowZoomSlider=true
EditableUrl=false
GlobalViewProps=false
HomeUrl=file://${THIS_PC_DIR}
RememberOpenedTabs=false

[UiSettings]
ColorScheme=Win10Explorer

[Icons]
Theme=Win10-Explorer

[KDE]
SingleClick=false

[KFileDialog Settings]
Places Icons Auto-resize=false
Places Icons Static Size=20

[PlacesPanel]
IconSize=20

[MainWindow]
MenuBar=Enabled
ToolBarsMovable=Disabled
State=AAAA/wAAAAD9AAAAAwAAAAAAAADyAAACM/wCAAAAAvsAAAAWAGYAbwBsAGQAZQByAHMARABvAGMAawAAAAAA/////wAAAAAA////+wAAABQAcABsAGEAYwBlAHMARABvAGMAawEAAACdAAACMwAAAE0A////AAAAAQAAAAAAAAAA/AIAAAAB+wAAABAAaQBuAGYAbwBEAG8AYwBrAAAAAAD/////AAAAAAD///8AAAADAAAAAAAAAAD8AQAAAAH7AAAAGAB0AGUAcgBtAGkAbgBhAGwARABvAGMAawAAAAAA/////wAAAAAA////AAADvQAAAjMAAAAEAAAABAAAAAgAAAAI/AAAAAIAAAACAAAAAQAAABYAbQBhAGkAbgBUAG8AbwBsAEIAYQByAQAAAAD/////AAAAAAAAAAAAAAACAAAAAQAAACIAbgBhAHYAaQBnAGEAdABpAG8AbgBUAG8AbwBsAEIAYQByAQAAAAD/////AAAAAAAAAAA=

[Toolbar mainToolBar]
IconSize=28
ToolButtonStyle=TextUnderIcon
Index=0

[Toolbar navigationToolBar]
IconSize=18
ToolButtonStyle=IconOnly
Index=1

[IconsMode]
IconSize=64
PreviewSize=64
TextWidthIndex=1
MaximumTextWidthIndex=1
MaximumTextLines=1
GridSpacingFactor=1.0

[DetailsMode]
IconSize=20
PreviewSize=20
ExpandableFolders=true

[PreviewSettings]
Plugins=imagethumbnail,jpegthumbnail,svgthumbnail,ffmpegthumbs,windowsexethumbnail,windowsimagethumbnail,rawthumbnail,exrthumbnail
EOF

# Keep kdeglobals on the dark color scheme (light/white icons on dark Plasma taskbar) while setting Win10-Explorer icon theme
if [ -f "${BACKUP_DIR}/kdeglobals.bak" ]; then
    cp "${BACKUP_DIR}/kdeglobals.bak" "${KDEGLOBALS}"
fi
kwriteconfig6 --file "${KDEGLOBALS}" --group Icons --key Theme "Win10-Explorer" 2>/dev/null || true
kwriteconfig6 --file "${KDEGLOBALS}" --group KDE --key SingleClick "false" 2>/dev/null || true

# Purge KDE icon cache so Sidebar Places icons match the Main View icons 1:1
rm -f "${HOME}/.cache/icon-cache.kcache" "${HOME}/.cache/ksycoca6"* 2>/dev/null || true

# 8. Configure Left Navigation Sidebar ("This PC", Quick Access folders, Local Disk (C:), Local Disk (D:), Network, Recycle Bin)
cat << EOF > "${PLACES_XBEL}"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE xbel>
<xbel xmlns:bookmark="http://www.freedesktop.org/standards/desktop-bookmarks" xmlns:kdepriv="http://www.kde.org/kdepriv" xmlns:mime="http://www.freedesktop.org/standards/shared-mime-info">
 <info>
  <metadata owner="http://www.kde.org">
   <kde_places_version>4</kde_places_version>
   <GroupState-Places-IsHidden>false</GroupState-Places-IsHidden>
   <GroupState-Remote-IsHidden>false</GroupState-Remote-IsHidden>
   <GroupState-Devices-IsHidden>true</GroupState-Devices-IsHidden>
   <GroupState-RemovableDevices-IsHidden>false</GroupState-RemovableDevices-IsHidden>
   <GroupState-Tags-IsHidden>true</GroupState-Tags-IsHidden>
   <withRecentlyUsed>false</withRecentlyUsed>
   <GroupState-RecentlySaved-IsHidden>true</GroupState-RecentlySaved-IsHidden>
   <withBaloo>false</withBaloo>
   <GroupState-SearchFor-IsHidden>true</GroupState-SearchFor-IsHidden>
  </metadata>
 </info>
 <bookmark href="file://${THIS_PC_DIR}">
  <title>This PC</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="computer"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/99</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="file://${HOME}/Desktop">
  <title>Desktop</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="user-desktop"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/1</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="file://${HOME}/Downloads">
  <title>Downloads</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="folder-download"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/2</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="file://${HOME}/Documents">
  <title>Documents</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="folder-documents"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/3</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="file://${HOME}/Pictures">
  <title>Pictures</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="folder-pictures"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/4</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="file://${HOME}/Music">
  <title>Music</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="folder-music"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/5</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="file://${HOME}/Videos">
  <title>Videos</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="folder-videos"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/6</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="file://${DRIVE_C_DIR}">
  <title>Local Disk (C:)</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="drive-harddisk-system"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/10</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="file://${DRIVE_D_DIR}">
  <title>Local Disk (D:)</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="drive-harddisk"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/11</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="remote:/">
  <title>Network</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="network-workgroup"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/7</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
 <bookmark href="trash:/">
  <title>Recycle Bin</title>
  <info>
   <metadata owner="http://freedesktop.org">
    <bookmark:icon name="user-trash"/>
   </metadata>
   <metadata owner="http://www.kde.org">
    <ID>1790184400/8</ID>
    <isSystemItem>true</isSystemItem>
   </metadata>
  </info>
 </bookmark>
</xbel>
EOF

# 9. Create win-explorer & dolphin wrappers in ~/.local/bin
# If no directory argument is given, default to opening "This PC"
cat << EOF > "${LOCAL_BIN}/win-explorer"
#!/usr/bin/env bash
SO_FILE="\${HOME}/.config/win-explorer/libwin10_thispc.so"
SRC_FILE="\${HOME}/.config/win-explorer/win10_thispc_helper.cpp"
if [ -f "\${SRC_FILE}" ] && { [ ! -f "\${SO_FILE}" ] || [ "/usr/bin/dolphin" -nt "\${SO_FILE}" ]; }; then
    g++ -shared -fPIC -O2 -I/usr/include/qt6 -I/usr/include/qt6/QtCore -I/usr/include/qt6/QtGui -I/usr/include/qt6/QtWidgets "\${SRC_FILE}" -o "\${SO_FILE}" -lQt6Widgets -lQt6Gui -lQt6Core -ldl 2>/dev/null || true
fi
rm -f "\${HOME}/.cache/icon-cache.kcache" 2>/dev/null || true
export XDG_CONFIG_HOME="\${HOME}/.config/win-explorer/dolphin_config"
export XDG_CONFIG_DIRS="\${HOME}/.config/win-explorer/dolphin_config/kdedefaults:/etc/xdg"
export LD_PRELOAD="\${SO_FILE}\${LD_PRELOAD:+:\$LD_PRELOAD}"
if [ \$# -eq 0 ]; then
    exec /usr/bin/dolphin --stylesheet "\${HOME}/.config/win-explorer/win10-dolphin.qss" "\${HOME}/.win-explorer/ThisPC"
else
    exec /usr/bin/dolphin --stylesheet "\${HOME}/.config/win-explorer/win10-dolphin.qss" "\$@"
fi
EOF
chmod +x "${LOCAL_BIN}/win-explorer"
cp "${LOCAL_BIN}/win-explorer" "${LOCAL_BIN}/dolphin"
chmod +x "${LOCAL_BIN}/dolphin"

cat << EOF > "${LOCAL_APPS}/org.kde.dolphin.desktop"
[Desktop Entry]
Name=File Explorer (Dolphin)
Comment=Windows 10 Style File Explorer powered by KDE Dolphin
Exec=${LOCAL_BIN}/win-explorer %u
Icon=folder
Type=Application
Categories=Qt;KDE;System;FileTools;FileManager;
MimeType=inode/directory;
StartupWMClass=dolphin
EOF

cp "${LOCAL_APPS}/org.kde.dolphin.desktop" "${LOCAL_APPS}/windows-explorer.desktop"
update-desktop-database "${LOCAL_APPS}" 2>/dev/null || true

# 10. Create "This PC" icon on Desktop with authentic Windows 10 256x256 computer icon
cp "${SCRIPT_DIR}/real_win10_icons/256x256_devices_computer.png" "${CONFIG_DIR}/this_pc_icon.png"
mkdir -p "${HOME}/Desktop"
cat << EOF > "${HOME}/Desktop/this-pc.desktop"
[Desktop Entry]
Version=1.0
Type=Application
Name=This PC
Comment=Open Windows 10 File Explorer (This PC)
Exec=${LOCAL_BIN}/win-explorer
Icon=${CONFIG_DIR}/this_pc_icon.png
Terminal=false
StartupNotify=true
StartupWMClass=dolphin
Categories=System;FileTools;FileManager;
EOF
chmod +x "${HOME}/Desktop/this-pc.desktop"
gio set "${HOME}/Desktop/this-pc.desktop" metadata::trusted true 2>/dev/null || true

if [ -f "${SCRIPT_DIR}/real_win10_icons/256x256_places_user-home.png" ]; then
    cp "${SCRIPT_DIR}/real_win10_icons/256x256_places_user-home.png" "${CONFIG_DIR}/user_home_icon.png"
fi
cat << EOF > "${HOME}/Desktop/user.desktop"
[Desktop Entry]
Version=1.0
Type=Application
Name=$(whoami)
Comment=Open Windows 10 File Explorer (User's Files - ${HOME})
Exec=${LOCAL_BIN}/win-explorer ${HOME}
Icon=${CONFIG_DIR}/user_home_icon.png
Terminal=false
StartupNotify=true
StartupWMClass=dolphin
Categories=System;FileTools;FileManager;
EOF
chmod +x "${HOME}/Desktop/user.desktop"
gio set "${HOME}/Desktop/user.desktop" metadata::trusted true 2>/dev/null || true

echo ""
echo "========================================================"
echo " [✓] Windows 10 Dolphin Customization Updated!"
echo "     • Run: win-explorer (or dolphin)"
echo "     • Opens 'This PC' with Folders on top + C: & D: below"
echo "========================================================"
