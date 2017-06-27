#include "mainwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont font  = qApp->font();
    font.setFamily("Calibri");
    font.setPointSize(11);
    qApp->setFont(font);

    QString FileName = QCoreApplication::applicationDirPath() + "/config.ini";
    if(false == QFile::exists(FileName))
    {
        QSize screenSize = qApp->primaryScreen()->size();
        QSettings *pConfig = new QSettings(FileName, QSettings::IniFormat);
        pConfig->setValue("Basic/PositionX", screenSize.width() / 2);
        pConfig->setValue("Basic/PositionY", screenSize.height() / 2);
        pConfig->setValue("Basic/Color", "0 255 0");
        pConfig->setValue("Other/Version", 1);
        delete pConfig;
    }

    QSettings *pConfig = new QSettings(FileName, QSettings::IniFormat);
    g_Config.PosX = pConfig->value("Basic/PositionX").toInt();
    g_Config.PosY = pConfig->value("Basic/PositionY").toInt();
    QString r,g,b;
    QTextStream stream(pConfig->value("Basic/Color").toByteArray());
    stream>>r>>g>>b;
    g_Config.Color = QColor(r.toInt(), g.toInt(), b.toInt());
    g_Config.Version = pConfig->value("Basic/Version").toInt();

    MainWidget w;
    w.setWindowFlags(w.windowFlags() | Qt::WindowStaysOnTopHint);
    w.move(g_Config.PosX, g_Config.PosY);
    w.show();

    return a.exec();
}
