import subprocess
import time

code = '''
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
                ts << a->objectName() << " | " << a->text() << "\\n";
            }
        }
    });
}
'''
with open('dump_actions.cpp', 'w') as f:
    f.write(code)

subprocess.run(["g++", "-shared", "-fPIC", "dump_actions.cpp", "-o", "libdump.so", 
                "-I/usr/include/qt6/QtWidgets", "-I/usr/include/qt6", "-I/usr/include/qt6/QtGui", 
                "-I/usr/include/qt6/QtCore", "-lQt6Widgets", "-lQt6Core"])
