#include "mainwidget.h"
#include "widgetlib/ratepainter.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    m_dpi = qApp->primaryScreen()->logicalDotsPerInchX() / 120.0;

    CpuRate_Label = new QLabel(this);
    RamRate_Label = new QLabel(this);
    uploadSpeed_Label = new QLabel(this);
    downloadSpeed_Label = new QLabel(this);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout_slot()));
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    this->setAttribute(Qt::WA_TranslucentBackground, true);

    m_QuitAction = new QAction("Quit",this);
    m_Menu = new QMenu((QWidget*)QApplication::desktop());
    m_Menu->addAction(m_QuitAction);

    m_TrayIcon = new QSystemTrayIcon(this);
    m_TrayIcon->setIcon(QIcon(":/monitor.ico"));
    m_TrayIcon->setContextMenu(m_Menu);
    m_TrayIcon->show();
    connect(m_QuitAction,SIGNAL(triggered()),this, SLOT(quitApp_slot()));

    this->setFixedSize(160 * m_dpi, 160 * m_dpi);

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

    m_timer->start(1000);
}

MainWidget::~MainWidget()
{
    m_timer->stop();
}

void MainWidget::quitApp_slot(void)
{
    qApp->quit();
}

void MainWidget::paintEvent(QPaintEvent *event)
{
    RatePainter paint1(this);
    paint1.setColor(QColor(70, 70, 70));
    paint1.drawRate(paint1.getPenwidth(), paint1.getPenwidth(), this->width() - paint1.getPenwidth() * 2, this->width() - paint1.getPenwidth() * 2, m_CpuRate);
    paint1.setColor(QColor(0, 187, 158));
    paint1.drawRate(paint1.getPenwidth() * 2, paint1.getPenwidth() * 2, this->width() - paint1.getPenwidth() * 4, this->width() - paint1.getPenwidth() * 4, m_MemeoryRate);
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
    CpuRate_Label->move(this->width() * 0.37, this->height() - 25 * m_dpi);
    RamRate_Label->move(this->width() * 0.37, this->height() - 37 * m_dpi);
    uploadSpeed_Label->move(this->width() * 0.2, this->height() * 0.35);
    downloadSpeed_Label->move(this->width() * 0.2, this->height() * 0.55);
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

        if(false == bExist)
        {
            typeList.append(Row.dwType);
            NowIn += Row.dwInOctets;
            NowOut += Row.dwOutOctets;
        }
    }
    delete []m_pTable;

    if(0 != m_preNetOut && 0 != m_preNetIn)
    {
        double coeffcient = (double)(1000 + nowTime - m_preTime) / 1000;
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
