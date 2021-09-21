#include "mainwidget.h"
#include <QApplication>

void setHighDpiEnvironmentVariable()
{
#ifdef Q_OS_WIN
    if (!qEnvironmentVariableIsSet("QT_OPENGL"))
        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif

    if (!qEnvironmentVariableIsSet("QT_DEVICE_PIXEL_RATIO") // legacy in 5.6, but still functional
        && !qEnvironmentVariableIsSet("QT_AUTO_SCREEN_SCALE_FACTOR")
        && !qEnvironmentVariableIsSet("QT_SCALE_FACTOR")
        && !qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS")) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    }
}

int main(int argc, char *argv[])
{
    setHighDpiEnvironmentVariable();

    QApplication a(argc, argv);

#ifdef Q_OS_WIN
    QFont font  = qApp->font();
    font.setFamily("Calibri");
    qApp->setFont(font);
#endif

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
    w.setWindowIcon(QIcon(":/monitor.png"));
    w.setWindowFlags(w.windowFlags() | Qt::WindowStaysOnTopHint);
    w.move(screenSize.width() * rx / 1000, screenSize.height() *ry / 1000);
    w.show();

    return a.exec();
}
