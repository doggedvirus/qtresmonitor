#include "mainwidget.h"
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QDataStream>
#include <QWaitCondition>

#ifdef Q_OS_WIN
#define _WIN32_WINNT 0x0600
#include "iphlpapi.h"
#endif


#ifdef Q_OS_OSX
TopThread::TopThread(QObject *parent) :
    QThread(parent)
{
    m_interval = 1000;
    m_MemeoryRate = 0;
    m_CpuRate = 0;
    m_Upload = "0B/s";
    m_Download = "0B/s";
    m_Over = false;
    m_preIn = 0;
    m_preOut = 0;
    m_preTime = 0;
}

TopThread::~TopThread()
{
}

void TopThread::run(void)
{
    QProcess proc;
    QStringList args;
    args <<"-c"<<"top -d -F -R -o cpu";
    proc.start( "/bin/bash", args );
    proc.waitForStarted();
    while(m_Over == false)
    {
        QEventLoop loop;
        QTimer::singleShot(m_interval, &loop, &QEventLoop::quit);
        loop.exec();

        QString oneStatus = proc.readAll();
        if(oneStatus.isEmpty())
        {
            continue;
        }
        int begin = oneStatus.indexOf(" sys, ");
        begin = begin + 6;
        int end = begin + oneStatus.midRef(begin).indexOf("% idle");
        if(end > begin)
        {
            m_CpuRate = 100 - oneStatus.midRef(begin, end - begin).toFloat();
        }

        begin = oneStatus.indexOf("PhysMem: ");
        begin = begin + 9;
        end = begin + oneStatus.midRef(begin).indexOf("M used");
        if(end > begin)
        {
            int usedMem = oneStatus.midRef(begin, end - begin).toInt();
            begin = end + oneStatus.midRef(end).indexOf("(");
            begin = begin + 1;
            end = begin + oneStatus.midRef(begin).indexOf("M wired");
            if(end > begin)
            {
                int wiredMem = oneStatus.midRef(begin, end - begin).toInt();
                begin = end + oneStatus.midRef(end).indexOf("wired), ");
                begin = begin + 8;
                end = begin + oneStatus.midRef(begin).indexOf("M unused");
                if(end > begin)
                {
                    int unusedMem = oneStatus.midRef(begin, end - begin).toInt();
                    if(usedMem + unusedMem > 0)
                    {
                        m_MemeoryRate = (usedMem - wiredMem) * 100 / (usedMem + unusedMem);
                    }
                }
            }
        }

        begin = oneStatus.indexOf("Networks: packets: ");
        begin = begin + oneStatus.midRef(begin).indexOf("/");
        begin = begin + 1;
        end = begin + oneStatus.midRef(begin).indexOf(" in");
        if(end > begin)
        {
            m_Download = oneStatus.midRef(begin, end - begin).toString();
            if(m_Download.length() > 10) {
                m_Download = "0B";
            }

            if(!m_Download.endsWith("B")) {
                m_Download += "B";
            }

            m_Download += "/s";
        }

        begin = end + oneStatus.midRef(end).indexOf("/");
        begin = begin + 1;
        end = begin + oneStatus.midRef(begin).indexOf(" out");
        if(end > begin)
        {
            m_Upload = oneStatus.midRef(begin, end - begin).toString();
            if(m_Upload.length() > 10) {
                m_Upload = "0B";
            }

            if(!m_Upload.endsWith("B")) {
                m_Upload += "B";
            }

            m_Upload += "/s";
        }
    }
}

uint TopThread::getMemoryRate(void)
{
    uint ret = m_MemeoryRate;
    return ret;
}

uint TopThread::getCpuRate(void)
{
    uint ret = m_CpuRate;
    return ret;
}

QString TopThread::getUpload(void)
{
    QString ret = m_Upload;
    return ret;
}

QString TopThread::getDownload(void)
{
    QString ret = m_Download;
    return ret;
}

void TopThread::setInterval(int mSeconds)
{
    m_interval = mSeconds;
}

