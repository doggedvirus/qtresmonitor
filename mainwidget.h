#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtWidgets>
#include "thread/cpurate.h"

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = 0);
    ~MainWidget();
public slots:
    void timeout_slot(void);
    void updateCPURate(void);
protected:
    void paintEvent(QPaintEvent *event);
private:
    QLabel* CpuRate_Label;

    void layoutInit(void);

    CPURate* m_CpurateThread;
    QTimer* m_timer;


};

#endif // MAINWIDGET_H
