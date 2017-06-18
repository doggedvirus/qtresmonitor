#include "mainwidget.h"
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QStringList>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    m_dpi = qApp->primaryScreen()->logicalDotsPerInchX() / 120.0;

    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::green);

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
    m_PeriodAction = new QMenu("Period",this);
    m_PeriodAction_1 = new QAction("1 Second",this);
    m_PeriodAction_1->setCheckable(true);
    m_PeriodAction_2 = new QAction("2 Seconds",this);
    m_PeriodAction_2->setCheckable(true);
    m_PeriodAction_3 = new QAction("3 Seconds",this);
    m_PeriodAction_3->setCheckable(true);
    m_PeriodAction_4 = new QAction("4 Seconds",this);
    m_PeriodAction_4->setCheckable(true);
    Perion_ActionGroup = new QActionGroup(this);
    Perion_ActionGroup->addAction(m_PeriodAction_1);
    Perion_ActionGroup->addAction(m_PeriodAction_2);
    Perion_ActionGroup->addAction(m_PeriodAction_3);
    Perion_ActionGroup->addAction(m_PeriodAction_4);
    m_PeriodAction_1->setChecked(true);
    m_PeriodAction->addAction(m_PeriodAction_1);
    m_PeriodAction->addAction(m_PeriodAction_2);
    m_PeriodAction->addAction(m_PeriodAction_3);
    m_PeriodAction->addAction(m_PeriodAction_4);
    m_Menu = new QMenu(this);
    m_Menu->addMenu(m_PeriodAction);
    m_Menu->addAction(m_AboutAction);
    m_Menu->addAction(m_QuitAction);

    m_TrayIcon = new QSystemTrayIcon(this);
    m_TrayIcon->setIcon(QIcon(":/monitor.png"));
    m_TrayIcon->setContextMenu(m_Menu);
    m_TrayIcon->show();

    connect(m_QuitAction,SIGNAL(triggered()),this, SLOT(quitApp_slot()));
    connect(m_AboutAction,SIGNAL(triggered()),this, SLOT(about_slot()));
    connect(m_PeriodAction_1,SIGNAL(triggered()),this, SLOT(period1s_slot()));
    connect(m_PeriodAction_2,SIGNAL(triggered()),this, SLOT(period2s_slot()));
    connect(m_PeriodAction_3,SIGNAL(triggered()),this, SLOT(period3s_slot()));
    connect(m_PeriodAction_4,SIGNAL(triggered()),this, SLOT(period4s_slot()));

    this->setFixedSize(220 * m_dpi, 105 * m_dpi);

    layoutInit();

#ifdef Q_OS_WIN32
    //usually,iphlpapi.dll has existed in windows
    m_lib.setFileName("iphlpapi.dll");
    m_lib.load();
    m_funcGetIfTable = (GetIfTable)m_lib.resolve("GetIfTable");

    m_preNetIn = 0;
    m_preNetOut = 0;
    memset(&m_preIdleTime, 0, sizeof(FILETIME));
    memset(&m_preKernelTime, 0, sizeof(FILETIME));
    memset(&m_preUserTime, 0, sizeof(FILETIME));
 #endif

    m_MemeoryRate = 0;
    m_CpuRate = 0;

    m_Angle = 0;
    m_Period = 1000;

    m_timer->start(m_Period);
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
    QMessageBox::information(this, "About", QString("Version:0.0.1")
                                            + "<br/>Github:<a href=\"https://github.com/doggedvirus/qtresmonitor\">https://github.com/doggedvirus/qtresmonitor</a>"
                                            + "<br/>Author:<a href=\"https://doggedvirus.com/about\">https://doggedvirus.com/about</a>"
                                            + "<br/>Icon Designer:<a href=\"http://weibo.com/foreverdrawing\">http://weibo.com/foreverdrawing</a>");
}

void MainWidget::period1s_slot(void)
{
    m_PeriodAction_1->setChecked(true);
    m_Period = 1000;
    m_timer->stop();
    m_timer->start(m_Period);
}

void MainWidget::period2s_slot(void)
{
    m_PeriodAction_2->setChecked(true);
    m_Period = 2000;
    m_timer->stop();
    m_timer->start(m_Period);
}

void MainWidget::period3s_slot(void)
{
    m_PeriodAction_3->setChecked(true);
    m_Period = 3000;
    m_timer->stop();
    m_timer->start(m_Period);
}

void MainWidget::period4s_slot(void)
{
    m_PeriodAction_4->setChecked(true);
    m_Period = 4000;
    m_timer->stop();
    m_timer->start(m_Period);
}

