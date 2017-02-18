#ifndef CPURATE_H
#define CPURATE_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <windows.h>

class CPURate : public QThread
{
public:
    CPURate(QObject* parent);

    int getRate(void);
private:
    void run(void);
    int delOfInt64(FILETIME pre, FILETIME now);

    int m_rate;
};

#endif // CPURATE_H
