#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtWidgets>
#include <QLibrary>

#ifdef Q_OS_WIN32
//to use dll directory,define then by self
#define MAX_INTERFACE_NAME_LEN 256
#define MAXLEN_PHYSADDR 8
#define MAXLEN_IFDESCR 256
#define ANY_SIZE 1

typedef struct _MIB_IFROW
{
    WCHAR wszName[MAX_INTERFACE_NAME_LEN];
    DWORD dwIndex;
    DWORD dwType;
    DWORD dwMtu;
    DWORD dwSpeed;
    DWORD dwPhysAddrLen;
    BYTE bPhysAddr[MAXLEN_PHYSADDR];
    DWORD dwAdminStatus;
    DWORD dwOperStatus;
    DWORD dwLastChange;
    DWORD dwInOctets;
    DWORD dwInUcastPkts;
    DWORD dwInNUcastPkts;
    DWORD dwInDiscards;
    DWORD dwInErrors;
    DWORD dwInUnknownProtos;
    DWORD dwOutOctets;
    DWORD dwOutUcastPkts;
    DWORD dwOutNUcastPkts;
    DWORD dwOutDiscards;
    DWORD dwOutErrors;
    DWORD dwOutQLen;
    DWORD dwDescrLen;
    BYTE bDescr[MAXLEN_IFDESCR];
} MIB_IFROW,*PMIB_IFROW;

typedef struct _MIB_IFTABLE
{
    DWORD dwNumEntries;
    MIB_IFROW table[ANY_SIZE];
} MIB_IFTABLE, *PMIB_IFTABLE;

typedef DWORD (*GetIfTable)(PMIB_IFTABLE, PULONG, BOOL);
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
    void period1s_slot(void);
    void period2s_slot(void);
    void period3s_slot(void);
    void period4s_slot(void);
protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
private:
    QLabel* CpuRate_Label;
    QLabel* RamRate_Label;
    QLabel* uploadSpeed_Label;
    QLabel* downloadSpeed_Label;

    QSystemTrayIcon* m_TrayIcon;
    QAction* m_QuitAction;
    QAction* m_AboutAction;
    QMenu* m_PeriodAction;
    QActionGroup* Perion_ActionGroup;
    QAction* m_PeriodAction_1;
    QAction* m_PeriodAction_2;
    QAction* m_PeriodAction_3;
    QAction* m_PeriodAction_4;
    QMenu* m_Menu;

    void layoutInit(void);
    QString getSpeedInfo(double downloadSpeed, double uploadSpeed);

    bool getRamRate(void);
    bool getCpuRate(void);
    bool getNetworkSpeed(void);

    QTimer* m_timer;
    QTimer* m_scanTimer;

    int m_preTime;
#ifdef Q_OS_WIN32
    int delOfInt64(FILETIME subtrahend, FILETIME minuend);
    GetIfTable m_funcGetIfTable;
    QLibrary m_lib;
    DWORD m_preNetIn;
    DWORD m_preNetOut;
    FILETIME m_preIdleTime;
    FILETIME m_preKernelTime;
    FILETIME m_preUserTime;
#elif defined Q_OS_LINUX
    long long m_preIdleTime;
    long long m_preAllTime;
    long long m_preNetIn;
    long long m_preNetOut;
#endif

    uint m_MemeoryRate;
    uint m_CpuRate;
    QString m_Upload;
    QString m_Download;

    float m_dpi;
    QPoint m_dragPosition;
    double m_Angle;
    int m_Period;
};

#endif // MAINWIDGET_H
