#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chatserver.h"
#include <QMainWindow>
#include <QEvent>
#include <QThread>
#include <QVector>
#include <iphlpapi.h>
#include <winsock2.h>
#include "models.h"
#include "assistserver.h"
#include <QVector>


struct IP_MASK
{
    in_addr addr; //IP地址
    in_addr subnet_mask; //子网掩码
};



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT
    QThread FileThread;
    QThread AssistThread;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);

    void getLANInfo(QVector<IP_MASK> &ip_maskVec);
    void getConnParam();
    void refreshLAN();

signals:
    void signal_startServer();
    void signal_stopServer();

    void signal_startAssistServer();

    void signal_RecvMsg(WPARAM w, LPARAM l);


public slots:
    void slot_ErrorMsg(int type);

private slots:
    void on_startButton_released();

    void on_stopButton_released();


    //void slot_recvStopMsg();

private:
    Ui::MainWindow *ui;
    ChatServer *chatServer;
    AssistServer *assistServer;
    bool state_chatServer;
    bool state_assistServer;
    QVector<IP_MASK> ip_maskVec;

    ConnectParam connParam;
};
#endif // MAINWINDOW_H
