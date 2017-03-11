#include "mainwidget.h"

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
    m_Menu = new QMenu((QWidget*)QApplication::desktop());    
    m_Menu->addAction(m_AboutAction);
    m_Menu->addAction(m_QuitAction);

    m_TrayIcon = new QSystemTrayIcon(this);
    m_TrayIcon->setIcon(QIcon(":/monitor.png"));
    m_TrayIcon->setContextMenu(m_Menu);
    m_TrayIcon->show();

    this->addAction(m_AboutAction);
    this->addAction(m_QuitAction);
    this->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(m_QuitAction,SIGNAL(triggered()),this, SLOT(quitApp_slot()));
    connect(m_AboutAction,SIGNAL(triggered()),this, SLOT(about_slot()));

    this->setFixedSize(220 * m_dpi, 105 * m_dpi);

    layoutInit();

    //usually,iphlpapi.dll has existed in windows
    m_lib.setFileName("iphlpapi.dll");
    m_lib.load();
    m_funcGetIfTable = (GetIfTable)m_lib.resolve("GetIfTable");

    m_preNetIn = 0;
    m_preNetOut = 0;
    memset(&m_preIdleTime, 0, sizeof(FILETIME));
    memset(&m_preKernelTime, 0, sizeof(FILETIME));
    memset(&m_preUserTime, 0, sizeof(FILETIME));

    m_MemeoryRate = 0;
    m_CpuRate = 0;

    m_Angle = 0;
    m_Period = 2000;

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
                                            + "<br/>Author:<a href=\"https://doggedvirus.com/about\">https://doggedvirus.com/about</a>"
                                            + "<br/>Github:<a href=\"https://github.com/doggedvirus/qtresmonitor\">https://github.com/doggedvirus/qtresmonitor</a>");
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
    MEMORYSTATUSEX memsStat;
    memsStat.dwLength = sizeof(memsStat);
    if(GlobalMemoryStatusEx(&memsStat))
    {
        int nMemFree = memsStat.ullAvailPhys / (1024 * 1024);
        int nMemTotal = memsStat.ullTotalPhys / (1024 * 1024);

        m_MemeoryRate = (nMemTotal - nMemFree) * 100 / nMemTotal;
        RamRate_Label->setText(QString("RAM %1\%").arg(m_MemeoryRate));
        RamRate_Label->adjustSize();
    }

    //cpu
    FILETIME IdleTime;
    FILETIME KernelTime;
    FILETIME UserTime;
    if(GetSystemTimes(&IdleTime, &KernelTime, &UserTime))
    {
        if(0 != m_preIdleTime.dwHighDateTime &&  0 != m_preIdleTime.dwLowDateTime)
        {
            int idle = delOfInt64(IdleTime, m_preIdleTime);
            int kernel = delOfInt64(KernelTime,m_preKernelTime);
            int user = delOfInt64(UserTime,m_preUserTime);

            //confirm rate > 0
            m_CpuRate = (double)(kernel + user - idle) / (double)(kernel + user) * 100 / 1;
            CpuRate_Label->setText(QString("CPU %1\%").arg(m_CpuRate));
            CpuRate_Label->adjustSize();
        }
        m_preIdleTime = IdleTime;
        m_preKernelTime = KernelTime;
        m_preUserTime = UserTime;
    }

    //network
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
        QStringList speedlist = getSpeedInfo((int)(NowIn - m_preNetIn) / coeffcient, (int)(NowOut - m_preNetOut) / coeffcient).split("|");
        QString uploadString = speedlist.at(0);
        QString downloadString = speedlist.at(1);
        uploadSpeed_Label->setText("↑ " + uploadString);
        uploadSpeed_Label->adjustSize();
        downloadSpeed_Label->setText("↓ " + downloadString);
        downloadSpeed_Label->adjustSize();
    }
    m_preTime = nowTime;
    m_preNetOut = NowOut;
    m_preNetIn = NowIn;

    //refresh widget
    update();
}

//There is an interesting test of this function:
//I tried change this 2 input parameters' types to double,
//then in release mode,crash;in debug mode, run correctly.
QString MainWidget::getSpeedInfo(int idownloadSpeed, int iuploadSpeed)
{
    QString speedString = "B/s";
    double downloadSpeed = idownloadSpeed;
    double uploadSpeed = iuploadSpeed;
    if(downloadSpeed >= 1024 || uploadSpeed >= 1024)
    {
        speedString = "KB/s";

        downloadSpeed = (double)(int((downloadSpeed / 1024.0) * 100)) / 100;
        uploadSpeed = (double)(int((uploadSpeed / 1024.0) * 100)) / 100;
        if(downloadSpeed >= 1024 || uploadSpeed >= 1024)
        {
            speedString = "MB/s";

            //retain 2 decimals
            downloadSpeed = (double)(int((downloadSpeed / 1024.0) * 100)) / 100;
            uploadSpeed = (double)(int((uploadSpeed / 1024.0) * 100)) / 100;

            if(downloadSpeed >= 1024 || uploadSpeed >= 1024)
            {
                speedString = "GB/s";

                //retain 2 decimals
                downloadSpeed = (double)(int((downloadSpeed / 1024.0) * 100)) / 100;
                uploadSpeed = (double)(int((uploadSpeed / 1024.0) * 100)) / 100;
            }
        }
    }

    return QString("%1%2|%3%2").arg(uploadSpeed).arg(speedString).arg(downloadSpeed);
}

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
