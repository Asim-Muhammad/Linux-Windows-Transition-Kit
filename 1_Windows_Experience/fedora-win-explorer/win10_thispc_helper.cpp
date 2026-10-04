#include <QTextStream>
#include <QDateTime>
#include <QAbstractProxyModel>
#include <QRegularExpression>
#include <QString>
#include <QUrl>
#include <QDesktopServices>
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
#include <QFontMetricsF>
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
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <algorithm>
#include <QElapsedTimer>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

class GlobalHider : public QObject {
public:
    GlobalHider(QObject* parent = nullptr) : QObject(parent) {}
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Show && obj->inherits("DolphinSearchBox")) {
            QTimer::singleShot(0, [obj]() { 
                if (QWidget* w = qobject_cast<QWidget*>(obj)) w->hide();
            });
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
};

// Office templates
#include "office_templates.h"

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
[[maybe_unused]] static int call_widgetIndex(const void* self) {
    static auto fn = reinterpret_cast<int(*)(const void*)>(dlsym(RTLD_DEFAULT, "_ZNK15KItemListWidget5indexEv"));
    return fn ? fn(self) : -1;
}
[[maybe_unused]] static const void* call_controllerView(const void* controller) {
    static auto fn = reinterpret_cast<const void*(*)(const void*)>(dlsym(RTLD_DEFAULT, "_ZNK19KItemListController4viewEv"));
    return fn ? fn(controller) : nullptr;
}
static const QMetaObject* get_standardWidgetMetaObject() {
    static const auto* mo = reinterpret_cast<const QMetaObject*>(dlsym(RTLD_DEFAULT, "_ZN23KStandardItemListWidget16staticMetaObjectE"));
    return mo;
}
[[maybe_unused]] static void call_prepareGeometryChange(void* qgraphicsItem) {
    static auto fn = reinterpret_cast<void(*)(void*)>(dlsym(RTLD_DEFAULT, "_ZN13QGraphicsItem21prepareGeometryChangeEv"));
    if (fn && qgraphicsItem) fn(qgraphicsItem);
}

static int g_driveCount = 2;
static int g_folderCount = 6;
static QElapsedTimer g_syncTimer;
static bool g_qtimerStarted = false;

static bool s_showExtensions = false;
static bool s_extensionsInitialized = false;

static QString getExtensionsConfigPath() {
    return QDir::homePath() + QStringLiteral("/.config/win-explorer/show_extensions.conf");
}

static bool loadShowExtensionsConfig() {
    QFile f(getExtensionsConfigPath());
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray data = f.readAll().trimmed();
        f.close();
        return data == "1" || data == "true";
    }
    return false; // Windows 10 default: hidden
}

static void saveShowExtensionsConfig(bool show) {
    QFile f(getExtensionsConfigPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(show ? "1\n" : "0\n");
        f.close();
    }
}

static void initShowExtensions() {
    if (!s_extensionsInitialized) {
        s_showExtensions = loadShowExtensionsConfig();
        s_extensionsInitialized = true;
    }
}

static void triggerNavigationAction(const QString& actionName, Qt::Key fallbackKey);
static void performMoveToTrash(const QStringList& paths);
static QStringList getSelectedMainViewPaths();

static void refreshAllViews() {
    for (QWidget* w : QApplication::allWidgets()) {
        if (!w || !w->isVisible()) continue;
        if (strcmp(w->metaObject()->className(), "DolphinView") == 0 ||
            w->inherits("KItemListWidget") ||
            w->inherits("KItemListView") ||
            w->inherits("DolphinViewContainer")) {
            w->update();
        }
    }
    triggerNavigationAction(QStringLiteral("view_redisplay"), Qt::Key_F5);
}

static void setShowExtensions(bool show) {
    initShowExtensions();
    if (s_showExtensions != show) {
        s_showExtensions = show;
        saveShowExtensionsConfig(show);
        refreshAllViews();
    }
}

[[maybe_unused]] static void toggleShowExtensions() {
    initShowExtensions();
    setShowExtensions(!s_showExtensions);
}

static QString getRawItemText(const KFileItem* self) {
    if (!self) return QString();
    static auto real_text = reinterpret_cast<QString(*)(const KFileItem*)>(
        dlsym(RTLD_NEXT, "_ZNK9KFileItem4textEv"));
    QString t = real_text ? real_text(self) : QString();
    if (t == QStringLiteral("Drive_D")) return QStringLiteral("Local Disk (D:)");
    if (t == QStringLiteral("Drive_C")) return QStringLiteral("Local Disk (C:)");
    if (t == QStringLiteral("ThisPC"))  return QStringLiteral("This PC");

    initShowExtensions();
    if (!s_showExtensions && !self->isDir()) {
        int lastDot = t.lastIndexOf(QLatin1Char('.'));
        if (lastDot > 0) {
            QString base = t.left(lastDot);
            if (!base.isEmpty()) {
                return base;
            }
        }
    }
    return t;
}

// ============================================================
// FONT HOOKS: Strip italic and bold/medium from ALL text draws
// ============================================================

// Hook QPainter::setFont — intercepts the low-level paint engine
extern "C" void _ZN8QPainter7setFontERK5QFont(void* self, const QFont& f) {
    static auto real_fn = reinterpret_cast<void(*)(void*, const QFont&)>(
        dlsym(RTLD_NEXT, "_ZN8QPainter7setFontERK5QFont"));
    if (!real_fn) return;
    QFont clean = f;
    if (clean.italic() || clean.weight() > QFont::Normal) {
        clean.setItalic(false);
        clean.setBold(false);
        clean.setWeight(QFont::Normal);
    }
    
    real_fn(self, clean);
}

// Hook QFont::setItalic to aggressively kill italics everywhere (including KItemListWidget)
extern "C" void _ZN5QFont9setItalicEb(void* self, bool b) {
    static auto real_fn = reinterpret_cast<void(*)(void*, bool)>(
        dlsym(RTLD_NEXT, "_ZN5QFont9setItalicEb"));
    if (real_fn) real_fn(self, false);
}

// Hook KFileItem::isLink() to kill symlink italic text and arrow overlays globally (perfect for "This PC" items)
extern "C" bool _ZNK9KFileItem6isLinkEv(const void* self) {
    return false;
}

// Hook QTextLayout::setFont — intercepts KDE's text layout engine (used for file labels)
extern "C" void _ZN11QTextLayout7setFontERK5QFont(void* self, const QFont& f) {
    static auto real_fn = reinterpret_cast<void(*)(void*, const QFont&)>(
        dlsym(RTLD_NEXT, "_ZN11QTextLayout7setFontERK5QFont"));
    if (!real_fn) return;
    QFont clean = f;
    if (clean.italic() || clean.weight() > QFont::Normal) {
        clean.setItalic(false);
        clean.setBold(false);
        clean.setWeight(QFont::Normal);
    }
    
    real_fn(self, clean);
}

// ============================================================

