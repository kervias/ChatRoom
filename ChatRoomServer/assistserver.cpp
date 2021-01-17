#include "assistserver.h"
#include <winsock2.h>
#include <QDebug>
#include <QDir>
#include <math.h>

AssistServer::AssistServer(QObject *parent) : QObject(parent)
{

}


void AssistServer::slot_startAssistServer()
{
    if(initSocket())
    {
        emit signal_ErrorMsg(3);
    }else{
        emit signal_ErrorMsg(2);
    }
}
bool AssistServer::initSocket()
{
    if((assistSocket = socket(AF_INET, SOCK_DGRAM,0)) < 0)
    {
        int error = WSAGetLastError(); //错误代码
        //emit signal_Error_TOMain("创建套接字失败", error);
        qDebug("创建套接字失败!");
        WSACleanup();
        return false;
    }

    sockaddr_in addr;
    memset((void *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    addr.sin_port = htons(this->connParam.port);
    addr.sin_addr.s_addr = this->connParam.bind_addr.s_addr;


   // addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(assistSocket, (sockaddr*) &addr, sizeof(addr)) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        qDebug()<<error;
        //emit signal_Error_TOMain("套接字绑定失败", error);
        qDebug("套接字绑定失败!");
        WSACleanup();
        return false;
    }

    bool isBroadcast = true;

    if(setsockopt(assistSocket, SOL_SOCKET, SO_BROADCAST, (char *)&isBroadcast, sizeof(isBroadcast)) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        //emit signal_Error_TOMain("广播设置失败", error);
        qDebug("广播设置失败!");
        WSACleanup();
       return false;
    }

    if(WSAAsyncSelect(assistSocket, (HWND)this->hWIND, MSG_01, FD_READ) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        qDebug("绑定异步事件失败!");
        closesocket(assistSocket);
        //emit signal_Error_TOMain("绑定异步事件失败", error);
        WSACleanup();
        return false;
    }
    return true;

}


void AssistServer::setHWind(WId hWind)
{
    this->hWIND = hWind;
}


void AssistServer::slot_RecvMsg(WPARAM w, LPARAM l)
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
            case 6:
            {
                QString filePath="./UploadFiles/";
                QDir *dir=new QDir(filePath);
                QStringList filter;
                QList<QFileInfo> *fileInfo=new QList<QFileInfo>(dir->entryInfoList(filter));
                qDebug()<<fileInfo->count()<<"count";
                for(int i = 2;i<fileInfo->count(); i++)
                {
                    NoticeMsg sendMsg;
                    sendMsg.type = 5;
                    sendMsg.ip.S_un.S_addr = this->connParam.bind_addr.S_un.S_addr;
                    sendMsg.opo_ip.S_un.S_addr = (unsigned long)(ceil(fileInfo->at(i).size()/1024.0)); //文件大小,KB为单位
                    QByteArray qba =  fileInfo->at(i).fileName().toUtf8();

                    memset(sendMsg.username, '\0', sizeof (sendMsg.username));
                    memcpy(sendMsg.username, qba.data(), qba.length());  //username复用为文件名
                    this->send_assistMsg(sendMsg, skAddr.sin_addr.S_un.S_addr);
                }
                break;
            }

        }
    }else{
        qDebug()<<"--";
        qDebug()<<"ASSIST接收到无法转换的信息";
    }
}


void AssistServer::send_assistMsg(NoticeMsg &assistMsg, unsigned long addr)
{
    sockaddr_in skAddr;
    skAddr.sin_addr.s_addr = addr;
    switch (assistMsg.type)
    {
        case 3:
        {
            assistMsg.opo_ip.S_un.S_addr = htonl(assistMsg.opo_ip.S_un.S_addr);
            break;
        }
        case 6:
        {
            assistMsg.opo_ip.S_un.S_addr = htonl(assistMsg.opo_ip.S_un.S_addr);
            break;
        }
        case 5:
        {
            break;
        }
        default:
            return;
    }
    assistMsg.type = htons(assistMsg.type);
    //assistMsg.ip.S_un.S_addr = htonl(assistMsg.ip.S_un.S_addr);
    skAddr.sin_family = AF_INET;
    skAddr.sin_port = htons(this->connParam.broadcast_port);
    sendto(assistSocket, (char*) &assistMsg, sizeof (assistMsg), 0, (sockaddr *) &skAddr, sizeof (skAddr));
}

void AssistServer::initConnParam(ConnectParam &connParam)
{
    this->connParam.port = connParam.port;
    this->connParam.broadcast_port = connParam.broadcast_port ;
    this->connParam.bind_addr.S_un.S_addr = connParam.bind_addr.S_un.S_addr;
    this->connParam.broadcast_addr.S_un.S_addr = connParam.broadcast_addr.S_un.S_addr;
}


void AssistServer::closeAll()
{
    closesocket(this->assistSocket);
}

void AssistServer::slot_addFileMsg(FileMsg filemsg)
{
    NoticeMsg sendMsg;
    sendMsg.type = 5;
    sendMsg.ip.S_un.S_addr = this->connParam.bind_addr.S_un.S_addr;
    sendMsg.opo_ip.S_un.S_addr = (unsigned long)(ceil(filemsg.filesize/1024.0));
    QByteArray qba = filemsg.filename.toUtf8();
    memset(sendMsg.username, '\0', sizeof (sendMsg.username));
    memcpy(sendMsg.username, qba.data(), qba.length());  //username复用为文件名
    this->send_assistMsg(sendMsg,this->connParam.broadcast_addr.S_un.S_addr);
}
