#include "mainwidget.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
{
    CpuRate_Label = new QLabel(this);
    RamRate_Label = new QLabel(this);
    m_timer = new QTimer(this);
    m_CpurateThread = NULL;
    m_RamrateThread = NULL;
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout_slot()));
    this->setMinimumSize(200, 200);
    layoutInit();
    timeout_slot();
    m_timer->start(1000);
}

MainWidget::~MainWidget()
{
    m_timer->stop();
    while(!m_CpuThreadDestroy || !m_RamThreadDestroy)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
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
}

void MainWidget::timeout_slot(void)
{
    m_CpurateThread = new CPURate(this);
    connect(m_CpurateThread, SIGNAL(finished()), this, SLOT(updateCPURate()));
    connect(m_CpurateThread, SIGNAL(destroyed(QObject*)), this, SLOT(updateCpuThreadStatus()));
    m_CpurateThread->start();
    m_CpuThreadDestroy = false;

    m_RamrateThread = new RAMRate(this);
    connect(m_RamrateThread, SIGNAL(finished()), this, SLOT(updateRAMRate()));
    connect(m_RamrateThread, SIGNAL(destroyed(QObject*)), this, SLOT(updateRamThreadStatus()));
    m_RamrateThread->start();
    m_RamThreadDestroy = false;
}

void MainWidget::updateCPURate(void)
{
    CpuRate_Label->setText(QString("CPU rate:%1\%").arg(m_CpurateThread->getRate()));
    CpuRate_Label->adjustSize();
    m_CpurateThread->deleteLater();
}

void MainWidget::updateRAMRate(void)
{
    RamRate_Label->setText(QString("RAM rate:%1\%").arg(m_RamrateThread->getRate()));
    RamRate_Label->adjustSize();
    m_RamrateThread->deleteLater();
}

void MainWidget::updateCpuThreadStatus(void)
{
    m_CpuThreadDestroy = true;
}

void MainWidget::updateRamThreadStatus(void)
{
    m_RamThreadDestroy = true;
}

