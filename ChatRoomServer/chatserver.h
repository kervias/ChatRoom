#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>

#include <winsock2.h>
#include <string.h>
#include <process.h>
#include <vector>
#include <iostream>
#include <fstream>
#include "models.h"
using namespace std;

#define BUFFERSIZE 1032
#define LISTEN_PORT 12345
#define MAX_QUEUE_CLIENT 20
#define MAX_THREAD_NUM 8
#define EXIT_CODE 9999
#define FILE_BLOCK_SIZE 1024
#define RECV_BUFF_SIZE 4096*2

enum OP_TYPE{
    TYPE_UPLOAD, TYPE_DOWNLOAD, TYPE_UPLOAD_IMG, TYPE_UNKNOWN
};

enum OP_STATE{
    STATE_UPLOADING, STATE_DOWNLOADING, STATE_NOTHING, STATE_UPLOADING_IMG
};

struct TRANS_FILE_INFO{ //上传和下载通用
    unsigned short op; // op=0: 上传图片 op=1: 上传文件 op=2: 下载文件
    char filename[256]; //文件名
    unsigned int filesize; //文件大小
};

struct TRANS_DATA_BLOCK{
    unsigned short msg;
    // msg=0: 取消上传
    // msg=1: 上传文件信息，要上传文件
    // msg=2: 上传文件块
    // msg=3: 最后分片
    // msg=4: 询问文件信息，要下载文件
    // msg=5: 发送文件信息，提供文件信息
    // msg=6: 下载文件块
    // msg=7: 取消下载或接收
    char dataBlock[FILE_BLOCK_SIZE]; // 文件内容
    unsigned short len; //块大小
    u_long pos;
};

struct EXP_OVERLAPPED : OVERLAPPED{
    SOCKET socket;
    OP_TYPE type; // type=true：用于上传文件  type=false: 用于下载文件
    bool lastSendOrRecv;
    //OP_STATE state; // state = true:正在传输文件 state=false:尚未传输文件
    OP_STATE lastState; //上一个状态
    fstream pfile; //文件操作指针
    QString *filename; //文件名
    char BUFFER[BUFFERSIZE];
    char BUFFERSEND[BUFFERSIZE];
    char TEMP_BUFF[RECV_BUFF_SIZE];
    unsigned short TEMP_BUFF_POS;
    unsigned short TEMP_SEND_SIZE;
    TRANS_DATA_BLOCK tdb2;
    unsigned long last_pos;
    WSABUF wsaBuffer;
    EXP_OVERLAPPED(SOCKET sock) : socket(sock) {
        wsaBuffer.buf = BUFFER;
        wsaBuffer.len = BUFFERSIZE;
        type = TYPE_UNKNOWN;
        //state = STATE_NOTHING;
        lastState = STATE_NOTHING;
        TEMP_BUFF_POS = 0;
        TEMP_SEND_SIZE = 0;
        last_pos = 0;
        memset(this,0,sizeof(OVERLAPPED));
    }
};


class ChatServer: public QObject
{
    Q_OBJECT
    friend DWORD WINAPI WorkerThread (LPVOID para);

public:
    explicit ChatServer(QObject *parent = nullptr);
    ~ChatServer();
    void initConnParam(ConnectParam &connParm);
    bool initListenSocket();
    bool initIOCP();
    void closeAll();

    void addEXP_OVERLAPPED(EXP_OVERLAPPED *exp_ol);
    void removeEXP_OVERLAPPED(EXP_OVERLAPPED *exp_ol);
    void removeErrorEXP_OVERLAPPED(EXP_OVERLAPPED *exp_ol);

    void start();

public slots:
    void slot_startServer();

    //void slot_stopServer();

signals:
    void signal_ErrorMsg(int type); //报错信息
    void signal_addFileMsg(FileMsg fileMsg);

public:
    SOCKET socketListen;

private:

    bool state;
    HANDLE hThreads[MAX_THREAD_NUM]; //线程句柄
    HANDLE hIOCP; //IOCP对象
    vector<EXP_OVERLAPPED *> connList; //所有连接对象
    CRITICAL_SECTION mutexConnList; //互斥锁

    ConnectParam connParam;

};

#endif // CHATSERVER_H


