#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dlfcn.h>

// Forward declarations & pure logic implementations under test
// matching win10_thispc_helper.cpp exactly

static QString toWinPath(const QString& linuxPath) {
    if (linuxPath.isEmpty()) return QString();
    QString p = QDir::cleanPath(linuxPath);
    if (p == QStringLiteral("/home/user")) {
        return QStringLiteral("D:\\");
    }
    if (p.startsWith(QStringLiteral("/home/user/"))) {
        p = QStringLiteral("D:") + p.mid(10);
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
        p = QStringLiteral("/home/user") + p.mid(2);
    } else if (p.startsWith(QStringLiteral("C:"), Qt::CaseInsensitive)) {
        p = p.mid(2);
    }
    p = QDir::cleanPath(p);
    if (p.isEmpty() || !p.startsWith(QLatin1Char('/'))) {
        p = QLatin1Char('/') + p;
    }
    return p;
}

static QString formatGigabytes(quint64 bytes) {
    double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    if (gb < 10.0) {
        return QString::asprintf("%.1f GB", gb);
    }
    return QString::asprintf("%.0f GB", gb);
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

static bool isSpecialSystemFolder(const QString& displayName) {
    return displayName == QStringLiteral("Desktop") ||
           displayName == QStringLiteral("Documents") ||
           displayName == QStringLiteral("Downloads") ||
           displayName == QStringLiteral("Pictures") ||
           displayName == QStringLiteral("Music") ||
           displayName == QStringLiteral("Videos");
}

static bool isThisPcPath(const QString& path) {
    return path.contains(QStringLiteral("/.win-explorer/ThisPC"));
}

static bool isDirectThisPcItem(const QString& path) {
    QString p = QDir::cleanPath(path);
    QString base = QDir::cleanPath(QDir::homePath() + QStringLiteral("/.win-explorer/ThisPC"));
    if (!p.startsWith(base)) return false;
    QString sub = p.mid(base.length());
    if (sub.startsWith(QLatin1Char('/'))) sub = sub.mid(1);
    if (sub.isEmpty()) return false;
    return !sub.contains(QLatin1Char('/'));
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

static bool isPreviewableTextFile(const QString& filePath, QString* outContent = nullptr) {
    QFileInfo fi(filePath);
    if (!fi.exists() || fi.isDir()) return false;
    if (fi.size() > 5 * 1024 * 1024) return false;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray raw = f.read(64 * 1024);
    f.close();

    if (raw.isEmpty()) {
        if (outContent) *outContent = QString();
        return true;
    }

    int checkLen = qMin<int>(raw.size(), 4096);
    for (int i = 0; i < checkLen; ++i) {
        if (raw.at(i) == '\0') {
            return false;
        }
    }
    if (outContent) *outContent = QString::fromUtf8(raw);
    return true;
}

static QString getMimeCommentForPath(const QString& localPath, const QString& rawName, bool isDir) {
    if (rawName.startsWith(QStringLiteral("Local Disk")) ||
        rawName.startsWith(QStringLiteral("USB Drive (")) ||
        rawName.startsWith(QStringLiteral("Removable Disk ("))) {
        if (localPath.contains(QStringLiteral("/.win-explorer/ThisPC/"))) {
            return QStringLiteral("\u3164Devices and drives");
        }
        return QStringLiteral("Local Disk (NTFS)");
    }
    if (localPath.contains(QStringLiteral("/.win-explorer/ThisPC/")) || isSpecialSystemFolder(rawName)) {
        return QStringLiteral("Folders");
    }
    if (isDir) {
        return QStringLiteral("File folder");
    }

    QString ext = QFileInfo(localPath).suffix().toLower();
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

    return QStringLiteral("File");
}

static QString getIconNameForPath(const QString& localPath, const QString& rawName, bool isDir) {
    static const QString iconDir = QDir::homePath() + QStringLiteral("/.win-explorer/Win10-Explorer-icons/");
    if (isDir) {
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
        return QStringLiteral("folder");
    }

    QString ext = QFileInfo(localPath).suffix().toLower();
    if (ext == QStringLiteral("txt") || ext == QStringLiteral("log") || ext == QStringLiteral("conf")) {
        return iconDir + QStringLiteral("notepad.png");
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
    return QStringLiteral("text-x-generic");
}

static bool testSafeMoveToRecycleBin(const QString& local, QString& outInfoPath) {
    if (local.isEmpty()) return false;
    QFileInfo fi(local);
    if (!fi.exists() && !fi.isSymLink()) return false;

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
        outInfoPath = targetInfo;
        return true;
    }
    return false;
}

// ============================================================================
// TEST HARNESS
// ============================================================================
struct TestCase {
    int id;
    std::string category;
    std::string name;
    std::function<bool()> testFn;
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::vector<TestCase> suite;

    auto add = [&](const std::string& cat, const std::string& name, std::function<bool()> fn) {
        suite.push_back({static_cast<int>(suite.size() + 1), cat, name, fn});
    };

    // ------------------------------------------------------------------------
    // Category 1: Path Conversions (toWinPath & toLinuxPath)
    // ------------------------------------------------------------------------
    add("Path Conversion", "toWinPath /home/user -> D:\\", []() {
        return toWinPath("/home/user") == "D:\\";
    });
    add("Path Conversion", "toWinPath /home/user/Documents -> D:\\Documents", []() {
        return toWinPath("/home/user/Documents") == "D:\\Documents";
    });
    add("Path Conversion", "toWinPath /home/user/Downloads/sub/file.txt -> D:\\Downloads\\sub\\file.txt", []() {
        return toWinPath("/home/user/Downloads/sub/file.txt") == "D:\\Downloads\\sub\\file.txt";
    });
    add("Path Conversion", "toWinPath / -> C:\\", []() {
        return toWinPath("/") == "C:\\";
    });
    add("Path Conversion", "toWinPath /usr -> C:\\usr", []() {
        return toWinPath("/usr") == "C:\\usr";
    });
    add("Path Conversion", "toWinPath /usr/bin/bash -> C:\\usr\\bin\\bash", []() {
        return toWinPath("/usr/bin/bash") == "C:\\usr\\bin\\bash";
    });
    add("Path Conversion", "toWinPath /var/log -> C:\\var\\log", []() {
        return toWinPath("/var/log") == "C:\\var\\log";
    });
    add("Path Conversion", "toWinPath empty path -> empty string", []() {
        return toWinPath("").isEmpty();
    });
    add("Path Conversion", "toLinuxPath D:\\ -> /home/user", []() {
        return toLinuxPath("D:\\") == "/home/user";
    });
    add("Path Conversion", "toLinuxPath D: -> /home/user", []() {
        return toLinuxPath("D:") == "/home/user";
    });
    add("Path Conversion", "toLinuxPath D:\\Documents -> /home/user/Documents", []() {
        return toLinuxPath("D:\\Documents") == "/home/user/Documents";
    });
    add("Path Conversion", "toLinuxPath C:\\ -> /", []() {
        return toLinuxPath("C:\\") == "/";
    });
    add("Path Conversion", "toLinuxPath C:\\usr\\bin -> /usr/bin", []() {
        return toLinuxPath("C:\\usr\\bin") == "/usr/bin";
    });
    add("Path Conversion", "toLinuxPath C:\\etc\\passwd -> /etc/passwd", []() {
        return toLinuxPath("C:\\etc\\passwd") == "/etc/passwd";
    });
    add("Path Conversion", "Path Roundtrip Idempotency D:\\Projects", []() {
        QString original = "/home/user/Projects";
        return toLinuxPath(toWinPath(original)) == original;
    });

    // ------------------------------------------------------------------------
    // Category 2: Drive Storage & Formatting (formatGigabytes)
    // ------------------------------------------------------------------------
    add("Storage Formatting", "formatGigabytes 0 bytes", []() {
        return formatGigabytes(0) == "0.0 GB";
    });
    add("Storage Formatting", "formatGigabytes 512 MB (0.5 GB)", []() {
        return formatGigabytes(512ULL * 1024ULL * 1024ULL) == "0.5 GB";
    });
    add("Storage Formatting", "formatGigabytes 1 GB", []() {
        return formatGigabytes(1024ULL * 1024ULL * 1024ULL) == "1.0 GB";
    });
    add("Storage Formatting", "formatGigabytes 9.5 GB", []() {
        return formatGigabytes(static_cast<quint64>(9.5 * 1024 * 1024 * 1024)) == "9.5 GB";
    });
    add("Storage Formatting", "formatGigabytes 10 GB rounded", []() {
        return formatGigabytes(10ULL * 1024ULL * 1024ULL * 1024ULL) == "10 GB";
    });
    add("Storage Formatting", "formatGigabytes 100 GB", []() {
        return formatGigabytes(100ULL * 1024ULL * 1024ULL * 1024ULL) == "100 GB";
    });
    add("Storage Formatting", "formatGigabytes 500 GB", []() {
        return formatGigabytes(500ULL * 1024ULL * 1024ULL * 1024ULL) == "500 GB";
    });
    add("Storage Formatting", "formatGigabytes 1 TB (1024 GB)", []() {
        return formatGigabytes(1024ULL * 1024ULL * 1024ULL * 1024ULL) == "1024 GB";
    });
    add("Storage Formatting", "Storage percentage calculation sanity", []() {
        quint64 total = 1000, free = 250;
        quint64 used = total - free;
        int pct = static_cast<int>((used * 100ULL) / total);
        return pct == 75;
    });
    add("Storage Formatting", "Zero total storage percentage guard", []() {
        quint64 total = 0, used = 0;
        int pct = (total > 0) ? static_cast<int>((used * 100ULL) / total) : 0;
        return pct == 0;
    });

    // ------------------------------------------------------------------------
    // Category 3: Drive Index Mapping (getDriveIndexFromName)
    // ------------------------------------------------------------------------
    add("Drive Index", "Local Disk (C:) -> 0", []() {
        return getDriveIndexFromName("Local Disk (C:)") == 0;
    });
    add("Drive Index", "Local Disk (D:) -> 1", []() {
        return getDriveIndexFromName("Local Disk (D:)") == 1;
    });
    add("Drive Index", "USB Drive (E:) -> 2", []() {
        return getDriveIndexFromName("USB Drive (E:)") == 2;
    });
    add("Drive Index", "USB Drive (F:) -> 3", []() {
        return getDriveIndexFromName("USB Drive (F:)") == 3;
    });
    add("Drive Index", "USB Drive (G:) -> 4", []() {
        return getDriveIndexFromName("USB Drive (G:)") == 4;
    });
    add("Drive Index", "Removable Disk (E:) -> 2", []() {
        return getDriveIndexFromName("Removable Disk (E:)") == 2;
    });
    add("Drive Index", "Removable Disk (H:) -> 5", []() {
        return getDriveIndexFromName("Removable Disk (H:)") == 5;
    });
    add("Drive Index", "Unknown or invalid drive name fallback to 0", []() {
        return getDriveIndexFromName("Network Share") == 0;
    });

    // ------------------------------------------------------------------------
    // Category 4: Special System Folders (isSpecialSystemFolder)
    // ------------------------------------------------------------------------
    add("System Folders", "Desktop recognized", []() { return isSpecialSystemFolder("Desktop"); });
    add("System Folders", "Documents recognized", []() { return isSpecialSystemFolder("Documents"); });
    add("System Folders", "Downloads recognized", []() { return isSpecialSystemFolder("Downloads"); });
    add("System Folders", "Pictures recognized", []() { return isSpecialSystemFolder("Pictures"); });
    add("System Folders", "Music recognized", []() { return isSpecialSystemFolder("Music"); });
    add("System Folders", "Videos recognized", []() { return isSpecialSystemFolder("Videos"); });
    add("System Folders", "This PC is not a user folder", []() { return !isSpecialSystemFolder("This PC"); });
    add("System Folders", "Random folder rejected", []() { return !isSpecialSystemFolder("MyFolder"); });
    add("System Folders", "Empty string rejected", []() { return !isSpecialSystemFolder(""); });

    // ------------------------------------------------------------------------
    // Category 5: This PC Path & Protection (isThisPcPath, isDirectThisPcItem)
    // ------------------------------------------------------------------------
    add("This PC Protection", "isThisPcPath root check", []() {
        return isThisPcPath("/home/user/.win-explorer/ThisPC");
    });
    add("This PC Protection", "isThisPcPath child item check", []() {
        return isThisPcPath("/home/user/.win-explorer/ThisPC/Drive_C");
    });
    add("This PC Protection", "isThisPcPath negative check", []() {
        return !isThisPcPath("/home/user/Documents");
    });
    add("This PC Protection", "isDirectThisPcItem Drive_C is direct", []() {
        return isDirectThisPcItem(QDir::homePath() + "/.win-explorer/ThisPC/Drive_C");
    });
    add("This PC Protection", "isDirectThisPcItem Drive_D is direct", []() {
        return isDirectThisPcItem(QDir::homePath() + "/.win-explorer/ThisPC/Drive_D");
    });
    add("This PC Protection", "isDirectThisPcItem Desktop is direct", []() {
        return isDirectThisPcItem(QDir::homePath() + "/.win-explorer/ThisPC/Desktop");
    });
    add("This PC Protection", "isDirectThisPcItem nested subfolder is NOT direct", []() {
        return !isDirectThisPcItem(QDir::homePath() + "/.win-explorer/ThisPC/Drive_C/Windows");
    });
    add("This PC Protection", "isDirectThisPcItem user folder is NOT direct", []() {
        return !isDirectThisPcItem(QDir::homePath() + "/Documents/File.txt");
    });
    add("This PC Protection", "isDirectThisPcItem ThisPC folder itself is NOT a child item", []() {
        return !isDirectThisPcItem(QDir::homePath() + "/.win-explorer/ThisPC");
    });

    // ------------------------------------------------------------------------
    // Category 6: Archive Detection (isArchiveFile)
    // ------------------------------------------------------------------------
    add("Archive Detection", ".zip is archive", []() { return isArchiveFile("archive.zip"); });
    add("Archive Detection", ".ZIP uppercase is archive", []() { return isArchiveFile("archive.ZIP"); });
    add("Archive Detection", ".tar is archive", []() { return isArchiveFile("data.tar"); });
    add("Archive Detection", ".tar.gz is archive", []() { return isArchiveFile("backup.tar.gz"); });
    add("Archive Detection", ".tgz is archive", []() { return isArchiveFile("package.tgz"); });
    add("Archive Detection", ".tar.bz2 is archive", []() { return isArchiveFile("src.tar.bz2"); });
    add("Archive Detection", ".tbz2 is archive", []() { return isArchiveFile("src.tbz2"); });
    add("Archive Detection", ".tar.xz is archive", []() { return isArchiveFile("code.tar.xz"); });
    add("Archive Detection", ".txz is archive", []() { return isArchiveFile("code.txz"); });
    add("Archive Detection", ".7z is archive", []() { return isArchiveFile("archive.7z"); });
    add("Archive Detection", ".rar is archive", []() { return isArchiveFile("archive.rar"); });
    add("Archive Detection", ".iso is archive", []() { return isArchiveFile("system.iso"); });
    add("Archive Detection", ".gz is archive", []() { return isArchiveFile("file.gz"); });
    add("Archive Detection", ".bz2 is archive", []() { return isArchiveFile("file.bz2"); });
    add("Archive Detection", ".xz is archive", []() { return isArchiveFile("file.xz"); });
    add("Archive Detection", ".txt is not archive", []() { return !isArchiveFile("doc.txt"); });
    add("Archive Detection", ".pdf is not archive", []() { return !isArchiveFile("paper.pdf"); });
    add("Archive Detection", ".exe is not archive", []() { return !isArchiveFile("setup.exe"); });
    add("Archive Detection", "Empty string is not archive", []() { return !isArchiveFile(""); });

    // ------------------------------------------------------------------------
    // Category 7: Text Previewability (isPreviewableTextFile)
    // ------------------------------------------------------------------------
    QString scratchDir = QDir::tempPath() + QStringLiteral("/test_qa_sandbox/");
    QDir().mkpath(scratchDir);

    add("Text Preview", "Plain UTF-8 text file previewable", [scratchDir]() {
        QString p = scratchDir + "sample.txt";
        QFile f(p);
        f.open(QIODevice::WriteOnly);
        f.write("Hello Windows 10 Explorer QA Suite!");
        f.close();
        QString content;
        bool ok = isPreviewableTextFile(p, &content);
        QFile::remove(p);
        return ok && content.contains("Windows 10 Explorer");
    });
    add("Text Preview", "Empty text file previewable with empty content", [scratchDir]() {
        QString p = scratchDir + "empty.txt";
        QFile f(p);
        f.open(QIODevice::WriteOnly);
        f.close();
        QString content;
        bool ok = isPreviewableTextFile(p, &content);
        QFile::remove(p);
        return ok && content.isEmpty();
    });
    add("Text Preview", "Binary file with NUL byte rejected", [scratchDir]() {
        QString p = scratchDir + "binary.dat";
        QFile f(p);
        f.open(QIODevice::WriteOnly);
        const char binData[] = {'E', 'L', 'F', '\0', 0x01, 0x02};
        f.write(binData, sizeof(binData));
        f.close();
        bool ok = isPreviewableTextFile(p);
        QFile::remove(p);
        return !ok;
    });
    add("Text Preview", "Non-existent file rejected", []() {
        return !isPreviewableTextFile("/non/existent/path/file.txt");
    });
    add("Text Preview", "Directory path rejected", [scratchDir]() {
        return !isPreviewableTextFile(scratchDir);
    });

    // ------------------------------------------------------------------------
    // Category 8: File Extension & MIME Comment Mapping
    // ------------------------------------------------------------------------
    add("MIME Comment", ".txt -> Text Document", []() {
        return getMimeCommentForPath("notes.txt", "notes.txt", false) == "Text Document";
    });
    add("MIME Comment", ".log -> Text Document", []() {
        return getMimeCommentForPath("app.log", "app.log", false) == "Text Document";
    });
    add("MIME Comment", ".cfg -> Configuration settings", []() {
        return getMimeCommentForPath("settings.cfg", "settings.cfg", false) == "Configuration settings";
    });
    add("MIME Comment", ".ini -> Configuration settings", []() {
        return getMimeCommentForPath("config.ini", "config.ini", false) == "Configuration settings";
    });
    add("MIME Comment", ".conf -> Configuration settings", []() {
        return getMimeCommentForPath("daemon.conf", "daemon.conf", false) == "Configuration settings";
    });
    add("MIME Comment", ".html -> Microsoft Edge HTML Document", []() {
        return getMimeCommentForPath("index.html", "index.html", false) == "Microsoft Edge HTML Document";
    });
    add("MIME Comment", ".md -> Markdown Document", []() {
        return getMimeCommentForPath("README.md", "README.md", false) == "Markdown Document";
    });
    add("MIME Comment", ".js -> JavaScript File", []() {
        return getMimeCommentForPath("index.js", "index.js", false) == "JavaScript File";
    });
    add("MIME Comment", ".ts -> TypeScript File", []() {
        return getMimeCommentForPath("app.ts", "app.ts", false) == "TypeScript File";
    });
    add("MIME Comment", ".json -> JSON File", []() {
        return getMimeCommentForPath("package.json", "package.json", false) == "JSON File";
    });
    add("MIME Comment", ".py -> Python File", []() {
        return getMimeCommentForPath("script.py", "script.py", false) == "Python File";
    });
    add("MIME Comment", ".cpp -> C/C++ Source File", []() {
        return getMimeCommentForPath("main.cpp", "main.cpp", false) == "C/C++ Source File";
    });
    add("MIME Comment", ".pdf -> PDF Document", []() {
        return getMimeCommentForPath("doc.pdf", "doc.pdf", false) == "PDF Document";
    });
    add("MIME Comment", ".zip -> Compressed (zipped) Folder", []() {
        return getMimeCommentForPath("files.zip", "files.zip", false) == "Compressed (zipped) Folder";
    });
    add("MIME Comment", ".rar -> WinRAR archive", []() {
        return getMimeCommentForPath("files.rar", "files.rar", false) == "WinRAR archive";
    });
    add("MIME Comment", ".7z -> 7-Zip archive", []() {
        return getMimeCommentForPath("files.7z", "files.7z", false) == "7-Zip archive";
    });
    add("MIME Comment", ".exe -> Application", []() {
        return getMimeCommentForPath("setup.exe", "setup.exe", false) == "Application";
    });
    add("MIME Comment", ".bat -> Windows Batch File", []() {
        return getMimeCommentForPath("run.bat", "run.bat", false) == "Windows Batch File";
    });
    add("MIME Comment", ".csv -> Microsoft Excel Comma Separated Values File", []() {
        return getMimeCommentForPath("data.csv", "data.csv", false) == "Microsoft Excel Comma Separated Values File";
    });
    add("MIME Comment", "Directory -> File folder", []() {
        return getMimeCommentForPath("/home/user/MyFolder", "MyFolder", true) == "File folder";
    });
    add("MIME Comment", "This PC drive category -> Devices and drives", []() {
        return getMimeCommentForPath("/home/user/.win-explorer/ThisPC/Drive_C", "Local Disk (C:)", true).contains("Devices and drives");
    });
    add("MIME Comment", "This PC special folder category -> Folders", []() {
        return getMimeCommentForPath("/home/user/.win-explorer/ThisPC/Documents", "Documents", true) == "Folders";
    });

    // ------------------------------------------------------------------------
    // Category 9: Windows 10 Icon Mapping
    // ------------------------------------------------------------------------
    add("Icon Mapping", "This PC directory -> computer", []() {
        return getIconNameForPath("", "This PC", true) == "computer";
    });
    add("Icon Mapping", "Local Disk (C:) -> drive-harddisk-system", []() {
        return getIconNameForPath("", "Local Disk (C:)", true) == "drive-harddisk-system";
    });
    add("Icon Mapping", "Local Disk (D:) -> drive-harddisk", []() {
        return getIconNameForPath("", "Local Disk (D:)", true) == "drive-harddisk";
    });
    add("Icon Mapping", "USB Drive (E:) -> drive-removable-media-usb", []() {
        return getIconNameForPath("", "USB Drive (E:)", true) == "drive-removable-media-usb";
    });
    add("Icon Mapping", "Desktop folder -> user-desktop", []() {
        return getIconNameForPath("", "Desktop", true) == "user-desktop";
    });
    add("Icon Mapping", "Downloads folder -> folder-downloads", []() {
        return getIconNameForPath("", "Downloads", true) == "folder-downloads";
    });
    add("Icon Mapping", "Documents folder -> folder-documents", []() {
        return getIconNameForPath("", "Documents", true) == "folder-documents";
    });
    add("Icon Mapping", ".txt icon contains notepad.png", []() {
        return getIconNameForPath("file.txt", "file.txt", false).endsWith("notepad.png");
    });
    add("Icon Mapping", ".zip icon contains zip.png", []() {
        return getIconNameForPath("file.zip", "file.zip", false).endsWith("zip.png");
    });
    add("Icon Mapping", ".rar icon contains rar.png", []() {
        return getIconNameForPath("file.rar", "file.rar", false).endsWith("rar.png");
    });
    add("Icon Mapping", ".7z icon contains 7z.png", []() {
        return getIconNameForPath("file.7z", "file.7z", false).endsWith("7z.png");
    });

    // ------------------------------------------------------------------------
    // Category 10: FreeDesktop Recycle Bin Engine
    // ------------------------------------------------------------------------
    add("Recycle Bin Engine", "Safe move to Trash creates valid .trashinfo metadata", [scratchDir]() {
        QString p = scratchDir + "test_recycle_bin.txt";
        QFile f(p);
        f.open(QIODevice::WriteOnly);
        f.write("Important User File to be protected!");
        f.close();

        QString infoPath;
        bool ok = testSafeMoveToRecycleBin(p, infoPath);
        if (!ok) return false;

        QFile info(infoPath);
        bool infoOk = info.open(QIODevice::ReadOnly);
        if (!infoOk) return false;
        QString text = QString::fromUtf8(info.readAll());
        info.close();

        // Clean up
        QFile::remove(infoPath);
        QString trashFile = QDir::homePath() + "/.local/share/Trash/files/test_recycle_bin.txt";
        QFile::remove(trashFile);

        return text.contains("[Trash Info]") && text.contains("Path=") && text.contains("DeletionDate=");
    });
    add("Recycle Bin Engine", "Collision renaming file (1).txt when duplicate in Trash", [scratchDir]() {
        QString trashFile = QDir::homePath() + "/.local/share/Trash/files/collision_test.txt";
        QString trashInfo = QDir::homePath() + "/.local/share/Trash/info/collision_test.txt.trashinfo";
        QFile tf(trashFile); tf.open(QIODevice::WriteOnly); tf.write("existing"); tf.close();
        QFile ti(trashInfo); ti.open(QIODevice::WriteOnly); ti.write("existing"); ti.close();

        QString p = scratchDir + "collision_test.txt";
        QFile f(p); f.open(QIODevice::WriteOnly); f.write("new file"); f.close();

        QString outInfo;
        bool ok = testSafeMoveToRecycleBin(p, outInfo);

        // Verify that collision file (1) was created
        QString expectedFile = QDir::homePath() + "/.local/share/Trash/files/collision_test (1).txt";
        bool existsRenamed = QFile::exists(expectedFile);

        // Cleanup
        QFile::remove(trashFile);
        QFile::remove(trashInfo);
        QFile::remove(expectedFile);
        QFile::remove(outInfo);

        return ok && existsRenamed;
    });
    add("Recycle Bin Engine", "Non-existent file to trash safely returns false", []() {
        QString dummy;
        return !testSafeMoveToRecycleBin("/non/existent/file/test.bin", dummy);
    });

    // ------------------------------------------------------------------------
    // Category 11: Real Shared Library Symbol Audit via dlopen
    // ------------------------------------------------------------------------
    dlopen("/usr/lib64/libKF6KIOCore.so.6", RTLD_NOW | RTLD_GLOBAL);
    void* lib = dlopen("/home/user/.config/win-explorer/libwin10_thispc.so", RTLD_NOW | RTLD_GLOBAL);
    if (!lib) {
        lib = dlopen("/home/user/.config/win-explorer/libwin10_thispc.so", RTLD_LAZY | RTLD_GLOBAL);
    }
    add("Library Hook Audit", "libwin10_thispc.so is dynamically loadable", [lib]() {
        return lib != nullptr;
    });
    add("Library Hook Audit", "DeleteOrTrashJob C1 constructor symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN3KIO16DeleteOrTrashJobC1ERK5QListI4QUrlENS_22AskUserActionInterface12DeletionTypeENS6_16ConfirmationTypeEP7QObject") != nullptr;
    });
    add("Library Hook Audit", "DeleteOrTrashJob C2 constructor symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN3KIO16DeleteOrTrashJobC2ERK5QListI4QUrlENS_22AskUserActionInterface12DeletionTypeENS6_16ConfirmationTypeEP7QObject") != nullptr;
    });
    add("Library Hook Audit", "KIO::file_delete symbol exported and wired", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN3KIO11file_deleteERK4QUrl6QFlagsINS_7JobFlagEE") != nullptr;
    });
    add("Library Hook Audit", "KIO::del single URL symbol exported and wired", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN3KIO3delERK4QUrl6QFlagsINS_7JobFlagEE") != nullptr;
    });
    add("Library Hook Audit", "KIO::del URL list symbol exported and wired", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN3KIO3delERK5QListI4QUrlE6QFlagsINS_7JobFlagEE") != nullptr;
    });
    add("Library Hook Audit", "KFileItem::iconName symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZNK9KFileItem8iconNameEv") != nullptr;
    });
    add("Library Hook Audit", "KFileItem::mimeComment symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZNK9KFileItem11mimeCommentEv") != nullptr;
    });
    add("Library Hook Audit", "KFileItem::isLink symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZNK9KFileItem6isLinkEv") != nullptr;
    });
    add("Library Hook Audit", "KFileItem::text symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZNK9KFileItem4textEv") != nullptr;
    });
    add("Library Hook Audit", "KFileItem::isHidden symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZNK9KFileItem8isHiddenEv") != nullptr;
    });
    add("Library Hook Audit", "ViewProperties C1 constructor symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN14ViewPropertiesC1ERK4QUrl") != nullptr;
    });
    add("Library Hook Audit", "ViewProperties C2 constructor symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN14ViewPropertiesC2ERK4QUrl") != nullptr;
    });
    add("Library Hook Audit", "ViewProperties hiddenFilesShown symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZNK14ViewProperties16hiddenFilesShownEv") != nullptr;
    });
    add("Library Hook Audit", "ViewProperties sortHiddenLast symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZNK14ViewProperties14sortHiddenLastEv") != nullptr;
    });
    add("Library Hook Audit", "QPainter::setFont symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN8QPainter7setFontERK5QFont") != nullptr;
    });
    add("Library Hook Audit", "QTextLayout::setFont symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN11QTextLayout7setFontERK5QFont") != nullptr;
    });
    add("Library Hook Audit", "QFont::setItalic symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "_ZN5QFont9setItalicEb") != nullptr;
    });
    add("Library Hook Audit", "readdir64 symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "readdir64") != nullptr;
    });
    add("Library Hook Audit", "readdir symbol exported", [lib]() {
        if (!lib) return false;
        return dlsym(lib, "readdir") != nullptr;
    });

    // ------------------------------------------------------------------------
    // EXECUTE ALL TESTS
    // ------------------------------------------------------------------------
    std::cout << "======================================================================\n";
    std::cout << "🚀 STARTING COMPREHENSIVE QA AUTOMATED TEST SUITE FOR WIN10 EXPLORER\n";
    std::cout << "======================================================================\n";
    std::cout << "Total Test Cases: " << suite.size() << "\n\n";

    int passed = 0;
    int failed = 0;

    for (const auto& tc : suite) {
        bool res = false;
        try {
            res = tc.testFn();
        } catch (const std::exception& e) {
            res = false;
        } catch (...) {
            res = false;
        }

        if (res) {
            passed++;
            std::cout << "  [PASS] Test #" << tc.id << " [" << tc.category << "] " << tc.name << "\n";
        } else {
            failed++;
            std::cout << "  [FAIL] Test #" << tc.id << " [" << tc.category << "] " << tc.name << "\n";
        }
    }

    std::cout << "\n======================================================================\n";
    std::cout << "🏁 QA TEST SUITE SUMMARY: " << passed << " PASSED, " << failed << " FAILED\n";
    std::cout << "======================================================================\n";

    return (failed == 0) ? 0 : 1;
}