extern "C" QString _ZNK9KFileItem4textEv(const KFileItem* self) {
    return getRawItemText(self);
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

    // Remove any previous dynamic USB Drive (X:) bookmark blocks
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

    QString hiddenRecents = QStringLiteral(
        "\n <bookmark href=\"recentlyused:/files\"><title>Recent Files</title><info><metadata owner=\"http://www.kde.org\"><ID>1790184400/90</ID><isSystemItem>true</isSystemItem><isHidden>true</isHidden></metadata></info></bookmark>"
        "\n <bookmark href=\"recentlyused:/locations\"><title>Recent Locations</title><info><metadata owner=\"http://www.kde.org\"><ID>1790184400/91</ID><isSystemItem>true</isSystemItem><isHidden>true</isHidden></metadata></info></bookmark>");

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
    // Reduced from 30000ms to 500ms to provide real-time storage updates
    // while still protecting against 60fps paint() spam CPU usage.
    if (it != s_spaceCache.end() && (now - it.value().first) < 500) {
        return it.value().second;
    }
    DriveSpaceInfo computed = getLiveDriveSpaceUncached(rawName);
    s_spaceCache.insert(rawName, qMakePair(now, computed));
    return computed;
}
static DriveSpaceInfo getLiveDriveSpaceUncached(const QString& rawName) {
    DriveSpaceInfo info{0, 0, 14, QStringLiteral("")};
    const quint64 tenGB = 10ULL * 1024ULL * 1024ULL * 1024ULL;

    if (rawName == QStringLiteral("Local Disk (C:)")) {
        struct statvfs stRoot{};
        bool okRoot = (statvfs("/var", &stRoot) == 0 && (quint64)stRoot.f_blocks * stRoot.f_frsize > tenGB)
                   || (statvfs("/", &stRoot) == 0 && (quint64)stRoot.f_blocks * stRoot.f_frsize > tenGB);
        if (okRoot) {
            quint64 rootTotal = static_cast<quint64>(stRoot.f_blocks) * static_cast<quint64>(stRoot.f_frsize);
            quint64 rootUsed  = static_cast<quint64>(stRoot.f_blocks - stRoot.f_bavail) * static_cast<quint64>(stRoot.f_frsize);
            quint64 cFree     = (rootTotal > rootUsed) ? (rootTotal - rootUsed) : 0;
            info.totalBytes  = rootTotal;
            info.freeBytes   = cFree;
            info.usedPct     = (rootTotal > 0) ? static_cast<int>((rootUsed * 100ULL) / rootTotal) : 0;
            info.summaryText = QStringLiteral("%1 free of %2").arg(formatGigabytes(cFree), formatGigabytes(rootTotal));
        }
        return info;
    }

    if (rawName == QStringLiteral("Local Disk (D:)")) {
        struct statvfs stHome{};
        if (statvfs(QDir::homePath().toUtf8().constData(), &stHome) == 0) {
            quint64 dTotal = static_cast<quint64>(stHome.f_blocks) * static_cast<quint64>(stHome.f_frsize);
            quint64 dUsed  = static_cast<quint64>(stHome.f_blocks - stHome.f_bavail) * static_cast<quint64>(stHome.f_frsize);
            quint64 dFree  = (dTotal > dUsed) ? (dTotal - dUsed) : 0;
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
                    true) {
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
    if (driveCDir.exists()) {
        QString tsLink = driveCDir.filePath(QStringLiteral("timeshift"));
        if (QDir(QStringLiteral("/timeshift")).exists()) {
            if (QFileInfo(tsLink).symLinkTarget() != QStringLiteral("/timeshift")) {
                QFile::remove(tsLink);
                QFile::link(QStringLiteral("/timeshift"), tsLink);
            }
        } else {
            QString tsFallback = QDir::homePath() + QStringLiteral("/.win-explorer/timeshift_dir");
            QDir().mkpath(tsFallback);
            if (!QFile::exists(tsLink)) {
                QFile::remove(tsLink);
                QFile::link(tsFallback, tsLink);
            }
        }
    }
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
        static const QSet<QString> xdgDirs = {
            QStringLiteral("Desktop"), QStringLiteral("Documents"), QStringLiteral("Downloads"),
            QStringLiteral("Music"), QStringLiteral("Pictures"), QStringLiteral("Public"),
            QStringLiteral("Templates"), QStringLiteral("Videos")
        };
        QDir homeDir(home);
        for (const QString& item : homeDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
            if (xdgDirs.contains(item) || item.startsWith(QLatin1Char('.'))) continue;
            QString linkPath = driveCDir.filePath(item);
            if (!QFile::exists(linkPath) && !QFileInfo(linkPath).isSymLink()) {
                QFile::link(homeDir.filePath(item), linkPath);
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
                auto* proc = new QProcess();
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.remove(QStringLiteral("LD_PRELOAD"));
                proc->setProcessEnvironment(env);
                QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [proc](int, QProcess::ExitStatus) {
                    proc->deleteLater();
                    QTimer::singleShot(50, qApp, []() {
                        syncRemovableDrives();
                        refreshAllViews();
                    });
                });
                proc->start(QStringLiteral("udisksctl"),
                    {QStringLiteral("mount"), QStringLiteral("-b"), devNode, QStringLiteral("--no-user-interaction")});
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
    static const QSet<QString> s_permanentFolders = {
        QStringLiteral("Desktop"), QStringLiteral("Documents"),
        QStringLiteral("Downloads"), QStringLiteral("Music"),
        QStringLiteral("Pictures"), QStringLiteral("Videos"),
        QStringLiteral(".directory")
    };
    for (const QString& entry : thisPcDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System)) {
        if (entry.startsWith(QStringLiteral("Local Disk")) ||
            entry.startsWith(QStringLiteral("USB Drive (")) ||
            entry.startsWith(QStringLiteral("Removable Disk ("))) {
            if ((entry.startsWith(QStringLiteral("USB Drive (")) || entry.startsWith(QStringLiteral("Removable Disk ("))) &&
                !expectedLinkNames.contains(entry)) {
                QFile::remove(thisPcDir.filePath(entry));
                changed = true;
                continue;
            }
            driveCnt++;
        } else if (s_permanentFolders.contains(entry)) {
            folderCnt++;
        } else {
            // Any unauthorized folder or file dropped/created in This PC is purged immediately
            QString rogue = thisPcDir.filePath(entry);
            if (QFileInfo(rogue).isDir()) {
                QDir(rogue).removeRecursively();
            } else {
                QFile::remove(rogue);
            }
            changed = true;
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

#include <QLabel>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>

static bool isPreviewableTextFile(const QString& filePath, QString* outContent) {
    if (filePath.isEmpty()) return false;
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile() || fi.isDir()) return false;

    static const QSet<QString> kTextExts = {
        QStringLiteral("txt"), QStringLiteral("md"), QStringLiteral("markdown"),
        QStringLiteral("log"), QStringLiteral("ini"), QStringLiteral("cfg"),
        QStringLiteral("conf"), QStringLiteral("json"), QStringLiteral("xml"),
        QStringLiteral("yaml"), QStringLiteral("yml"), QStringLiteral("toml"),
        QStringLiteral("csv"), QStringLiteral("tsv"), QStringLiteral("py"),
        QStringLiteral("sh"), QStringLiteral("bash"), QStringLiteral("zsh"),
        QStringLiteral("c"), QStringLiteral("cpp"), QStringLiteral("cc"),
        QStringLiteral("h"), QStringLiteral("hpp"), QStringLiteral("js"),
        QStringLiteral("ts"), QStringLiteral("jsx"), QStringLiteral("tsx"),
        QStringLiteral("html"), QStringLiteral("htm"), QStringLiteral("css"),
        QStringLiteral("rs"), QStringLiteral("go"), QStringLiteral("java"),
        QStringLiteral("sql"), QStringLiteral("cmake"), QStringLiteral("desktop")
    };

    QString ext = fi.suffix().toLower();
    bool knownTextExt = kTextExts.contains(ext);

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray raw = f.read(65536); // Read up to 64 KB for instant preview
    f.close();

    if (raw.isEmpty()) {
        if (knownTextExt) {
            if (outContent) *outContent = QString();
            return true;
        }
        return false;
    }

    // Check for binary NUL bytes in the first 4 KB
    int checkLen = qMin<int>(raw.size(), 4096);
    for (int i = 0; i < checkLen; ++i) {
        if (raw.at(i) == '\0') {
            return false;
        }
    }

    if (outContent) {
        *outContent = QString::fromUtf8(raw);
    }
    return true;
}

static QString getSelectedMainViewFilePath() {
    QString testFile = qEnvironmentVariable("WIN10_PREVIEW_TEST_FILE");
    if (!testFile.isEmpty() && QFile::exists(testFile)) {
        return testFile;
    }

    static auto fn_hasSelection = reinterpret_cast<bool(*)(const void*)>(
        dlsym(RTLD_DEFAULT, "_ZNK25KItemListSelectionManager12hasSelectionEv"));
    static auto fn_currentItem = reinterpret_cast<int(*)(const void*)>(
        dlsym(RTLD_DEFAULT, "_ZNK25KItemListSelectionManager11currentItemEv"));
    static auto fn_isSelected = reinterpret_cast<bool(*)(const void*, int)>(
        dlsym(RTLD_DEFAULT, "_ZNK25KItemListSelectionManager10isSelectedEi"));
    static auto fn_model = reinterpret_cast<const void*(*)(const void*)>(
        dlsym(RTLD_DEFAULT, "_ZNK25KItemListSelectionManager5modelEv"));
    // System V AMD64 ABI: KFileItem has non-trivial dtor, so %rdi = sretBuf, %rsi = modelThis, %edx = index
    static auto fn_fileItemSret = reinterpret_cast<void(*)(void*, const void*, int)>(
        dlsym(RTLD_DEFAULT, "_ZNK14KFileItemModel8fileItemEi"));
    static auto fn_fileItemDtor = reinterpret_cast<void(*)(void*)>(
        dlsym(RTLD_DEFAULT, "_ZN9KFileItemD1Ev"));
    static auto fn_selCount = reinterpret_cast<int(*)(const void*)>(
        dlsym(RTLD_DEFAULT, "_ZNK11DolphinView18selectedItemsCountEv"));
    static auto fn_count = reinterpret_cast<int(*)(const void*)>(
        dlsym(RTLD_DEFAULT, "_ZNK14KFileItemModel5countEv"));

    if (!fn_hasSelection || !fn_currentItem || !fn_isSelected || !fn_model || !fn_fileItemSret || !fn_fileItemDtor) {
        return QString();
    }

    QWidget* activeWin = QApplication::activeWindow();
    if (!activeWin) {
        for (QWidget* tl : QApplication::topLevelWidgets()) {
            if (tl && tl->isVisible() && tl->inherits("QMainWindow")) { activeWin = tl; break; }
        }
    }
    if (!activeWin) return QString();

    QWidget* w = nullptr;
    for (QWidget* child : activeWin->findChildren<QWidget*>()) {
        if (strcmp(child->metaObject()->className(), "DolphinView") == 0 && child->isVisible()) {
            w = child;
            break;
        }
    }
    if (!w) return QString();
    if (fn_selCount && fn_selCount(w) != 1) {
        return QString();
    }
        for (QObject* child : w->findChildren<QObject*>()) {
            if (strcmp(child->metaObject()->className(), "KItemListSelectionManager") == 0) {
                if (!fn_hasSelection(child)) return QString();
                const void* model = fn_model(child);
                if (!model || strcmp(reinterpret_cast<const QObject*>(model)->metaObject()->className(), "KFileItemModel") != 0) {
                    return QString();
                }
                int idx = fn_currentItem(child);
                if (idx < 0 || !fn_isSelected(child, idx)) {
                    idx = -1;
                    int total = fn_count ? fn_count(const_cast<const void*>(model)) : 100000;
                    for (int k = 0; k < total; ++k) {
                        if (fn_isSelected(child, k)) {
                            idx = k;
                            break;
                        }
                    }
                }
                if (idx < 0) return QString();
                alignas(64) unsigned char itemBuf[256] = {};
                fn_fileItemSret(itemBuf, model, idx);
                const auto* itemPtr = reinterpret_cast<const KFileItem*>(itemBuf);
                QString lpath = itemPtr->localPath();
                fn_fileItemDtor(itemBuf);
                if (lpath.isEmpty()) return QString();
                return lpath;
            }
        }
    return QString();
}

#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QKeyEvent>

static void (*fn_navLocationUrl)(void*, const void*, int) = reinterpret_cast<void(*)(void*, const void*, int)>(
    dlsym(RTLD_DEFAULT, "_ZNK13KUrlNavigator11locationUrlEi"));
static void (*fn_navSetLocationUrl)(void*, const QUrl&) = reinterpret_cast<void(*)(void*, const QUrl&)>(
    dlsym(RTLD_DEFAULT, "_ZN13KUrlNavigator14setLocationUrlERK4QUrl"));
static void (*fn_navGoUp)(void*) = reinterpret_cast<void(*)(void*)>(
    dlsym(RTLD_DEFAULT, "_ZN13KUrlNavigator4goUpEv"));
static void (*fn_navGoBack)(void*) = reinterpret_cast<void(*)(void*)>(
    dlsym(RTLD_DEFAULT, "_ZN13KUrlNavigator6goBackEv"));
static void (*fn_navGoForward)(void*) = reinterpret_cast<void(*)(void*)>(
    dlsym(RTLD_DEFAULT, "_ZN13KUrlNavigator9goForwardEv"));

static QWidget* getActiveUrlNavigator() {
    for (QWidget* tl : QApplication::topLevelWidgets()) {
        if (!tl || !tl->isVisible()) continue;
        for (QWidget* w : tl->findChildren<QWidget*>()) {
            if (strcmp(w->metaObject()->className(), "KUrlNavigator") == 0) {
                return w;
            }
        }
    }
    for (QWidget* w : QApplication::allWidgets()) {
        if (w && strcmp(w->metaObject()->className(), "KUrlNavigator") == 0) {
            return w;
        }
    }
    return nullptr;
}

static QString getCurrentNavigatorPath(QWidget* nav) {
    if (!nav || !fn_navLocationUrl) return QString();
    alignas(16) unsigned char qurlBuf[32] = {};
    fn_navLocationUrl(qurlBuf, nav, -1);
    auto* u = reinterpret_cast<QUrl*>(qurlBuf);
    QString path = u->toLocalFile();
    u->~QUrl();
    return QDir::cleanPath(path);
}

static bool isCurrentNavigatorTrash(QWidget* nav) {
    if (!nav || !fn_navLocationUrl) return false;
    alignas(16) unsigned char qurlBuf[32] = {};
    fn_navLocationUrl(qurlBuf, nav, -1);
    auto* u = reinterpret_cast<QUrl*>(qurlBuf);
    bool isTrash = (u->scheme() == QStringLiteral("trash"));
    u->~QUrl();
    return isTrash;
}


static void triggerGoBack() {
    QWidget* nav = getActiveUrlNavigator();
    if (nav && fn_navGoBack) {
        fn_navGoBack(nav);
        return;
    }
    for (QWidget* tl : QApplication::topLevelWidgets()) {
        if (!tl) continue;
        for (QAction* act : tl->findChildren<QAction*>()) {
            if (act->objectName() == QStringLiteral("go_back") ||
                act->objectName().compare(QStringLiteral("back"), Qt::CaseInsensitive) == 0) {
                act->trigger();
                return;
            }
        }
    }
}

static void triggerGoForward() {
    QWidget* nav = getActiveUrlNavigator();
    if (nav && fn_navGoForward) {
        fn_navGoForward(nav);
        return;
    }
    for (QWidget* tl : QApplication::topLevelWidgets()) {
        if (!tl) continue;
        for (QAction* act : tl->findChildren<QAction*>()) {
            if (act->objectName() == QStringLiteral("go_forward") ||
                act->objectName().compare(QStringLiteral("forward"), Qt::CaseInsensitive) == 0) {
                act->trigger();
                return;
            }
        }
    }
}

static void triggerGoUp() {
    QWidget* nav = getActiveUrlNavigator();
    if (nav && fn_navSetLocationUrl) {
        QString currentPath = getCurrentNavigatorPath(nav);
        const QString home = QDir::homePath();
        const QString thisPcPath = home + QStringLiteral("/.win-explorer/ThisPC");
        const QString driveDPath = home + QStringLiteral("/.win-explorer/Drive_D");
        const QString driveCPath = home + QStringLiteral("/.win-explorer/Drive_C");

        // 1. If already at This PC, stay at This PC
        if (currentPath == thisPcPath) {
            return;
        }

        // 2. If at root of a drive or standard user folder, navigate directly to This PC
        if (currentPath == driveDPath ||
            currentPath == driveCPath ||
            currentPath == home + QStringLiteral("/Desktop") ||
            currentPath == home + QStringLiteral("/Documents") ||
            currentPath == home + QStringLiteral("/Downloads") ||
            currentPath == home + QStringLiteral("/Pictures") ||
            currentPath == home + QStringLiteral("/Music") ||
            currentPath == home + QStringLiteral("/Videos") ||
            (currentPath.startsWith(QStringLiteral("/run/media/")) && currentPath.count(QLatin1Char('/')) <= 5)) {
            fn_navSetLocationUrl(nav, QUrl::fromLocalFile(thisPcPath));
            return;
        }

        // 3. For any subfolder, compute parent directory
        if (!currentPath.isEmpty()) {
            QDir d(currentPath);
            if (d.cdUp()) {
                QString parentPath = QDir::cleanPath(d.absolutePath());
                // If parent would escape into Linux user home or .win-explorer, go to This PC
                if (parentPath == home + QStringLiteral("/.win-explorer") ||
                    parentPath == home ||
                    parentPath == QStringLiteral("/home") ||
                    parentPath == QStringLiteral("/")) {
                    fn_navSetLocationUrl(nav, QUrl::fromLocalFile(thisPcPath));
                    return;
                }
                fn_navSetLocationUrl(nav, QUrl::fromLocalFile(parentPath));
                return;
            }
        }

        // Fallback: standard KUrlNavigator goUp
        if (fn_navGoUp) {
            fn_navGoUp(nav);
            return;
        }
    }

    // Secondary fallback: trigger Dolphin's native go_up action directly without keyboard events
    for (QWidget* tl : QApplication::topLevelWidgets()) {
        if (!tl) continue;
        for (QAction* act : tl->findChildren<QAction*>()) {
            if (act->objectName() == QStringLiteral("go_up")) {
                act->trigger();
                return;
            }
        }
    }
}

static void triggerNavigationAction(const QString& actionName, Qt::Key fallbackKey) {
    QString altAction = (actionName == QStringLiteral("delete")) ? QStringLiteral("deletefile") : QString();
    for (QWidget* tl : QApplication::topLevelWidgets()) {
        if (!tl) continue;
        for (QAction* act : tl->findChildren<QAction*>()) {
            if (act->objectName() == actionName || act->objectName().compare(actionName, Qt::CaseInsensitive) == 0 ||
                (!altAction.isEmpty() && act->objectName().compare(altAction, Qt::CaseInsensitive) == 0)) {
                act->trigger();
                return;
            }
        }
    }
    for (QAction* act : qApp->findChildren<QAction*>()) {
        if (act->objectName() == actionName || act->objectName().compare(actionName, Qt::CaseInsensitive) == 0 ||
            (!altAction.isEmpty() && act->objectName().compare(altAction, Qt::CaseInsensitive) == 0)) {
            act->trigger();
            return;
        }
    }
    if (fallbackKey == Qt::Key_Delete) {
        QWidget* target = QApplication::focusWidget();
        if (!target) target = getActiveUrlNavigator();
        if (target) {
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
            QApplication::sendEvent(target, &press);
            QKeyEvent release(QEvent::KeyRelease, Qt::Key_Delete, Qt::NoModifier);
            QApplication::sendEvent(target, &release);
        }
        return;
    }
    if (fallbackKey == Qt::Key_F5) {
        QWidget* target = QApplication::focusWidget();
        if (!target) {
            for (QWidget* tl : QApplication::topLevelWidgets()) {
                if (tl->isVisible()) { target = tl; break; }
            }
        }
        if (target) {
            QKeyEvent press(QEvent::KeyPress, Qt::Key_F5, Qt::NoModifier);
            QApplication::sendEvent(target, &press);
            QKeyEvent release(QEvent::KeyRelease, Qt::Key_F5, Qt::NoModifier);
            QApplication::sendEvent(target, &release);
        }
    }
}

static void ensureNavButtonsOnLeft() {



    QToolBar* ribbonTb = nullptr;
    QAction* firstRibbonAct = nullptr;
    QList<QAction*> navActs;
    QList<QToolBar*> otherTbs;

    for (QWidget* w : QApplication::allWidgets()) {
        if (!w->inherits("QToolBar")) continue;
        auto* tb = static_cast<QToolBar*>(w);
        const QList<QAction*> acts = tb->actions();
        bool hasRibbonItems = false;
        for (QAction* a : acts) {
            if (a->isSeparator()) continue;
            QString txt = a->text();
            txt.remove(QLatin1Char('&'));
            txt = txt.trimmed();
            QString itxt = a->iconText();
            itxt.remove(QLatin1Char('&'));
            itxt = itxt.trimmed();
            if (txt.startsWith(QStringLiteral("Paste"), Qt::CaseInsensitive) ||
                itxt.startsWith(QStringLiteral("Paste"), Qt::CaseInsensitive) ||
                txt.startsWith(QStringLiteral("Cut"), Qt::CaseInsensitive) ||
                itxt.startsWith(QStringLiteral("Cut"), Qt::CaseInsensitive) ||
                txt.startsWith(QStringLiteral("Copy"), Qt::CaseInsensitive) ||
                itxt.startsWith(QStringLiteral("Copy"), Qt::CaseInsensitive)) {
                hasRibbonItems = true;
                if (!firstRibbonAct) firstRibbonAct = a;
            }
        }
        if (hasRibbonItems && !ribbonTb) {
            ribbonTb = tb;
        } else if (!hasRibbonItems) {
            otherTbs.append(tb);
        }
    }

    if (!ribbonTb || !firstRibbonAct) return;

    // Hide any other toolbars (e.g. secondary navigation bar)
    for (QToolBar* tb : otherTbs) {
        for (QAction* a : tb->actions()) {
            if (a->isSeparator()) continue;
            QString txt = a->text();
            txt.remove(QLatin1Char('&'));
            txt = txt.trimmed();
            QString itxt = a->iconText();
            itxt.remove(QLatin1Char('&'));
            itxt = itxt.trimmed();
            QString on = a->objectName();
            if (txt.compare(QStringLiteral("Back"), Qt::CaseInsensitive) == 0 ||
                itxt.compare(QStringLiteral("Back"), Qt::CaseInsensitive) == 0 ||
                on.compare(QStringLiteral("go_back"), Qt::CaseInsensitive) == 0 ||
                txt.compare(QStringLiteral("Forward"), Qt::CaseInsensitive) == 0 ||
                itxt.compare(QStringLiteral("Forward"), Qt::CaseInsensitive) == 0 ||
                on.compare(QStringLiteral("go_forward"), Qt::CaseInsensitive) == 0 ||
                txt.compare(QStringLiteral("Up"), Qt::CaseInsensitive) == 0 ||
                itxt.compare(QStringLiteral("Up"), Qt::CaseInsensitive) == 0 ||
                on.compare(QStringLiteral("go_up"), Qt::CaseInsensitive) == 0) {
                if (!navActs.contains(a)) navActs.append(a);
            }
        }
        // No longer hide secondary toolbars, they usually contain the URL address bar in Dolphin
        if (tb->isVisible() && tb->objectName() != QStringLiteral("urlToolBar") && !tb->objectName().contains("navigation", Qt::CaseInsensitive)) {
            // Only hide unknown toolbars, let urlToolBar and navigationToolBar live
            tb->hide();
        }
    }

    const QList<QAction*> rActs = ribbonTb->actions();
    int firstRibIdx = rActs.indexOf(firstRibbonAct);
    for (int i = firstRibIdx + 1; i < rActs.size(); ++i) {
        QAction* a = rActs[i];
        if (a->isSeparator()) continue;
        QString txt = a->text();
        txt.remove(QLatin1Char('&'));
        txt = txt.trimmed();
        QString itxt = a->iconText();
        itxt.remove(QLatin1Char('&'));
        itxt = itxt.trimmed();
        QString on = a->objectName();
        QWidget* aw = ribbonTb->widgetForAction(a);
        QString cls = aw ? QString::fromLatin1(aw->metaObject()->className()) : QString();

        // We no longer hide the URL Navigator or Search because the user needs the address bar
        bool isSearch = txt.contains(QStringLiteral("Search"), Qt::CaseInsensitive) || 
                        itxt.contains(QStringLiteral("Search"), Qt::CaseInsensitive) ||
                        on.contains(QStringLiteral("search"), Qt::CaseInsensitive) ||
                        cls.contains(QStringLiteral("Search"), Qt::CaseInsensitive);
        
        if ((aw && !aw->inherits("QToolButton") && !aw->inherits("QFrame") && !cls.contains(QStringLiteral("Navigator"), Qt::CaseInsensitive) && !isSearch)) {
            a->setVisible(false);
            if (aw) aw->setVisible(false);
            continue;
        }

        bool isNav = txt.compare(QStringLiteral("Back"), Qt::CaseInsensitive) == 0 ||
                     itxt.compare(QStringLiteral("Back"), Qt::CaseInsensitive) == 0 ||
                     txt.compare(QStringLiteral("Forward"), Qt::CaseInsensitive) == 0 ||
                     itxt.compare(QStringLiteral("Forward"), Qt::CaseInsensitive) == 0 ||
                     txt.compare(QStringLiteral("Up"), Qt::CaseInsensitive) == 0 ||
                     itxt.compare(QStringLiteral("Up"), Qt::CaseInsensitive) == 0 ||
                     on.compare(QStringLiteral("go_back"), Qt::CaseInsensitive) == 0 ||
                     on.compare(QStringLiteral("go_forward"), Qt::CaseInsensitive) == 0 ||
                     on.compare(QStringLiteral("go_up"), Qt::CaseInsensitive) == 0;
        if (isNav && !navActs.contains(a)) {
            navActs.append(a);
        }
    }

    // Sort navActs: Back first, Forward second, Up third
    QList<QAction*> sortedNavActs;
    QAction* backAct = nullptr;
    QAction* fwdAct = nullptr;
    QAction* upAct = nullptr;
    for (QAction* a : navActs) {
        QString txt = a->text();
        txt.remove(QLatin1Char('&'));
        txt = txt.trimmed();
        QString on = a->objectName();
        if (txt.compare(QStringLiteral("Back"), Qt::CaseInsensitive) == 0 ||
            on.compare(QStringLiteral("go_back"), Qt::CaseInsensitive) == 0) {
            backAct = a;
        } else if (txt.compare(QStringLiteral("Forward"), Qt::CaseInsensitive) == 0 ||
                   on.compare(QStringLiteral("go_forward"), Qt::CaseInsensitive) == 0) {
            fwdAct = a;
        } else if (txt.compare(QStringLiteral("Up"), Qt::CaseInsensitive) == 0 ||
                   on.compare(QStringLiteral("go_up"), Qt::CaseInsensitive) == 0) {
            upAct = a;
        }
    }
    if (backAct) sortedNavActs.append(backAct);
    if (fwdAct) sortedNavActs.append(fwdAct);
    if (upAct) sortedNavActs.append(upAct);

    // Instead of forcing navigation buttons into the top ribbon (which breaks the layout),
    // we let them naturally fall inline with the URL address bar!
    
    if (!ribbonTb->property("win10FontButtonsInjected").toBool()) {
        ribbonTb->setProperty("win10FontButtonsInjected", true);
        
        QAction* incAct = new QAction(QIcon::fromTheme("zoom-in"), "Font +", ribbonTb);
        QAction* decAct = new QAction(QIcon::fromTheme("zoom-out"), "Font -", ribbonTb);
        
        QObject::connect(incAct, &QAction::triggered, []() {
            QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Plus, Qt::ControlModifier);
            QApplication::postEvent(QApplication::activeWindow(), event);
        });
        QObject::connect(decAct, &QAction::triggered, []() {
            QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Minus, Qt::ControlModifier);
            QApplication::postEvent(QApplication::activeWindow(), event);
        });
        
        ribbonTb->addAction(incAct);
        ribbonTb->addAction(decAct);
    }

    if (!ribbonTb->property("win10NavReordered").toBool()) {
        ribbonTb->setProperty("win10NavReordered", true);
        // Removed the forceful ribbonTb->insertAction injection
    }

    // Style and wire the navigation buttons
    QToolBar* activeNavTb2 = nullptr;
    for (QWidget* w : QApplication::allWidgets()) {
        if (w->inherits("QToolBar") && w->objectName().contains("navigation", Qt::CaseInsensitive)) {
            activeNavTb2 = static_cast<QToolBar*>(w); break;
        }
    }
    for (QAction* na : sortedNavActs) {
        QWidget* aw = ribbonTb->widgetForAction(na);
        if (!aw && activeNavTb2) aw = activeNavTb2->widgetForAction(na);
        if (aw) {
            if (aw->inherits("QToolButton")) {
                auto* btn = static_cast<QToolButton*>(aw);
                btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
                btn->setPopupMode(QToolButton::DelayedPopup);
                btn->setMenu(nullptr);
                btn->setEnabled(true);
                btn->setStyleSheet(QStringLiteral(
                    "QToolButton { padding: 4px; border: 1px solid transparent; border-radius: 2px; }"
                    "QToolButton:hover { background: #E5F3FF; border: 1px solid #CCE8FF; }"
                    "QToolButton:pressed { background: #CCE8FF; border: 1px solid #99D1FF; }"
                    "QToolButton::menu-indicator, QToolButton::menu-arrow, QToolButton::menu-button { image: none; width: 0px; height: 0px; margin: 0px; padding: 0px; border: none; }"
                ));

                QString txt = na->text();
                txt.remove(QLatin1Char('&'));
                txt = txt.trimmed();
                QString on = na->objectName();
                if (!btn->property("win10NavWired").toBool()) {
                    btn->setProperty("win10NavWired", true);
                    if (txt.compare(QStringLiteral("Back"), Qt::CaseInsensitive) == 0 ||
                        on.contains(QStringLiteral("back"), Qt::CaseInsensitive)) {
                        QObject::connect(btn, &QToolButton::clicked, btn, []() { triggerGoBack(); });
                    } else if (txt.compare(QStringLiteral("Forward"), Qt::CaseInsensitive) == 0 ||
                               on.contains(QStringLiteral("forward"), Qt::CaseInsensitive)) {
                        QObject::connect(btn, &QToolButton::clicked, btn, []() { triggerGoForward(); });
                    } else if (txt.compare(QStringLiteral("Up"), Qt::CaseInsensitive) == 0 ||
                               on.contains(QStringLiteral("go_up"), Qt::CaseInsensitive)) {
                        QObject::connect(btn, &QToolButton::clicked, btn, []() { triggerGoUp(); });
                    }
                }
            }
        }
    }


    // --- INJECT PROXY SEARCH BOX & HIDER ---
    static bool s_hiderInstalled = false;
    if (!s_hiderInstalled && qApp) {
        s_hiderInstalled = true;
        qApp->installEventFilter(new GlobalHider(qApp));
    }
    
    auto hideNativeRecentPlaces = []() {
        for (QWidget* w : QApplication::allWidgets()) {
            if (w->inherits("KFilePlacesView") || w->inherits("KPlacesView")) {
                QListView* view = static_cast<QListView*>(w);
                if (!view->property("win10PlacesSized").toBool()) {
                    view->setProperty("win10PlacesSized", true);
                    view->setIconSize(QSize(20, 20));
                    view->setStyleSheet(
                        "QListView { font-size: 10pt; outline: none; }"
                        "QListView::item { padding-top: 4px; padding-bottom: 4px; padding-left: 4px; padding-right: 4px; }"
                        "QListView::item:selected { background: #cce8ff; color: #000000; border: none; }"
                        "QListView::item:hover { background: #e5f3ff; }"
                    );
                }
                if (view && view->model()) {
                    static auto fn_setPlaceHidden = reinterpret_cast<void(*)(void*, const QModelIndex&, bool)>(
                        dlsym(RTLD_DEFAULT, "_ZN16KFilePlacesModel14setPlaceHiddenERK11QModelIndexb"));
                    if (!view->property("win10PlacesModelHooked").toBool()) {
                        view->setProperty("win10PlacesModelHooked", true);
                        auto recheck = [view]() {
                            if (!view || !view->model()) return;
                            static auto s_fn = reinterpret_cast<void(*)(void*, const QModelIndex&, bool)>(
                                dlsym(RTLD_DEFAULT, "_ZN16KFilePlacesModel14setPlaceHiddenERK11QModelIndexb"));
                            if (!s_fn) return;
                            int r = view->model()->rowCount();
                            for (int k = 0; k < r; ++k) {
                                QModelIndex idx = view->model()->index(k, 0);
                                QString label = view->model()->data(idx, Qt::DisplayRole).toString().trimmed();
                                QVariant uVar = view->model()->data(idx, Qt::UserRole + 1);
                                QString uStr = uVar.toString();
                                if (label == QStringLiteral("Recent Locations") ||
                                    label == QStringLiteral("Recent") ||
                                    uStr.contains(QStringLiteral("recentlyused:"), Qt::CaseInsensitive) ||
                                    uStr.contains(QStringLiteral("timeline:"), Qt::CaseInsensitive) ||
                                    (label == QStringLiteral("Recent Files") && uStr.contains(QStringLiteral("recentlyused"), Qt::CaseInsensitive))) {
                                    void* targetModel = const_cast<QAbstractItemModel*>(view->model());
                                    if (auto* proxy = qobject_cast<QAbstractProxyModel*>(view->model())) {
                                        targetModel = proxy->sourceModel();
                                        s_fn(targetModel, proxy->mapToSource(idx), true);
                                    } else {
                                        s_fn(targetModel, idx, true);
                                    }
                                }
                            }
                        };
                        QObject::connect(view->model(), &QAbstractItemModel::rowsInserted, view, recheck);
                        QObject::connect(view->model(), &QAbstractItemModel::modelReset, view, recheck);
                        QObject::connect(view->model(), &QAbstractItemModel::layoutChanged, view, recheck);
                    }
                    int rows = view->model()->rowCount();
                    for (int i = 0; i < rows; ++i) {
                        QModelIndex idx = view->model()->index(i, 0);
                        QString label = view->model()->data(idx, Qt::DisplayRole).toString().trimmed();
                        QVariant uVar = view->model()->data(idx, Qt::UserRole + 1);
                        QString uStr = uVar.toString();
                        if (label == QStringLiteral("Recent Locations") ||
                            label == QStringLiteral("Recent") ||
                            uStr.contains(QStringLiteral("recentlyused:"), Qt::CaseInsensitive) ||
                            uStr.contains(QStringLiteral("timeline:"), Qt::CaseInsensitive) ||
                            (label == QStringLiteral("Recent Files") && uStr.contains(QStringLiteral("recentlyused"), Qt::CaseInsensitive))) {
                            if (fn_setPlaceHidden) {
                                void* targetModel = const_cast<QAbstractItemModel*>(view->model());
                                if (auto* proxy = qobject_cast<QAbstractProxyModel*>(view->model())) {
                                    targetModel = proxy->sourceModel();
                                    fn_setPlaceHidden(targetModel, proxy->mapToSource(idx), true);
                                } else {
                                    fn_setPlaceHidden(targetModel, idx, true);
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    // Run immediately now
    hideNativeRecentPlaces();
    QTimer::singleShot(50, qApp, hideNativeRecentPlaces);
    QTimer::singleShot(250, qApp, hideNativeRecentPlaces);

    static bool s_styleTimerCreated = false;
    if (!s_styleTimerCreated) {
        s_styleTimerCreated = true;
        QTimer* styleTimer = new QTimer(qApp);
        QObject::connect(styleTimer, &QTimer::timeout, hideNativeRecentPlaces);
        styleTimer->start(5000);
    }

    QToolBar* activeNavTb = nullptr;
    for (QWidget* w : QApplication::allWidgets()) {
        if (w->inherits("KUrlNavigator")) {
            QWidget* p = w->parentWidget();
            while (p) {
                if (p->inherits("QToolBar")) {
                    activeNavTb = static_cast<QToolBar*>(p);
                    break;
                }
                p = p->parentWidget();
            }
            if (activeNavTb) break;
        }
    }

    if (activeNavTb && !activeNavTb->property("win10SearchInjected").toBool()) {
        activeNavTb->setProperty("win10SearchInjected", true);
        
        for (QAction* a : activeNavTb->actions()) {
            if (a->objectName() == "toggle_search" || a->text().contains("Search", Qt::CaseInsensitive)) {
                a->setVisible(false);
            }
        }

        QWidget* spacer = new QWidget(activeNavTb);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        activeNavTb->addWidget(spacer);

        QLineEdit* searchBox = new QLineEdit(activeNavTb);
        searchBox->setPlaceholderText(QStringLiteral("Search This PC"));
        searchBox->setMinimumWidth(250);
        searchBox->setMaximumWidth(300);
        searchBox->setStyleSheet(QStringLiteral(
            "QLineEdit { border: 1px solid #CCCCCC; background-color: white; padding-left: 6px; padding-right: 24px; }"
            "QLineEdit:focus { border: 1px solid #0078D7; }"
        ));
        
        searchBox->addAction(QIcon::fromTheme(QStringLiteral("edit-find")), QLineEdit::TrailingPosition);
        
        QObject::connect(searchBox, &QLineEdit::returnPressed, [searchBox, activeNavTb]() {
            QString text = searchBox->text();
            for (QWidget* w : QApplication::allWidgets()) {
                if (w->inherits("KUrlNavigator")) {
                    QUrl u(QStringLiteral("baloosearch:/?query=") + text);
                    QMetaObject::invokeMethod(w, "setLocationUrl", Q_ARG(QUrl, u));
                    break;
                }
            }
        });
        activeNavTb->addWidget(searchBox);
    }

    // Ensure "File name extensions" button exists on the ribbon toolbar
    if (ribbonTb && !ribbonTb->property("win10ExtensionsBtnAdded").toBool()) {
        ribbonTb->setProperty("win10ExtensionsBtnAdded", true);
        auto* extAct = new QAction(ribbonTb);
        extAct->setObjectName(QStringLiteral("win10_file_extensions_ribbon"));
        extAct->setCheckable(true);
        extAct->setChecked(s_showExtensions);
        extAct->setText(QStringLiteral("File name\nextensions"));
        extAct->setIcon(QIcon::fromTheme(QStringLiteral("system-file-manager")));
        extAct->setToolTip(QStringLiteral("Show or hide the file name extensions (.txt, .png, etc.)"));
        QObject::connect(extAct, &QAction::toggled, [](bool checked) {
            setShowExtensions(checked);
        });
        ribbonTb->addAction(extAct);
    }

    // Ensure toolbar Delete button always safely moves to Trash (Recycle Bin)
    if (ribbonTb) {
        for (QAction* a : ribbonTb->actions()) {
            if (!a || a->isSeparator()) continue;
            QString txt = a->text();
            txt.remove(QLatin1Char('&'));
            txt = txt.trimmed();
            QString on = a->objectName();
            if (txt.compare(QStringLiteral("Delete"), Qt::CaseInsensitive) == 0 ||
                on.compare(QStringLiteral("delete"), Qt::CaseInsensitive) == 0 ||
                on.compare(QStringLiteral("deletefile"), Qt::CaseInsensitive) == 0 ||
                on.contains(QStringLiteral("trash"), Qt::CaseInsensitive) ||
                on.contains(QStringLiteral("delete"), Qt::CaseInsensitive)) {
                if (!a->property("win10TrashWired").toBool()) {
                    a->setProperty("win10TrashWired", true);
                    QObject::disconnect(a, &QAction::triggered, nullptr, nullptr);
                    QObject::connect(a, &QAction::triggered, []() {
                        bool isTrash = isCurrentNavigatorTrash(getActiveUrlNavigator());
                        if (isTrash) {
                            triggerNavigationAction(QStringLiteral("delete"), Qt::Key_Delete);
                            return;
                        }
                        QStringList paths = getSelectedMainViewPaths();
                        if (paths.isEmpty()) {
                            QString single = getSelectedMainViewFilePath();
                            if (!single.isEmpty()) paths << single;
                        }
                        if (!paths.isEmpty()) {
                            performMoveToTrash(paths);
                        }
                    });
                }
            }
        }
    }
}

static void polishDetailsPane() {
    QString selectedFile = getSelectedMainViewFilePath();
    QString textContent;
    const bool showTextPreview = isPreviewableTextFile(selectedFile, &textContent);

    for (QWidget* w : QApplication::allWidgets()) {
        if (!w->inherits("QDockWidget")) continue;
        if (!w->objectName().contains(QStringLiteral("info"), Qt::CaseInsensitive)) continue;
        auto* dock = static_cast<QDockWidget*>(w);

        if (!w->property("win10DockStyled").toBool()) {
            w->setProperty("win10DockStyled", true);
            if (!w->isVisible()) w->setVisible(true);
            w->setMinimumWidth(260);
            w->setMaximumWidth(320);
            if (!dock->titleBarWidget() || dock->titleBarWidget()->height() > 2) {
                auto* dummyTitle = new QWidget(dock);
                dummyTitle->setFixedHeight(0);
                dock->setTitleBarWidget(dummyTitle);
            }
            w->setStyleSheet(QStringLiteral(
                "QDockWidget { background: #FFFFFF; border-left: 1px solid #D9D9D9; }"
                "QWidget { background: #FFFFFF; color: #1E1E1E; }"
            ));
        }

        QWidget* contentRoot = dock->widget();
        if (!contentRoot) continue;

        auto* previewEdit = contentRoot->findChild<QPlainTextEdit*>(QStringLiteral("win10TextPreviewEdit"));
        if (!previewEdit) {
            previewEdit = new QPlainTextEdit(contentRoot);
            previewEdit->setObjectName(QStringLiteral("win10TextPreviewEdit"));
            previewEdit->setReadOnly(true);
            previewEdit->setFrameShape(QFrame::NoFrame);
            previewEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            previewEdit->setStyleSheet(QStringLiteral(
                "QPlainTextEdit {"
                "  background: #FFFFFF;"
                "  color: #1E1E1E;"
                "  border: none;"
                "  padding: 12px 14px;"
                "  font-family: 'Noto Sans Mono', 'Consolas', monospace;"
                "  font-size: 9.5pt;"
                "  selection-background-color: #CCE8FF;"
                "  selection-color: #000000;"
                "}"
            ));
        }

        // Hide all native Dolphin InformationPanel child widgets (icons, labels, metadata rows)
        if (!contentRoot->property("win10NativeChildrenHidden").toBool()) {
            contentRoot->setProperty("win10NativeChildrenHidden", true);
            for (QWidget* ch : contentRoot->findChildren<QWidget*>()) {
                if (ch == previewEdit || previewEdit->isAncestorOf(ch)) continue;
                if (ch->parentWidget() == contentRoot) {
                    ch->setVisible(false);
                }
            }
        }

        if (showTextPreview) {
            previewEdit->setGeometry(0, 0, contentRoot->width(), contentRoot->height());
            if (previewEdit->property("lastPreviewPath").toString() != selectedFile ||
                previewEdit->toPlainText() != textContent) {
                previewEdit->setProperty("lastPreviewPath", selectedFile);
                previewEdit->setPlainText(textContent);
            }
            previewEdit->setVisible(true);
            previewEdit->raise();
        } else {
            // Folder, Drive, non-text file, or empty space clicked -> show NOTHING (blank white pane)
            previewEdit->clear();
            previewEdit->setProperty("lastPreviewPath", QString());
            previewEdit->setVisible(false);
        }
    }
}

#include <QSocketNotifier>
#include <fcntl.h>

static dev_t g_homeDev = 0;
static ino_t g_homeIno = 0;

static void initKernelInodes() {
    if (g_homeIno != 0) return;
    struct stat st{};
    if (stat(QDir::homePath().toUtf8().constData(), &st) == 0) {
        g_homeDev = st.st_dev;
        g_homeIno = st.st_ino;
    }
}

static bool isHomeInode(const QString& path) {
    initKernelInodes();
    if (g_homeIno == 0 || path.isEmpty()) return false;
    struct stat st{};
    if (stat(path.toUtf8().constData(), &st) == 0) {
        return (st.st_ino == g_homeIno && st.st_dev == g_homeDev);
    }
    return false;
}

static bool g_polishPending = false;
static bool isDetailsPaneOpen() {
    for (QWidget* tl : QApplication::topLevelWidgets()) {
        if (!tl || !tl->isVisible()) continue;
        for (QWidget* w : tl->findChildren<QWidget*>()) {
            if (w->inherits("QDockWidget") &&
                w->objectName().contains(QStringLiteral("info"), Qt::CaseInsensitive)) {
                return w->isVisible();
            }
        }
    }
    return false;
}

static void scheduleDetailsPanePolish(int delayMs = 15) {
    if (g_polishPending || !qApp) return;
    if (!isDetailsPaneOpen()) return; // Zero overhead when details pane is closed
    g_polishPending = true;
    QTimer::singleShot(delayMs, qApp, []() {
        g_polishPending = false;
        polishDetailsPane();
    });
}

extern "C" void _ZN25KItemListSelectionManager16selectionChangedERK8KItemSetS2_(void* self, const void* current, const void* previous) {
    static auto real_fn = reinterpret_cast<void(*)(void*, const void*, const void*)>(
        dlsym(RTLD_NEXT, "_ZN25KItemListSelectionManager16selectionChangedERK8KItemSetS2_"));
    if (real_fn) real_fn(self, current, previous);
    scheduleDetailsPanePolish(5);
}

extern "C" void _ZN25KItemListSelectionManager14currentChangedEii(void* self, int current, int previous) {
    static auto real_fn = reinterpret_cast<void(*)(void*, int, int)>(
        dlsym(RTLD_NEXT, "_ZN25KItemListSelectionManager14currentChangedEii"));
    if (real_fn) real_fn(self, current, previous);
    scheduleDetailsPanePolish(5);
}

#include <QMenu>

static QStringList getSelectedMainViewPaths() {
    QStringList result;
    static auto fn_hasSelection = reinterpret_cast<bool(*)(const void*)>(
        dlsym(RTLD_DEFAULT, "_ZNK25KItemListSelectionManager12hasSelectionEv"));
    static auto fn_currentItem = reinterpret_cast<int(*)(const void*)>(
        dlsym(RTLD_DEFAULT, "_ZNK25KItemListSelectionManager11currentItemEv"));
    static auto fn_isSelected = reinterpret_cast<bool(*)(const void*, int)>(
        dlsym(RTLD_DEFAULT, "_ZNK25KItemListSelectionManager10isSelectedEi"));
    static auto fn_model = reinterpret_cast<const void*(*)(const void*)>(
        dlsym(RTLD_DEFAULT, "_ZNK25KItemListSelectionManager5modelEv"));
    static auto fn_fileItemSret = reinterpret_cast<void(*)(void*, const void*, int)>(
        dlsym(RTLD_DEFAULT, "_ZNK14KFileItemModel8fileItemEi"));
    static auto fn_fileItemDtor = reinterpret_cast<void(*)(void*)>(
        dlsym(RTLD_DEFAULT, "_ZN9KFileItemD1Ev"));
    static auto fn_count = reinterpret_cast<int(*)(const void*)>(
        dlsym(RTLD_DEFAULT, "_ZNK14KFileItemModel5countEv"));

    if (!fn_hasSelection || !fn_isSelected || !fn_model || !fn_fileItemSret || !fn_fileItemDtor) {
        return result;
    }

    for (QWidget* w : QApplication::allWidgets()) {
        if (!w->isVisible()) continue;
        if (strcmp(w->metaObject()->className(), "DolphinView") != 0) continue;
        for (QObject* child : w->findChildren<QObject*>()) {
            if (strcmp(child->metaObject()->className(), "KItemListSelectionManager") == 0) {
                const void* model = fn_model(child);
                if (!model || strcmp(reinterpret_cast<const QObject*>(model)->metaObject()->className(), "KFileItemModel") != 0) {
                    continue;
                }
                int total = fn_count ? fn_count(model) : 2000;
                for (int k = 0; k < total; ++k) {
                    if (fn_isSelected(child, k)) {
                        alignas(64) unsigned char itemBuf[256] = {};
                        fn_fileItemSret(itemBuf, model, k);
                        const auto* itemPtr = reinterpret_cast<const KFileItem*>(itemBuf);
                        QString lpath = itemPtr->localPath();
                        fn_fileItemDtor(itemBuf);
                        if (!lpath.isEmpty()) {
                            result.append(lpath);
                        }
                    }
                }
                if (result.isEmpty() && fn_currentItem) {
                    int idx = fn_currentItem(child);
                    if (idx >= 0 && idx < total) {
                        alignas(64) unsigned char itemBuf[256] = {};
                        fn_fileItemSret(itemBuf, model, idx);
                        const auto* itemPtr = reinterpret_cast<const KFileItem*>(itemBuf);
                        QString lpath = itemPtr->localPath();
                        fn_fileItemDtor(itemBuf);
                        if (!lpath.isEmpty()) {
                            result.append(lpath);
                        }
                    }
                }
                if (!result.isEmpty()) return result;
            }
        }
    }
    return result;
}

static QString find7zExecutable() {
    static QString s_cached7z;
    if (!s_cached7z.isEmpty()) return s_cached7z;
    const QStringList candidates = {
        QStringLiteral("7z"),
        QStringLiteral("7za"),
        QStringLiteral("7zz"),
        QStringLiteral("/usr/bin/7z"),
        QStringLiteral("/usr/bin/7za"),
        QStringLiteral("/usr/bin/7zz")
    };
    for (const auto& c : candidates) {
        if (!QStandardPaths::findExecutable(c).isEmpty() || QFile::exists(c)) {
            s_cached7z = c;
            return s_cached7z;
        }
    }
    s_cached7z = QStringLiteral("7z");
    return s_cached7z;
}

static void perform7zCompression(const QStringList& paths, const QString& format = QStringLiteral("7z")) {
    if (paths.isEmpty()) return;
    QFileInfo firstFi(paths[0]);
    QString dir = firstFi.path();
    QString baseName = firstFi.completeBaseName();
    if (baseName.isEmpty()) baseName = firstFi.fileName();
    if (baseName.isEmpty()) baseName = QStringLiteral("Archive");

    QString ext = (format.compare(QStringLiteral("zip"), Qt::CaseInsensitive) == 0) ? QStringLiteral(".zip") : QStringLiteral(".7z");
    QString dest = dir + QStringLiteral("/") + baseName + ext;
    int counter = 2;
    while (QFile::exists(dest)) {
        dest = dir + QStringLiteral("/") + baseName + QStringLiteral(" (") + QString::number(counter++) + QStringLiteral(")") + ext;
    }

    auto* proc = new QProcess();
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [proc](int, QProcess::ExitStatus) {
        proc->deleteLater();
        refreshAllViews();
    });
    QStringList args;
    args << QStringLiteral("a") << dest;
    args.append(paths);
    proc->start(find7zExecutable(), args);
}

static void performZipCompression(const QStringList& paths) {
    perform7zCompression(paths, QStringLiteral("zip"));
}

static void performRarCompression(const QStringList& paths) {
    if (paths.isEmpty()) return;
    QFileInfo firstFi(paths[0]);
    QString dir = firstFi.path();
    QString baseName = firstFi.completeBaseName();
    if (baseName.isEmpty()) baseName = firstFi.fileName();
    if (baseName.isEmpty()) baseName = QStringLiteral("Archive");

    QString dest = dir + QStringLiteral("/") + baseName + QStringLiteral(".rar");
    int counter = 2;
    while (QFile::exists(dest)) {
        dest = dir + QStringLiteral("/") + baseName + QStringLiteral(" (") + QString::number(counter++) + QStringLiteral(").rar");
    }

    auto* proc = new QProcess();
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [proc](int, QProcess::ExitStatus) {
        proc->deleteLater();
        refreshAllViews();
    });

    QString rarBin = QStandardPaths::findExecutable(QStringLiteral("rar"));
    if (!rarBin.isEmpty()) {
        QStringList args;
        args << QStringLiteral("a") << dest;
        args.append(paths);
        proc->start(rarBin, args);
    } else {
        QStringList args;
        args << QStringLiteral("a") << dest;
        args.append(paths);
        proc->start(find7zExecutable(), args);
    }
}

static void perform7zExtractHere(const QString& archivePath) {
    if (archivePath.isEmpty() || !QFile::exists(archivePath)) return;
    QFileInfo fi(archivePath);
    QString dir = fi.path();

    auto* proc = new QProcess();
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [proc](int, QProcess::ExitStatus) {
        proc->deleteLater();
        refreshAllViews();
    });
    proc->start(find7zExecutable(), {QStringLiteral("x"), archivePath, QStringLiteral("-o") + dir, QStringLiteral("-y")});
}

static void perform7zExtractToFolder(const QString& archivePath) {
    if (archivePath.isEmpty() || !QFile::exists(archivePath)) return;
    QFileInfo fi(archivePath);
    QString dir = fi.path();
    QString base = fi.completeBaseName();
    if (base.isEmpty()) base = fi.fileName();
    QString targetDir = dir + QStringLiteral("/") + base;

    auto* proc = new QProcess();
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [proc](int, QProcess::ExitStatus) {
        proc->deleteLater();
        refreshAllViews();
    });
    proc->start(find7zExecutable(), {QStringLiteral("x"), archivePath, QStringLiteral("-o") + targetDir, QStringLiteral("-y")});
}

static void performUnrarExtractHere(const QString& archivePath) {
    if (archivePath.isEmpty() || !QFile::exists(archivePath)) return;
    QFileInfo fi(archivePath);
    QString dir = fi.path() + QStringLiteral("/");

    auto* proc = new QProcess();
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [proc](int, QProcess::ExitStatus) {
        proc->deleteLater();
        refreshAllViews();
    });
    proc->start(QStringLiteral("unrar"), {QStringLiteral("x"), QStringLiteral("-y"), archivePath, dir});
}

static void performUnrarExtractToFolder(const QString& archivePath) {
    if (archivePath.isEmpty() || !QFile::exists(archivePath)) return;
    QFileInfo fi(archivePath);
    QString dir = fi.path();
    QString base = fi.completeBaseName();
    if (base.isEmpty()) base = fi.fileName();
    QString targetDir = dir + QStringLiteral("/") + base + QStringLiteral("/");

    auto* proc = new QProcess();
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [proc](int, QProcess::ExitStatus) {
        proc->deleteLater();
        refreshAllViews();
    });
    proc->start(QStringLiteral("unrar"), {QStringLiteral("x"), QStringLiteral("-y"), archivePath, targetDir});
}

static bool safeMoveToRecycleBin(const QString& local) {
    if (local.isEmpty()) return false;
    QFileInfo fi(local);
    if (!fi.exists() && !fi.isSymLink()) return false;

    // 1. Try native Qt moveToTrash first
    if (QFile::moveToTrash(local)) {
        return true;
    }

    // 2. Cross-filesystem fallback: Move into ~/.local/share/Trash/files/
    QString trashBase = QDir::homePath() + QStringLiteral("/.local/share/Trash");
    QDir().mkpath(trashBase + QStringLiteral("/files"));
    QDir().mkpath(trashBase + QStringLiteral("/info"));

    QString baseName = fi.fileName();
    QString targetFile = trashBase + QStringLiteral("/files/") + baseName;
    QString targetInfo = trashBase + QStringLiteral("/info/") + baseName + QStringLiteral(".trashinfo");

    int counter = 1;
    while (QFile::exists(targetFile) || QFile::exists(targetInfo)) {
        QString stem = fi.completeBaseName();
        QString ext = fi.suffix();
        QString newName = ext.isEmpty() ? QStringLiteral("%1 (%2)").arg(stem).arg(counter)
                                        : QStringLiteral("%1 (%2).%3").arg(stem).arg(counter).arg(ext);
        targetFile = trashBase + QStringLiteral("/files/") + newName;
        targetInfo = trashBase + QStringLiteral("/info/") + newName + QStringLiteral(".trashinfo");
        counter++;
    }

    QFile infoFile(targetInfo);
    if (infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QByteArray infoData = "[Trash Info]\nPath=" + fi.absoluteFilePath().toUtf8() +
                              "\nDeletionDate=" + QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8() + "\n";
        infoFile.write(infoData);
        infoFile.close();
    }

    if (QFile::rename(local, targetFile)) {
        return true;
    }

    int res = QProcess::execute(QStringLiteral("/bin/mv"), {QStringLiteral("-f"), local, targetFile});
    if (res == 0) {
        return true;
    }

    QFile::remove(targetInfo);
    return false;
}

static void performMoveToTrash(const QStringList& paths) {
    if (paths.isEmpty()) return;
    for (const QString& p : paths) {
        safeMoveToRecycleBin(p);
    }
}

struct DesktopAppInfo {
    QString name;
    QString exec;
    QString desktopFile;
    QString iconName;
    QStringList mimeTypes;
};

static QList<DesktopAppInfo> getInstalledDesktopApps() {
    static QList<DesktopAppInfo> s_cache;
    static qint64 s_lastScanTime = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!s_cache.isEmpty() && (now - s_lastScanTime < 30000)) {
        return s_cache;
    }
    s_lastScanTime = now;
    s_cache.clear();

    QStringList searchDirs = {
        QStringLiteral("/usr/share/applications"),
        QStringLiteral("/usr/local/share/applications"),
        QDir::homePath() + QStringLiteral("/.local/share/applications"),
        QStringLiteral("/var/lib/flatpak/exports/share/applications"),
        QDir::homePath() + QStringLiteral("/.local/share/flatpak/exports/share/applications")
    };

    QSet<QString> seenDesktopFiles;
    for (const QString& dpath : searchDirs) {
        QDir dir(dpath);
        if (!dir.exists()) continue;
        QStringList files = dir.entryList({QStringLiteral("*.desktop")}, QDir::Files);
        for (const QString& fn : files) {
            if (seenDesktopFiles.contains(fn)) continue;
            seenDesktopFiles.insert(fn);

            QFile f(dir.filePath(fn));
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
            QTextStream in(&f);
            QString name, exec, icon;
            QStringList mimeTypes;
            bool noDisplay = false;
            bool inDesktopEntry = false;
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line == QStringLiteral("[Desktop Entry]")) {
                    inDesktopEntry = true;
                    continue;
                }
                if (line.startsWith(QLatin1Char('['))) {
                    inDesktopEntry = false;
                    continue;
                }
                if (!inDesktopEntry) continue;

                if (line.startsWith(QStringLiteral("Name=")) && name.isEmpty()) {
                    name = line.mid(5).trimmed();
                } else if (line.startsWith(QStringLiteral("Exec=")) && exec.isEmpty()) {
                    exec = line.mid(5).trimmed();
                } else if (line.startsWith(QStringLiteral("Icon=")) && icon.isEmpty()) {
                    icon = line.mid(5).trimmed();
                } else if (line.startsWith(QStringLiteral("NoDisplay=true"), Qt::CaseInsensitive)) {
                    noDisplay = true;
                } else if (line.startsWith(QStringLiteral("MimeType=")) && mimeTypes.isEmpty()) {
                    QString rawMimes = line.mid(9).trimmed();
                    mimeTypes = rawMimes.split(QLatin1Char(';'), Qt::SkipEmptyParts);
                }
            }
            if (!noDisplay && !name.isEmpty() && !exec.isEmpty()) {
                exec.remove(QStringLiteral("%u"), Qt::CaseInsensitive);
                exec.remove(QStringLiteral("%U"), Qt::CaseInsensitive);
                exec.remove(QStringLiteral("%f"), Qt::CaseInsensitive);
                exec.remove(QStringLiteral("%F"), Qt::CaseInsensitive);
                exec.remove(QStringLiteral("%c"), Qt::CaseInsensitive);
                exec.remove(QStringLiteral("%k"), Qt::CaseInsensitive);
                exec = exec.trimmed();
                s_cache.append({name, exec, fn, icon, mimeTypes});
            }
        }
    }
    std::sort(s_cache.begin(), s_cache.end(), [](const DesktopAppInfo& a, const DesktopAppInfo& b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });
    return s_cache;
}

static bool isArchiveFile(const QString& filePath) {
    if (filePath.isEmpty()) return false;
    QString lower = filePath.toLower();
    return lower.endsWith(QStringLiteral(".zip")) ||
           lower.endsWith(QStringLiteral(".tar")) ||
           lower.endsWith(QStringLiteral(".tar.gz")) ||
           lower.endsWith(QStringLiteral(".tgz")) ||
           lower.endsWith(QStringLiteral(".tar.bz2")) ||
           lower.endsWith(QStringLiteral(".tbz2")) ||
           lower.endsWith(QStringLiteral(".tar.xz")) ||
           lower.endsWith(QStringLiteral(".txz")) ||
           lower.endsWith(QStringLiteral(".7z")) ||
           lower.endsWith(QStringLiteral(".rar")) ||
           lower.endsWith(QStringLiteral(".gz")) ||
           lower.endsWith(QStringLiteral(".bz2")) ||
           lower.endsWith(QStringLiteral(".xz")) ||
           lower.endsWith(QStringLiteral(".iso"));
}

static QList<DesktopAppInfo> getAppsForFile(const QString& filePath) {
    QList<DesktopAppInfo> result;
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();

    QList<DesktopAppInfo> all = getInstalledDesktopApps();

    auto addByName = [&](const QString& search) {
        for (const auto& app : all) {
            if (app.name.contains(search, Qt::CaseInsensitive) || app.desktopFile.contains(search, Qt::CaseInsensitive)) {
                bool exists = false;
                for (const auto& r : result) {
                    if (r.desktopFile == app.desktopFile) { exists = true; break; }
                }
                if (!exists) result.append(app);
            }
        }
    };

    // 1. High priority intuitive defaults based on file extension
    if (ext == QStringLiteral("txt") || ext == QStringLiteral("log") || ext == QStringLiteral("cfg") || ext == QStringLiteral("ini") || ext == QStringLiteral("conf")) {
        addByName(QStringLiteral("kwrite"));
        addByName(QStringLiteral("kate"));
        addByName(QStringLiteral("notepad"));
        addByName(QStringLiteral("gedit"));
        addByName(QStringLiteral("code"));
    } else if (ext == QStringLiteral("html") || ext == QStringLiteral("htm") || ext == QStringLiteral("xhtml")) {
        addByName(QStringLiteral("firefox"));
        addByName(QStringLiteral("brave"));
        addByName(QStringLiteral("chrome"));
        addByName(QStringLiteral("chromium"));
        addByName(QStringLiteral("edge"));
    } else if (ext == QStringLiteral("pdf")) {
        addByName(QStringLiteral("okular"));
        addByName(QStringLiteral("evince"));
        addByName(QStringLiteral("firefox"));
    } else if (ext == QStringLiteral("js") || ext == QStringLiteral("ts") || ext == QStringLiteral("py") || ext == QStringLiteral("json") || ext == QStringLiteral("md") || ext == QStringLiteral("cpp") || ext == QStringLiteral("c") || ext == QStringLiteral("css")) {
        addByName(QStringLiteral("code"));
        addByName(QStringLiteral("kwrite"));
        addByName(QStringLiteral("kate"));
    } else if (isArchiveFile(filePath)) {
        addByName(QStringLiteral("ark"));
        addByName(QStringLiteral("file-roller"));
    }

    // 2. MIME database matching across all installed desktop apps
    QMimeDatabase mdb;
    QMimeType mt = mdb.mimeTypeForFile(filePath);
    if (mt.isValid()) {
        QString mimeName = mt.name();
        QStringList aliases = mt.aliases();
        QStringList allMimes = aliases;
        allMimes.prepend(mimeName);

        for (const auto& app : all) {
            bool matches = false;
            for (const QString& m : allMimes) {
                if (app.mimeTypes.contains(m, Qt::CaseInsensitive)) {
                    matches = true;
                    break;
                }
            }
            if (matches) {
                bool exists = false;
                for (const auto& r : result) {
                    if (r.desktopFile == app.desktopFile) { exists = true; break; }
                }
                if (!exists) result.append(app);
            }
        }
    }

    if (result.isEmpty()) {
        addByName(QStringLiteral("kwrite"));
        addByName(QStringLiteral("firefox"));
        addByName(QStringLiteral("libreoffice"));
    }

    return result;
}

static void showChooseAnotherAppDialog(const QString& filePath, QWidget* parent) {
    auto* dlg = new QDialog(parent);
    dlg->setWindowTitle(QStringLiteral("How do you want to open this file?"));
    dlg->resize(440, 520);
    QWidget* topWin = parent ? parent->window() : nullptr;
    if (!topWin) {
        for (QWidget* w : QApplication::topLevelWidgets()) {
            if (w->isVisible() && w->width() > 400) {
                topWin = w;
                break;
            }
        }
    }
    if (topWin) {
        dlg->move(topWin->x() + (topWin->width() - dlg->width()) / 2,
                  topWin->y() + (topWin->height() - dlg->height()) / 2);
    }
    dlg->setStyleSheet(QStringLiteral(
        "QDialog { background-color: #FFFFFF; font-family: 'Segoe UI', 'Noto Sans', sans-serif; }"
        "QLabel { color: #000000; font-size: 10pt; }"
        "QLineEdit { background-color: #FFFFFF; border: 1px solid #7A7A7A; padding: 6px; font-size: 9.5pt; color: #000000; }"
        "QLineEdit:focus { border: 2px solid #0078D7; }"
        "QListWidget { background-color: #FFFFFF; border: 1px solid #CCCCCC; font-size: 9.5pt; color: #000000; outline: none; }"
        "QListWidget::item { padding: 8px 12px; height: 32px; border: 1px solid transparent; }"
        "QListWidget::item:hover { background-color: #E5F3FF; }"
        "QListWidget::item:selected { background-color: #CCE8FF; color: #000000; border: 1px solid #99D1FF; }"
        "QCheckBox { color: #000000; font-size: 9pt; spacing: 6px; }"
        "QPushButton { background-color: #E1E1E1; border: 1px solid #ADADAD; min-width: 90px; height: 26px; font-size: 9pt; color: #000000; }"
        "QPushButton:hover { background-color: #E5F1FB; border-color: #0078D7; }"
        "QPushButton:pressed { background-color: #CCE4F7; border-color: #005499; }"
        "QPushButton#btnOk { background-color: #0078D7; color: #FFFFFF; border: 1px solid #005499; font-weight: bold; }"
        "QPushButton#btnOk:hover { background-color: #1984D8; }"
    ));

    auto* layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto* lblHeader = new QLabel(QStringLiteral("Select an app to open this file:"), dlg);
    QFont hf = lblHeader->font();
    hf.setPointSize(11);
    hf.setBold(false);
    lblHeader->setFont(hf);
    layout->addWidget(lblHeader);

    QFileInfo fi(filePath);
    auto* lblFile = new QLabel(fi.fileName(), dlg);
    lblFile->setStyleSheet(QStringLiteral("color: #555555; font-size: 9pt;"));
    layout->addWidget(lblFile);

    auto* searchBox = new QLineEdit(dlg);
    searchBox->setPlaceholderText(QStringLiteral("Search installed apps..."));
    layout->addWidget(searchBox);

    auto* listWidget = new QListWidget(dlg);
    layout->addWidget(listWidget);

    QList<DesktopAppInfo> allApps = getInstalledDesktopApps();
    auto populateList = [&](const QString& filter) {
        listWidget->clear();
        for (const auto& app : allApps) {
            if (filter.isEmpty() || app.name.contains(filter, Qt::CaseInsensitive)) {
                auto* item = new QListWidgetItem(app.name, listWidget);
                item->setData(Qt::UserRole, app.exec);
                item->setData(Qt::UserRole + 1, app.desktopFile);
                if (!app.iconName.isEmpty()) {
                    item->setIcon(QIcon::fromTheme(app.iconName));
                }
            }
        }
        if (listWidget->count() > 0) listWidget->setCurrentRow(0);
    };
    populateList(QString());

    QObject::connect(searchBox, &QLineEdit::textChanged, [populateList](const QString& txt) {
        populateList(txt);
    });

    auto* chkAlways = new QCheckBox(QStringLiteral("Always use this app to open .") + fi.suffix() + QStringLiteral(" files"), dlg);
    layout->addWidget(chkAlways);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    auto* btnOk = new QPushButton(QStringLiteral("OK"), dlg);
    btnOk->setObjectName(QStringLiteral("btnOk"));
    auto* btnCancel = new QPushButton(QStringLiteral("Cancel"), dlg);
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);
    layout->addLayout(btnLayout);

    QObject::connect(btnCancel, &QPushButton::clicked, dlg, &QDialog::reject);

    auto launchSelected = [listWidget, chkAlways, filePath, fi, dlg]() {
        auto* item = listWidget->currentItem();
        if (!item) return;
        QString exec = item->data(Qt::UserRole).toString();
        QString desktopFile = item->data(Qt::UserRole + 1).toString();
        if (chkAlways->isChecked() && !fi.suffix().isEmpty()) {
            QMimeDatabase mimeDb;
            QMimeType mt = mimeDb.mimeTypeForFile(filePath);
            QString mimeName = mt.isValid() ? mt.name() : QStringLiteral("text/plain");
            QProcess::startDetached(QStringLiteral("xdg-mime"), {QStringLiteral("default"), desktopFile, mimeName});
        }
        QProcess::startDetached(exec, {filePath});
        dlg->accept();
    };

    QObject::connect(btnOk, &QPushButton::clicked, launchSelected);
    QObject::connect(listWidget, &QListWidget::itemDoubleClicked, launchSelected);

    dlg->exec();
    dlg->deleteLater();
}

