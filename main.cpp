#include "mainwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont font  = qApp->font();
    font.setFamily("Calibri");
    qApp->setFont(font);

    QString FileName = QCoreApplication::applicationDirPath() + "/config.ini";
    if(false == QFile::exists(FileName))
    {
        QSettings *pConfig = new QSettings(FileName, QSettings::IniFormat);
        pConfig->setValue("Basic/RelativeX", 500);
        pConfig->setValue("Basic/RelativeY", 500);
        pConfig->setValue("Basic/Hide", false);
        pConfig->setValue("Basic/DisplayC", 100);
        pConfig->setValue("Basic/Color", "0 255 0");
        pConfig->setValue("Other/Version", 1);
        delete pConfig;
    }

    QSettings *pConfig = new QSettings(FileName, QSettings::IniFormat);
    int rx = pConfig->value("Basic/RelativeX").toInt();
    int ry = pConfig->value("Basic/RelativeY").toInt();
    delete pConfig;

    QSize screenSize = qApp->primaryScreen()->size();
    MainWidget w;
    w.setWindowFlags(w.windowFlags() | Qt::WindowStaysOnTopHint);
    w.move(screenSize.width() * rx / 1000, screenSize.height() *ry / 1000);
    w.show();

    return a.exec();
}
