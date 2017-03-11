#include "mainwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont font  = qApp->font();
    font.setFamily("Calibri");
    font.setPointSize(11);
    qApp->setFont(font);

    MainWidget w;
    w.setWindowFlags(w.windowFlags() | Qt::WindowStaysOnTopHint);
    w.show();

    return a.exec();
}
