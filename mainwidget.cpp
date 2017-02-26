#include "mainwidget.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    CpuRate_Label = new QLabel(this);
    RamRate_Label = new QLabel(this);
    Speed_Label = new QLabel(this);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout_slot()));
    this->setMinimumSize(200, 200);
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

    m_timer->start(1000);
}

MainWidget::~MainWidget()
{
    m_timer->stop();
}

void MainWidget::paintEvent(QPaintEvent *event)
{
    layoutInit();
    QWidget::paintEvent(event);
}

void MainWidget::layoutInit(void)
{
    CpuRate_Label->move(this->width() / 2 - CpuRate_Label->width() / 2, 20);
    RamRate_Label->move(this->width() / 2 - CpuRate_Label->width() / 2, CpuRate_Label->y() + CpuRate_Label->height() + 10);
    Speed_Label->move(this->width() / 2 - Speed_Label->width() / 2, RamRate_Label->y() + RamRate_Label->height() + 10);
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

        RamRate_Label->setText(QString("RAM rate:%1\%").arg((nMemTotal - nMemFree) * 100 / nMemTotal));
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

            int rate = (kernel + user - idle) * 100 / (kernel + user);
            CpuRate_Label->setText(QString("CPU rate:%1\%").arg(rate));
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
        Speed_Label->setText(getSpeedInfo((int)(NowIn - m_preNetIn) / coeffcient, (int)(NowOut - m_preNetOut) / coeffcient));
        Speed_Label->adjustSize();
    }
    m_preTime = nowTime;
    m_preNetOut = NowOut;
    m_preNetIn = NowIn;

}

//There is an interesting test of this function:
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

    return QString("download:%1%2\nupload:%3%2").arg(uploadSpeed).arg(speedString).arg(downloadSpeed);
}

int MainWidget::delOfInt64(FILETIME subtrahend, FILETIME minuend)
{
    __int64 a = (__int64)(subtrahend.dwHighDateTime) << 32 | (__int64)subtrahend.dwLowDateTime ;
    __int64 b = (__int64)(minuend.dwHighDateTime) << 32 | (__int64)minuend.dwLowDateTime ;

    int answer = 0;
    if(b > a)
    {
        answer = b - a;
    }
    else
    {
        answer = 0xFFFFFFFFFFFFFFFF - a + b;
    }
    return answer;
}