void MainWidget::paintEvent(QPaintEvent *event)
{
    //draw a radar
    QPainter painter_horizon(this);
    painter_horizon.setPen(QPen(Qt::green));
    QConicalGradient conicalGradient(50 * m_dpi,50 * m_dpi,180.0 - m_Angle);
    conicalGradient.setColorAt(0,Qt::green);
    conicalGradient.setColorAt(1.0,QColor(255,255,255,0));
    painter_horizon.setBrush(QBrush(conicalGradient));
    painter_horizon.drawEllipse(0 * m_dpi,0 * m_dpi,100 * m_dpi,100 * m_dpi);

    QPainter painter(this);
    painter.setPen(QPen(Qt::green));
    painter.drawLine(0,50 * m_dpi,100 * m_dpi,50 * m_dpi);
    painter.drawLine(50 * m_dpi,0,50 * m_dpi,100 * m_dpi);
    painter.drawEllipse(20 * m_dpi,20 * m_dpi,60 * m_dpi,60 * m_dpi);

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

    layoutInit();
    QWidget::paintEvent(event);
}
void MainWidget::mousePressEvent(QMouseEvent *event)
{
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

void MainWidget::scanTimeout_slot(void)
{
    m_Angle += 18000.0 / m_Period;
    if(m_Angle >= 360)
    {
        m_Angle = 0;
    }
    repaint();
}

void MainWidget::timeout_slot(void)
{
    //memory
    if(getRamRate())
    {
        RamRate_Label->setText(QString("RAM %1\%").arg(m_MemeoryRate));
        RamRate_Label->adjustSize();
    }

//    MEMORYSTATUSEX memsStat;
//    memsStat.dwLength = sizeof(memsStat);
//    if(GlobalMemoryStatusEx(&memsStat))
//    {
//        int nMemFree = memsStat.ullAvailPhys / (1024 * 1024);
//        int nMemTotal = memsStat.ullTotalPhys / (1024 * 1024);

//        m_MemeoryRate = (nMemTotal - nMemFree) * 100 / nMemTotal;
//        RamRate_Label->setText(QString("RAM %1\%").arg(m_MemeoryRate));
//        RamRate_Label->adjustSize();
//    }

    if(getCpuRate())
    {
        CpuRate_Label->setText(QString("CPU %1\%").arg(m_CpuRate));
        CpuRate_Label->adjustSize();
    }

    //cpu
//    FILETIME IdleTime;
//    FILETIME KernelTime;
//    FILETIME UserTime;
//    if(GetSystemTimes(&IdleTime, &KernelTime, &UserTime))
//    {
//        if(0 != m_preIdleTime.dwHighDateTime &&  0 != m_preIdleTime.dwLowDateTime)
//        {
//            int idle = delOfInt64(IdleTime, m_preIdleTime);
//            int kernel = delOfInt64(KernelTime,m_preKernelTime);
//            int user = delOfInt64(UserTime,m_preUserTime);

//            //confirm rate > 0
//            m_CpuRate = (double)(kernel + user - idle) / (double)(kernel + user) * 100 / 1;
//            CpuRate_Label->setText(QString("CPU %1\%").arg(m_CpuRate));
//            CpuRate_Label->adjustSize();
//        }
//        m_preIdleTime = IdleTime;
//        m_preKernelTime = KernelTime;
//        m_preUserTime = UserTime;
//    }

    if(getNetworkSpeed())
    {
        uploadSpeed_Label->setText("↑ " + m_Upload);
        uploadSpeed_Label->adjustSize();
        downloadSpeed_Label->setText("↓ " + m_Download);
        downloadSpeed_Label->adjustSize();
    }

    //network
//    PMIB_IFTABLE m_pTable = NULL;
//    DWORD m_dwAdapters = 0;
//    //first call is just get the m_dwAdapters's value
//    //more detail,pls see https://msdn.microsoft.com/en-us/library/windows/desktop/aa365943(v=vs.85).aspx
//    m_funcGetIfTable(m_pTable, &m_dwAdapters, FALSE);

//    m_pTable = (PMIB_IFTABLE)new BYTE[m_dwAdapters];
//    //speed = sum / time,so it should record the time
//    int nowTime = QDateTime().currentDateTime().toString("zzz").toInt();
//    m_funcGetIfTable(m_pTable, &m_dwAdapters, FALSE);
//    DWORD NowIn = 0;
//    DWORD NowOut = 0;
//    QList<int> typeList;
//    for (UINT i = 0; i < m_pTable->dwNumEntries; i++)
//    {
//        MIB_IFROW Row = m_pTable->table[i];
//        //1 type should only be count only once
//        bool bExist = false;
//        for(int j = 0;j < typeList.count();j++)
//        {
//            if(typeList.at(j) == (int)Row.dwType)
//            {
//                bExist = true;
//                break;
//            }
//        }

//        if(false == bExist
//           && (Row.dwInOctets != 0 || Row.dwOutOctets != 0))
//        {
//            typeList.append(Row.dwType);
//            NowIn += Row.dwInOctets;
//            NowOut += Row.dwOutOctets;
//        }
//    }
//    delete []m_pTable;

//    if(0 != m_preNetOut && 0 != m_preNetIn)
//    {
//        double coeffcient = (double)(m_Period + nowTime - m_preTime) / 1000;
//        //download and upload speed should keep same unit
//        QStringList speedlist = getSpeedInfo((int)(NowIn - m_preNetIn) / coeffcient, (int)(NowOut - m_preNetOut) / coeffcient).split("|");
//        m_Upload = speedlist.at(0);
//        m_Download = speedlist.at(1);
//        uploadSpeed_Label->setText("↑ " + m_Upload);
//        uploadSpeed_Label->adjustSize();
//        downloadSpeed_Label->setText("↓ " + m_Download);
//        downloadSpeed_Label->adjustSize();
//    }
//    m_preTime = nowTime;
//    m_preNetOut = NowOut;
//    m_preNetIn = NowIn;

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

#ifdef Q_OS_WIN32
int MainWidget::delOfInt64(FILETIME subtrahend, FILETIME minuend)
{
    __int64 a = (__int64)(subtrahend.dwHighDateTime) << 32 | (__int64)subtrahend.dwLowDateTime ;
    __int64 b = (__int64)(minuend.dwHighDateTime) << 32 | (__int64)minuend.dwLowDateTime ;

    uint answer = 0;
    if(a > b)
    {
        answer = a - b;
    }
    else
    {
        answer = 0xFFFFFFFFFFFFFFFF - a + b;
    }
    return answer;
}
#endif

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
    FILETIME IdleTime;
    FILETIME KernelTime;
    FILETIME UserTime;
    bRet = GetSystemTimes(&IdleTime, &KernelTime, &UserTime);
    if(bRet)
    {
        if(0 != m_preIdleTime.dwHighDateTime &&  0 != m_preIdleTime.dwLowDateTime)
        {
            int idle = delOfInt64(IdleTime, m_preIdleTime);
            int kernel = delOfInt64(KernelTime,m_preKernelTime);
            int user = delOfInt64(UserTime,m_preUserTime);

            //confirm rate > 0
            m_CpuRate = (double)(kernel + user - idle) / (double)(kernel + user) * 100 / 1;
        }
        m_preIdleTime = IdleTime;
        m_preKernelTime = KernelTime;
        m_preUserTime = UserTime;
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
    PMIB_IFTABLE m_pTable = NULL;
    DWORD m_dwAdapters = 0;
    //first call is just get the m_dwAdapters's value
    //more detail,pls see https://msdn.microsoft.com/en-us/library/windows/desktop/aa365943(v=vs.85).aspx

    m_funcGetIfTable(m_pTable, &m_dwAdapters, FALSE);

    m_pTable = (PMIB_IFTABLE)new BYTE[m_dwAdapters];
    //speed = sum / time,so it should record the time
    int nowTime = QDateTime().currentDateTime().toString("zzz").toInt();
    m_funcGetIfTable(m_pTable, &m_dwAdapters, FALSE);
    DWORD NowIn = 0;
    DWORD NowOut = 0;
    QList<int> typeList;
    for (UINT i = 0; i < m_pTable->dwNumEntries; i++)
    {
        MIB_IFROW Row = m_pTable->table[i];
        //1 type should only be count only once
        bool bExist = false;
        for(int j = 0;j < typeList.count();j++)
        {
            if(typeList.at(j) == (int)Row.dwType)
            {
                bExist = true;
                break;
            }
        }

        if(false == bExist
           && (Row.dwInOctets != 0 || Row.dwOutOctets != 0))
        {
            typeList.append(Row.dwType);
            NowIn += Row.dwInOctets;
            NowOut += Row.dwOutOctets;
        }
    }
    delete []m_pTable;

    if(0 != m_preNetOut && 0 != m_preNetIn)
    {
        double coeffcient = (double)(m_Period + nowTime - m_preTime) / 1000;
        //download and upload speed should keep same unit
        QStringList speedlist = getSpeedInfo(((double)(NowIn - m_preNetIn)) / coeffcient, ((double)(NowOut - m_preNetOut)) / coeffcient).split("|");
        m_Upload = speedlist.at(0);
        m_Download = speedlist.at(1);
        bRet = true;
    }
    m_preTime = nowTime;
    m_preNetOut = NowOut;
    m_preNetIn = NowIn;
#endif
    return bRet;
}