void TopThread::setOver(bool over)
{
    m_Over = over;
}
#endif

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    QString FileName = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings *pConfig = new QSettings(FileName, QSettings::IniFormat);
    m_Color = getColorFromArray(pConfig->value("Basic/Color").toByteArray());
    m_rx = pConfig->value("Basic/RelativeX").toInt();
    m_ry = pConfig->value("Basic/RelativeY").toInt();
    m_hide = pConfig->value("Basic/Hide").toBool();
    delete pConfig;

    QPalette pa;
    pa.setColor(QPalette::WindowText, m_Color);

    CpuRate_Label = new QLabel(this);
    CpuRate_Label->setPalette(pa);
    RamRate_Label = new QLabel(this);
    RamRate_Label->setPalette(pa);
    uploadSpeed_Label = new QLabel(this);
    uploadSpeed_Label->setPalette(pa);
    downloadSpeed_Label = new QLabel(this);
    downloadSpeed_Label->setPalette(pa);

    if(m_hide) {
        CpuRate_Label->hide();
        RamRate_Label->hide();
        uploadSpeed_Label->hide();
        downloadSpeed_Label->hide();
    }

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout_slot()));
    m_scanTimer = new QTimer(this);
    connect(m_scanTimer, SIGNAL(timeout()), this, SLOT(scanTimeout_slot()));

    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
#ifdef Q_OS_WIN
                        | Qt::Tool
#endif
                   );
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_QuitAction = new QAction("Quit",this);
    m_AboutAction = new QAction("About",this);
    m_ColorMenu = new QMenu("Color", this);
    m_ColorMenu->addAction("Green");
    m_ColorMenu->addAction("Gray");
    m_ColorMenu->addAction("Blue");
    m_ColorMenu->addAction("Custom");
    m_Menu = new QMenu(this);
    m_Menu->addMenu(m_ColorMenu);
    m_Menu->addAction(m_AboutAction);
    m_Menu->addAction(m_QuitAction);

    m_TrayIcon = new QSystemTrayIcon(this);
    m_TrayIcon->setIcon(QIcon(":/monitor.png"));
    m_TrayIcon->setContextMenu(m_Menu);
    m_TrayIcon->show();

    connect(m_QuitAction,SIGNAL(triggered()),this, SLOT(quitApp_slot()));
    connect(m_AboutAction,SIGNAL(triggered()),this, SLOT(about_slot()));
    connect(m_ColorMenu,SIGNAL(triggered(QAction*)),this, SLOT(changeColor_slot(QAction*)));

    layoutInit();

    m_preNetIn = 0;
    m_preNetOut = 0;
    m_preIdleTime = 0;
    m_preAllTime = 0;

    m_MemeoryRate = 0;
    m_CpuRate = 0;

    m_Angle = 0;
    m_iPreAngleTime = 9888;//just initial value
    m_timer->start(1000);
    m_scanTimer->start(50);

#if defined Q_OS_OSX
    //use top to get all data,top need run all the time,
    //so create a thread to run  it.
    thread = new TopThread();
    thread->start();
#endif
    QFont font  = qApp->font();
    font.setPixelSize(20);
    qApp->setFont(font);
    setMouseTracking(true);

}

MainWidget::~MainWidget()
{
    m_timer->stop();
}

void MainWidget::quitApp_slot(void)
{
    qApp->quit();
}

void MainWidget::about_slot(void)
{
    QMessageBox::information(this, "About", QString("Version:1.0.0")
                                            + "<br/>Source:<a href=\"https://github.com/doggedvirus/qtresmonitor\">https://github.com/doggedvirus/qtresmonitor</a>"
                                            + "<br/>Author:<a href=\"https://doggedvirus.com/about\">https://doggedvirus.com/about</a>"
                                            + "<br/>Icon Designer:<a href=\"http://weibo.com/foreverdrawing\">http://weibo.com/foreverdrawing</a>");
}

