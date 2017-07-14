#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtWidgets>

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

struct CONFIG_S
{
    int PosX;
    int PosY;
    QColor Color;
    int Version;
};

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
    void changeDisplay_slot(QAction* action);
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
    QMenu* m_DisplayMenu;
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
    long m_preNetIn;
    long m_preNetOut;
    long long m_preIdleTime;
    long long m_preAllTime;

    uint m_MemeoryRate;
    uint m_CpuRate;
    QString m_Upload;
    QString m_Download;

    float m_dpi;
    QPoint m_dragPosition;
    int m_iPreAngleTime;
    double m_Angle;
    QSize m_preScreenSize;
    QColor m_Color;
    int m_displayC;
    bool m_hide;
    int m_rx;
    int m_ry;
};

#endif // MAINWIDGET_H
