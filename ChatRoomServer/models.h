#ifndef MODELS_H
#define MODELS_H
#include <winsock2.h>
#include <QString>

#define MSG_01 WM_USER+103

struct ConnectParam
{
    unsigned short port; //端口
    in_addr bind_addr;  //绑定的IP地址
    in_addr broadcast_addr; //广播地址
    unsigned short broadcast_port;
};


struct FileMsg //文件消息，服务器传来文件名和文件大小
{
    QString filename;
    unsigned long filesize; //单位：KB
};

#endif // MODELS_H
