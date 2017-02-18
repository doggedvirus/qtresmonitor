#include "cpurate.h"
#include <QWaitCondition>
#include <QMutex>
CPURate::CPURate(QObject* parent)
    :QThread(parent)
{
    m_rate = 0;
}

void CPURate::run(void)
{
    FILETIME preIdleTime;
    FILETIME preKernelTime;
    FILETIME preUserTime;

    if(false == GetSystemTimes(&preIdleTime, &preKernelTime, &preUserTime))
    {
        return;
    }

    //等待500毫秒
    QMutex mutex;
    QWaitCondition sleep;
    mutex.lock();
    sleep.wait(&mutex, 500);
    mutex.unlock();

    FILETIME idleTime;
    FILETIME kernelTime;
    FILETIME userTime;
    if(false == GetSystemTimes(&idleTime,&kernelTime,&userTime))
    {
        return;
    }

    int idle = delOfInt64(preIdleTime, idleTime);
    int kernel = delOfInt64(preKernelTime,kernelTime);
    int user = delOfInt64(preUserTime,userTime);

    m_rate = (kernel + user - idle) * 100 / (kernel + user);
    return;
}

int CPURate::delOfInt64(FILETIME pre, FILETIME now)
{
    __int64 a = (__int64)(pre.dwHighDateTime) << 32 | (__int64)pre.dwLowDateTime ;
    __int64 b = (__int64)(now.dwHighDateTime) << 32 | (__int64)now.dwLowDateTime ;

    return (b - a);
}

int CPURate::getRate(void)
{
    return m_rate;
}
