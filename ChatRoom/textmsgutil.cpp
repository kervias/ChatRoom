#include "textmsgutil.h"
#include <QDebug>
#include <synchapi.h>


TextMsgUtil::TextMsgUtil(QObject *parent) :QObject(parent)
{
    this->state = false;
    this->assistSocket = INVALID_SOCKET;
    this->msgSocket = INVALID_SOCKET;
}


bool TextMsgUtil::initAll(ConnectParam *param)
{
    this->ConnParam.bind_addr = param->bind_addr;
    this->ConnParam.port = param->port;
    this->ConnParam.username = param->username;
    this->ConnParam.broadcast_addr = param->broadcast_addr;
    this->ConnParam.server_ip = param->server_ip;
    this->ConnParam.server_port = param->server_port;
    this->state =  this->initConnect();
    return this->state;
}

bool TextMsgUtil::initConnect()
{
    if((msgSocket = socket(AF_INET, SOCK_DGRAM,0)) < 0)
    {
        int error = WSAGetLastError(); //错误代码
        emit signal_Error_TOMain("创建套接字失败", error);
        qDebug("创建套接字失败!");

        return false;
    }

    sockaddr_in addr;
    memset((void *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    //addr.sin_port = htons(this->ConnParam.port);
    addr.sin_addr.s_addr = this->ConnParam.bind_addr.s_addr;
    //addr.sin_addr.s_addr=INADDR_ANY;


    int bOptval=1;
    int retSetsockopt;
//    retSetsockopt =setsockopt(msgSocket,SOL_SOCKET ,SO_REUSEADDR ,(char *)&bOptval,sizeof(bOptval));
//    if (SOCKET_ERROR==retSetsockopt)
//    {
//        int error = WSAGetLastError(); //错误代码
//        qDebug()<<error;
//        emit signal_Error_TOMain("套接字设置复用失败", error);
//        qDebug("套接字设置复用失败!");

//        return false;
//    }





    if(bind(msgSocket, (sockaddr*) &addr, sizeof(addr)) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        qDebug()<<error;
        emit signal_Error_TOMain("套接字绑定失败", error);
        qDebug("套接字绑定失败!");

        return false;
    }

    bool isBroadcast = true;

    if(setsockopt(msgSocket, SOL_SOCKET, SO_BROADCAST, (char *)&isBroadcast, sizeof(isBroadcast)) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        emit signal_Error_TOMain("广播设置失败", error);
        qDebug("广播设置失败!");

       return false;
    }
//    int len = 5000;
//    setsockopt(msgSocket, SOL_SOCKET, SO_RCVBUF, (char *)&len, sizeof(int));
//    qDebug()<<GetLastError();
//    setsockopt(msgSocket, SOL_SOCKET, SO_SNDBUF, (char *)&len, sizeof(int));
//    qDebug()<<GetLastError();
    if(WSAAsyncSelect(msgSocket, (HWND)this->hHWIND, MSG_01, FD_READ) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        qDebug("绑定异步事件失败!");
        closesocket(msgSocket);
        emit signal_Error_TOMain("绑定异步事件失败", error);

        return false;
    }

    // ---------------以下代码创建assitSocket-------------------

    if((assistSocket = socket(AF_INET, SOCK_DGRAM,0)) < 0)
    {
        int error = WSAGetLastError(); //错误代码
        emit signal_Error_TOMain("创建assist套接字失败", error);
        qDebug("创建assist套接字失败!");

        return false;
    }

    sockaddr_in assist_addr;
    memset((void *)&assist_addr, 0, sizeof(assist_addr));
    assist_addr.sin_family = AF_INET;
    assist_addr.sin_port = htons(this->ConnParam.port+1);
    assist_addr.sin_addr.s_addr = this->ConnParam.bind_addr.s_addr;
    //addr.sin_addr.s_addr=INADDR_ANY;


    bOptval=1;
    retSetsockopt=setsockopt(assistSocket,SOL_SOCKET ,SO_REUSEADDR ,(char *)&bOptval,sizeof(bOptval));
    if (SOCKET_ERROR==retSetsockopt)
    {
        int error = WSAGetLastError(); //错误代码
        qDebug()<<error;
        emit signal_Error_TOMain("辅助套接字设置复用失败", error);
        qDebug("辅助套接字设置复用失败!");

        return false;
    }



    if(bind(assistSocket, (sockaddr*) &assist_addr, sizeof(assist_addr)) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        qDebug()<<error;
        emit signal_Error_TOMain("assist套接字绑定失败", error);
        qDebug("assist套接字绑定失败!");

        return false;
    }

    bool assist_isBroadcast = true;

    if(setsockopt(assistSocket, SOL_SOCKET, SO_BROADCAST, (char *)&assist_isBroadcast, sizeof(assist_isBroadcast)) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        emit signal_Error_TOMain("assist广播设置失败", error);
        qDebug("assist广播设置失败!");

       return false;
    }

    if(WSAAsyncSelect(assistSocket, (HWND)this->hHWIND, MSG_02, FD_READ) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        qDebug("assist绑定异步事件失败!");
        closesocket(assistSocket);
        emit signal_Error_TOMain("assist绑定异步事件失败", error);

        return false;
    }

    return true;
}



void TextMsgUtil::closeAll()
{
    if(this->state == false)
        return;

    this->state = false;

    NoticeMsg assistMsg;
    assistMsg.ip.S_un.S_addr = this->ConnParam.bind_addr.S_un.S_addr;
    strcpy(assistMsg.username, this->ConnParam.username.toStdString().data());
    assistMsg.type = 2;
    this->send_assistMsg(assistMsg);
    if(this->msgSocket != INVALID_SOCKET)
        closesocket(msgSocket);
    if(this->assistSocket != INVALID_SOCKET)
        closesocket(assistSocket);
    this->onlinePeoMap.clear();
    this->assistSocket = INVALID_SOCKET;
    this->msgSocket = INVALID_SOCKET;

}

void TextMsgUtil::slot_RecvMsg(WPARAM w, LPARAM l)
{
    try{
    qDebug("进入recv---------------");
    ChatMsg recvMsg;
    sockaddr_in skAddr;
    int len = sizeof(skAddr);
    int recvBytesLen = recvfrom(msgSocket, (char *)&recvMsg, sizeof(recvMsg),0, (sockaddr *) &skAddr, &len);
    qDebug()<<WSAGetLastError();
    //qDebug()<<"*****************";
    if(recvBytesLen == sizeof (recvMsg))
    {
//        qDebug()<<buf;
//        qDebug()<<strlen(buf);

        if(!this->onlinePeoMap.contains(skAddr.sin_addr.S_un.S_addr))
         {
            qDebug("返回return-----------");
            return;
        }

        recvMsg.len = ntohs(recvMsg.len);
        ExtPerson *extp = this->onlinePeoMap[skAddr.sin_addr.S_un.S_addr];
        memcpy(extp->msg+extp->nBytesMsgPos, recvMsg.msg, recvMsg.len);
        extp->nBytesMsgPos += recvMsg.len;
        if(recvMsg.final == true)
        {
            extp->nBytesMsgPos = 0;
            qDebug()<<inet_ntoa(skAddr.sin_addr);
            MYMSG mymsg;
            mymsg.ip.S_un.S_addr = skAddr.sin_addr.S_un.S_addr;
            mymsg.msg = QString(extp->msg);
            memset(extp->msg, '\0', sizeof (extp->msg));
            mymsg.username = this->onlinePeoMap[skAddr.sin_addr.S_un.S_addr]->username;
            emit signal_RecvMsg_TOMain(1, mymsg);
        }


    }else{
        qDebug()<<"size<=0";
    }
     qDebug("退出recv---------------");
    }
   catch(...)
    {
        qDebug()<<"Recv_MSG异常";
    }
}

void TextMsgUtil::send_Msg(QString msg)
{
    try{
    //std::string str = msg.toStdString();
    char *charMsg = nullptr;
    QByteArray qba = msg.toUtf8();
    charMsg = new char[qba.length()];
    memset(charMsg, '\0', qba.length());
    memcpy(charMsg, qba.data(), qba.length());
    charMsg[qba.length()] = '\0';

    sockaddr_in skAddr;
    skAddr.sin_addr.s_addr = this->ConnParam.broadcast_addr.S_un.S_addr;
    skAddr.sin_family = AF_INET;
    skAddr.sin_port = htons(this->ConnParam.port);

    int pos = 0;
    int send_size;
    ChatMsg sendMsg;

    while (pos < qba.length()) {
        sendMsg.final = ((qba.length() - pos)<=1024)?true:false;
        send_size =  ((qba.length() - pos)<=1024)?(qba.length() - pos):1024;
        memcpy(sendMsg.msg, charMsg+pos, send_size);
        sendMsg.len = htons(send_size);
        sendto(msgSocket, (char*)&sendMsg, sizeof (ChatMsg), 0, (sockaddr *) &skAddr, sizeof (skAddr));
        //qDebug()<<"发送错误码"<<WSAGetLastError();
        pos += send_size;
    }


    }
    catch(...)
    {
        qDebug()<<"Send_MSG异常";
    }
}


void TextMsgUtil::initSocket()
{

}


void TextMsgUtil::getLANInfo(QVector<IP_MASK> &ip_maskVec)
{
    IP_ADAPTER_INFO  *pai = new IP_ADAPTER_INFO();
    unsigned long paiSize = sizeof(IP_ADAPTER_INFO);
    if(GetAdaptersInfo(pai, &paiSize) == ERROR_BUFFER_OVERFLOW) //防止申请的内存不够
    {
        pai = (IP_ADAPTER_INFO *)new byte[paiSize];
        GetAdaptersInfo(pai, &paiSize);
    }
    while(pai)  //遍历所有网卡
    {
        IP_ADDR_STRING *ipList = &pai->IpAddressList;
        while(ipList)  //遍历一个网卡中的IP
        {

            IP_MASK ip_mask;
            IP_ADDRESS_STRING ip = ipList->IpAddress;
            IP_MASK_STRING mask = ipList->IpMask;
            ipList = ipList->Next;
            ip_mask.addr.S_un.S_addr = inet_addr(ip.String);
            ip_mask.subnet_mask.S_un.S_addr = inet_addr(mask.String);
            if(ip_mask.addr.S_un.S_addr == 0)
                 continue;
            ip_maskVec.push_back(ip_mask);

        }
        pai = pai->Next;
    }
//    // 添加本地回环地址 127.0.0.1
//    IP_MASK ip_mask;
//    ip_mask.addr.S_un.S_addr = inet_addr("127.0.0.1");
//    ip_mask.subnet_mask.S_un.S_addr = inet_addr("255.0.0.0");
//    ip_maskVec.push_back(ip_mask);
}


void TextMsgUtil::slot_RecvMsg_ASSIST(WPARAM w, LPARAM l)
{
    NoticeMsg assistMsg;
    sockaddr_in skAddr;
    int len = sizeof(skAddr);

    int recvBytesLen = recvfrom(assistSocket, (char*) &assistMsg, sizeof(assistMsg),0, (sockaddr *) &skAddr, &len);
    if(recvBytesLen  == sizeof(NoticeMsg))
    {
        assistMsg.type = ntohs(assistMsg.type);
        qDebug()<<"---------------------";
        qDebug()<<assistMsg.type;
        qDebug()<<assistMsg.username;
        qDebug()<<inet_ntoa(assistMsg.ip);
        qDebug()<<ntohs(skAddr.sin_port);
        qDebug()<<"---------------------";
//        if(assistMsg.ip.S_un.S_addr != skAddr.sin_addr.S_un.S_addr) //防止虚假消息
//            return;

        switch (assistMsg.type)
        {
            case 0:
            {
                Person per;
                per.ip.S_un.S_addr = assistMsg.ip.S_un.S_addr;
                per.username = assistMsg.username;
                this->addPerson(per);
                if(per.ip.S_un.S_addr != this->ConnParam.bind_addr.S_un.S_addr)
                {
                    NoticeMsg assistMsg2;
                    assistMsg2.ip.S_un.S_addr = this->ConnParam.bind_addr.S_un.S_addr;
                    strcpy(assistMsg2.username,this->ConnParam.username.toStdString().data());
                    assistMsg2.type = 1;
                    assistMsg2.opo_ip.S_un.S_addr = skAddr.sin_addr.S_un.S_addr;
                    this->send_assistMsg(assistMsg2);
                }
                break;
            }
            case 1:
            {
                Person per;
                per.ip.S_un.S_addr = assistMsg.ip.S_un.S_addr;
                per.username = assistMsg.username;
                this->addPerson(per);
                break;
            }
            case 2:
            {
                Person per;
                per.ip.S_un.S_addr = assistMsg.ip.S_un.S_addr;
                per.username = assistMsg.username;
                this->delPerson(per);
                break;
            }
            case 4: // 服务器ip和端口
            {
                qDebug()<<"服务器IP:"<<inet_ntoa(assistMsg.ip);
                qDebug()<<"端口:"<<assistMsg.opo_ip.S_un.S_addr;
                break;
            }
            case 5: //文件列表项
            {
                FileMsg filemsg;
                filemsg.filename = QString(assistMsg.username);
                filemsg.filesize = (unsigned long)assistMsg.opo_ip.S_un.S_addr;
                emit signal_addFilemsg(filemsg);
                break;
            }
        }
    }else{
        qDebug()<<"--";
        qDebug()<<"ASSIST接收到无法转换的信息";
    }
}

void TextMsgUtil::send_assistMsg(NoticeMsg &assistMsg)
{
    sockaddr_in skAddr;
    switch (assistMsg.type)
    {
        case 0:
        {
            skAddr.sin_addr.s_addr = this->ConnParam.broadcast_addr.S_un.S_addr;
            skAddr.sin_port = htons(this->ConnParam.port+1);
            break;
        }
        case 1:
        {
            skAddr.sin_addr.s_addr = assistMsg.opo_ip.S_un.S_addr;
            skAddr.sin_port = htons(this->ConnParam.port+1);
            break;
        }
        case 2:
        {
            skAddr.sin_addr.s_addr = this->ConnParam.broadcast_addr.S_un.S_addr;
            skAddr.sin_port = htons(this->ConnParam.port+1);
            break;
        }
        case 6:// 向服务器询问文件
        {
            skAddr.sin_addr.s_addr = this->ConnParam.server_ip.S_un.S_addr;
            skAddr.sin_port = htons(this->ConnParam.server_port);
            qDebug()<<inet_ntoa(this->ConnParam.server_ip);
            qDebug()<<this->ConnParam.server_port;
            break;
        }
        default:
            return;
    }
    assistMsg.type = htons(assistMsg.type);
    skAddr.sin_family = AF_INET;

    sendto(assistSocket, (char*) &assistMsg, sizeof (assistMsg), 0, (sockaddr *) &skAddr, sizeof (skAddr));
    int code = WSAGetLastError();
    qDebug()<<"lastError:"<<code;
}

void TextMsgUtil::addPerson(Person &per)
{

    if(!this->onlinePeoMap.contains(per.ip.S_un.S_addr) && per.ip.S_un.S_addr != this->ConnParam.bind_addr.S_un.S_addr)
    {
        ExtPerson *extp = new ExtPerson;
        extp->ip = per.ip;
        extp->username = per.username;
        this->onlinePeoMap.insert(per.ip.S_un.S_addr, extp);
        emit signal_curdPerson_TOMain(0, per);
    }


}


void TextMsgUtil::delPerson(Person &per)
{
    if(this->onlinePeoMap.contains(per.ip.S_un.S_addr))
    {
        auto it = this->onlinePeoMap.find(per.ip.S_un.S_addr);
        this->onlinePeoMap.erase(it);
        emit signal_curdPerson_TOMain(1, per);
    }

}

const ConnectParam TextMsgUtil::getConnParam()
{
    return this->ConnParam;
}

void TextMsgUtil::slot_CommonMsg(int type, QString msg)
{
    //qDebug("111");
    if(type == 0)
    {
        this->send_Msg(msg);
    }
}


void TextMsgUtil::setHWind(WId hWind)
{
    this->hHWIND = hWind;
}

ConnectParam TextMsgUtil::getParam()
{
    return this->ConnParam;
}

void TextMsgUtil::sendInitAssitMsg()
{
    NoticeMsg assistMsg;
    assistMsg.type = 0;
    strcpy(assistMsg.username, this->ConnParam.username.toStdString().data());
    assistMsg.ip.S_un.S_addr = this->ConnParam.bind_addr.S_un.S_addr;
    this->send_assistMsg(assistMsg);
    assistMsg.type = 6;
    this->send_assistMsg(assistMsg);
}
