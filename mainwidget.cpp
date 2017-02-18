#include "mainwidget.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    CpuRate_Label = new QLabel(this);
    m_timer = new QTimer(this);
    m_CpurateThread = NULL;
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout_slot()));
    this->setMinimumSize(200, 200);
    layoutInit();
    timeout_slot();
    m_timer->start(1000);
}

MainWidget::~MainWidget()
{

}

void MainWidget::paintEvent(QPaintEvent *event)
{
    layoutInit();
    QWidget::paintEvent(event);
}

void MainWidget::layoutInit(void)
{
    CpuRate_Label->move(this->width() / 2 - CpuRate_Label->width() / 2, 20);
}

void MainWidget::timeout_slot(void)
{
    m_CpurateThread = new CPURate(this);
    connect(m_CpurateThread, SIGNAL(finished()), this, SLOT(updateCPURate()));
    m_CpurateThread->start();
}

void MainWidget::updateCPURate(void)
{
    CpuRate_Label->setText(QString("CPU rate:%1\%").arg(m_CpurateThread->getRate()));
    CpuRate_Label->adjustSize();
    m_CpurateThread->deleteLater();
    m_CpurateThread = NULL;
}


