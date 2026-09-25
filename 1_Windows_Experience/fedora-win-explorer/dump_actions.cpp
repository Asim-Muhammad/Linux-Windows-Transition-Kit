
#include <QApplication>
#include <QAction>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <dlfcn.h>
#include <stdio.h>

__attribute__((constructor)) void init_dump() {
    QTimer::singleShot(2000, qApp, []() {
        QFile f("/home/user/.gemini/antigravity/scratch/actions.txt");
        if (f.open(QIODevice::WriteOnly)) {
            QTextStream ts(&f);
            for (QAction* a : qApp->findChildren<QAction*>()) {
                ts << a->objectName() << " | " << a->text() << "\n";
            }
        }
    });
}
