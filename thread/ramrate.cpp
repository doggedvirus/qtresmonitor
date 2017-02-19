#include "ramrate.h"
RAMRate::RAMRate(QObject* parent)
    :QThread(parent)
{
    m_rate = 0;
}

void RAMRate::run(void)
{
    int nMemTotal = 0;
    int nMemUsed = 0;
    GetSysMemory(nMemTotal,nMemUsed);
    m_rate = nMemUsed * 100 / nMemTotal;
    return;
}

bool RAMRate::GetSysMemory(int& nMemTotal,int& nMemUsed)
{
    MEMORYSTATUSEX memsStat;
    memsStat.dwLength = sizeof(memsStat);
    if(!GlobalMemoryStatusEx(&memsStat))//如果获取系统内存信息不成功，就直接返回
    {
        nMemTotal = -1;
        nMemUsed  = -1;
        return false;
    }
    int nMemFree = memsStat.ullAvailPhys/( 1024.0*1024.0 );
    nMemTotal = memsStat.ullTotalPhys/( 1024.0*1024.0 );
    nMemUsed= nMemTotal- nMemFree;
    return true;
}

int RAMRate::getRate(void)
{
    return m_rate;
}