static QString getCurrentFolderPath() {
    QWidget* nav = getActiveUrlNavigator();
    if (nav) {
        QString navPath = getCurrentNavigatorPath(nav);
        if (!navPath.isEmpty() && QDir(navPath).exists()) {
            return navPath;
        }
    }
    static auto fn_viewUrl = reinterpret_cast<QUrl(*)(const void*)>(dlsym(RTLD_DEFAULT, "_ZNK11DolphinView3urlEv"));
    for (QWidget* w : QApplication::allWidgets()) {
        if (!w->isVisible()) continue;
        if (strcmp(w->metaObject()->className(), "DolphinView") == 0) {
            if (fn_viewUrl) {
                QUrl u = fn_viewUrl(w);
                if (u.isLocalFile()) return QDir::cleanPath(u.toLocalFile());
            }
        }
    }
    QStringList sel = getSelectedMainViewPaths();
    if (!sel.isEmpty()) {
        return QFileInfo(sel.first()).path();
    }
    return QDir::homePath();
}

static void triggerRenameAfterCreate() {
    refreshAllViews();
    QTimer::singleShot(250, qApp, []() {
        triggerNavigationAction(QStringLiteral("rename"), Qt::Key_F2);
    });
}

static void showCreateShortcutDialog(const QString& targetFolder, QWidget* parent) {
    auto* dlg = new QDialog(parent);
    dlg->setWindowTitle(QStringLiteral("Create Shortcut"));
    dlg->resize(440, 240);
    QWidget* topWin = parent ? parent->window() : nullptr;
    if (!topWin) {
        for (QWidget* w : QApplication::topLevelWidgets()) {
            if (w->isVisible() && w->width() > 400) { topWin = w; break; }
        }
    }
    if (topWin) {
        dlg->move(topWin->x() + (topWin->width() - dlg->width()) / 2,
                  topWin->y() + (topWin->height() - dlg->height()) / 2);
    }
    dlg->setStyleSheet(QStringLiteral(
        "QDialog { background-color: #FFFFFF; font-family: 'Segoe UI', 'Noto Sans', sans-serif; }"
        "QLabel { color: #000000; font-size: 9.5pt; }"
        "QLineEdit { background-color: #FFFFFF; border: 1px solid #7A7A7A; padding: 6px; font-size: 9.5pt; color: #000000; }"
        "QLineEdit:focus { border: 2px solid #0078D7; }"
        "QPushButton { background-color: #E1E1E1; border: 1px solid #ADADAD; min-width: 90px; height: 26px; font-size: 9pt; color: #000000; }"
        "QPushButton:hover { background-color: #E5F1FB; border-color: #0078D7; }"
        "QPushButton#btnFinish { background-color: #0078D7; color: #FFFFFF; border: 1px solid #005499; font-weight: bold; }"
        "QPushButton#btnFinish:hover { background-color: #1984D8; }"
    ));

    auto* layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    auto* lblPrompt = new QLabel(QStringLiteral("Type the location of the item:"), dlg);
    layout->addWidget(lblPrompt);

    auto* locEdit = new QLineEdit(dlg);
    locEdit->setPlaceholderText(QStringLiteral("/path/to/target or application name"));
    layout->addWidget(locEdit);

    auto* lblName = new QLabel(QStringLiteral("Type a name for this shortcut:"), dlg);
    layout->addWidget(lblName);

    auto* nameEdit = new QLineEdit(dlg);
    nameEdit->setPlaceholderText(QStringLiteral("New Shortcut"));
    layout->addWidget(nameEdit);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    auto* btnFinish = new QPushButton(QStringLiteral("Finish"), dlg);
    btnFinish->setObjectName(QStringLiteral("btnFinish"));
    auto* btnCancel = new QPushButton(QStringLiteral("Cancel"), dlg);
    btnLayout->addWidget(btnFinish);
    btnLayout->addWidget(btnCancel);
    layout->addLayout(btnLayout);

    QObject::connect(btnCancel, &QPushButton::clicked, dlg, &QDialog::reject);

    QObject::connect(btnFinish, &QPushButton::clicked, [locEdit, nameEdit, targetFolder, dlg]() {
        QString targetLoc = locEdit->text().trimmed();
        QString sName = nameEdit->text().trimmed();
        if (targetLoc.isEmpty()) return;
        if (sName.isEmpty()) {
            sName = QFileInfo(targetLoc).fileName();
            if (sName.isEmpty()) sName = QStringLiteral("Shortcut");
        }
        QString destLink = targetFolder + QStringLiteral("/") + sName;
        int counter = 2;
        while (QFile::exists(destLink)) {
            destLink = targetFolder + QStringLiteral("/") + sName + QStringLiteral(" (") + QString::number(counter++) + QStringLiteral(")");
        }
        QFile::link(targetLoc, destLink);
        triggerRenameAfterCreate();
        dlg->accept();
    });

    dlg->exec();
    dlg->deleteLater();
}

