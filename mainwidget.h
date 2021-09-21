#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtWidgets>

struct CONFIG_S
{
    int PosX;
    int PosY;
    QColor Color;
    int Version;
};

#ifdef Q_OS_OSX
class TopThread : public QThread
{
    Q_OBJECT

public:
    explicit TopThread(QObject *parent = 0);
    ~TopThread();
    uint getMemoryRate(void);
    uint getCpuRate(void);
    QString getUpload(void);
    QString getDownload(void);
    void setInterval(int mSeconds);
    void setOver(bool over);
private:
    void run(void);

    int m_interval;
    uint m_MemeoryRate;
    uint m_CpuRate;
    QString m_Upload;
    QString m_Download;
    bool m_Over;
    long m_preIn;
    long m_preOut;
    int m_preTime;
};
#endif

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = 0);
    ~MainWidget();

public slots:
    void timeout_slot(void);
    void scanTimeout_slot(void);
    void quitApp_slot(void);
    void about_slot(void);
    void changeColor_slot(QAction *action);
protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
private:

    QLabel* CpuRate_Label;
    QLabel* RamRate_Label;
    QLabel* uploadSpeed_Label;
    QLabel* downloadSpeed_Label;

    QSystemTrayIcon* m_TrayIcon;
    QAction* m_QuitAction;
    QAction* m_AboutAction;
    QMenu* m_ColorMenu;
    QMenu* m_Menu;

    void layoutInit(void);
    QColor getColorFromArray(QByteArray array);
    QString getSpeedInfo(double downloadSpeed, double uploadSpeed);

    bool getRamRate(void);
    bool getCpuRate(void);
    bool getNetworkSpeed(void);

    QTimer* m_timer;
    QTimer* m_scanTimer;

    int m_preTime;
    unsigned long m_preNetIn;
    unsigned long m_preNetOut;
    long long m_preIdleTime;
    long long m_preAllTime;

    uint m_MemeoryRate;
    uint m_CpuRate;
    QString m_Upload;
    QString m_Download;

    QPoint m_dragPosition;
    int m_iPreAngleTime;
    double m_Angle;
    QSize m_preScreenSize;
    QColor m_Color;
    bool m_hide;
    int m_rx;
    int m_ry;
    qint64 mMoveTime = 0;

#ifdef Q_OS_OSX
    TopThread* thread;
#endif
};

#endif // MAINWIDGET_H
