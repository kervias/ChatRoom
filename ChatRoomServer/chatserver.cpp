#include "chatserver.h"
#include <winsock2.h>
#include <conio.h>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <QDir>
typedef unsigned (__stdcall *PTHREAD_START) (void *);


ChatServer::ChatServer(QObject *parent) : QObject(parent)
{
    this->socketListen = INVALID_SOCKET;
    this->hIOCP = NULL;
    this->state = false;
    QString folderpath = "./UploadFiles/";
    QDir dir;
    if(!dir.exists(folderpath))
        dir.mkpath(folderpath);
    folderpath = "./TempFiles/";
    if(!dir.exists(folderpath))
        dir.mkpath(folderpath);

}

DWORD WorkerThread(LPVOID para)
{
    ChatServer *PTHIS = (ChatServer*)para;
    EXP_OVERLAPPED* exp_ol = NULL;
    DWORD dwBytes = 0;
    unsigned long long ulKey = 0;
   // DWORD ulKey = 0;
    DWORD dwFlags = 0;
    int lastError = 0;
    int ret;
    int recv_or_send = 0;
    while(true)
    {
        BOOL OK = GetQueuedCompletionStatus(PTHIS->hIOCP, &dwBytes, &ulKey, (LPOVERLAPPED*)&exp_ol, INFINITE);
        recv_or_send = 0;
        //qDebug("----------\n";
        if(OK && ulKey == EXIT_CODE)
            break;
        if(!OK)
        {
            lastError = WSAGetLastError();
            qDebug("线程返回错误 ERROR: %d",lastError);
            if(exp_ol != NULL)
            {
                qDebug("IO错误");
                if(exp_ol->Internal != 0)
                {
                    if(exp_ol->Internal == 0xC000013C || exp_ol->Internal == 0xc000020d)
                    {
                        qDebug("连接被意外关闭");
                    }
                }
                PTHIS->removeErrorEXP_OVERLAPPED(exp_ol);
            }else if(OK == WAIT_TIMEOUT)
            {
                qDebug("超时");
            }
            continue;
        }
        if(dwBytes == 0)
        { //正常关闭连接
            qDebug("正常关闭");
            PTHIS->removeErrorEXP_OVERLAPPED(exp_ol);
            continue;
        }
//        if(exp_ol->lastSendOrRecv == false)
//            recv_or_send = 1;
//        else
//            recv_or_send = 2;
        if(exp_ol->lastSendOrRecv == false)
        {
            //qDebug("scamcas1");
            //qDebug()<<dwBytes;
//            exp_ol->TEMP_SEND_SIZE += dwBytes;
//            if(exp_ol->TEMP_SEND_SIZE == sizeof (TRANS_DATA_BLOCK))
//            {
//                recv_or_send = 1;
//                exp_ol->lastSendOrRecv = true;
//                exp_ol->TEMP_SEND_SIZE = 0;
//            }else{
//                qDebug()<<"未发送完全";
//                recv_or_send = 2;
//                continue;
//            }
            //qDebug()<<exp_ol->InternalHigh;
            if(dwBytes != sizeof (TRANS_DATA_BLOCK))
                qDebug("发送失败");
            recv_or_send = 1;
            //qDebug("00-false");
            //continue;
        }else{
            //qDebug()<<"scamcas2";
            memcpy(exp_ol->TEMP_BUFF + exp_ol->TEMP_BUFF_POS, exp_ol->BUFFER, dwBytes); //先将收到的内容复制到临时缓存
            exp_ol->TEMP_BUFF_POS += dwBytes;
            //qDebug("%d",exp_ol->TEMP_BUFF_POS);
            while(exp_ol->TEMP_BUFF_POS >= sizeof (TRANS_DATA_BLOCK))
            {
                //qDebug()<<"循环";
                //qDebug("%d",exp_ol->TEMP_BUFF_POS);
                TRANS_DATA_BLOCK* tdb = (TRANS_DATA_BLOCK *)exp_ol->TEMP_BUFF;
                tdb->msg = ntohs(tdb->msg);
                //qDebug()<<tdb->msg;
                tdb->len = ntohs(tdb->len);
                tdb->pos = ntohl(tdb->pos);
                switch (tdb->msg)
                {
                    case 1://上传：文件信息
                    {
                    qDebug("MSG1");
                        recv_or_send = 1;
                        if(exp_ol->lastState != STATE_NOTHING)
                        {
                            qDebug("MSG1异常");
                            recv_or_send = 0;
                            break;
                        }
                        qDebug("---- 上传 -----");
                        qDebug("%d",tdb->msg);
                        qDebug("%s",tdb->dataBlock); //文件名
                        qDebug("%d",tdb->len); //文件大小
                        qDebug("---- 上传 -----\n");
                        char filepath[1050]="./TempFiles/";
                        strcat(filepath, tdb->dataBlock);
                        exp_ol->pfile.open(filepath, ios::out | ios::binary | ios::trunc);
                        if(!exp_ol->pfile.is_open())
                        {
                            qDebug("文件创建失败");
                            recv_or_send = 0;
                            PTHIS->removeErrorEXP_OVERLAPPED(exp_ol);
                        }
                        exp_ol->filename = new QString(filepath);
                        exp_ol->lastState = STATE_UPLOADING;

                        break;
                    }
                    case 2:
                    {
                    qDebug("MSG2");
                        recv_or_send = 1;
                        if(exp_ol->lastState != STATE_UPLOADING)
                        {
                            qDebug("MSG2异常");
                            recv_or_send = 0;
                            break;
                        }
                        exp_ol->pfile.write(tdb->dataBlock, tdb->len);
                        exp_ol->pfile.flush();
                        break;
                    }
                    case 3:
                    {
                    qDebug("MSG3");
                        recv_or_send = 1;
                        if(exp_ol->lastState != STATE_UPLOADING)
                        {
                            qDebug("MSG3异常");
                            recv_or_send = 0;
                            break;
                        }
                        exp_ol->lastState = STATE_NOTHING;
                        exp_ol->pfile.flush();
                        exp_ol->pfile.close();
                        QFileInfo fileinfo(*exp_ol->filename);
                        FileMsg msg;
                        msg.filename = fileinfo.fileName();
                        msg.filesize = fileinfo.size();
                        QString filename = QString("./UploadFiles/%1").arg(fileinfo.fileName());
                        QFile::rename(*exp_ol->filename, filename); //移动文件
                        emit PTHIS->signal_addFileMsg(msg);
                        break;
                    }
                    case 0:
                    {
                    qDebug("MSG0");
                        recv_or_send = 1;
                        if(exp_ol->lastState != STATE_UPLOADING)
                        {
                            qDebug("MSG0异常");
                            recv_or_send = 0;
                            break;
                        }
                        exp_ol->lastState = STATE_NOTHING;
                        exp_ol->pfile.close();
                        break;
                    }
                    case 4: //客户发送下载文件的文件名
                    {
                    //qDebug("MSG4");
                        recv_or_send = 2;
                        if(exp_ol->lastState != STATE_NOTHING)
                        {
                            qDebug("MSG4异常");
                            recv_or_send = 0;
                            break;
                        }

                        //exp_ol->lastState = STATE_DOWNLOADING;

                        // 获取文件夹下所有文件名
                        QString filePath="./UploadFiles/";
                        QDir *dir=new QDir(filePath);
                        QStringList filter;
                        QList<QFileInfo> *fileInfo=new QList<QFileInfo>(dir->entryInfoList(filter));
                        char getFilename[255];
                        memcpy(getFilename, tdb->dataBlock,255);
                        qDebug()<<QString(getFilename);
                        int  ExistI = -1;
                        for(int i = 0;i<fileInfo->count(); i++)
                        {
                            if(fileInfo->at(i).fileName().compare(QString(getFilename)) == 0)
                            {
                                ExistI = i;
                                break;
                            }
                        }

                        exp_ol->tdb2.msg = htons(5);
                        if(ExistI != -1)
                        { //找到文件, 开始传输
                            exp_ol->tdb2.pos = htonl(fileInfo->at(ExistI).size());
                            memcpy(exp_ol->tdb2.dataBlock, getFilename, 255);
                            exp_ol->tdb2.len = htons(1);
                            exp_ol->lastState = STATE_DOWNLOADING;
                            exp_ol->pfile.open(fileInfo->at(ExistI).filePath().toLocal8Bit().constData(), ios::in | ios::binary);
                            if(!exp_ol->pfile.is_open())
                            {
                                qDebug()<<"download文件打开失败";
                                recv_or_send = 0;
                                PTHIS->removeErrorEXP_OVERLAPPED(exp_ol);
                            }
                            exp_ol->pfile.seekg(tdb->pos);
                            //exp_ol->last_pos = 0;
                        }else{ //未找到文件，发送未找到消息
                             exp_ol->tdb2.len = htons(0); //0 代表不存在这个文件
                            exp_ol->lastState = STATE_NOTHING;
                        }
                        memcpy(exp_ol->BUFFERSEND, (char *)& exp_ol->tdb2, sizeof(TRANS_DATA_BLOCK));
                        break;
                    }
                    case 6:
                    {
                        //qDebug("MSG6");
                        recv_or_send = 2;
                        if(exp_ol->lastState != STATE_DOWNLOADING)
                        {
                            qDebug("MSG6异常");
                            qDebug()<<tdb->msg;
                            qDebug()<<tdb->pos;
                            recv_or_send = 0;
                            break;
                        }

                        exp_ol->tdb2.pos = htonl(exp_ol->last_pos);//tdb->pos;
                        exp_ol->pfile.seekg(exp_ol->last_pos);

                        exp_ol->tdb2.msg = htons(6);
                        exp_ol->pfile.read(exp_ol->tdb2.dataBlock, FILE_BLOCK_SIZE);
                        exp_ol->tdb2.len =  htons(exp_ol->pfile.gcount());

                        if(exp_ol->pfile.eof())
                        {
                            qDebug()<<"最后分片";
                            exp_ol->tdb2.msg = htons(3);
                        }
                        exp_ol->last_pos += exp_ol->pfile.gcount();
                        memcpy(exp_ol->BUFFERSEND, (char *)& exp_ol->tdb2, sizeof(TRANS_DATA_BLOCK));
                        exp_ol->TEMP_SEND_SIZE = 0;
                        break;
                    }
                    case 7:
                    {
                        //qDebug("MSG7");
                        recv_or_send = 1;
                        if(exp_ol->lastState != STATE_DOWNLOADING)
                        {
                            recv_or_send = 0;
                            break;
                        }
                        qDebug()<<"客户端文件下载完毕";
                        exp_ol->last_pos = 0;
                        exp_ol->pfile.close();
                        exp_ol->lastState = STATE_NOTHING;
                        break;
                    }
                    default:
                    {
                        qDebug("default");
                        recv_or_send = 0;
                    }

                }

                memmove(exp_ol->TEMP_BUFF, exp_ol->TEMP_BUFF+ sizeof(TRANS_DATA_BLOCK), exp_ol->TEMP_BUFF_POS - sizeof(TRANS_DATA_BLOCK));
                exp_ol->TEMP_BUFF_POS -=  sizeof(TRANS_DATA_BLOCK);
                if(recv_or_send == 2)
                    break;
                 //qDebug("%d",exp_ol->TEMP_BUFF_POS);
            }
        }
        if(recv_or_send == 1)
        {
            dwFlags = 0;
            exp_ol->wsaBuffer.buf = exp_ol->BUFFER;
            exp_ol->lastSendOrRecv = true;
            ret = WSARecv(exp_ol->socket, &(exp_ol->wsaBuffer), 1, NULL, &dwFlags, (LPWSAOVERLAPPED)exp_ol, NULL);
            lastError = WSAGetLastError();
            //qDebug(lastError);
            if(ret == SOCKET_ERROR && WSA_IO_PENDING != lastError)
            {
                qDebug("WSARecv error %d",lastError);
                PTHIS->removeErrorEXP_OVERLAPPED(exp_ol);
            }

            qDebug()<<"11";
        }
        else if(recv_or_send == 2)
        {
            dwFlags = 0;
            exp_ol->wsaBuffer.buf = exp_ol->BUFFERSEND;
            exp_ol->lastSendOrRecv = false;
            ret = WSASend(exp_ol->socket, &(exp_ol->wsaBuffer), 1, NULL, dwFlags, (LPWSAOVERLAPPED)exp_ol, NULL);
            lastError = WSAGetLastError();
            //qDebug(lastError);
            if(ret == SOCKET_ERROR && WSA_IO_PENDING != lastError)
            {
                qDebug("WSASend error %d",lastError);
                PTHIS->removeErrorEXP_OVERLAPPED(exp_ol);
            }

            qDebug()<<"22";
        }

    }

    return 0;
}