void MainWidget::paintEvent(QPaintEvent *event)
{
    if(m_hide && !mOnWidget) {
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        painter.setPen(QPen(m_Color, 1));
        painter.setBrush(QBrush(Qt::white));
        painter.drawRect(0, 99, 9, -99);
        painter.setBrush(QBrush(m_Color));
        painter.drawRect(0, 99, 9, -m_MemeoryRate);
    } else {
        int start = 2;
        int min = 2;
        int width = 100 + min;
        int max = 100 + start;

        if(0 != (max + min) % 2)
        {
            max++;
            width++;
        }
        int middle = (min + max) / 2;

        //draw a radar
        QPainter painter_horizon(this);
        painter_horizon.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        painter_horizon.setPen(QPen(m_Color));
        QConicalGradient conicalGradient(middle,middle,180.0 - m_Angle);
        conicalGradient.setColorAt(0, m_Color);
        conicalGradient.setColorAt(1.0, QColor(255,255,255,0));
        painter_horizon.setBrush(QBrush(conicalGradient));
        painter_horizon.drawEllipse(start,start,width,width);
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        painter.setPen(QPen(m_Color, 1));

        painter.drawLine(min + 1, middle, max + 1, middle);
        painter.drawLine(middle, min + 1, middle, max + 1);
        painter.drawEllipse(22, 22, 60, 60);

        //draw the line from radar to data
        QPoint p1;
        QPoint p2;
        QPoint p3;
        if(m_Angle >= 120 && m_Angle < 240)
        {
            p1 = QPoint(60, 32);
            p2 = QPoint(72, 20);
            p3 = QPoint(105, 20);
            painter.drawLine(p1, p2);
            painter.drawLine(p2, p3);
        }

        if(m_Angle >= 150 && m_Angle < 270)
        {
            p1 = QPoint(57, 45);
            p2 = QPoint(62, 40);
            p3 = QPoint(105, 40);
            painter.drawLine(p1, p2);
            painter.drawLine(p2, p3);
        }

        if(m_Angle >= 210 && m_Angle < 330)
        {
            p1 = QPoint(57, 55);
            p2 = QPoint(62, 60);
            p3 = QPoint(105, 60);
            painter.drawLine(p1, p2);
            painter.drawLine(p2, p3);
        }

        if(m_Angle >= 240 && m_Angle < 360)
        {
            p1 = QPoint(60, 67);
            p2 = QPoint(72, 80);
            p3 = QPoint(105, 80);
            painter.drawLine(p1, p2);
            painter.drawLine(p2, p3);
        }
    }

    QWidget::paintEvent(event);
}
void MainWidget::mousePressEvent(QMouseEvent *event)
{
    m_hide = false;
    if (event->button() == Qt::LeftButton)
    {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void MainWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    } else if(event->buttons() == Qt::NoButton){
        checkShowAndHide();
    }

    QWidget::mouseMoveEvent(event);
}
void MainWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QSize screenSize = qApp->primaryScreen()->size();
    QPoint now = pos();
    m_rx = now.x() * 1000 / screenSize.width();
    m_ry = now.y() * 1000 / screenSize.height();

    //hide radar when it move to right hand
    if(now.x() + 100 > screenSize.width())
    {
        m_hide = true;
        m_rx = (screenSize.width() - 10) * 1000 / screenSize.width();
    }
    else if(now.x() < 0)
    {
        m_hide = true;
        m_rx = 0;
    }
    else
    {
        m_hide = false;
    }

    if(now.y() + 100 > screenSize.height())
    {
        m_ry = 900;
    }
    else if(now.x()  + 100 < 0)
    {
        m_ry = 100;
    }

    QString FileName = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings *pConfig = new QSettings(FileName, QSettings::IniFormat);
    pConfig->setValue("Basic/RelativeX", m_rx);
    pConfig->setValue("Basic/RelativeY", m_ry);
    pConfig->setValue("Basic/Hide", m_hide);
    delete pConfig;
    move(screenSize.width() * m_rx / 1000, screenSize.height() *m_ry / 1000);
    checkShowAndHide();
    QWidget::mouseReleaseEvent(event);
}

void MainWidget::contextMenuEvent(QContextMenuEvent *event)
{
    m_Menu->exec(QCursor::pos());
    QWidget::contextMenuEvent(event);
}

void MainWidget::layoutInit(void)
{
    CpuRate_Label->move(110, 10);
    RamRate_Label->move(110, 30);
    uploadSpeed_Label->move(110, 50);
    downloadSpeed_Label->move(110, 70);
}

QColor MainWidget::getColorFromArray(QByteArray array)
{
    QString r,g,b;
    QTextStream stream(array);
    stream>>r>>g>>b;
    return QColor(r.toInt(), g.toInt(), b.toInt());
}

