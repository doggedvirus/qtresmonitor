#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtWidgets>
#include "thread/cpurate.h"
#include "thread/ramrate.h"

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = 0);
    ~MainWidget();
public slots:
    void timeout_slot(void);
    void updateCPURate(void);
    void updateRAMRate(void);
    void updateCpuThreadStatus(void);
    void updateRamThreadStatus(void);
protected:
    void paintEvent(QPaintEvent *event);
private:
    QLabel* CpuRate_Label;
    QLabel* RamRate_Label;

    void layoutInit(void);

    CPURate* m_CpurateThread;
    bool m_CpuThreadDestroy;
    RAMRate* m_RamrateThread;
    bool m_RamThreadDestroy;
    QTimer* m_timer;


};

#endif // MAINWIDGET_H