static QString getWin10MenuQss() {
    static QString s_cachedQss;
    if (s_cachedQss.isEmpty()) {
        QString arrowPath = QDir::homePath() + QStringLiteral("/.config/win-explorer/menu_arrow.png");
        s_cachedQss = QStringLiteral(
            "QMenu {"
            "  background-color: #F2F2F2;"
            "  color: #000000;"
            "  border: 1px solid #CCCCCC;"
            "  padding: 2px 0px 2px 0px;"
            "  font-family: 'Segoe UI', 'Noto Sans', sans-serif;"
            "  font-size: 9pt;"
            "  min-width: 240px;"
            "}"
            "QMenu::item {"
            "  background-color: transparent;"
            "  padding: 3px 28px 3px 24px;"
            "  color: #000000;"
            "  border: 1px solid transparent;"
            "  font-family: 'Segoe UI', 'Noto Sans', sans-serif;"
            "  font-size: 9pt;"
            "  min-width: 210px;"
            "  height: 22px;"
            "}"
            "QMenu::item:selected {"
            "  background-color: #91C9F7;"
            "  color: #000000;"
            "  border: 1px solid transparent;"
            "}"
            "QMenu::item:disabled {"
            "  color: #7D7D7D;"
            "}"
            "QMenu::separator {"
            "  height: 1px;"
            "  background: #D7D7D7;"
            "  margin: 3px 1px 3px 1px;"
            "}"
            "QMenu::icon {"
            "  width: 0px;"
            "  height: 0px;"
            "  margin: 0px;"
            "}"
            "QMenu::right-arrow {"
            "  image: url(%1);"
            "  width: 6px;"
            "  height: 10px;"
            "  padding-right: 10px;"
            "}"
        ).arg(arrowPath);
    }
    return s_cachedQss;
}


static bool isThisPcPath(const QString& path) {
    if (path.isEmpty()) return false;
    QString p = QDir::cleanPath(path);
    QString thisPc = QDir::cleanPath(QDir::homePath() + QStringLiteral("/.win-explorer/ThisPC"));
    return (p == thisPc);
}

static bool isDirectThisPcItem(const QString& path) {
    if (path.isEmpty()) return false;
    QString p = QDir::cleanPath(path);
    QString thisPc = QDir::cleanPath(QDir::homePath() + QStringLiteral("/.win-explorer/ThisPC"));
    if (p == thisPc) return true;
    QFileInfo fi(p);
    return (QDir::cleanPath(fi.path()) == thisPc);
}

static void cleanContextMenu(QMenu* menu) {
    if (!menu) return;
    if (menu->objectName() == QStringLiteral("win10_sendto_menu") ||
        menu->objectName() == QStringLiteral("win10_openwith_menu") ||
        menu->objectName() == QStringLiteral("win10_7zip_menu") ||
        menu->objectName() == QStringLiteral("win10_winrar_menu") ||
        menu->objectName() == QStringLiteral("win10_new_menu")) return;
    if (menu->parentWidget() && (menu->parentWidget()->inherits("QMenu") || menu->parentWidget()->inherits("QMenuBar"))) return;
    if (menu->menuAction()) {
        for (QObject* ao : menu->menuAction()->associatedObjects()) {
            if (ao && (ao->inherits("QMenu") || ao->inherits("QMenuBar"))) {
                return;
            }
        }
    }
    if (menu->actions().isEmpty()) {
        if (!menu->property("win10_hooked_show").toBool()) {
            menu->setProperty("win10_hooked_show", true);
            QObject::connect(menu, &QMenu::aboutToShow, [menu]() {
                cleanContextMenu(menu);
            });
        }
        return;
    }

#ifdef WIN10_DEBUG
    {
        QFile logFile(QDir::homePath() + QStringLiteral("/.win-explorer/menu_debug.log"));
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream ts(&logFile);
            ts << "=== cleanContextMenu for: " << menu->metaObject()->className() << " title: " << menu->title() << " ===\n";
            for (QAction* a : menu->actions()) {
                ts << "  Action: '" << a->text() << "' obj: '" << a->objectName() << "' hasMenu: " << (a->menu() != nullptr) << "\n";
                if (a->menu()) {
                    ts << "    SubMenu class: " << a->menu()->metaObject()->className() << " obj: " << a->menu()->objectName() << "\n";
                    for (QAction* sa : a->menu()->actions()) {
                        ts << "    SubAction: '" << sa->text() << "' obj: '" << sa->objectName() << "'\n";
                    }
                }
            }
            ts << "========================================\n";
        }
    }