void MainWidget::scanTimeout_slot(void)
{
    int iMSecond = QDateTime().currentDateTime().toString("zzz").toInt();
    if(9888 != m_iPreAngleTime)
    {
        int iDifferent = 0;
        if(iMSecond <= m_iPreAngleTime)
        {
            iDifferent = iMSecond + 1000 - m_iPreAngleTime;
        }
        else
        {
            iDifferent = iMSecond - m_iPreAngleTime;
        }

        m_Angle =  m_Angle + 360.0 / 1000 * iDifferent;
        if(m_Angle >= 360)
        {
            m_Angle = 0;
        }
        repaint();
    }
    m_iPreAngleTime = iMSecond;
}

void MainWidget::timeout_slot(void)
{
    //memory
    if(getRamRate())
    {
        RamRate_Label->setText(QString("RAM %1\%").arg(m_MemeoryRate));
        RamRate_Label->adjustSize();
    }

    if(getCpuRate())
    {
        CpuRate_Label->setText(QString("CPU %1\%").arg(m_CpuRate));
        CpuRate_Label->adjustSize();
    }

    if(getNetworkSpeed())
    {
        uploadSpeed_Label->setText("↑ " + m_Upload);
        uploadSpeed_Label->adjustSize();
        downloadSpeed_Label->setText("↓ " + m_Download);
        downloadSpeed_Label->adjustSize();
    }

    checkShowAndHide();

    //refresh widget
    update();
}

void MainWidget::checkShowAndHide(void) {
    QScreen *pscreen = qApp->primaryScreen();
    QSize screenSize = pscreen->size();

    //if screen change,to confirm widget will fit new screen
    if(m_preScreenSize != screenSize)
    {
        move(screenSize.width() * m_rx / 1000, screenSize.height() * m_ry / 1000);
        QFont font  = qApp->font();
        font.setPixelSize(20);
        qApp->setFont(font);
        m_preScreenSize = screenSize;
    }

    //use mouse's position to judge if it is on the widget
    QPoint mouse = cursor().pos();
    if(mouse.x() >= this->x() && mouse.x() <= this->x() + this->width()
       && mouse.y() >= this->y() && mouse.y() <= this->y() + this->height()) {
        mOnWidget = true;
    } else {
        mOnWidget = false;
    }

    if(m_hide && !mOnWidget)
    {
        if(this->x() != (screenSize.width() - 10) * 1000 / screenSize.width())
        {
            move(screenSize.width() * m_rx / 1000, screenSize.height() * m_ry / 1000);
        }
        setFixedSize(10, 100);
        CpuRate_Label->hide();
        RamRate_Label->hide();
        uploadSpeed_Label->hide();
        downloadSpeed_Label->hide();
    } else {
        setFixedSize(230, 110);

        //show complete information when mouse on it
        if(m_hide && mOnWidget)
        {
            if(m_rx > 500)
            {
                move(screenSize.width() - 230, screenSize.height() * m_ry / 1000);
            }
            else
            {
                move(0, screenSize.height() * m_ry / 1000);
            }
        }
        CpuRate_Label->show();
        RamRate_Label->show();
        uploadSpeed_Label->show();
        downloadSpeed_Label->show();
    }
}

QString MainWidget::getSpeedInfo(double downloadSpeed, double uploadSpeed)
{
    QString speedString = "B/s";
    if(downloadSpeed >= 1024 || uploadSpeed >= 1024)
    {
        speedString = "KB/s";

        downloadSpeed = downloadSpeed / 1024.0;
        uploadSpeed = uploadSpeed / 1024.0;
        if(downloadSpeed >= 1024 || uploadSpeed >= 1024)
        {
            speedString = "MB/s";

            downloadSpeed = downloadSpeed / 1024.0;
            uploadSpeed = uploadSpeed / 1024.0;

            if(downloadSpeed >= 1024 || uploadSpeed >= 1024)
            {
                speedString = "GB/s";

                downloadSpeed = downloadSpeed / 1024.0;
                uploadSpeed = uploadSpeed / 1024.0;
            }
        }
    }

    //retain 2 decimals
    downloadSpeed = (double)(int(downloadSpeed * 100)) / 100;
    uploadSpeed = (double)(int(uploadSpeed * 100)) / 100;
    QString ret = QString("%1%2|%3%2").arg(uploadSpeed).arg(speedString).arg(downloadSpeed);
    return ret;
}

