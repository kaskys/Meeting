#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    MeetingCore lib_core;
    MeetingAdapter madapter;
    madapter.onCorrelateControlCore(lib_core.onControlHolder());
    w.setAdapter(&madapter);
    w.show();
    return a.exec();
}
