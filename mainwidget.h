#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtWidgets>
#include <QLibrary>

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

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = 0);
    ~MainWidget();
public slots:
    void timeout_slot(void);
    void quitApp_slot(void);
protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
private:
    QLabel* CpuRate_Label;
    QLabel* RamRate_Label;
    QLabel* uploadSpeed_Label;
    QLabel* downloadSpeed_Label;

    QSystemTrayIcon* m_TrayIcon;
    QAction* m_QuitAction;
    QMenu* m_Menu;

    void layoutInit(void);
    QString getSpeedInfo(int downloadSpeed, int uploadSpeed);
    int delOfInt64(FILETIME subtrahend, FILETIME minuend);

    QTimer* m_timer;

    GetIfTable m_funcGetIfTable;
    QLibrary m_lib;
    DWORD m_preNetIn;
    DWORD m_preNetOut;
    int m_preTime;
    FILETIME m_preIdleTime;
    FILETIME m_preKernelTime;
    FILETIME m_preUserTime;

    uint m_MemeoryRate;
    uint m_CpuRate;

    float m_dpi;
    QPoint m_dragPosition;
};

#endif // MAINWIDGET_H