bool MainWidget::getRamRate(void)
{
    bool bRet = false;
#ifdef Q_OS_WIN32
    MEMORYSTATUSEX memsStat;
    memsStat.dwLength = sizeof(memsStat);
    bRet = GlobalMemoryStatusEx(&memsStat);
    if(bRet)
    {
        int nMemFree = memsStat.ullAvailPhys / (1024 * 1024);
        int nMemTotal = memsStat.ullTotalPhys / (1024 * 1024);

        m_MemeoryRate = (nMemTotal - nMemFree) * 100 / nMemTotal;
    }
#elif defined Q_OS_LINUX
    QFile file("/proc/meminfo");
    if(file.open(QIODevice::ReadOnly))
    {
        int iMemTotal = 0;
        int iMemFree = 0;
        QStringList list = QString(file.readLine()).split(' ');
        for(int i = 0;i < list.count();i++)
        {
            QString s = list.at(i);
            if(s.toInt() > 0)
            {
                iMemTotal = s.toInt();
                break;
            }
        }

        list = QString(file.readLine()).split(' ');
        for(int i = 0;i < list.count();i++)
        {
            QString s = list.at(i);
            if(s.toInt() > 0)
            {
                iMemFree = s.toInt();
                break;
            }
        }

        if(iMemTotal > iMemFree)
        {
            m_MemeoryRate = (iMemTotal - iMemFree) * 100 / iMemTotal;
            bRet = true;
        }

        file.close();
    }
#elif defined Q_OS_OSX
    m_MemeoryRate = thread->getMemoryRate();
    bRet = true;
#endif
    return bRet;
}

bool MainWidget::getCpuRate(void)
{
    bool bRet = false;
#ifdef Q_OS_WIN32
    long long IdleTime;
    long long KernelTime;
    long long UserTime;
    if(GetSystemTimes((LPFILETIME)&IdleTime, (LPFILETIME)&KernelTime, (LPFILETIME)&UserTime))
    {
        if(0 != m_preIdleTime)
        {
            uint idle = IdleTime - m_preIdleTime;
            uint all = KernelTime + UserTime - m_preAllTime;

            //confirm rate > 0
            m_CpuRate = (all - idle) * 100 / all;
            bRet = true;
        }
        m_preIdleTime = IdleTime;
        m_preAllTime = KernelTime + UserTime;
    }
#elif defined Q_OS_LINUX
    QFile file("/proc/stat");
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray array = file.readLine();
        QStringList list = QString(array).split(' ');
        long long currentAllTime = 0;
        long long currentIdleTime = 0;
        for(int i = 1;i < list.count();i++)
        {
            QString string = list.at(i);
            currentAllTime += string.toLongLong();
            if(5 == i)
            {
                currentIdleTime = string.toLongLong();
            }
        }

        if(0 != m_preAllTime && 0 != m_preIdleTime)
        {
            long long RealAllTime = currentAllTime - m_preAllTime;
            long long RealIdleTime = currentIdleTime - m_preIdleTime;
            //confirm rate > 0
            if(RealAllTime > RealIdleTime)
            {
                m_CpuRate = (double)(RealAllTime - RealIdleTime) / (double)(RealIdleTime) * 100 / 1;
                bRet = true;
            }
        }
        m_preAllTime = currentAllTime;
        m_preIdleTime = currentIdleTime;
        file.close();
    }
#elif defined Q_OS_OSX
    m_CpuRate = thread->getCpuRate();
    bRet = true;
#endif
    return bRet;
}

