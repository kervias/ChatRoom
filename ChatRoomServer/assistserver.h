#ifndef ASSISTSERVER_H
#define ASSISTSERVER_H

#include <QObject>
#include <winsock2.h>
#include <QWidget>
#include "models.h"

#define USERNAME_LEN 255


struct NoticeMsg
{
    int type;
    /*
     *  type = 0: 广播上线报文：广播通知自己上线
     *  type = 1: 单播报文： 告诉自己在线
     *  type = 2: 广播下线报文：广播通知自己下线
     *  type = 3: 询问报文，询问服务器的地址
     *  type = 4: 单播报文：告诉自己是服务器
     *  type = 5: 单播报文：单播文件名
     *  type = 6: 请求获取所有文件
     */
    in_addr ip;
    char username[USERNAME_LEN];
    in_addr opo_ip; // Type=1用到
};




class AssistServer : public QObject
{
    Q_OBJECT
public:
    explicit AssistServer(QObject *parent = nullptr);
    void initConnParam(ConnectParam &connParam);
    bool initSocket();
    void setHWind(WId hWind);
    void closeAll();

    void send_assistMsg(NoticeMsg &assistMsg,unsigned long addr);
signals:
    void signal_ErrorMsg(int type); //报错信息
public slots:
    void slot_RecvMsg(WPARAM w, LPARAM l);

    void slot_startAssistServer();

    void slot_addFileMsg(FileMsg filemsg);

private:
    ConnectParam connParam;
    SOCKET assistSocket;
    WId hWIND;

};

#endif // ASSISTSERVER_H
