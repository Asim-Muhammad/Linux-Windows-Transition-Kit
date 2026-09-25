
#include <QCoreApplication>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <dlfcn.h>
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    dlopen("/usr/lib64/qt6/plugins/plasma/applets/org.kde.plasma.kickoff.so", RTLD_NOW);
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString p = it.next();
        if (p.contains("kickoff") && (p.endsWith(".qml") || p.endsWith(".xml") || p.endsWith(".json"))) {
            QTextStream(stdout) << "=== " << p << " ===\n";
            if (p.endsWith("main.qml") || p.endsWith("Header.qml") || p.endsWith("CompactRepresentation.qml") || p.endsWith("main.xml") || p.endsWith("FullRepresentation.qml")) {
                QFile f(p);
                if (f.open(QIODevice::ReadOnly)) {
                    QTextStream(stdout) << f.readAll() << "\n";
                }
            }
        }
    }
    return 0;
}