bool MainWidget::getNetworkSpeed(void)
{
    bool bRet = false;
#ifdef Q_OS_WIN32
    PMIB_IFTABLE m_pTable = NULL;
    DWORD m_dwAdapters = 0;
    //first call is just get the m_dwAdapters's value
    //more detail,pls see https://msdn.microsoft.com/en-us/library/windows/desktop/aa365943(v=vs.85).aspx

    GetIfTable(m_pTable, &m_dwAdapters, FALSE);

    m_pTable = (PMIB_IFTABLE)new BYTE[m_dwAdapters];
    //speed = sum / time,so it should record the time
    int nowTime = QDateTime().currentDateTime().toString("zzz").toInt();
    GetIfTable(m_pTable, &m_dwAdapters, FALSE);
    DWORD NowIn = 0;
    DWORD NowOut = 0;
    QString Desc;
    for (UINT i = 0; i < m_pTable->dwNumEntries; i++)
    {
        MIB_IFROW Row = m_pTable->table[i];
        Desc = QString::fromLatin1((char*)Row.bDescr, Row.dwDescrLen - 1);

        //get rid of unexcept adapter
        if(false == Desc.contains("Virtual")
           && false == Desc.contains("Filter")
           && false == Desc.contains("QoS")
           && false == Desc.contains("Bridge")
           && false == Desc.contains("Pseudo")
           && 0 != QString(QByteArray((char*)Row.bPhysAddr, 8).toHex()).toUpper().compare("0000000000000000"))
        {
            NowIn = NowIn + Row.dwInOctets;
            NowOut = NowOut + Row.dwOutOctets;
        }
    }
    delete []m_pTable;

    if(0 != m_preNetOut && 0 != m_preNetIn)
    {
        double coeffcient = (1000 + nowTime - m_preTime) / 1000.0;
        //download and upload speed should keep same unit
        if(NowIn >= m_preNetIn && NowOut >= m_preNetOut)
        {
            double downloadSpeed = ((double)((unsigned long)NowIn - m_preNetIn)) / coeffcient;
            double uploadSpeed = ((double)((unsigned long)NowOut - m_preNetOut)) / coeffcient;
            QString speedInfo = getSpeedInfo(downloadSpeed, uploadSpeed);
            QStringList speedlist = speedInfo.split("|");
            m_Upload = speedlist.at(0);
            m_Download = speedlist.at(1);
            bRet = true;
        }
    }

    m_preTime = nowTime;
    m_preNetOut = NowOut;
    m_preNetIn = NowIn;
#elif defined Q_OS_LINUX
    QFile file("/proc/net/dev");
    if(file.open(QIODevice::ReadOnly))
    {
        int nowTime = QDateTime().currentDateTime().toString("zzz").toInt();
        long long NowIn = 0;
        long long NowOut = 0;
        QByteArray array = file.readLine();
        array = file.readLine();
        array = file.readLine();
        while(array.count() > 0)
        {
            QTextStream s(array);
            QString s1,s2,s3,s4,s5,s6,s7,s8,s9,s10;
            s>>s1>>s2>>s3>>s4>>s5>>s6>>s7>>s8>>s9>>s10;
            NowIn += s2.toLong();
            NowOut += s10.toLong();
            array = file.readLine();
        }

        if(0 != m_preNetOut && 0 != m_preNetIn)
        {
            double coeffcient = (double)(1000 + nowTime - m_preTime) / 1000;
            //download and upload speed should keep same unit
            QStringList speedlist = getSpeedInfo(((double)(NowIn - m_preNetIn)) / coeffcient, ((double)(NowOut - m_preNetOut)) / coeffcient).split("|");
            m_Upload = speedlist.at(0);
            m_Download = speedlist.at(1);
            bRet = true;
        }
        m_preTime = nowTime;
        m_preNetOut = NowOut;
        m_preNetIn = NowIn;
        file.close();
    }
#elif defined Q_OS_OSX
    m_Upload = thread->getUpload();
    m_Download = thread->getDownload();
    bRet = true;
#endif

    return bRet;
}

void MainWidget::changeColor_slot(QAction *action)
{
    if(0 == action->text().compare("Green"))
    {
        m_Color = QColor(Qt::green);
    }
    else if(0 == action->text().compare("Gray"))
    {
        m_Color = QColor(Qt::gray);
    }
    else if(0 == action->text().compare("Blue"))
    {
        m_Color = QColor(Qt::blue);
    }
    else
    {
        QPalette palette = QPalette(m_Color);
        QColor selectcolor = QColorDialog::getColor(palette.color(QPalette::Button),this);
        if (false == selectcolor.isValid())
        {
            return;
        }

        m_Color = selectcolor;
    }

    QString FileName = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings *pConfig = new QSettings(FileName, QSettings::IniFormat);
    pConfig->setValue("Basic/Color", QString("%1 %2 %3").arg(m_Color.red()).arg(m_Color.green()).arg(m_Color.blue()));
    delete pConfig;

    QPalette pa;
    pa.setColor(QPalette::WindowText, m_Color);
    CpuRate_Label->setPalette(pa);
    RamRate_Label->setPalette(pa);
    uploadSpeed_Label->setPalette(pa);
    downloadSpeed_Label->setPalette(pa);

    repaint();
}