#endif

    struct CleanGuard {
        bool& flag;
        CleanGuard(bool& f) : flag(f) { flag = true; }
        ~CleanGuard() { flag = false; }
    };
    if (menu->property("win10_cleaned").toBool()) return;
    static bool s_inClean = false;
    if (s_inClean) return;
    CleanGuard guard(s_inClean);
    menu->setProperty("win10_cleaned", true);

    bool hasArchiveFromKdeAction = false;
    QAction* actView = nullptr;
    QAction* actSort = nullptr;
    QAction* actRefresh = nullptr;
    QAction* actPaste = nullptr;
    QAction* actSelectAll = nullptr;
    QAction* actNew = nullptr;
    QAction* actOpen = nullptr;
    QAction* actOpenInNewWindow = nullptr;
    QAction* actOpenWith = nullptr;
    QAction* actCut = nullptr;
    QAction* actCopy = nullptr;
    QAction* actRename = nullptr;
    QAction* actProperties = nullptr;
    QAction* actTerminal = nullptr;
    QAction* actRestore = nullptr;
    QAction* actEmptyTrash = nullptr;
    QList<QAction*> otherActs;

    for (QAction* a : menu->actions()) {
        if (!a) continue;
        a->setIcon(QIcon());
        a->setIconVisibleInMenu(false);
        a->setShortcut(QKeySequence());
        a->setShortcutVisibleInContextMenu(false);

        if (a->text().contains(QStringLiteral("Restore"), Qt::CaseInsensitive) || a->objectName().contains(QStringLiteral("restore"), Qt::CaseInsensitive)) actRestore = a;
        if (a->text().contains(QStringLiteral("Empty"), Qt::CaseInsensitive) || a->objectName().contains(QStringLiteral("empty"), Qt::CaseInsensitive)) actEmptyTrash = a;

        if (a->menu()) {
            for (QAction* subA : a->menu()->actions()) {
                if (subA) {
                    subA->setIcon(QIcon());
                    subA->setIconVisibleInMenu(false);
                    subA->setShortcut(QKeySequence());
                    subA->setShortcutVisibleInContextMenu(false);
                }
            }
        }
        if (a->isSeparator()) {
            a->setVisible(false);
            continue;
        }

        QString txt = a->text();
        txt.remove(QLatin1Char('&'));
        txt = txt.trimmed();

        QString itxt = a->iconText();
        itxt.remove(QLatin1Char('&'));
        itxt = itxt.trimmed();

        QString on = a->objectName();

        // 1. Detect and hide Linux clutter items
        bool isClutter = false;

        // Activities
        if (txt.contains(QStringLiteral("Activities"), Qt::CaseInsensitive) ||
            txt.contains(QStringLiteral("Activity"), Qt::CaseInsensitive) ||
            itxt.contains(QStringLiteral("Activities"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("activities"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("kactivitymanagerd"), Qt::CaseInsensitive)) {
            isClutter = true;
        }

        // Assign Tags / Tags
        if (txt.contains(QStringLiteral("Assign Tags"), Qt::CaseInsensitive) ||
            txt.compare(QStringLiteral("Tags"), Qt::CaseInsensitive) == 0 ||
            txt.startsWith(QStringLiteral("Tags..."), Qt::CaseInsensitive) ||
            txt.contains(QStringLiteral("Edit tags"), Qt::CaseInsensitive) ||
            itxt.contains(QStringLiteral("Assign Tags"), Qt::CaseInsensitive) ||
            itxt.compare(QStringLiteral("Tags"), Qt::CaseInsensitive) == 0 ||
            on.contains(QStringLiteral("tag"), Qt::CaseInsensitive)) {
            isClutter = true;
        }

        // Extract (Ark)
        if (txt.contains(QStringLiteral("Extract"), Qt::CaseInsensitive) ||
            itxt.contains(QStringLiteral("Extract"), Qt::CaseInsensitive)) {
            hasArchiveFromKdeAction = true;
            isClutter = true;
        }

        // Compress / Ark
        if (txt.startsWith(QStringLiteral("Compress"), Qt::CaseInsensitive) ||
            itxt.startsWith(QStringLiteral("Compress"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("compress"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("ark"), Qt::CaseInsensitive)) {
            isClutter = true;
        }

        // Add to Places
        if (txt.contains(QStringLiteral("Add to Places"), Qt::CaseInsensitive) ||
            itxt.contains(QStringLiteral("Add to Places"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("add_to_places"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("addToPlaces"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("addtoplaces"), Qt::CaseInsensitive)) {
            isClutter = true;
        }

        // Kleopatra / GPG / Sign / Encrypt
        if (txt.contains(QStringLiteral("Encrypt"), Qt::CaseInsensitive) ||
            txt.contains(QStringLiteral("Sign"), Qt::CaseInsensitive) ||
            txt.contains(QStringLiteral("Kleopatra"), Qt::CaseInsensitive) ||
            itxt.contains(QStringLiteral("Encrypt"), Qt::CaseInsensitive) ||
            itxt.contains(QStringLiteral("Sign"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("kleopatra"), Qt::CaseInsensitive)) {
            isClutter = true;
        }

        // Move to New Folder / Share / Copy path
        if (txt.contains(QStringLiteral("Move to New Folder"), Qt::CaseInsensitive) ||
            txt.compare(QStringLiteral("Share"), Qt::CaseInsensitive) == 0 ||
            txt.contains(QStringLiteral("Copy path"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("share"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("copy_path"), Qt::CaseInsensitive)) {
            isClutter = true;
        }

        // Open Folder With / Actions (Linux clutter)
        if (txt.contains(QStringLiteral("Open Folder With"), Qt::CaseInsensitive) ||
            txt.compare(QStringLiteral("Actions"), Qt::CaseInsensitive) == 0 ||
            itxt.contains(QStringLiteral("Open Folder With"), Qt::CaseInsensitive) ||
            itxt.compare(QStringLiteral("Actions"), Qt::CaseInsensitive) == 0 ||
            on.contains(QStringLiteral("open_folder_with"), Qt::CaseInsensitive) ||
            on.compare(QStringLiteral("actions"), Qt::CaseInsensitive) == 0 ||
            on.contains(QStringLiteral("servicemenu"), Qt::CaseInsensitive)) {
            isClutter = true;
        }

        if (isClutter) {
            a->setVisible(false);
            continue;
        }

        // 2. Classify actions for Windows 10 reordering and apply Windows 10 titles
        if (txt.contains(QStringLiteral("View Mode"), Qt::CaseInsensitive) ||
            on.contains(QStringLiteral("view_mode"), Qt::CaseInsensitive)) {
            actView = a;
            a->setText(QStringLiteral("&View"));
        } else if (txt.contains(QStringLiteral("Sort by"), Qt::CaseInsensitive) ||
                   on.contains(QStringLiteral("sort"), Qt::CaseInsensitive)) {
            actSort = a;
            a->setText(QStringLiteral("&Sort by"));
        } else if (txt.contains(QStringLiteral("Redisplay"), Qt::CaseInsensitive) ||
                   txt.contains(QStringLiteral("Refresh"), Qt::CaseInsensitive) ||
                   on.contains(QStringLiteral("redisplay"), Qt::CaseInsensitive) ||
                   on.contains(QStringLiteral("refresh"), Qt::CaseInsensitive)) {
            actRefresh = a;
            a->setText(QStringLiteral("&Refresh"));
        } else if (txt.startsWith(QStringLiteral("Paste"), Qt::CaseInsensitive) ||
                   on.contains(QStringLiteral("paste"), Qt::CaseInsensitive)) {
            actPaste = a;
            a->setText(QStringLiteral("&Paste"));
        } else if (txt.compare(QStringLiteral("Select All"), Qt::CaseInsensitive) == 0 ||
                   on.contains(QStringLiteral("select_all"), Qt::CaseInsensitive)) {
            actSelectAll = a;
        } else if (txt.contains(QStringLiteral("Create New"), Qt::CaseInsensitive) ||
                   txt.compare(QStringLiteral("New"), Qt::CaseInsensitive) == 0 ||
                   on.contains(QStringLiteral("new_menu"), Qt::CaseInsensitive)) {
            actNew = a;
            a->setText(QStringLiteral("Ne&w"));
        } else if (txt.compare(QStringLiteral("Open"), Qt::CaseInsensitive) == 0 ||
                   on.compare(QStringLiteral("open"), Qt::CaseInsensitive) == 0) {
            actOpen = a;
        } else if (txt.contains(QStringLiteral("Open in New Window"), Qt::CaseInsensitive) ||
                   on.contains(QStringLiteral("open_in_new_window"), Qt::CaseInsensitive)) {
            actOpenInNewWindow = a;
        } else if (txt.startsWith(QStringLiteral("Open with"), Qt::CaseInsensitive) ||
                   on.contains(QStringLiteral("open_with"), Qt::CaseInsensitive)) {
            isClutter = true;
        } else if (txt.compare(QStringLiteral("Cut"), Qt::CaseInsensitive) == 0 ||
                   on.compare(QStringLiteral("cut"), Qt::CaseInsensitive) == 0) {
            actCut = a;
        } else if (txt.compare(QStringLiteral("Copy"), Qt::CaseInsensitive) == 0 ||
                   on.compare(QStringLiteral("copy"), Qt::CaseInsensitive) == 0) {
            actCopy = a;
        } else if (txt.compare(QStringLiteral("Rename"), Qt::CaseInsensitive) == 0 ||
                   on.compare(QStringLiteral("rename"), Qt::CaseInsensitive) == 0) {
            actRename = a;
        } else if (txt.contains(QStringLiteral("Move to Trash"), Qt::CaseInsensitive) ||
                   txt.compare(QStringLiteral("Delete"), Qt::CaseInsensitive) == 0 ||
                   on.contains(QStringLiteral("trash"), Qt::CaseInsensitive) ||
                   on.contains(QStringLiteral("delete"), Qt::CaseInsensitive)) {
            isClutter = true;
        } else if (txt.contains(QStringLiteral("Properties"), Qt::CaseInsensitive) ||
                   on.contains(QStringLiteral("properties"), Qt::CaseInsensitive)) {
            actProperties = a;
            a->setText(QStringLiteral("P&roperties"));
        } else if (txt.contains(QStringLiteral("Terminal"), Qt::CaseInsensitive) ||
                   on.contains(QStringLiteral("terminal"), Qt::CaseInsensitive)) {
            actTerminal = a;
        } else {
            otherActs.append(a);
        }
    }

    // Determine menu type: Background vs Item context menu
    const bool isBackgroundMenu = (actView != nullptr || actSort != nullptr || (actNew != nullptr && actCut == nullptr));

    // Ensure "File name extensions" toggle is available inside View submenu
    if (actView && actView->menu()) {
        QMenu* vm = actView->menu();
        bool hasExt = false;
        for (QAction* va : vm->actions()) {
            if (va->objectName() == QStringLiteral("win10_toggle_ext")) {
                hasExt = true;
                va->setChecked(s_showExtensions);
                break;
            }
        }
        if (!hasExt) {
            vm->addSeparator();
            auto* extAction = new QAction(QStringLiteral("File name extensions"), vm);
            extAction->setObjectName(QStringLiteral("win10_toggle_ext"));
            extAction->setCheckable(true);
            extAction->setChecked(s_showExtensions);
            extAction->setIcon(QIcon());
            extAction->setIconVisibleInMenu(false);
            QObject::connect(extAction, &QAction::toggled, [](bool checked) {
                setShowExtensions(checked);
            });
            vm->addAction(extAction);
        }
    }

    // Ensure Refresh action exists for background menu
    if (!actRefresh && isBackgroundMenu) {
        for (QWidget* tl : QApplication::topLevelWidgets()) {
            if (!tl) continue;
            for (QAction* a : tl->findChildren<QAction*>()) {
                if (a->objectName() == QStringLiteral("view_redisplay") ||
                    a->objectName() == QStringLiteral("refresh") ||
                    a->objectName().compare(QStringLiteral("view_redisplay"), Qt::CaseInsensitive) == 0) {
                    actRefresh = a;
                    break;
                }
            }
            if (actRefresh) break;
        }
        if (!actRefresh) {
            auto* ref = new QAction(QStringLiteral("&Refresh"), menu);
            QObject::connect(ref, &QAction::triggered, []() {
                triggerNavigationAction(QStringLiteral("view_redisplay"), Qt::Key_F5);
            });
            actRefresh = ref;
        }
    }
    if (actRefresh) {
        actRefresh->setText(QStringLiteral("&Refresh"));
        actRefresh->setIcon(QIcon());
        actRefresh->setIconVisibleInMenu(false);
    }

    QList<QAction*> orderedList;
    auto addSep = [&]() {
        if (!orderedList.isEmpty() && !orderedList.last()->isSeparator()) {
            auto* s = new QAction(menu);
            s->setSeparator(true);
            orderedList.append(s);
        }
    };

    if (isBackgroundMenu) {
        const bool isThisPc = isThisPcPath(getCurrentFolderPath());
        // Windows 10 Background Menu Order:
        // Section 1: View, Sort by, Refresh
        // Section 2: Paste (omitted in This PC)
        // Section 3: New (omitted in This PC)
        // Section 4: Properties
        if (actView) orderedList.append(actView);
        if (actSort) orderedList.append(actSort);
        if (actRefresh) orderedList.append(actRefresh);

        if (!isThisPc && actPaste) {
            addSep();
            orderedList.append(actPaste);
        }

        if (actNew) {
            actNew->setVisible(false);
            
        }

        if (!isThisPc) {
            auto* win10NewAct = new QAction(QStringLiteral("Ne&w"), menu);
            auto* win10NewMenu = new QMenu(menu);
            win10NewMenu->setObjectName(QStringLiteral("win10_new_menu"));
            win10NewMenu->setProperty("win10_cleaned", true);
            win10NewMenu->setStyleSheet(getWin10MenuQss());

            QString iconsDir = QDir::homePath() + QStringLiteral("/.config/win-explorer/icons/");

            auto createNewFileHelper = [](const QString& baseName, const QString& ext, const unsigned char* data, int dataSize) {
                QString dir = getCurrentFolderPath();
                if (dir.isEmpty()) return;
                QString target = dir + QStringLiteral("/") + baseName + ext;
                int counter = 2;
                while (QFile::exists(target)) {
                    target = dir + QStringLiteral("/") + baseName + QStringLiteral(" (") + QString::number(counter++) + QStringLiteral(")") + ext;
                }
                QFile f(target);
                if (f.open(QIODevice::WriteOnly)) {
                    if (data && dataSize > 0) {
                        f.write(reinterpret_cast<const char*>(data), dataSize);
                    }
                    f.close();
                }
                triggerRenameAfterCreate();
            };

            // 1. Folder
            auto* actNewFolder = new QAction(QStringLiteral("&Folder"), win10NewMenu);
            QIcon folderIcon = QIcon::fromTheme(QStringLiteral("folder"));
            if (!folderIcon.isNull()) actNewFolder->setIcon(folderIcon);
            QObject::connect(actNewFolder, &QAction::triggered, []() {
                QString dir = getCurrentFolderPath();
                if (dir.isEmpty()) return;
                QString target = dir + QStringLiteral("/New folder");
                int counter = 2;
                while (QDir(target).exists()) {
                    target = dir + QStringLiteral("/New folder (") + QString::number(counter++) + QStringLiteral(")");
                }
                QDir().mkdir(target);
                triggerRenameAfterCreate();
            });

            // 2. Shortcut
            auto* actNewShortcut = new QAction(QStringLiteral("&Shortcut"), win10NewMenu);
            QIcon shortcutIcon = QIcon::fromTheme(QStringLiteral("emblem-symbolic-link"));
            if (!shortcutIcon.isNull()) actNewShortcut->setIcon(shortcutIcon);
            QObject::connect(actNewShortcut, &QAction::triggered, [menu]() {
                showCreateShortcutDialog(getCurrentFolderPath(), menu);
            });

            // 3. Microsoft Word Document (.docx / LibreOffice Writer)
            auto* actNewWord = new QAction(QStringLiteral("Microsoft &Word Document"), win10NewMenu);
            QIcon wordIcon(iconsDir + QStringLiteral("docx.png"));
            if (!wordIcon.isNull()) actNewWord->setIcon(wordIcon);
            QObject::connect(actNewWord, &QAction::triggered, [createNewFileHelper]() {
                createNewFileHelper(QStringLiteral("New Microsoft Word Document"), QStringLiteral(".docx"), s_docxTemplate, sizeof(s_docxTemplate));
            });

            // 4. Microsoft PowerPoint Presentation (.pptx / LibreOffice Impress)
            auto* actNewPpt = new QAction(QStringLiteral("Microsoft &PowerPoint Presentation"), win10NewMenu);
            QIcon pptIcon(iconsDir + QStringLiteral("pptx.png"));
            if (!pptIcon.isNull()) actNewPpt->setIcon(pptIcon);
            QObject::connect(actNewPpt, &QAction::triggered, [createNewFileHelper]() {
                createNewFileHelper(QStringLiteral("New Microsoft PowerPoint Presentation"), QStringLiteral(".pptx"), s_pptxTemplate, sizeof(s_pptxTemplate));
            });

            // 5. Microsoft Excel Worksheet (.xlsx / LibreOffice Calc)
            auto* actNewExcel = new QAction(QStringLiteral("Microsoft &Excel Worksheet"), win10NewMenu);
            QIcon excelIcon(iconsDir + QStringLiteral("xlsx.png"));
            if (!excelIcon.isNull()) actNewExcel->setIcon(excelIcon);
            QObject::connect(actNewExcel, &QAction::triggered, [createNewFileHelper]() {
                createNewFileHelper(QStringLiteral("New Microsoft Excel Worksheet"), QStringLiteral(".xlsx"), s_xlsxTemplate, sizeof(s_xlsxTemplate));
            });

            // 6. WinRAR archive (.rar)
            auto* actNewWinRar = new QAction(QStringLiteral("&WinRAR archive"), win10NewMenu);
            actNewWinRar->setObjectName(QStringLiteral("win10_new_rar"));
            QIcon rarIcon(iconsDir + QStringLiteral("rar.png"));
            if (!rarIcon.isNull()) actNewWinRar->setIcon(rarIcon);
            QObject::connect(actNewWinRar, &QAction::triggered, [createNewFileHelper]() {
                createNewFileHelper(QStringLiteral("New WinRAR archive"), QStringLiteral(".rar"), nullptr, 0);
            });

            // 7. Compressed (zipped) Folder (.zip)
            auto* actNewZip = new QAction(QStringLiteral("&Compressed (zipped) Folder"), win10NewMenu);
            actNewZip->setObjectName(QStringLiteral("win10_new_zip"));
            QIcon zipIcon(iconsDir + QStringLiteral("zip.png"));
            if (!zipIcon.isNull()) actNewZip->setIcon(zipIcon);
            QObject::connect(actNewZip, &QAction::triggered, [createNewFileHelper]() {
                static const unsigned char zipHdr[22] = {'P','K', 5, 6, 0,0, 0,0, 0,0, 0,0, 0,0,0,0, 0,0,0,0, 0,0};
                createNewFileHelper(QStringLiteral("New Compressed (zipped) Folder"), QStringLiteral(".zip"), zipHdr, 22);
            });

            // 8. Rich Text Document (.rtf)
            auto* actNewRtf = new QAction(QStringLiteral("&Rich Text Document"), win10NewMenu);
            QIcon rtfIcon = QIcon::fromTheme(QStringLiteral("x-office-document"));
            if (!rtfIcon.isNull()) actNewRtf->setIcon(rtfIcon);
            QObject::connect(actNewRtf, &QAction::triggered, [createNewFileHelper]() {
                static const char rtfHdr[] = "{\\rtf1\\ansi\\ansicpg1252\\deff0\\nouicompat\\deflang1033{\\fonttbl{\\f0\\fnil\\fcharset0 Calibri;}}\\viewkind4\\uc1\\pard\\sa200\\sl276\\slmult1\\f0\\fs22\\lang9\\par}";
                createNewFileHelper(QStringLiteral("New Rich Text Document"), QStringLiteral(".rtf"), reinterpret_cast<const unsigned char*>(rtfHdr), sizeof(rtfHdr) - 1);
            });

            // 9. Text Document (.txt)
            auto* actNewText = new QAction(QStringLiteral("&Text Document"), win10NewMenu);
            QIcon textIcon(iconsDir + QStringLiteral("notepad.png"));
            if (!textIcon.isNull()) actNewText->setIcon(textIcon);
            QObject::connect(actNewText, &QAction::triggered, [createNewFileHelper]() {
                createNewFileHelper(QStringLiteral("New Text Document"), QStringLiteral(".txt"), nullptr, 0);
            });

            win10NewMenu->addAction(actNewFolder);
            win10NewMenu->addAction(actNewShortcut);
            win10NewMenu->addSeparator();
            win10NewMenu->addAction(actNewWord);
            win10NewMenu->addAction(actNewPpt);
            win10NewMenu->addAction(actNewExcel);
            win10NewMenu->addSeparator();
            win10NewMenu->addAction(actNewWinRar);
            win10NewMenu->addAction(actNewZip);
            win10NewMenu->addSeparator();
            win10NewMenu->addAction(actNewRtf);
            win10NewMenu->addAction(actNewText);

            win10NewAct->setMenu(win10NewMenu);

            addSep();
            orderedList.append(win10NewAct);

            QString trigSub = qEnvironmentVariable("WIN10_TRIGGER_SUBMENU");
            if (trigSub == QStringLiteral("new")) {
                QTimer::singleShot(300, qApp, [menu, win10NewAct, win10NewMenu]() {
                    if (!menu || !menu->isVisible()) return;
                    menu->setActiveAction(win10NewAct);
                    QRect g = menu->actionGeometry(win10NewAct);
                    QPoint subPos = menu->mapToGlobal(QPoint(menu->width() - 2, g.top()));
                    win10NewMenu->popup(subPos);
                });
            }
        }

        if (actProperties) {
            addSep();
            orderedList.append(actProperties);
        }

        // Hide any remaining unclassified actions, terminal, or select all on background
        for (QAction* oa : otherActs) {
            if (oa) oa->setVisible(false);
        }
        if (actTerminal) actTerminal->setVisible(false);
        if (actSelectAll) actSelectAll->setVisible(false);
    } else {
        // Retrieve selected paths to determine if archive or folder is selected
        QStringList selPaths = getSelectedMainViewPaths();
        bool isThisPcItem = false;
        for (const QString& p : selPaths) { if (isDirectThisPcItem(p)) { isThisPcItem = true; break; } }
        
        bool hasArchive = hasArchiveFromKdeAction;
        bool isFolder = false;
        QString primaryArchive;
        for (const QString& p : selPaths) {
            QFileInfo fi(p);
            if (fi.isDir()) isFolder = true;
            if (isArchiveFile(p)) {
                hasArchive = true;
                if (primaryArchive.isEmpty()) primaryArchive = p;
            }
        }

        if (!actOpen) {
            actOpen = new QAction(QStringLiteral("&Open"), menu);
            actOpen->setIcon(QIcon());
            actOpen->setIconVisibleInMenu(false);
            actOpen->setShortcut(QKeySequence());
            actOpen->setShortcutVisibleInContextMenu(false);
            QFont f = actOpen->font();
            f.setBold(false);
            actOpen->setFont(f);
            QObject::connect(actOpen, &QAction::triggered, [selPaths]() {
                QStringList paths = selPaths;
                if (paths.isEmpty()) paths = getSelectedMainViewPaths();
                for (const QString& p : paths) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(p));
                }
            });
        } else {
            QFont f = actOpen->font();
            f.setBold(false);
            actOpen->setFont(f);
        }

        // 1. In Windows 10, for archives: Extract All...
        QAction* actExtractAll = nullptr;
        if (hasArchive) {
            actExtractAll = new QAction(QStringLiteral("Extract &All..."), menu);
            actExtractAll->setIcon(QIcon());
            actExtractAll->setIconVisibleInMenu(false);
            actExtractAll->setShortcut(QKeySequence());
            actExtractAll->setShortcutVisibleInContextMenu(false);
            QObject::connect(actExtractAll, &QAction::triggered, [primaryArchive, selPaths]() {
                QString p = primaryArchive;
                if (p.isEmpty()) {
                    QStringList paths = selPaths;
                    if (paths.isEmpty()) paths = getSelectedMainViewPaths();
                    for (const QString& item : paths) {
                        if (isArchiveFile(item)) {
                            p = item;
                            break;
                        }
                    }
                    if (p.isEmpty() && !paths.isEmpty()) p = paths.first();
                }
                if (!p.isEmpty()) perform7zExtractToFolder(p);
            });
        }

        // 1. Open with > Submenu (Authentic Windows 10 style)
        QMenu* openWithMenu = nullptr;
        if (!isFolder && !selPaths.isEmpty()) {
            actOpenWith = new QAction(QStringLiteral("Open &with"), menu);
            actOpenWith->setIcon(QIcon());
            actOpenWith->setIconVisibleInMenu(false);
            actOpenWith->setShortcut(QKeySequence());
            actOpenWith->setShortcutVisibleInContextMenu(false);

            openWithMenu = new QMenu(menu);
            openWithMenu->setObjectName(QStringLiteral("win10_openwith_menu"));
            openWithMenu->setProperty("win10_cleaned", true);
            openWithMenu->setStyleSheet(getWin10MenuQss());
            actOpenWith->setMenu(openWithMenu);

            QString targetFile = selPaths.first();
            QList<DesktopAppInfo> fileApps = getAppsForFile(targetFile);
            for (const auto& app : fileApps) {
                auto* appAction = new QAction(app.name, openWithMenu);
                appAction->setIcon(QIcon());
                appAction->setIconVisibleInMenu(false);
                appAction->setShortcut(QKeySequence());
                appAction->setShortcutVisibleInContextMenu(false);
                QString appExec = app.exec;
                QObject::connect(appAction, &QAction::triggered, [appExec, targetFile]() {
                    QProcess::startDetached(appExec, {targetFile});
                });
                openWithMenu->addAction(appAction);
            }
            if (!fileApps.isEmpty()) {
                openWithMenu->addSeparator();
            }
            auto* actChooseAnother = new QAction(QStringLiteral("&Choose another app"), openWithMenu);
            actChooseAnother->setIcon(QIcon());
            actChooseAnother->setIconVisibleInMenu(false);
            actChooseAnother->setShortcut(QKeySequence());
            actChooseAnother->setShortcutVisibleInContextMenu(false);
            QObject::connect(actChooseAnother, &QAction::triggered, [targetFile, menu]() {
                showChooseAnotherAppDialog(targetFile, menu);
            });
            openWithMenu->addAction(actChooseAnother);
        }

        // 2. 7-Zip Submenu
        auto* act7Zip = new QAction(QStringLiteral("&7-Zip"), menu);
        act7Zip->setIcon(QIcon());
        act7Zip->setIconVisibleInMenu(false);
        act7Zip->setShortcut(QKeySequence());
        act7Zip->setShortcutVisibleInContextMenu(false);

        auto* sevenZipMenu = new QMenu(menu);
        sevenZipMenu->setObjectName(QStringLiteral("win10_7zip_menu"));
        sevenZipMenu->setProperty("win10_cleaned", true);
        sevenZipMenu->setStyleSheet(getWin10MenuQss());
        act7Zip->setMenu(sevenZipMenu);

        QFileInfo firstFi(selPaths.isEmpty() ? QString() : selPaths.first());
        QString baseName = firstFi.completeBaseName();
        if (baseName.isEmpty()) baseName = firstFi.fileName();
        if (baseName.isEmpty()) baseName = QStringLiteral("Archive");

        if (hasArchive) {
            auto* aOpen = new QAction(QStringLiteral("Open archive"), sevenZipMenu);
            aOpen->setIcon(QIcon());
            aOpen->setIconVisibleInMenu(false);
            QObject::connect(aOpen, &QAction::triggered, [primaryArchive]() {
                QProcess::startDetached(QStringLiteral("ark"), {primaryArchive});
            });
            sevenZipMenu->addAction(aOpen);

            auto* aExtractFiles = new QAction(QStringLiteral("Extract files..."), sevenZipMenu);
            aExtractFiles->setIcon(QIcon());
            aExtractFiles->setIconVisibleInMenu(false);
            QObject::connect(aExtractFiles, &QAction::triggered, [primaryArchive]() {
                perform7zExtractToFolder(primaryArchive);
            });
            sevenZipMenu->addAction(aExtractFiles);

            auto* aExtractHere = new QAction(QStringLiteral("Extract Here"), sevenZipMenu);
            aExtractHere->setIcon(QIcon());
            aExtractHere->setIconVisibleInMenu(false);
            QObject::connect(aExtractHere, &QAction::triggered, [primaryArchive]() {
                perform7zExtractHere(primaryArchive);
            });
            sevenZipMenu->addAction(aExtractHere);

            auto* aExtractToFolder = new QAction(QStringLiteral("Extract to \"") + baseName + QStringLiteral("\\\""), sevenZipMenu);
            aExtractToFolder->setIcon(QIcon());
            aExtractToFolder->setIconVisibleInMenu(false);
            QObject::connect(aExtractToFolder, &QAction::triggered, [primaryArchive]() {
                perform7zExtractToFolder(primaryArchive);
            });
            sevenZipMenu->addAction(aExtractToFolder);

            auto* aTest = new QAction(QStringLiteral("Test archive"), sevenZipMenu);
            aTest->setIcon(QIcon());
            aTest->setIconVisibleInMenu(false);
            QObject::connect(aTest, &QAction::triggered, [primaryArchive]() {
                auto* proc = new QProcess();
                QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [proc](int, QProcess::ExitStatus) {
                    proc->deleteLater();
                });
                proc->start(find7zExecutable(), {QStringLiteral("t"), primaryArchive});
            });
            sevenZipMenu->addAction(aTest);

            sevenZipMenu->addSeparator();
        }

        auto* aAddArchive = new QAction(QStringLiteral("Add to archive..."), sevenZipMenu);
        aAddArchive->setIcon(QIcon());
        aAddArchive->setIconVisibleInMenu(false);
        QObject::connect(aAddArchive, &QAction::triggered, [selPaths]() {
            perform7zCompression(selPaths, QStringLiteral("7z"));
        });
        sevenZipMenu->addAction(aAddArchive);

        auto* aCompressEmail = new QAction(QStringLiteral("Compress and email..."), sevenZipMenu);
        aCompressEmail->setIcon(QIcon());
        aCompressEmail->setIconVisibleInMenu(false);
        sevenZipMenu->addAction(aCompressEmail);

        auto* aAdd7z = new QAction(QStringLiteral("Add to \"") + baseName + QStringLiteral(".7z\""), sevenZipMenu);
        aAdd7z->setIcon(QIcon());
        aAdd7z->setIconVisibleInMenu(false);
        QObject::connect(aAdd7z, &QAction::triggered, [selPaths]() {
            perform7zCompression(selPaths, QStringLiteral("7z"));
        });
        sevenZipMenu->addAction(aAdd7z);

        auto* aCompressEmail7z = new QAction(QStringLiteral("Compress to \"") + baseName + QStringLiteral(".7z\" and email"), sevenZipMenu);
        aCompressEmail7z->setIcon(QIcon());
        aCompressEmail7z->setIconVisibleInMenu(false);
        sevenZipMenu->addAction(aCompressEmail7z);

        auto* aAddZip = new QAction(QStringLiteral("Add to \"") + baseName + QStringLiteral(".zip\""), sevenZipMenu);
        aAddZip->setIcon(QIcon());
        aAddZip->setIconVisibleInMenu(false);
        QObject::connect(aAddZip, &QAction::triggered, [selPaths]() {
            perform7zCompression(selPaths, QStringLiteral("zip"));
        });
        sevenZipMenu->addAction(aAddZip);

        auto* aCompressEmailZip = new QAction(QStringLiteral("Compress to \"") + baseName + QStringLiteral(".zip\" and email"), sevenZipMenu);
        aCompressEmailZip->setIcon(QIcon());
        aCompressEmailZip->setIconVisibleInMenu(false);
        sevenZipMenu->addAction(aCompressEmailZip);

        // 3. WinRAR Submenu
        auto* actWinRar = new QAction(QStringLiteral("&WinRAR"), menu);
        actWinRar->setIcon(QIcon());
        actWinRar->setIconVisibleInMenu(false);
        actWinRar->setShortcut(QKeySequence());
        actWinRar->setShortcutVisibleInContextMenu(false);

        auto* winRarMenu = new QMenu(menu);
        winRarMenu->setObjectName(QStringLiteral("win10_winrar_menu"));
        winRarMenu->setProperty("win10_cleaned", true);
        winRarMenu->setStyleSheet(getWin10MenuQss());
        actWinRar->setMenu(winRarMenu);

        if (hasArchive) {
            auto* rExtractFiles = new QAction(QStringLiteral("Extract files..."), winRarMenu);
            rExtractFiles->setIcon(QIcon());
            rExtractFiles->setIconVisibleInMenu(false);
            QObject::connect(rExtractFiles, &QAction::triggered, [primaryArchive]() {
                performUnrarExtractToFolder(primaryArchive);
            });
            winRarMenu->addAction(rExtractFiles);

            auto* rExtractHere = new QAction(QStringLiteral("Extract Here"), winRarMenu);
            rExtractHere->setIcon(QIcon());
            rExtractHere->setIconVisibleInMenu(false);
            QObject::connect(rExtractHere, &QAction::triggered, [primaryArchive]() {
                performUnrarExtractHere(primaryArchive);
            });
            winRarMenu->addAction(rExtractHere);

            auto* rExtractToFolder = new QAction(QStringLiteral("Extract to ") + baseName + QStringLiteral("\\"), winRarMenu);
            rExtractToFolder->setIcon(QIcon());
            rExtractToFolder->setIconVisibleInMenu(false);
            QObject::connect(rExtractToFolder, &QAction::triggered, [primaryArchive]() {
                performUnrarExtractToFolder(primaryArchive);
            });
            winRarMenu->addAction(rExtractToFolder);

            winRarMenu->addSeparator();
        }

        auto* rAddArchive = new QAction(QStringLiteral("Add to archive..."), winRarMenu);
        rAddArchive->setIcon(QIcon());
        rAddArchive->setIconVisibleInMenu(false);
        QObject::connect(rAddArchive, &QAction::triggered, [selPaths]() {
            performRarCompression(selPaths);
        });
        winRarMenu->addAction(rAddArchive);

        auto* rAddRar = new QAction(QStringLiteral("Add to \"") + baseName + QStringLiteral(".rar\""), winRarMenu);
        rAddRar->setIcon(QIcon());
        rAddRar->setIconVisibleInMenu(false);
        QObject::connect(rAddRar, &QAction::triggered, [selPaths]() {
            performRarCompression(selPaths);
        });
        winRarMenu->addAction(rAddRar);

        auto* rCompressEmail = new QAction(QStringLiteral("Compress and email..."), winRarMenu);
        rCompressEmail->setIcon(QIcon());
        rCompressEmail->setIconVisibleInMenu(false);
        winRarMenu->addAction(rCompressEmail);

        auto* rCompressEmailRar = new QAction(QStringLiteral("Compress to \"") + baseName + QStringLiteral(".rar\" and email"), winRarMenu);
        rCompressEmailRar->setIcon(QIcon());
        rCompressEmailRar->setIconVisibleInMenu(false);
        winRarMenu->addAction(rCompressEmailRar);

        // 4. Windows 10 Send to > submenu
        auto* actSendTo = new QAction(QStringLiteral("Se&nd to"), menu);
        actSendTo->setIcon(QIcon());
        actSendTo->setIconVisibleInMenu(false);
        actSendTo->setShortcut(QKeySequence());
        actSendTo->setShortcutVisibleInContextMenu(false);

        auto* sendToMenu = new QMenu(menu);
        sendToMenu->setObjectName(QStringLiteral("win10_sendto_menu"));
        sendToMenu->setProperty("win10_cleaned", true);
        sendToMenu->setStyleSheet(getWin10MenuQss());
        actSendTo->setMenu(sendToMenu);

        // Send to: Compressed (zipped) folder
        auto* actSendToZip = new QAction(QStringLiteral("&Compressed (zipped) folder"), sendToMenu);
        actSendToZip->setIcon(QIcon());
        actSendToZip->setIconVisibleInMenu(false);
        actSendToZip->setShortcut(QKeySequence());
        actSendToZip->setShortcutVisibleInContextMenu(false);
        QObject::connect(actSendToZip, &QAction::triggered, [selPaths]() {
            QStringList paths = selPaths;
            if (paths.isEmpty()) paths = getSelectedMainViewPaths();
            if (!paths.isEmpty()) performZipCompression(paths);
        });
        sendToMenu->addAction(actSendToZip);

        // Send to: Desktop (create shortcut)
        auto* actSendToDesktop = new QAction(QStringLiteral("&Desktop (create shortcut)"), sendToMenu);
        actSendToDesktop->setIcon(QIcon());
        actSendToDesktop->setIconVisibleInMenu(false);
        actSendToDesktop->setShortcut(QKeySequence());
        actSendToDesktop->setShortcutVisibleInContextMenu(false);
        QObject::connect(actSendToDesktop, &QAction::triggered, [selPaths]() {
            QStringList paths = selPaths;
            if (paths.isEmpty()) paths = getSelectedMainViewPaths();
            QString desk = QDir::homePath() + QStringLiteral("/.win-explorer/Drive_C/Users/") + qEnvironmentVariable("USER", "user") + QStringLiteral("/Desktop");
            if (!QDir(desk).exists()) desk = QDir::homePath() + QStringLiteral("/Desktop");
            for (const QString& p : paths) {
                QFileInfo fi(p);
                QString linkTarget = desk + QStringLiteral("/") + fi.fileName();
                if (QFile::exists(linkTarget)) {
                    linkTarget = desk + QStringLiteral("/") + fi.completeBaseName() + QStringLiteral(" - Shortcut");
                    if (!fi.suffix().isEmpty()) linkTarget += QStringLiteral(".") + fi.suffix();
                }
                QFile::link(p, linkTarget);
            }
        });
        sendToMenu->addAction(actSendToDesktop);

        // Send to: Documents
        auto* actSendToDocs = new QAction(QStringLiteral("&Documents"), sendToMenu);
        actSendToDocs->setIcon(QIcon());
        actSendToDocs->setIconVisibleInMenu(false);
        actSendToDocs->setShortcut(QKeySequence());
        actSendToDocs->setShortcutVisibleInContextMenu(false);
        sendToMenu->addAction(actSendToDocs);

        // Send to: Mail recipient
        auto* actSendToMail = new QAction(QStringLiteral("&Mail recipient"), sendToMenu);
        actSendToMail->setIcon(QIcon());
        actSendToMail->setIconVisibleInMenu(false);
        actSendToMail->setShortcut(QKeySequence());
        actSendToMail->setShortcutVisibleInContextMenu(false);
        sendToMenu->addAction(actSendToMail);

        QString trigSub = qEnvironmentVariable("WIN10_TRIGGER_SUBMENU");
        if (!trigSub.isEmpty()) {
            QTimer::singleShot(250, menu, [menu, trigSub, actOpenWith, act7Zip, actWinRar, actSendTo, openWithMenu, sevenZipMenu, winRarMenu, sendToMenu]() {
                if (!menu || !menu->isVisible()) return;
                QAction* targetAct = actSendTo;
                QMenu* targetSub = sendToMenu;
                if (trigSub == QStringLiteral("openwith") || trigSub == QStringLiteral("open_with")) {
                    targetAct = actOpenWith; targetSub = openWithMenu;
                } else if (trigSub == QStringLiteral("7zip") || trigSub == QStringLiteral("sevenzip")) {
                    targetAct = act7Zip; targetSub = sevenZipMenu;
                } else if (trigSub == QStringLiteral("winrar") || trigSub == QStringLiteral("rar")) {
                    targetAct = actWinRar; targetSub = winRarMenu;
                }
                if (targetAct && targetSub) {
                    menu->setActiveAction(targetAct);
                    QRect g = menu->actionGeometry(targetAct);
                    QPoint subPos = menu->mapToGlobal(QPoint(menu->width() - 2, g.top()));
                    targetSub->popup(subPos);
                }
            });
        }

        // 5. Windows 10 Create shortcut action
        auto* actCreateShortcut = new QAction(QStringLiteral("Create &shortcut"), menu);
        actCreateShortcut->setIcon(QIcon());
        actCreateShortcut->setIconVisibleInMenu(false);
        actCreateShortcut->setShortcut(QKeySequence());
        actCreateShortcut->setShortcutVisibleInContextMenu(false);
        QObject::connect(actCreateShortcut, &QAction::triggered, [selPaths]() {
            QStringList paths = selPaths;
            if (paths.isEmpty()) paths = getSelectedMainViewPaths();
            for (const QString& p : paths) {
                QFileInfo fi(p);
                QString dir = fi.path();
                QString base = fi.completeBaseName();
                QString ext = fi.suffix();
                QString linkTarget = dir + QStringLiteral("/") + base + QStringLiteral(" - Shortcut");
                if (!ext.isEmpty()) linkTarget += QStringLiteral(".") + ext;
                int counter = 2;
                while (QFile::exists(linkTarget)) {
                    linkTarget = dir + QStringLiteral("/") + base + QStringLiteral(" - Shortcut (") + QString::number(counter++) + QStringLiteral(")");
                    if (!ext.isEmpty()) linkTarget += QStringLiteral(".") + ext;
                }
                QFile::link(p, linkTarget);
            }
            refreshAllViews();
        });

        // Exact Windows 10 File/Folder Item Menu Order:
        // Section 1: Open / Open in new window / Extract All... / Open with
        if (actOpen) orderedList.append(actOpen);
        if (actOpenInNewWindow) orderedList.append(actOpenInNewWindow);
        if (hasArchive && actExtractAll) orderedList.append(actExtractAll);
        if (actOpenWith) orderedList.append(actOpenWith);

        // Section 2: 7-Zip, WinRAR
        addSep();
        orderedList.append(act7Zip);
        orderedList.append(actWinRar);

        // Section 3: Send to >
        addSep();
        orderedList.append(actSendTo);

        
        for (const QString& p : selPaths) {
            if (isDirectThisPcItem(p)) {
                isThisPcItem = true;
                break;
            }
        }

        // Section 4: Cut, Copy
        if (!isThisPcItem && (actCut || actCopy)) {
            addSep();
            if (actCut) orderedList.append(actCut);
            if (actCopy) orderedList.append(actCopy);
        } else if (isThisPcItem && actCopy) {
            addSep();
            orderedList.append(actCopy);
        }

        // Section 5: Create shortcut, Delete, Rename (disabled for This PC items)
        if (!isThisPcItem) {
            addSep();
            orderedList.append(actCreateShortcut);

            auto* actWin10Delete = new QAction(QStringLiteral("&Delete"), menu);
            actWin10Delete->setIcon(QIcon());
            actWin10Delete->setIconVisibleInMenu(false);
            actWin10Delete->setShortcut(QKeySequence(Qt::Key_Delete));
            actWin10Delete->setShortcutVisibleInContextMenu(false);
            
            bool isTrash = isCurrentNavigatorTrash(getActiveUrlNavigator());
            
            QObject::connect(actWin10Delete, &QAction::triggered, [selPaths, isTrash]() {
                if (isTrash) {
                    triggerNavigationAction(QStringLiteral("delete"), Qt::Key_Delete);
                } else {
                    QStringList paths = selPaths;
                    if (paths.isEmpty()) paths = getSelectedMainViewPaths();
                    if (paths.isEmpty()) {
                        QString single = getSelectedMainViewFilePath();
                        if (!single.isEmpty()) paths << single;
                    }
                    if (!paths.isEmpty()) {
                        performMoveToTrash(paths);
                    }
                }
            });
            orderedList.append(actWin10Delete);

            if (actRename) orderedList.append(actRename);
        }


        
        if (actRestore) { addSep(); orderedList.append(actRestore); }
        if (actEmptyTrash) { addSep(); orderedList.append(actEmptyTrash); }

        // Section 6: Properties
        if (actProperties) {
            addSep();
            orderedList.append(actProperties);
        }
    }

    // Rearrange actions in menu by inserting each action at the end in sequence
    for (QAction* act : orderedList) {
        if (act) {
            act->setIcon(QIcon());
            act->setIconVisibleInMenu(false);
            act->setShortcut(QKeySequence());
            act->setShortcutVisibleInContextMenu(false);
            menu->insertAction(nullptr, act);
        }
    }

    // Remove or hide any remaining unlisted action so no clutter leaks in
    for (QAction* a : menu->actions()) {
        if (a && !orderedList.contains(a)) {
            a->setVisible(false);
            
        }
    }

    menu->setStyleSheet(getWin10MenuQss());

#ifdef WIN10_DEBUG
    fprintf(stderr, "=== CLEAN CONTEXT MENU: isBg=%d, total=%d ===\n", isBackgroundMenu, orderedList.size());
    for (QAction* act : orderedList) {
        if (act && act->isVisible()) {
            fprintf(stderr, "  ACTION: '%s'\n", act->text().toUtf8().constData());
        }
    }
#endif
}


static QString toWinPath(const QString& linuxPath) {
    if (linuxPath.isEmpty()) return QString();
    QString p = QDir::cleanPath(linuxPath);
    if (p == QDir::homePath()) {
        return QStringLiteral("D:\\");
    }
    if (p.startsWith(QDir::homePath() + QStringLiteral("/"))) {
        p = QStringLiteral("D:") + p.mid(QDir::homePath().length());
    } else if (p == QStringLiteral("/")) {
        return QStringLiteral("C:\\");
    } else if (p.startsWith(QStringLiteral("/"))) {
        p = QStringLiteral("C:") + p;
    }
    p.replace(QLatin1Char('/'), QLatin1Char('\\'));
    return p;
}

static QString toLinuxPath(const QString& winPath) {
    if (winPath.isEmpty()) return QStringLiteral("/");
    QString p = winPath.trimmed();
    p.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (p.startsWith(QStringLiteral("D:"), Qt::CaseInsensitive)) {
        p = QDir::homePath() + p.mid(2);
    } else if (p.startsWith(QStringLiteral("C:"), Qt::CaseInsensitive)) {
        p = p.mid(2);
    }
    p = QDir::cleanPath(p);
    if (p.isEmpty() || !p.startsWith(QLatin1Char('/'))) {
        p = QLatin1Char('/') + p;
    }
    return p;
}

class Win10KernelEventFilter : public QObject {
public:
    using QObject::QObject;
protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        const QEvent::Type t = event->type();
        if (t == QEvent::Show || t == QEvent::FocusIn) {
            if (watched && watched->isWidgetType()) {
                auto* w = static_cast<QWidget*>(watched);
                if (w->inherits("QLineEdit")) {
                    bool isNav = false;
                    QWidget* p = w->parentWidget();
                    while (p) {
                        if (strcmp(p->metaObject()->className(), "KUrlNavigator") == 0) {
                            isNav = true; break;
                        }
                        p = p->parentWidget();
                    }
                    if (isNav) {
                        auto* le = static_cast<QLineEdit*>(w);
                        QTimer::singleShot(10, le, [le]() {
                            QString p = toWinPath(le->text());
                            if (le->text() != p) {
                                bool b = le->blockSignals(true);
                                le->setText(p);
                                le->blockSignals(b);
                            }
                        });
                    }
                }
            }
        }
        
        if (t == QEvent::Show || t == QEvent::ShowToParent || t == QEvent::Polish) {
            if (watched && watched->isWidgetType()) {
                auto* w = static_cast<QWidget*>(watched);
                
                // Rename breadcrumbs in the URL Navigator to Windows drive names
                if (w->inherits("QAbstractButton")) {
                    bool isNavChild = false;
                    QWidget* p = w->parentWidget();
                    while (p) {
                        if (strcmp(p->metaObject()->className(), "KUrlNavigator") == 0) {
                            isNavChild = true; break;
                        }
                        p = p->parentWidget();
                    }
                    if (isNavChild) {
                        auto* btn = static_cast<QAbstractButton*>(w);
                        QString txt = btn->text();
                        if (txt == QStringLiteral("/") || txt == QStringLiteral("home")) {
                            btn->hide();
                            btn->setMaximumWidth(0);
                        } else if (txt == qgetenv("USER") || txt == QStringLiteral("user")) {
                            if (txt != QStringLiteral("This PC")) {
                                btn->setText(QStringLiteral("This PC"));
                            }
                        } else if (txt == QStringLiteral(".win-explorer")) {
                            btn->hide();
                            btn->setMaximumWidth(0);
                        } else if (txt == QStringLiteral("ThisPC")) {
                            btn->hide();
                            btn->setMaximumWidth(0);
                        }
                    }
                }
                
                if (w->inherits("QMenu")) {
                    cleanContextMenu(static_cast<QMenu*>(w));
                }
                if (w->inherits("QDockWidget") && !w->objectName().contains(QStringLiteral("info"), Qt::CaseInsensitive)) {
                    if (w->minimumWidth() < 230) w->setMinimumWidth(230);
                }
                const char* cls = w->metaObject()->className();
                if (strstr(cls, "MetaData") || strstr(cls, "InformationPanel") || w->objectName().contains(QStringLiteral("info"), Qt::CaseInsensitive)) {
                    scheduleDetailsPanePolish(20);
                }
            }
        } else if (t == QEvent::ChildPolished) {
            if (watched && watched->isWidgetType()) {
                auto* w = static_cast<QWidget*>(watched);
                if (w->inherits("QDockWidget") && !w->objectName().contains(QStringLiteral("info"), Qt::CaseInsensitive)) {
                    if (w->minimumWidth() < 230) w->setMinimumWidth(230);
                }
            }

        } else if (t == QEvent::KeyPress) {
            auto* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                if (watched && watched->isWidgetType()) {
                    auto* w = static_cast<QWidget*>(watched);
                    if (w->inherits("QLineEdit")) {
                        bool isNav = false;
                        QWidget* p = w->parentWidget();
                        while (p) {
                            if (strcmp(p->metaObject()->className(), "KUrlNavigator") == 0) {
                                isNav = true; break;
                            }
                            p = p->parentWidget();
                        }
                        if (isNav) {
                            auto* le = static_cast<QLineEdit*>(w);
                            QString p = toLinuxPath(le->text());
                            if (le->text() != p) {
                                bool b = le->blockSignals(true);
                                le->setText(p);
                                le->blockSignals(b);
                            }
                        }
                    }
                }
            }
            if (ke->modifiers() == Qt::AltModifier && ke->key() == Qt::Key_Up) {
                triggerGoUp();
                return true;
            }
            if (ke->modifiers() == Qt::AltModifier && ke->key() == Qt::Key_Left) {
                triggerGoBack();
                return true;
            }
            if (ke->modifiers() == Qt::AltModifier && ke->key() == Qt::Key_Right) {
                triggerGoForward();
                return true;
            }
            if (ke->key() == Qt::Key_Delete) {
                QWidget* fw = QApplication::focusWidget();
                if (fw && (fw->inherits("QLineEdit") || fw->inherits("QTextEdit") || fw->inherits("QPlainTextEdit"))) {
                    return QObject::eventFilter(watched, event);
                }
                if (ke->modifiers() & Qt::ShiftModifier) {
                    return QObject::eventFilter(watched, event);
                }
                
                bool isTrash = isCurrentNavigatorTrash(getActiveUrlNavigator());
                if (isTrash) {
                    // Let Dolphin natively handle the Delete key in Trash view
                    return QObject::eventFilter(watched, event);
                }
                
                QStringList paths = getSelectedMainViewPaths();
                if (paths.isEmpty()) {
                    QString single = getSelectedMainViewFilePath();
                    if (!single.isEmpty()) paths << single;
                }
                if (!paths.isEmpty()) {
                    performMoveToTrash(paths);
                    return true;
                }
            }
        } else if (t == QEvent::MouseButtonRelease) {
            scheduleDetailsPanePolish(40);
        }
        return QObject::eventFilter(watched, event);
    }
};

static QSet<const void*> s_knownStandardWidgets;
static QString getLiveWidgetDriveName(const void* self);

static void ensureSynced() {
    if (!isDolphinProcess()) return;
    if (!g_qtimerStarted && qApp) {
        initKernelInodes();
        g_qtimerStarted = true;

        // 1. Run once on startup and wire directly to Linux Kernel VFS /proc/self/mountinfo via QSocketNotifier
        static bool s_filmstripConfigured = false;
        if (!s_filmstripConfigured) {
            s_filmstripConfigured = true;
            QProcess::startDetached(QStringLiteral("kwriteconfig6"), {
                QStringLiteral("--file"), QStringLiteral("ffmpegthumbsrc"),
                QStringLiteral("--group"), QStringLiteral("General"),
                QStringLiteral("--key"), QStringLiteral("filmstrip"),
                QStringLiteral("false")
            });
        }
        QTimer::singleShot(0, qApp, []() {
            syncRemovableDrives();
        });
        int mountFd = open("/proc/self/mountinfo", O_RDONLY | O_CLOEXEC);
        if (mountFd >= 0) {
            auto* mountNotifier = new QSocketNotifier(mountFd, QSocketNotifier::Exception, qApp);
            QObject::connect(qApp, &QCoreApplication::aboutToQuit, [mountFd]() { close(mountFd); });
            QObject::connect(mountNotifier, &QSocketNotifier::activated, qApp, [mountFd]() {
                lseek(mountFd, 0, SEEK_SET);
                syncRemovableDrives();
                for (QWidget* w : QApplication::allWidgets()) {
                    if (w->inherits("KItemListWidget")) w->update();
                }
            });
        }

        // 2. Apply Win10 Palette & Font once
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
        QFont::insertSubstitution(QStringLiteral("Segoe UI"), QStringLiteral("Noto Sans"));
        QFont appFont(QStringLiteral("Segoe UI"));
        appFont.setPointSizeF(9.5);
        QApplication::setFont(appFont);
        for (QWidget* w : QApplication::allWidgets()) {
            w->setFont(appFont);
            if (w->inherits("QDockWidget") && !w->objectName().contains(QStringLiteral("info"), Qt::CaseInsensitive)) {
                w->setMinimumWidth(230);
            }
            if (w->inherits("QMenuBar") || w->inherits("QToolBar")) {
                w->setVisible(true);
            }
        }
        QIcon::setThemeName(QStringLiteral("Win10-Explorer"));

        // 3. Install zero-polling event filter for Details Pane & Dock sizing
        qApp->installEventFilter(new Win10KernelEventFilter(qApp));
        scheduleDetailsPanePolish(150);  // single guarded startup polish

        QTimer::singleShot(250, qApp, []() {
            ensureNavButtonsOnLeft();
        });

        // Auto-update drive storage bars in real-time
        QTimer* repaintTimer = new QTimer(qApp);
        QObject::connect(repaintTimer, &QTimer::timeout, qApp, []() {
            for (const void* ptr : s_knownStandardWidgets) {
                QString driveName = getLiveWidgetDriveName(ptr);
                if (!driveName.isEmpty()) {
                    auto* gw = reinterpret_cast<QGraphicsWidget*>(const_cast<void*>(ptr));
                    if (gw) gw->update();
                }
            }
        });
        repaintTimer->start(10000);

        QString autoSel = qEnvironmentVariable("WIN10_AUTO_SELECT_INDEX");
        if (!autoSel.isEmpty()) {
            int selIdx = autoSel.toInt();
            QTimer::singleShot(1100, qApp, [selIdx]() {
                static auto fn_setSelected = reinterpret_cast<void(*)(void*, int, int, int)>(
                    dlsym(RTLD_DEFAULT, "_ZN25KItemListSelectionManager11setSelectedEiiNS_13SelectionModeE"));
                if (!fn_setSelected) return;
                for (QWidget* w : QApplication::allWidgets()) {
                    if (!w->isVisible()) continue;
                    if (strcmp(w->metaObject()->className(), "DolphinView") != 0) continue;
                    for (QObject* child : w->findChildren<QObject*>()) {
                        if (strcmp(child->metaObject()->className(), "KItemListSelectionManager") == 0) {
                            fn_setSelected(child, selIdx, 1, 0);
                            polishDetailsPane();
                            break;
                        }
                    }
                }
            });
        }

        // Hardcoded resize removed: Dolphin should remember its own window size via KWindowConfig

        QString triggerMenu = qEnvironmentVariable("WIN10_TRIGGER_CONTEXT_MENU");
        if (!triggerMenu.isEmpty()) {
            QTimer::singleShot(1400, qApp, []() {
                QWidget* target = nullptr;
                for (QWidget* w : QApplication::allWidgets()) {
                    if (!w->isVisible()) continue;
                    if (strcmp(w->metaObject()->className(), "DolphinView") == 0) {
                        for (QWidget* child : w->findChildren<QWidget*>()) {
                            if (child->inherits("QAbstractScrollArea")) {
                                auto* sa = static_cast<QAbstractScrollArea*>(child);
                                QWidget* vp = sa->viewport();
                                if (vp && vp->isVisible()) {
                                    target = vp;
                                    break;
                                }
                            }
                        }
                        if (target) break;
                    }
                }
                if (target) {
                    int ptX = qEnvironmentVariable("WIN10_TRIGGER_PT_X", QStringLiteral("220")).toInt();
                    int ptY = qEnvironmentVariable("WIN10_TRIGGER_PT_Y", QStringLiteral("50")).toInt();
                    QPoint pt(ptX, ptY);
                    QMouseEvent mePress(QEvent::MouseButtonPress, pt, target->mapToGlobal(pt), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
                    QApplication::sendEvent(target, &mePress);
                    QMouseEvent meRelease(QEvent::MouseButtonRelease, pt, target->mapToGlobal(pt), Qt::RightButton, Qt::NoButton, Qt::NoModifier);
                    QApplication::sendEvent(target, &meRelease);
                    QContextMenuEvent ce(QContextMenuEvent::Mouse, pt, target->mapToGlobal(pt));
                    QApplication::sendEvent(target, &ce);
                }
            });
        }

        if (!qEnvironmentVariable("WIN10_TEST_CHOOSE_APP").isEmpty()) {
            QTimer::singleShot(1200, qApp, []() {
                showChooseAnotherAppDialog(QDir::homePath() + QStringLiteral("/Documents/Document.txt"), nullptr);
            });
        }

        QString shotPath = qEnvironmentVariable("WIN10_SCREENSHOT_OUT");
        if (!shotPath.isEmpty()) {
            QTimer::singleShot(2600, qApp, [shotPath]() {
                polishDetailsPane();
                for (QWidget* w : QApplication::topLevelWidgets()) {
                    if (w->isVisible() && w->width() > 400) {
                        QPixmap pix = w->grab();
                        QList<QWidget*> menusToDraw;
                        for (QWidget* tl : QApplication::topLevelWidgets()) {
                            if (tl->isVisible() && (tl->inherits("QMenu") || tl->inherits("QDialog"))) {
                                menusToDraw.append(tl);
                            }
                        }
                        std::sort(menusToDraw.begin(), menusToDraw.end(), [](QWidget* a, QWidget* b) {
                            if (a->parentWidget() == b) return false;
                            if (b->parentWidget() == a) return true;
                            return a < b;
                        });
                        QPainter p(&pix);
                        for (QWidget* tl : menusToDraw) {
                            if (tl->inherits("QMenu")) {
                                auto* menuW = static_cast<QMenu*>(tl);
                                fprintf(stderr, "[DEBUG MENU] QMenu visible: title='%s' actions=%lld size=%dx%d pos=(%d,%d)\n",
                                        qPrintable(menuW->title()), (long long)menuW->actions().size(), menuW->width(), menuW->height(), menuW->x(), menuW->y());
                            }
                            QPoint relPos = tl->mapToGlobal(QPoint(0, 0)) - w->mapToGlobal(QPoint(0, 0));
                            p.drawPixmap(relPos, tl->grab());
                        }
                        pix.save(shotPath);
                        qApp->quit();
                        break;
                    }
                }
            });
        }
    }
}



#include <QUrl>

static QSet<const void*> s_homeViewProps;

extern "C" void _ZN14ViewPropertiesC1ERK4QUrl(void* self, const QUrl& url) {
    static auto real_ctor = reinterpret_cast<void(*)(void*, const QUrl&)>(
        dlsym(RTLD_NEXT, "_ZN14ViewPropertiesC1ERK4QUrl"));
    if (real_ctor) real_ctor(self, url);
    if (self && isHomeInode(url.toLocalFile())) {
        s_homeViewProps.insert(self);
    }
}

extern "C" void _ZN14ViewPropertiesC2ERK4QUrl(void* self, const QUrl& url) {
    static auto real_ctor = reinterpret_cast<void(*)(void*, const QUrl&)>(
        dlsym(RTLD_NEXT, "_ZN14ViewPropertiesC2ERK4QUrl"));
    if (real_ctor) real_ctor(self, url);
    if (self && isHomeInode(url.toLocalFile())) {
        s_homeViewProps.insert(self);
    }
}

extern "C" void _ZN14ViewPropertiesD1Ev(void* self) {
    static auto real_dtor = reinterpret_cast<void(*)(void*)>(
        dlsym(RTLD_NEXT, "_ZN14ViewPropertiesD1Ev"));
    s_homeViewProps.remove(self);
    if (real_dtor) real_dtor(self);
}

extern "C" void _ZN14ViewPropertiesD2Ev(void* self) {
    static auto real_dtor = reinterpret_cast<void(*)(void*)>(
        dlsym(RTLD_NEXT, "_ZN14ViewPropertiesD2Ev"));
    s_homeViewProps.remove(self);
    if (real_dtor) real_dtor(self);
}

extern "C" bool _ZNK14ViewProperties16hiddenFilesShownEv(const void* self) {
    static auto real_fn = reinterpret_cast<bool(*)(const void*)>(
        dlsym(RTLD_NEXT, "_ZNK14ViewProperties16hiddenFilesShownEv"));
    if (self && s_homeViewProps.contains(self)) {
        return true;
    }
    return real_fn ? real_fn(self) : false;
}

extern "C" bool _ZNK14ViewProperties14sortHiddenLastEv(const void* self) {
    static auto real_fn = reinterpret_cast<bool(*)(const void*)>(
        dlsym(RTLD_NEXT, "_ZNK14ViewProperties14sortHiddenLastEv"));
    if (self && s_homeViewProps.contains(self)) {
        return true;
    }
    return real_fn ? real_fn(self) : false;
}

extern "C" bool _ZNK9KFileItem8isHiddenEv(const KFileItem* self) {
    static auto real_fn = reinterpret_cast<bool(*)(const KFileItem*)>(
        dlsym(RTLD_NEXT, "_ZNK9KFileItem8isHiddenEv"));
    if (self) {
        QString lpath = self->localPath();
        if (lpath.endsWith(QStringLiteral("/.directory")) || lpath.endsWith(QStringLiteral("/.win-explorer"))) {
            return true;
        }
    }
    return real_fn ? real_fn(self) : false;
}

#include <dirent.h>
extern "C" struct dirent64* readdir64(DIR* dirp) {
    static auto real_readdir64 = reinterpret_cast<struct dirent64*(*)(DIR*)>(
        dlsym(RTLD_NEXT, "readdir64"));
    if (!real_readdir64) return nullptr;
    initKernelInodes();
    int fd = dirfd(dirp);
    struct stat dirSt{};
    const bool isHomeDir = (fd >= 0 && g_homeIno != 0 && fstat(fd, &dirSt) == 0 &&
                            dirSt.st_ino == g_homeIno && dirSt.st_dev == g_homeDev);
    while (struct dirent64* entry = real_readdir64(dirp)) {
        if (isDolphinProcess()) {
            const char* name = entry->d_name;
            if (strcmp(name, ".directory") == 0 || strcmp(name, ".win-explorer") == 0) {
                continue;
            }
            if (isHomeDir) {
                if (strcmp(name, "harden_all.sh") == 0 ||
                    strcmp(name, "harden_phase1_2.sh") == 0 ||
                    strcmp(name, "rollback_phase1_2.sh") == 0 ||
                    strcmp(name, "setup_multiroute_ethernet.sh") == 0 ||
                    strcmp(name, "AGENTS.md") == 0 ||
                    strcmp(name, "CLAUDE.md") == 0 ||
                    strcmp(name, "GEMINI.md") == 0 ||
                    strcmp(name, ".bash_history") == 0 ||
                    strcmp(name, ".bash_logout") == 0 ||
                    strcmp(name, ".bash_profile") == 0 ||
                    strcmp(name, ".bashrc") == 0 ||
                    strcmp(name, ".gtkrc-2.0") == 0) {
                    continue;
                }
            }
        }
        return entry;
    }
    return nullptr;
}

extern "C" struct dirent* readdir(DIR* dirp) {
    return reinterpret_cast<struct dirent*>(readdir64(dirp));
}

extern "C" bool _ZNK19KFileItemListWidget8isHiddenEv(const void*) {
    return false;
}

extern "C" bool _ZNK23KStandardItemListWidget8isHiddenEv(const void*) {
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

extern "C" void _ZN23KStandardItemListWidgetD1Ev(void* self) {
    static auto real_dtor = reinterpret_cast<void(*)(void*)>(
        dlsym(RTLD_NEXT, "_ZN23KStandardItemListWidgetD1Ev"));
    s_knownStandardWidgets.remove(self);
    if (real_dtor) real_dtor(self);
}

extern "C" void _ZN23KStandardItemListWidgetD2Ev(void* self) {
    static auto real_dtor = reinterpret_cast<void(*)(void*)>(
        dlsym(RTLD_NEXT, "_ZN23KStandardItemListWidgetD2Ev"));
    s_knownStandardWidgets.remove(self);
    if (real_dtor) real_dtor(self);
}

extern "C" void _ZN15QGraphicsWidget6resizeERK6QSizeF(void* widgetSelf, const QSizeF& size) {
    static auto real_resize = reinterpret_cast<void(*)(void*, const QSizeF&)>(
        dlsym(RTLD_NEXT, "_ZN15QGraphicsWidget6resizeERK6QSizeF"));
    if (!real_resize) return;
    if (isStandardItemListWidget(widgetSelf)) {
        s_knownStandardWidgets.insert(widgetSelf);
        QString driveName = getLiveWidgetDriveName(widgetSelf);
        if (!driveName.isEmpty()) {
            real_resize(widgetSelf, QSizeF(276.0, 82.0));
            return;
        }
    }
    real_resize(widgetSelf, size);
}

extern "C" void _ZN13QGraphicsItem6setPosERK7QPointF(void* item, const QPointF& pos) {
    static auto real_setPos = reinterpret_cast<void(*)(void*, const QPointF&)>(
        dlsym(RTLD_NEXT, "_ZN13QGraphicsItem6setPosERK7QPointF"));
    if (!real_setPos) return;
    if (item) {
        static auto fn_toGraphicsObject = reinterpret_cast<QGraphicsObject*(*)(void*)>(
            dlsym(RTLD_DEFAULT, "_ZN13QGraphicsItem16toGraphicsObjectEv"));
        QGraphicsObject* gobj = fn_toGraphicsObject ? fn_toGraphicsObject(item) : nullptr;
        if (!gobj) {
            void* candidate = reinterpret_cast<char*>(item) - 16;
            if (s_knownStandardWidgets.contains(candidate)) {
                gobj = reinterpret_cast<QGraphicsObject*>(candidate);
            }
        }
        if (gobj) {
            const QMetaObject* mo = gobj->metaObject();
            if (mo && strcmp(mo->className(), "KStandardItemListWidget") == 0) {
                QString driveName = getLiveWidgetDriveName(gobj);
                if (!driveName.isEmpty()) {
                    s_knownStandardWidgets.insert(gobj);
                    static auto real_resize = reinterpret_cast<void(*)(void*, const QSizeF&)>(
                        dlsym(RTLD_NEXT, "_ZN15QGraphicsWidget6resizeERK6QSizeF"));
                    if (real_resize) {
                        real_resize(gobj, QSizeF(276.0, 82.0));
                    }
                    int driveIdx = getDriveIndexFromName(driveName);
                    QPointF shifted(16.0 + driveIdx * 288.0, pos.y());
                    real_setPos(item, shifted);
                    return;
                }
            }
        }
    }
    real_setPos(item, pos);
}

static qreal syncDriveWidgetGeometry(const void* self, const QString& driveName) {
    if (!self || driveName.isEmpty()) return 0.0;
    s_knownStandardWidgets.insert(self);
    static auto real_resize = reinterpret_cast<void(*)(void*, const QSizeF&)>(
        dlsym(RTLD_NEXT, "_ZN15QGraphicsWidget6resizeERK6QSizeF"));
    auto* gw = reinterpret_cast<QGraphicsWidget*>(const_cast<void*>(self));
    if (gw && gw->size().width() < 274.0 && real_resize) {
        real_resize(gw, QSizeF(276.0, 82.0));
    }
    int driveIdx = getDriveIndexFromName(driveName);
    qreal expectedX = 16.0 + driveIdx * 288.0;
    qreal curX = gw ? gw->pos().x() : expectedX;
    qreal dx = expectedX - curX;
    if (qAbs(dx) > 1.5 && gw) {
        QTimer::singleShot(0, gw, [gw, expectedX]() {
            if (!gw) return;
            static auto real_setPos = reinterpret_cast<void(*)(void*, const QPointF&)>(
                dlsym(RTLD_NEXT, "_ZN13QGraphicsItem6setPosERK7QPointF"));
            if (real_setPos) {
                real_setPos(static_cast<QGraphicsItem*>(gw), QPointF(expectedX, gw->pos().y()));
                gw->update();
            }
        });
    }
    return dx;
}

extern "C" QRectF _ZNK23KStandardItemListWidget17selectionRectCoreEv(const void* self) {
    static auto real_fn = reinterpret_cast<QRectF(*)(const void*)>(
        dlsym(RTLD_NEXT, "_ZNK23KStandardItemListWidget17selectionRectCoreEv"));
    s_knownStandardWidgets.insert(self);
    QString driveName = getLiveWidgetDriveName(self);
    if (!driveName.isEmpty()) {
        return QRectF(0, 6, 274, 70);
    }
    return real_fn ? real_fn(self) : QRectF(0, 0, 100, 80);
}

extern "C" QRectF _ZNK23KStandardItemListWidget17selectionRectFullEv(const void* self) {
    static auto real_fn = reinterpret_cast<QRectF(*)(const void*)>(
        dlsym(RTLD_NEXT, "_ZNK23KStandardItemListWidget17selectionRectFullEv"));
    s_knownStandardWidgets.insert(self);
    QString driveName = getLiveWidgetDriveName(self);
    if (!driveName.isEmpty()) {
        return QRectF(0, 6, 274, 70);
    }
    return real_fn ? real_fn(self) : QRectF(0, 0, 100, 80);
}

extern "C" bool _ZNK15KItemListWidget8containsERK7QPointF(const void* self, const QPointF& point) {
    static auto real_fn = reinterpret_cast<bool(*)(const void*, const QPointF&)>(
        dlsym(RTLD_NEXT, "_ZNK15KItemListWidget8containsERK7QPointF"));
    QString driveName = getLiveWidgetDriveName(self);
    if (!driveName.isEmpty()) {
        return QRectF(0, 6, 274, 70).contains(point);
    }
    return real_fn ? real_fn(self, point) : false;
}

extern "C" bool _ZThn16_NK15KItemListWidget8containsERK7QPointF(const void* thunkSelf, const QPointF& point) {
    const void* realSelf = reinterpret_cast<const char*>(thunkSelf) - 16;
    return _ZNK15KItemListWidget8containsERK7QPointF(realSelf, point);
}



static QString getLiveWidgetDriveName(const void* self) {
    if (!self) return QString();
    int iconSz = call_iconSize(self);
    if (iconSz < 48) return QString(); // Sidebar uses <=32px icons, only main view uses >=48px
    QString txt = call_widgetValue(self, QByteArrayLiteral("text")).toString();
    if (txt.startsWith(QStringLiteral("Local Disk")) ||
        txt.startsWith(QStringLiteral("USB Drive (")) ||
        txt.startsWith(QStringLiteral("Removable Disk ("))) {
        return txt;
    }
    return QString();
}

// Paint the exact 1:1 Original Windows 10 Horizontal Drive Tile:
// [52x52 3D Silver Drive on Left] + [Title (Top) / Blue-or-Red Bar (Middle) / "XX.X GB free of YY.Y GB" (Bottom) on Right]
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
    // Widget itself is natively positioned & sized to 276x82
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    // Tile bounding box (274px wide x 70px tall with smooth 3px rounded corners)
    QRectF tileRect(0, 6, 274, 70);
    if (selected) {
        painter->setPen(QPen(QColor(153, 209, 255), 1));
        painter->setBrush(QColor(204, 232, 255));
        painter->drawRoundedRect(tileRect, 3.0, 3.0);
    } else if (hovered) {
        painter->setPen(QPen(QColor(204, 232, 255), 1));
        painter->setBrush(QColor(229, 243, 255));
        painter->drawRoundedRect(tileRect, 3.0, 3.0);
    }

    // 1. Left: Genuine 3D Silver Windows 10 Drive Icon (52x52 at x=6, y=15)
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
        painter->drawPixmap(QRect(6, 15, 52, 52), drivePix);
    }

    // 2. Right Row 1 (x=66, y=9): Drive Name ("Local Disk (C:)", "Local Disk (D:)", "USB Drive (E:)")
    QFont titleFont = painter->font();
    titleFont.setPointSizeF(10.5);
    titleFont.setWeight(QFont::Normal);
    painter->setFont(titleFont);
    painter->setPen(QColor(25, 25, 25));
    painter->drawText(QRectF(66, 9, 202, 22), Qt::AlignLeft | Qt::AlignVCenter, rawName);

    // 3. Right Row 2 (x=66, y=33, width=196, height=16): Original Windows 10 Capacity Bar
    QRectF barOuter(66, 33, 196, 16);
    painter->setPen(QPen(QColor(210, 210, 210), 1));
    painter->setBrush(QColor(230, 230, 230));
    painter->drawRoundedRect(barOuter, 1.5, 1.5);

    int maxFillW = 194;
    int fillW = (sp.usedPct * maxFillW) / 100;
    if (fillW < 6 && sp.usedPct > 0) fillW = 6;
    if (fillW > maxFillW) fillW = maxFillW;
    if (fillW > 0) {
        QColor barColor = (sp.usedPct >= 90) ? QColor(218, 38, 38) : QColor(38, 160, 218);
        painter->setPen(Qt::NoPen);
        painter->setBrush(barColor);
        painter->drawRoundedRect(QRectF(67, 34, fillW, 14), 1.0, 1.0);
    }

    // 4. Right Row 3 (x=66, y=52): "89.5 GB free of 98.0 GB" in crisp readable dark grey (#464646)
    QFont subFont = painter->font();
    subFont.setPointSizeF(9.5);
    painter->setFont(subFont);
    painter->setPen(QColor(70, 70, 70));
    painter->drawText(QRectF(66, 52, 202, 20), Qt::AlignLeft | Qt::AlignVCenter, sp.summaryText);

    painter->restore();
}

extern "C" void _ZN23KStandardItemListWidget5paintEP8QPainterPK24QStyleOptionGraphicsItemP7QWidget(
    void* self, QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    static auto real_paint = reinterpret_cast<void(*)(void*, QPainter*, const QStyleOptionGraphicsItem*, QWidget*)>(
        dlsym(RTLD_NEXT, "_ZN23KStandardItemListWidget5paintEP8QPainterPK24QStyleOptionGraphicsItemP7QWidget"));

    QString driveName = getLiveWidgetDriveName(self);
    if (!driveName.isEmpty()) {
        paintWin10DriveTile(self, driveName, painter);
        return;
    }
    
    // Force standard file items to use Segoe UI 9pt, overriding Dolphin's cached view settings
    auto* gw = reinterpret_cast<QGraphicsWidget*>(self);
    if (gw) {
        QFont f = gw->font();
        if (f.family() != QStringLiteral("Segoe UI") || qAbs(f.pointSizeF() - 9.5) > 0.1) {
            f.setFamily(QStringLiteral("Segoe UI"));
            f.setPointSizeF(9.5);
            gw->setFont(f);
        }
    }
    
    if (real_paint) real_paint(self, painter, option, widget);
}

extern "C" void _ZThn16_N23KStandardItemListWidget5paintEP8QPainterPK24QStyleOptionGraphicsItemP7QWidget(
    void* thunkSelf, QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    void* realSelf = reinterpret_cast<char*>(thunkSelf) - 16;
    QString driveName = getLiveWidgetDriveName(realSelf);
    if (!driveName.isEmpty()) {
        paintWin10DriveTile(realSelf, driveName, painter);
        return;
    }
    
    auto* gw = reinterpret_cast<QGraphicsWidget*>(realSelf);
    if (gw) {
        QFont f = gw->font();
        if (f.family() != QStringLiteral("Segoe UI") || qAbs(f.pointSizeF() - 9.5) > 0.1) {
            f.setFamily(QStringLiteral("Segoe UI"));
            f.setPointSizeF(9.5);
            gw->setFont(f);
        }
    }
    
    static auto real_thunk = reinterpret_cast<void(*)(void*, QPainter*, const QStyleOptionGraphicsItem*, QWidget*)>(
        dlsym(RTLD_NEXT, "_ZThn16_N23KStandardItemListWidget5paintEP8QPainterPK24QStyleOptionGraphicsItemP7QWidget"));
    if (real_thunk) real_thunk(thunkSelf, painter, option, widget);
}

extern "C" QString _ZNK23KStandardItemListWidget9elideTextE7QStringd(
    const void* self,
    QString text,
    double width)
{
    if (width <= 0) return QString();

    QFont font;
    if (self) {
        font = reinterpret_cast<const QGraphicsWidget*>(self)->font();
    }
    if (font.family().isEmpty()) {
        font = QApplication::font();
    }
    QFontMetricsF fm(font);

    if (fm.horizontalAdvance(text) <= width) {
        return text;
    }

    int lastDot = text.lastIndexOf(QLatin1Char('.'));
    bool hasExt = false;
    QString baseName = text;
    QString ext;
    if (lastDot > 0 && lastDot < text.length() - 1) {
        QString possibleExt = text.mid(lastDot);
        if (!possibleExt.contains(QLatin1Char(' ')) && 
            !possibleExt.contains(QLatin1Char('/')) && 
            possibleExt.length() <= 12) {
            hasExt = true;
            baseName = text.left(lastDot);
            ext = possibleExt;
        }
    }

    if (hasExt) {
        qreal extW = fm.horizontalAdvance(ext);
        qreal availBase = width - extW;
        if (availBase > fm.horizontalAdvance(QStringLiteral("…"))) {
            QString elidedBase = fm.elidedText(baseName, Qt::ElideRight, availBase);
            while (!elidedBase.isEmpty() && fm.horizontalAdvance(elidedBase + ext) > width) {
                if (elidedBase.endsWith(QStringLiteral("…"))) {
                    elidedBase.chop(1);
                    if (!elidedBase.isEmpty()) elidedBase.chop(1);
                    elidedBase += QStringLiteral("…");
                } else {
                    elidedBase.chop(1);
                    elidedBase += QStringLiteral("…");
                }
            }
            if (!elidedBase.isEmpty()) {
                return elidedBase + ext;
            }
        }
        if (width >= fm.horizontalAdvance(QStringLiteral("…") + ext)) {
            return QStringLiteral("…") + ext;
        }
    }

    return fm.elidedText(text, Qt::ElideRight, width);
}

extern "C" QString _ZNK12QFontMetrics10elidedTextERK7QStringN2Qt13TextElideModeEii(
    const QFontMetrics* self,
    const QString& text,
    Qt::TextElideMode mode,
    int width,
    int flags)
{
    static auto real_fn = reinterpret_cast<QString(*)(const QFontMetrics*, const QString&, Qt::TextElideMode, int, int)>(
        dlsym(RTLD_NEXT, "_ZNK12QFontMetrics10elidedTextERK7QStringN2Qt13TextElideModeEii"));

    if (width <= 0) return QString();
    if (!real_fn) return text;

    if (self->horizontalAdvance(text) <= width) {
        return text;
    }

    int lastDot = text.lastIndexOf(QLatin1Char('.'));
    if (lastDot > 0 && lastDot < text.length() - 1) {
        QString ext = text.mid(lastDot);
        if (!ext.contains(QLatin1Char(' ')) && !ext.contains(QLatin1Char('/')) && ext.length() <= 12) {
            QString baseName = text.left(lastDot);
            int extW = self->horizontalAdvance(ext);
            int availBase = width - extW;
            if (availBase > self->horizontalAdvance(QStringLiteral("…"))) {
                QString elidedBase = real_fn(self, baseName, Qt::ElideRight, availBase, flags);
                while (!elidedBase.isEmpty() && self->horizontalAdvance(elidedBase + ext) > width) {
                    if (elidedBase.endsWith(QStringLiteral("…"))) {
                        elidedBase.chop(1);
                        if (!elidedBase.isEmpty()) elidedBase.chop(1);
                        elidedBase += QStringLiteral("…");
                    } else {
                        elidedBase.chop(1);
                        elidedBase += QStringLiteral("…");
                    }
                }
                if (!elidedBase.isEmpty()) {
                    return elidedBase + ext;
                }
            }
        }
    }

    return real_fn(self, text, mode, width, flags);
}

extern "C" QString _ZNK13QFontMetricsF10elidedTextERK7QStringN2Qt13TextElideModeEdi(
    const QFontMetricsF* self,
    const QString& text,
    Qt::TextElideMode mode,
    double width,
    int flags)
{
    static auto real_fn = reinterpret_cast<QString(*)(const QFontMetricsF*, const QString&, Qt::TextElideMode, double, int)>(
        dlsym(RTLD_NEXT, "_ZNK13QFontMetricsF10elidedTextERK7QStringN2Qt13TextElideModeEdi"));

    if (width <= 0) return QString();
    if (!real_fn) return text;

    if (self->horizontalAdvance(text) <= width) {
        return text;
    }

    int lastDot = text.lastIndexOf(QLatin1Char('.'));
    if (lastDot > 0 && lastDot < text.length() - 1) {
        QString ext = text.mid(lastDot);
        if (!ext.contains(QLatin1Char(' ')) && !ext.contains(QLatin1Char('/')) && ext.length() <= 12) {
            QString baseName = text.left(lastDot);
            qreal extW = self->horizontalAdvance(ext);
            qreal availBase = width - extW;
            if (availBase > self->horizontalAdvance(QStringLiteral("…"))) {
                QString elidedBase = real_fn(self, baseName, Qt::ElideRight, availBase, flags);
                while (!elidedBase.isEmpty() && self->horizontalAdvance(elidedBase + ext) > width) {
                    if (elidedBase.endsWith(QStringLiteral("…"))) {
                        elidedBase.chop(1);
                        if (!elidedBase.isEmpty()) elidedBase.chop(1);
                        elidedBase += QStringLiteral("…");
                    } else {
                        elidedBase.chop(1);
                        elidedBase += QStringLiteral("…");
                    }
                }
                if (!elidedBase.isEmpty()) {
                    return elidedBase + ext;
                }
            }
        }
    }

    return real_fn(self, text, mode, width, flags);
}

struct FolderIconCacheEntry {
    qint64 mtimeSec;
    qint64 mtimeNsec;
    QString iconName;
};

static QString buildDynamicFolderIcon(const QString& dirPath, const QString& displayName) {
    if (isSpecialSystemFolder(displayName)) {
        if (displayName == QStringLiteral("Desktop")) return QStringLiteral("user-desktop");
        return QStringLiteral("folder-") + displayName.toLower();
    }
    if (dirPath.isEmpty()) return QStringLiteral("folder");

    struct stat dirSt{};
    if (stat(dirPath.toUtf8().constData(), &dirSt) != 0) {
        return QStringLiteral("folder");
    }

    static QHash<QString, FolderIconCacheEntry> s_mtimeIconCache;
    auto itCache = s_mtimeIconCache.find(dirPath);
    if (itCache != s_mtimeIconCache.end() &&
        itCache->mtimeSec == static_cast<qint64>(dirSt.st_mtim.tv_sec) &&
        itCache->mtimeNsec == static_cast<qint64>(dirSt.st_mtim.tv_nsec)) {
        return itCache->iconName;
    }

    QString fullDocsIcon = QDir::homePath() + QStringLiteral("/.config/win-explorer/folder_with_contents_preview.png");
    QFileInfo dirFi(dirPath);
    QString realDir = dirFi.canonicalFilePath();
    if (realDir.isEmpty()) realDir = dirFi.isSymLink() ? dirFi.symLinkTarget() : dirPath;

    if (!realDir.startsWith(QDir::homePath()) && !realDir.startsWith(QStringLiteral("/run/media/"))) {
        s_mtimeIconCache.insert(dirPath, {static_cast<qint64>(dirSt.st_mtim.tv_sec), static_cast<qint64>(dirSt.st_mtim.tv_nsec), fullDocsIcon});
        return fullDocsIcon;
    }

    QString hash = QString::fromLatin1(QCryptographicHash::hash(realDir.toUtf8(), QCryptographicHash::Md5).toHex().left(16));
    QString pngPath = QDir::homePath() + QStringLiteral("/.config/win-explorer/peek_cache/w10dir_") + hash + QStringLiteral(".png");

    // 1. Fast Kernel stat() check on cached PNG before touching directory entries
    struct stat pngSt{};
    if (stat(pngPath.toUtf8().constData(), &pngSt) == 0 && pngSt.st_mtim.tv_sec >= dirSt.st_mtim.tv_sec) {
        s_mtimeIconCache.insert(dirPath, {static_cast<qint64>(dirSt.st_mtim.tv_sec), static_cast<qint64>(dirSt.st_mtim.tv_nsec), pngPath});
        return pngPath;
    }

    // 2. Kernel inode link-count check (st_nlink > 2 guarantees subdirectories exist without opendir)
    bool hasAnyEntry = (dirSt.st_nlink > 2);
    if (!hasAnyEntry) {
        QDirIterator it(realDir, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            it.next();
            QString fn = it.fileName();
            if (fn != QStringLiteral(".directory") && fn != QStringLiteral("desktop.ini")) {
                hasAnyEntry = true;
                break;
            }
        }
    }

    if (!hasAnyEntry) {
        if (QFile::exists(pngPath)) QFile::remove(pngPath);
        s_mtimeIconCache.insert(dirPath, {static_cast<qint64>(dirSt.st_mtim.tv_sec), static_cast<qint64>(dirSt.st_mtim.tv_nsec), QStringLiteral("folder")});
        return QStringLiteral("folder");
    }

    static QHash<QString, qint64> s_pendingJobs;
    qint64 dirSec = static_cast<qint64>(dirSt.st_mtim.tv_sec);
    if (s_pendingJobs.value(realDir, -1) != dirSec) {
        s_pendingJobs.insert(realDir, dirSec);
        QProcess::startDetached(QStringLiteral("python3"), {
            QDir::homePath() + QStringLiteral("/.config/win-explorer/build_folder_peeks.py"),
            realDir
        });
    }

    if (QFile::exists(pngPath)) {
        return pngPath;
    }
    return fullDocsIcon;
}

static QString getWin10IconsDir() {
    static const QString scratchDir = QDir::homePath() + QStringLiteral("/.win-explorer/Win10-Explorer-icons/");
    if (QFile::exists(scratchDir + QStringLiteral("notepad.png"))) {
        return scratchDir;
    }
    QString configDir = QDir::homePath() + QStringLiteral("/.config/win-explorer/icons/");
    if (QFile::exists(configDir + QStringLiteral("notepad.png"))) {
        return configDir;
    }
    return scratchDir;
}

extern "C" QString _ZNK9KFileItem8iconNameEv(const KFileItem* self) {
    ensureSynced();
    static auto real_fn = reinterpret_cast<QString(*)(const KFileItem*)>(
        dlsym(RTLD_NEXT, "_ZNK9KFileItem8iconNameEv"));
    QString rawName = getRawItemText(self);
    if (self->isDir()) {
        if (rawName == QStringLiteral("This PC")) return QStringLiteral("computer");
        if (rawName == QStringLiteral("Local Disk (C:)")) return QStringLiteral("drive-harddisk-system");
        if (rawName == QStringLiteral("Local Disk (D:)")) return QStringLiteral("drive-harddisk");
        if (rawName.startsWith(QStringLiteral("USB Drive (")) || rawName.startsWith(QStringLiteral("Removable Disk ("))) {
            return QStringLiteral("drive-removable-media-usb");
        }
        if (isSpecialSystemFolder(rawName)) {
            if (rawName == QStringLiteral("Desktop")) return QStringLiteral("user-desktop");
            return QStringLiteral("folder-") + rawName.toLower();
        }
        return buildDynamicFolderIcon(self->localPath(), rawName);
    }

    // Windows 10 file icon mapping for md, txt, js, and common file types
    QString ext = QFileInfo(self->localPath()).suffix().toLower();
    if (ext.isEmpty()) {
        static auto real_text_fn = reinterpret_cast<QString(*)(const KFileItem*)>(
            dlsym(RTLD_NEXT, "_ZNK9KFileItem4textEv"));
        QString realName = real_text_fn ? real_text_fn(self) : QString();
        int dot = realName.lastIndexOf(QLatin1Char('.'));
        if (dot > 0) {
            ext = realName.mid(dot + 1).toLower();
        }
    }
    QString iconDir = getWin10IconsDir();
    if (ext == QStringLiteral("txt") || ext == QStringLiteral("log") || ext == QStringLiteral("conf")) {
        return iconDir + QStringLiteral("notepad.png");
    }
    if (ext == QStringLiteral("ini") || ext == QStringLiteral("inf") || ext == QStringLiteral("cfg")) {
        return iconDir + QStringLiteral("ini.png");
    }
    if (ext == QStringLiteral("docx") || ext == QStringLiteral("doc") || ext == QStringLiteral("docm") || ext == QStringLiteral("dotx") || ext == QStringLiteral("rtf") || ext == QStringLiteral("odt")) {
        return iconDir + QStringLiteral("docx.png");
    }
    if (ext == QStringLiteral("xlsx") || ext == QStringLiteral("xls") || ext == QStringLiteral("xlsm") || ext == QStringLiteral("xltx") || ext == QStringLiteral("csv") || ext == QStringLiteral("ods")) {
        return iconDir + QStringLiteral("xlsx.png");
    }
    if (ext == QStringLiteral("pptx") || ext == QStringLiteral("ppt") || ext == QStringLiteral("pptm") || ext == QStringLiteral("potx") || ext == QStringLiteral("odp")) {
        return iconDir + QStringLiteral("pptx.png");
    }
    if (ext == QStringLiteral("pdf")) {
        return iconDir + QStringLiteral("pdf.png");
    }
    if (ext == QStringLiteral("md") || ext == QStringLiteral("markdown")) {
        return iconDir + QStringLiteral("markdown.png");
    }
    if (ext == QStringLiteral("js") || ext == QStringLiteral("mjs") || ext == QStringLiteral("cjs") || ext == QStringLiteral("jsx")) {
        return iconDir + QStringLiteral("javascript.png");
    }
    if (ext == QStringLiteral("ts") || ext == QStringLiteral("tsx")) {
        return iconDir + QStringLiteral("typescript.png");
    }
    if (ext == QStringLiteral("json")) {
        return iconDir + QStringLiteral("json.png");
    }
    if (ext == QStringLiteral("py") || ext == QStringLiteral("pyw")) {
        return iconDir + QStringLiteral("python.png");
    }
    if (ext == QStringLiteral("html") || ext == QStringLiteral("htm") || ext == QStringLiteral("xhtml") || ext == QStringLiteral("mhtml")) {
        return iconDir + QStringLiteral("html.png");
    }
    if (ext == QStringLiteral("css") || ext == QStringLiteral("scss") || ext == QStringLiteral("less")) {
        return iconDir + QStringLiteral("css.png");
    }
    if (ext == QStringLiteral("c") || ext == QStringLiteral("cpp") || ext == QStringLiteral("cc") || ext == QStringLiteral("h") || ext == QStringLiteral("hpp")) {
        return iconDir + QStringLiteral("cpp.png");
    }
    if (ext == QStringLiteral("zip")) {
        return iconDir + QStringLiteral("zip.png");
    }
    if (ext == QStringLiteral("rar")) {
        return iconDir + QStringLiteral("rar.png");
    }
    if (ext == QStringLiteral("7z")) {
        return iconDir + QStringLiteral("7z.png");
    }
    if (ext == QStringLiteral("tar") || ext == QStringLiteral("gz") || ext == QStringLiteral("bz2") || ext == QStringLiteral("xz") || ext == QStringLiteral("tgz") || ext == QStringLiteral("tbz2")) {
        return iconDir + QStringLiteral("zip.png");
    }
    if (ext == QStringLiteral("bat") || ext == QStringLiteral("cmd")) {
        return iconDir + QStringLiteral("bat.png");
    }
    if (ext == QStringLiteral("ps1") || ext == QStringLiteral("psm1")) {
        return iconDir + QStringLiteral("ps1.png");
    }
    if (ext == QStringLiteral("reg")) {
        return iconDir + QStringLiteral("reg.png");
    }
    if (ext == QStringLiteral("dll") || ext == QStringLiteral("sys") || ext == QStringLiteral("ocx")) {
        return iconDir + QStringLiteral("dll.png");
    }
    if (ext == QStringLiteral("exe") || ext == QStringLiteral("msi")) {
        return iconDir + QStringLiteral("exe.png");
    }

    return real_fn ? real_fn(self) : QStringLiteral("text-x-generic");
}

extern "C" QString _ZNK9KFileItem11mimeCommentEv(const KFileItem* self) {
    ensureSynced();
    static auto real_fn = reinterpret_cast<QString(*)(const KFileItem*)>(
        dlsym(RTLD_NEXT, "_ZNK9KFileItem11mimeCommentEv"));
    QString rawName = getRawItemText(self);
    if (rawName.startsWith(QStringLiteral("Local Disk")) ||
        rawName.startsWith(QStringLiteral("USB Drive (")) ||
        rawName.startsWith(QStringLiteral("Removable Disk ("))) {
        if (self->localPath().contains(QStringLiteral("/.win-explorer/ThisPC/"))) {
            return QStringLiteral("\u3164Devices and drives");
        }
        return QStringLiteral("Local Disk (NTFS)");
    }
    if (self->localPath().contains(QStringLiteral("/.win-explorer/ThisPC/")) || isSpecialSystemFolder(rawName)) {
        return QStringLiteral("Folders");
    }
    if (self->isDir()) {
        return QStringLiteral("File folder");
    }

    QString ext = QFileInfo(self->localPath()).suffix().toLower();
    if (ext == QStringLiteral("txt") || ext == QStringLiteral("log")) return QStringLiteral("Text Document");
    if (ext == QStringLiteral("cfg") || ext == QStringLiteral("ini") || ext == QStringLiteral("conf")) return QStringLiteral("Configuration settings");
    if (ext == QStringLiteral("html") || ext == QStringLiteral("htm")) return QStringLiteral("Microsoft Edge HTML Document");
    if (ext == QStringLiteral("md") || ext == QStringLiteral("markdown")) return QStringLiteral("Markdown Document");
    if (ext == QStringLiteral("js") || ext == QStringLiteral("mjs") || ext == QStringLiteral("cjs") || ext == QStringLiteral("jsx")) return QStringLiteral("JavaScript File");
    if (ext == QStringLiteral("ts") || ext == QStringLiteral("tsx")) return QStringLiteral("TypeScript File");
    if (ext == QStringLiteral("json")) return QStringLiteral("JSON File");
    if (ext == QStringLiteral("py") || ext == QStringLiteral("pyw")) return QStringLiteral("Python File");
    if (ext == QStringLiteral("css") || ext == QStringLiteral("scss") || ext == QStringLiteral("less")) return QStringLiteral("Cascading Style Sheet Document");
    if (ext == QStringLiteral("c") || ext == QStringLiteral("cpp") || ext == QStringLiteral("cc") || ext == QStringLiteral("h") || ext == QStringLiteral("hpp")) return QStringLiteral("C/C++ Source File");
    if (ext == QStringLiteral("pdf")) return QStringLiteral("PDF Document");
    if (ext == QStringLiteral("zip")) return QStringLiteral("Compressed (zipped) Folder");
    if (ext == QStringLiteral("rar")) return QStringLiteral("WinRAR archive");
    if (ext == QStringLiteral("7z")) return QStringLiteral("7-Zip archive");
    if (ext == QStringLiteral("tar") || ext == QStringLiteral("gz") || ext == QStringLiteral("bz2") || ext == QStringLiteral("xz") || ext == QStringLiteral("tgz") || ext == QStringLiteral("tbz2")) return QStringLiteral("Archive file");
    if (ext == QStringLiteral("iso")) return QStringLiteral("Disc Image File");
    if (ext == QStringLiteral("bat") || ext == QStringLiteral("cmd")) return QStringLiteral("Windows Batch File");
    if (ext == QStringLiteral("csv")) return QStringLiteral("Microsoft Excel Comma Separated Values File");
    if (ext == QStringLiteral("exe")) return QStringLiteral("Application");

    return real_fn ? real_fn(self) : QStringLiteral("File");
}

// -----------------------------------------------------------------------------
// Force All Deletions to FreeDesktop / Windows 10 Trash (Recycle Bin)
// Intercepts KIO deletion constructors so nothing is permanently removed
// unless Shift key is explicitly held down (Shift+Delete).
// -----------------------------------------------------------------------------
extern "C" void _ZN3KIO16DeleteOrTrashJobC1ERK5QListI4QUrlENS_22AskUserActionInterface12DeletionTypeENS6_16ConfirmationTypeEP7QObject(
    void* self, const QList<QUrl>& urls, int deletionType, int confirmType, QObject* parent) {
    static auto real_fn = reinterpret_cast<void(*)(void*, const QList<QUrl>&, int, int, QObject*)>(
        dlsym(RTLD_NEXT, "_ZN3KIO16DeleteOrTrashJobC1ERK5QListI4QUrlENS_22AskUserActionInterface12DeletionTypeENS6_16ConfirmationTypeEP7QObject"));
    
    bool shiftHeld = false;
    if (qApp) {
        shiftHeld = (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) != 0;
    }
    if (!shiftHeld) {
        for (const QUrl& u : urls) {
            if (u.isLocalFile() && !isDirectThisPcItem(u.toLocalFile())) {
                safeMoveToRecycleBin(u.toLocalFile());
            }
        }
        QList<QUrl> emptyUrls;
        if (real_fn) real_fn(self, emptyUrls, deletionType, confirmType, parent);
        return;
    }
    if (real_fn) real_fn(self, urls, deletionType, confirmType, parent);
}

extern "C" void _ZN3KIO16DeleteOrTrashJobC2ERK5QListI4QUrlENS_22AskUserActionInterface12DeletionTypeENS6_16ConfirmationTypeEP7QObject(
    void* self, const QList<QUrl>& urls, int deletionType, int confirmType, QObject* parent) {
    static auto real_fn = reinterpret_cast<void(*)(void*, const QList<QUrl>&, int, int, QObject*)>(
        dlsym(RTLD_NEXT, "_ZN3KIO16DeleteOrTrashJobC2ERK5QListI4QUrlENS_22AskUserActionInterface12DeletionTypeENS6_16ConfirmationTypeEP7QObject"));
    
    bool shiftHeld = false;
    if (qApp) {
        shiftHeld = (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) != 0;
    }
    if (!shiftHeld) {
        for (const QUrl& u : urls) {
            if (u.isLocalFile() && !isDirectThisPcItem(u.toLocalFile())) {
                safeMoveToRecycleBin(u.toLocalFile());
            }
        }
        QList<QUrl> emptyUrls;
        if (real_fn) real_fn(self, emptyUrls, deletionType, confirmType, parent);
        return;
    }
    if (real_fn) real_fn(self, urls, deletionType, confirmType, parent);
}


extern "C" void* _ZN3KIO11file_deleteERK4QUrl6QFlagsINS_7JobFlagEE(const QUrl& url, int flags) {
    if (isDirectThisPcItem(url.toLocalFile())) {
        return nullptr;
    }
    static auto real_fn = reinterpret_cast<void*(*)(const QUrl&, int)>(
        dlsym(RTLD_NEXT, "_ZN3KIO11file_deleteERK4QUrl6QFlagsINS_7JobFlagEE"));
    return real_fn ? real_fn(url, flags) : nullptr;
}

extern "C" void* _ZN3KIO3delERK4QUrl6QFlagsINS_7JobFlagEE(const QUrl& url, int flags) {
    if (isDirectThisPcItem(url.toLocalFile())) {
        return nullptr;
    }
    static auto real_fn = reinterpret_cast<void*(*)(const QUrl&, int)>(
        dlsym(RTLD_NEXT, "_ZN3KIO3delERK4QUrl6QFlagsINS_7JobFlagEE"));
    return real_fn ? real_fn(url, flags) : nullptr;
}

extern "C" void* _ZN3KIO3delERK5QListI4QUrlE6QFlagsINS_7JobFlagEE(const QList<QUrl>& urls, int flags) {
    QList<QUrl> filtered;
    for (const QUrl& u : urls) {
        if (!isDirectThisPcItem(u.toLocalFile())) {
            filtered.append(u);
        }
    }
    if (filtered.isEmpty()) return nullptr;
    static auto real_fn = reinterpret_cast<void*(*)(const QList<QUrl>&, int)>(
        dlsym(RTLD_NEXT, "_ZN3KIO3delERK5QListI4QUrlE6QFlagsINS_7JobFlagEE"));
    return real_fn ? real_fn(filtered, flags) : nullptr;
}




__attribute__((constructor))
static void initThisPcSync() {
    unsetenv("LD_PRELOAD");
    if (!isDolphinProcess()) return;
    initKernelInodes();
    g_syncTimer.start();
    syncRemovableDrives();
}