bool ChatServer::initListenSocket()
{
    InitializeCriticalSection(&this->mutexConnList);
    if((this->socketListen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        qDebug("创建监听套接字失败! 错误码 %d",WSAGetLastError());
        return false;
    }

    sockaddr_in addr;
    memset((void *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    addr.sin_addr.S_un.S_addr = this->connParam.bind_addr.S_un.S_addr;
    addr.sin_port = htons(this->connParam.port);
    if(bind(this->socketListen, (sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR)
    {
        qDebug("监听套接字绑定失败！错误码 %d",WSAGetLastError());
        closesocket(this->socketListen);
        return false;
    }

    if(listen(this->socketListen, MAX_QUEUE_CLIENT) == SOCKET_ERROR)
    {
        qDebug("监听套接字监听失败！错误码 %d",WSAGetLastError());
        closesocket(this->socketListen);
        return false;
    }

    return true;
}

bool ChatServer::initIOCP()
{
    for(int i = 0; i < MAX_THREAD_NUM; i++) {
           hThreads[i] = INVALID_HANDLE_VALUE;
    }
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
    if(hIOCP == NULL)
    {
         qDebug("创建hIOCP失败！错误码 %d",WSAGetLastError());
         this->closeAll();
         return false;
    }
    for(int i = 0;i < MAX_THREAD_NUM; i++)
    {
        unsigned int threadID = 0;
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, (PTHREAD_START)WorkerThread,this,0,&threadID);
        if(hThread == NULL)
        {
            qDebug("创建线程失败! 错误码 %d",WSAGetLastError());
            this->closeAll();
            return false;
        }
    }
    return true;
}

void ChatServer::closeAll()
{
    if(hIOCP != NULL)
    {
        for(int i=0; i<MAX_THREAD_NUM; i++)
            PostQueuedCompletionStatus(hIOCP, 0, EXIT_CODE, NULL); //通知线程结束
    }
    WaitForMultipleObjects(MAX_THREAD_NUM, this->hThreads, true, INFINITE); //等待线程完全结束
    for(int i = 0; i < MAX_THREAD_NUM; i++)
        CloseHandle(this->hThreads[i]);


    EnterCriticalSection(&this->mutexConnList);
    for(auto it = this->connList.begin(); it != this->connList.end(); it++)
    {
        closesocket((*it)->socket);
        delete (*it);
    }
    this->connList.clear();
    LeaveCriticalSection(&this->mutexConnList);

    if(hIOCP != NULL)
    {
        CloseHandle(this->hIOCP);
        hIOCP = NULL;
    }

    if(this->socketListen != INVALID_SOCKET)
    {
        closesocket(this->socketListen);
        this->socketListen = INVALID_SOCKET;
    }
    DeleteCriticalSection(&this->mutexConnList);
    this->state = false;
    qDebug()<<"关闭完全";
}

void ChatServer::addEXP_OVERLAPPED(EXP_OVERLAPPED *exp_ol)
{
    EnterCriticalSection(&this->mutexConnList);
    this->connList.push_back(exp_ol);
    LeaveCriticalSection(&this->mutexConnList);
}

void ChatServer::removeEXP_OVERLAPPED(EXP_OVERLAPPED *exp_ol)
{
    EnterCriticalSection(&this->mutexConnList);
    for(auto it=this->connList.begin(); it != this->connList.end(); it++)
    {
        if((*it) == exp_ol){
            this->connList.erase(it);
            break;
        }
    }
    LeaveCriticalSection(&this->mutexConnList);
}

ChatServer::~ChatServer()
{
    qDebug()<<"析构调用";
    if(this->state == true)
    {
        this->closeAll();
    }
    //WSACleanup();
}

void ChatServer::start()
{
    qDebug("服务器启动中...");
    sockaddr_in sock;
    int socksize = sizeof (sock);

    while(true)
    {
        SOCKET acceptSocket = accept(this->socketListen, (sockaddr *) &sock, &socksize);
        if(acceptSocket == INVALID_SOCKET)
        {
            qDebug("创建已连接套接字失败! %d",WSAGetLastError());
            this->closeAll();
            break;
        }
        qDebug("连接了一个套接字");
        hIOCP = CreateIoCompletionPort((HANDLE)acceptSocket, hIOCP, 0, 0);
        EXP_OVERLAPPED* exp_ol = new EXP_OVERLAPPED(acceptSocket);
        this->addEXP_OVERLAPPED(exp_ol);
        exp_ol->lastSendOrRecv = true;
        DWORD flag = 0;
        if(WSARecv(acceptSocket, &exp_ol->wsaBuffer, 1, NULL, &flag, (LPWSAOVERLAPPED)exp_ol, NULL) == SOCKET_ERROR)
        {
             int lastErrorCode = WSAGetLastError();
             if(lastErrorCode != ERROR_IO_PENDING)
             {
                 qDebug("第一个Recv套接字失败! %d",lastErrorCode);
                 this->removeErrorEXP_OVERLAPPED(exp_ol);
             }
        }
    }
}

void ChatServer::removeErrorEXP_OVERLAPPED(EXP_OVERLAPPED *exp_ol)
{
    if(closesocket(exp_ol->socket) == SOCKET_ERROR)
        qDebug("关闭套接字错误");
    else
        qDebug("关闭套接字成功");

    removeEXP_OVERLAPPED(exp_ol);
    delete exp_ol;
}

void ChatServer::slot_startServer()
{
    if(!this->state)
    {
        if(initListenSocket())
        {
            if(this->initIOCP())
            {
                this->state = true;
                emit signal_ErrorMsg(1);
                this->start();
            }else
             {
                 qDebug("错误");
                 this->closeAll();
                 emit signal_ErrorMsg(0);
            }

        }else
         {
            qDebug("错误");
            this->closeAll();
            emit signal_ErrorMsg(0);
        }

    }else{
        qDebug()<<"已启动chatServer";
        return;

    }

}

void ChatServer::initConnParam(ConnectParam &connParm)
{
    this->connParam.port = connParm.port;
    this->connParam.bind_addr.S_un.S_addr = connParm.bind_addr.S_un.S_addr;
}
