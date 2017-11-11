#include "mainwidget.h"
#include <QFile>
#include <QLibrary>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QDataStream>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    m_dpi = qApp->primaryScreen()->logicalDotsPerInchX() / 120.0;

    QString FileName = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings *pConfig = new QSettings(FileName, QSettings::IniFormat);
    m_Color = getColorFromArray(pConfig->value("Basic/Color").toByteArray());
    m_displayC = pConfig->value("Basic/DisplayC").toInt();
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
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout_slot()));
    m_scanTimer = new QTimer(this);
    connect(m_scanTimer, SIGNAL(timeout()), this, SLOT(scanTimeout_slot()));
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    this->setAttribute(Qt::WA_TranslucentBackground, true);

    m_QuitAction = new QAction("Quit",this);
    m_AboutAction = new QAction("About",this);
    m_ColorMenu = new QMenu("Color", this);
    m_ColorMenu->addAction("Green");
    m_ColorMenu->addAction("Gray");
    m_ColorMenu->addAction("Blue");
    m_ColorMenu->addAction("Custom");
    m_DisplayMenu = new QMenu("Display", this);
    m_DisplayMenu->addAction("80%");
    m_DisplayMenu->addAction("100%");
    m_DisplayMenu->addAction("150%");
    m_DisplayMenu->addAction("200%");
    m_Menu = new QMenu(this);
    m_Menu->addMenu(m_DisplayMenu);
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
    connect(m_DisplayMenu,SIGNAL(triggered(QAction*)),this, SLOT(changeDisplay_slot(QAction*)));

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
    QScreen *pscreen = qApp->primaryScreen();
    QSize screenSize = pscreen->size();
    //if screen change,to confirm widget will fit new screen
    if(m_preScreenSize != screenSize)
    {
        move(screenSize.width() * m_rx / 1000, screenSize.height() * m_ry / 1000);
        m_dpi = pscreen->physicalDotsPerInch() * pscreen->physicalSize().width() * m_displayC / 5316000;
        QFont font  = qApp->font();
        font.setPixelSize(20 * m_dpi);
        qApp->setFont(font);
        m_preScreenSize = screenSize;
    }

    //use mouse's position to judge if it is on the widget
    QPoint mouse = cursor().pos();
    bool bOnWidget = false;
    if(mouse.x() >= this->x() && mouse.x() <= this->x() + this->width()
       && mouse.y() >= this->y() && mouse.y() <= this->y() + this->height())
    {
        bOnWidget = true;
    }

    if(m_hide && false == bOnWidget)
    {
        if(this->x() != (screenSize.width() - 10 * m_dpi) * 1000 / screenSize.width())
        {
            move(screenSize.width() * m_rx / 1000, screenSize.height() * m_ry / 1000);
        }
        setFixedSize(10 * m_dpi, 100 * m_dpi);
        QPainter painter(this);
        painter.setPen(QPen(m_Color, m_dpi));
        painter.setBrush(QBrush(Qt::white));
        painter.drawRect(0, 99 * m_dpi, 9 * m_dpi, -99 * m_dpi);
        painter.setBrush(QBrush(m_Color));
        painter.drawRect(0, 99 * m_dpi, 9 * m_dpi, - (m_MemeoryRate * m_dpi));
    }
    else
    {
        //show complete information when mouse on it
        if(m_hide && bOnWidget)
        {
            if(m_rx > 500)
            {
                move(screenSize.width() - 220 * m_dpi, screenSize.height() * m_ry / 1000);
            }
            else
            {
                move(0, screenSize.height() * m_ry / 1000);
            }
        }

        setFixedSize(220 * m_dpi, 110 * m_dpi);
        //draw a radar
        QPainter painter_horizon(this);
        painter_horizon.setPen(QPen(m_Color, m_dpi));
        QConicalGradient conicalGradient(52 * m_dpi,52 * m_dpi,180.0 - m_Angle);
        conicalGradient.setColorAt(0, m_Color);
        conicalGradient.setColorAt(1.0, QColor(255,255,255,0));
        painter_horizon.setBrush(QBrush(conicalGradient));
        painter_horizon.drawEllipse(2 * m_dpi,2 * m_dpi,100 * m_dpi,100 * m_dpi);

        QPainter painter(this);
        painter.setPen(QPen(m_Color, m_dpi));
        painter.drawLine(2 * m_dpi, 52 * m_dpi, 102 * m_dpi, 52 * m_dpi);
        painter.drawLine(52 * m_dpi, 2 * m_dpi, 52 * m_dpi, 102 * m_dpi);
        painter.drawEllipse(22 * m_dpi, 22 * m_dpi, 60 * m_dpi, 60 * m_dpi);

        //draw the line from radar to data
        QPoint p1;
        QPoint p2;
        QPoint p3;
        if(m_Angle >= 120 && m_Angle < 240)
        {
            p1 = QPoint(60 * m_dpi, 32.7 * m_dpi);
            p2 = QPoint(72.7 * m_dpi, 20 * m_dpi);
            p3 = QPoint(105 * m_dpi, 20 * m_dpi);
            painter.drawLine(p1, p2);
            painter.drawLine(p2, p3);
        }

        if(m_Angle >= 150 && m_Angle < 270)
        {
            p1 = QPoint(57.6 * m_dpi, 45 * m_dpi);
            p2 = QPoint(62.6 * m_dpi, 40 * m_dpi);
            p3 = QPoint(105 * m_dpi, 40 * m_dpi);
            painter.drawLine(p1, p2);
            painter.drawLine(p2, p3);
        }

        if(m_Angle >= 210 && m_Angle < 330)
        {
            p1 = QPoint(57.6 * m_dpi, 55 * m_dpi);
            p2 = QPoint(62.6 * m_dpi, 60 * m_dpi);
            p3 = QPoint(105 * m_dpi, 60 * m_dpi);
            painter.drawLine(p1, p2);
            painter.drawLine(p2, p3);
        }

        if(m_Angle >= 240 && m_Angle < 360)
        {
            p1 = QPoint(60 * m_dpi, 67.3 * m_dpi);
            p2 = QPoint(72.7 * m_dpi, 80 * m_dpi);
            p3 = QPoint(105 * m_dpi, 80 * m_dpi);
            painter.drawLine(p1, p2);
            painter.drawLine(p2, p3);
        }
    }

    layoutInit();
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
    if (event->buttons() & Qt::LeftButton)
    {
        move(event->globalPos() - m_dragPosition);
        event->accept();
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
    if(now.x() + 100 * m_dpi > screenSize.width())
    {
        m_hide = true;
        m_rx = (screenSize.width() - 10 * m_dpi) * 1000 / screenSize.width();
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

    if(now.y() + 100 * m_dpi > screenSize.height())
    {
        m_ry = 900;
    }
    else if(now.x()  + 100 * m_dpi < 0)
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
    QWidget::mouseReleaseEvent(event);
}

void MainWidget::contextMenuEvent(QContextMenuEvent *event)
{
    m_Menu->exec(QCursor::pos());
    QWidget::contextMenuEvent(event);
}

void MainWidget::layoutInit(void)
{
    CpuRate_Label->move(110 * m_dpi, 10 * m_dpi);
    RamRate_Label->move(110 * m_dpi, 30 * m_dpi);
    uploadSpeed_Label->move(110 * m_dpi, 50 * m_dpi);
    downloadSpeed_Label->move(110 * m_dpi, 70 * m_dpi);
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

    //refresh widget
    update();
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
#endif
    return bRet;
}

bool MainWidget::getNetworkSpeed(void)
{
    bool bRet = false;
#ifdef Q_OS_WIN32
    GetIfTable func_GetIfTable;
    QLibrary lib;
    //usually,iphlpapi.dll has existed in windows
    lib.setFileName("iphlpapi.dll");
    lib.load();
    func_GetIfTable = (GetIfTable)lib.resolve("GetIfTable");

    if(NULL != func_GetIfTable)
    {
        PMIB_IFTABLE m_pTable = NULL;
        DWORD m_dwAdapters = 0;
        //first call is just get the m_dwAdapters's value
        //more detail,pls see https://msdn.microsoft.com/en-us/library/windows/desktop/aa365943(v=vs.85).aspx

        func_GetIfTable(m_pTable, &m_dwAdapters, FALSE);

        m_pTable = (PMIB_IFTABLE)new BYTE[m_dwAdapters];
        //speed = sum / time,so it should record the time
        int nowTime = QDateTime().currentDateTime().toString("zzz").toInt();
        func_GetIfTable(m_pTable, &m_dwAdapters, FALSE);
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
               && false == Desc.contains("QoS"))
            {
                NowIn = NowIn + Row.dwInOctets;
                NowOut = NowOut + Row.dwOutOctets;
            }
        }
        delete []m_pTable;

        if(0 != m_preNetOut && 0 != m_preNetIn)
        {
            double coeffcient = (double)(1000 + nowTime - m_preTime) / 1000;
            //download and upload speed should keep same unit
            if(NowIn >= m_preNetIn && NowOut >= m_preNetOut)
            {
                QStringList speedlist = getSpeedInfo(((double)(NowIn - m_preNetIn)) / coeffcient, ((double)(NowOut - m_preNetOut)) / coeffcient).split("|");
                m_Upload = speedlist.at(0);
                m_Download = speedlist.at(1);
                bRet = true;
            }
        }

        m_preTime = nowTime;
        m_preNetOut = NowOut;
        m_preNetIn = NowIn;
    }
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
        QColor selectcolor = QColorDialog::getColor(palette.color(QPalette::Button),this,QString(),0);
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

void MainWidget::changeDisplay_slot(QAction* action)
{
    m_displayC = action->text().left(action->text().length() - 1).toInt();

    QString FileName = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings *pConfig = new QSettings(FileName, QSettings::IniFormat);
    pConfig->setValue("Basic/DisplayC", m_displayC);
    delete pConfig;

    QScreen *pscreen = qApp->primaryScreen();
    m_dpi = pscreen->physicalDotsPerInch() * pscreen->physicalSize().width() * m_displayC / 5316000;
    QFont font  = qApp->font();
    font.setPixelSize(20 * m_dpi);
    qApp->setFont(font);
}
