#ifndef RAMRATE_H
#define RAMRATE_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <windows.h>

class RAMRate : public QThread
{
public:
    RAMRate(QObject* parent);

    int getRate(void);
private:
    void run(void);

    bool GetSysMemory(int& nMemTotal,int& nMemUsed);

    int m_rate;
};

#endif // RAMRATE_H
