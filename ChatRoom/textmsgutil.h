#ifndef TEXTMSGUTIL_H
#define TEXTMSGUTIL_H


#include <QWidget>
#include <winsock2.h>
#include <QString>
#include <QVector>
#include <iphlpapi.h>
#include <QTimer>
#include <QMap>
#include "models.h"

class TextMsgUtil : public QObject
{
    Q_OBJECT
public:
    explicit TextMsgUtil(QObject *parent = nullptr);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
    void setHWind(WId hWind);

    bool initConnect();
    void initSocket();
    void sendInitAssitMsg();

    bool initAll(ConnectParam *param);
    void closeAll();

    void send_Msg(QString msg);
    void send_assistMsg(NoticeMsg &assitMsg); //根据辅助消息类型发送消息

    void getLANInfo(QVector<IP_MASK> &ip_maskVec); //获取所有局域网IP地址和子网掩码
    const ConnectParam getConnParam();

    void addPerson(Person &per);
    void delPerson(Person &per);

    ConnectParam getParam();

signals:
    // 发给主窗口
    void signal_Error_TOMain(QString errorMsg, int errorCode);
    void signal_RecvMsg_TOMain(int type, MYMSG &mymsg);
    void signal_curdPerson_TOMain(int type, Person &per); //增加或删除
    void signal_addFilemsg(FileMsg &fileMsg);


public slots:
    //void slot_responseMsg();
    void slot_RecvMsg(WPARAM w, LPARAM l);
    void slot_RecvMsg_ASSIST(WPARAM w, LPARAM l);


    void slot_CommonMsg(int type, QString msg);

private:
    ConnectParam ConnParam;

    SOCKET msgSocket;  //用于消息传递
    bool state;

    SOCKET assistSocket; //用于检查在线人数

    QMap<unsigned long, ExtPerson*> onlinePeoMap; //在线好友列表
    WId hHWIND;


};

#endif // TEXTMSGUTIL_H
