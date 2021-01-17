#ifndef MODELS_H
#define MODELS_H

#include <winsock2.h>
#include <QString>


// define

#define MSG_01 WM_USER+100
#define MSG_02 WM_USER+101
#define USERNAME_LEN 255


// models

struct ConnectParam
{
    QString username;  //聊天用户名
    //QString key;  //聊天室密钥
    unsigned short port; //端口
    in_addr bind_addr;  //绑定的IP地址
    in_addr broadcast_addr; //广播地址
    unsigned short server_port;
    in_addr server_ip; //服务器地址
};

struct IP_MASK
{
    in_addr addr; //IP地址
    in_addr subnet_mask; //子网掩码
};

struct MYMSG  //接收消息时由kernel传递给main的信息
{
    QString username;
    in_addr ip;
    QString msg;
};

struct NoticeMsg
{
    int type;
    /*
     *  type = 0: 广播上线报文：广播通知自己上线
     *  type = 1: 单播报文： 告诉自己在线
     *  type = 2: 广播下线报文：广播通知自己下线
     *
     *
     */
    in_addr ip;
    char username[USERNAME_LEN];
    in_addr opo_ip; // Type=1用到
};

struct ChatMsg
{
    bool final; // final=true：最后一部分
    char msg[1024];
    unsigned short len;
};

struct Person
{
    in_addr ip;
    QString username;
};

struct ExtPerson:Person
{
    unsigned int nBytesMsgPos;
    unsigned int nBytesAssistPos;
    char msg[9000]; //最大消息总长度
    char AssitMsg[2*sizeof (NoticeMsg)]; //辅助消息中长度
    ExtPerson()
    {
        nBytesMsgPos = 0;
        nBytesAssistPos = 0;
        memset(msg, '\0', sizeof (msg));
    }
};


struct FileMsg //文件消息，服务器传来文件名和文件大小
{
    QString filename;
    unsigned long filesize; //单位：KB
};

#define UPLOAD_UNIT_LEN 1024  //文件分块上传大小
#define FILE_BLOCK_SIZE 1024



struct TRANS_DATA_BLOCK{
    unsigned short msg;
    // msg=0: 取消上传或下载
    // msg=1: 上传文件信息，要上传文件
    // msg=2: 文件块
    char dataBlock[FILE_BLOCK_SIZE]; // 文件内容
    unsigned short len; //块大小
    //streampos pos;
    u_long pos;
};



#endif // MODELS_H
