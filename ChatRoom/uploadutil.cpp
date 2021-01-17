#include "uploadutil.h"
#include <QDebug>
#include <winsock2.h>
#include <QFile>
#include <QCryptographicHash>

#include <QFileInfo>
#include <string.h>
#include <tchar.h>

bool bStopUpload=false;
QMutex mutexUpload;

UploadUtil::UploadUtil(QObject *parent) : QObject(parent)
{
    this->connState = false;
    this->uploadSocket = INVALID_SOCKET;
}

bool UploadUtil::initSocket(ConnectParam &conn)
{
    this->connParam.username = conn.username;
    this->connParam.bind_addr.S_un.S_addr = conn.bind_addr.S_un.S_addr;
    this->connParam.server_ip.S_un.S_addr = conn.server_ip.S_un.S_addr;
    this->connParam.server_port = conn.server_port;

    // 创建套接字
    if((uploadSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        //qDebug("uploadSocket创建失败 -- ERROCODE:%d\n", WSAGetLastError());
        emit signal_Error("uploadSocket创建失败", WSAGetLastError());
        return false;
    }

    // 绑定ip地址
    sockaddr_in addr;
    memset((void *) &addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    addr.sin_addr.s_addr = this->connParam.bind_addr.s_addr;
//    qDebug()<<inet_ntoa(this->connParam.bind_addr);
    if(bind(uploadSocket, (sockaddr *)&addr, sizeof(addr)) != 0)
    {
        //qDebug("uploadSocket地址绑定失败 -- ERROCODE:%d\n", WSAGetLastError());
        emit signal_Error("uploadSocket地址绑定失败",WSAGetLastError());
        closesocket(uploadSocket);
        return false;
    }

    // 与服务器连接
    memset((void *) &addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = this->connParam.server_ip.s_addr;
    addr.sin_port = htons(this->connParam.server_port);
    if(::connect(uploadSocket, (sockaddr *)&addr, sizeof(addr)) != 0)
    {
        //qDebug("uploadSocket与服务器连接失败 -- ERROCODE:%d\n", WSAGetLastError());
        emit signal_Error("uploadSocket与服务器连接失败",WSAGetLastError());
        closesocket(uploadSocket);
        return false;
    }
    qDebug()<<"与服务器连接成功";
    this->connState = true;
    return true;
}


QByteArray UploadUtil::getMD5(QString filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QByteArray ba = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5);
    file.close();
    return ba;
}


void UploadUtil::slot_startUpload(QString filepath)
{
    //md5 = this->getMD5(filepath);
//    if(this->connState == false)
//    {
//        ConnectParam conn;
//        conn.key ="12345";
//        conn.port = 12112;
//        conn.username = "2121";
//        conn.bind_addr.S_un.S_addr = inet_addr("192.168.1.230");
//        this->initSocket(conn);
//    }
    mutexUpload.lock();
    bStopUpload = false;
    mutexUpload.unlock();
    QFileInfo file1(filepath);
    QString filename = file1.fileName();
    qDebug()<<filepath;

    ifstream in_file(filepath.toLocal8Bit().constData(), ios::binary);
    qDebug()<<filepath.toLocal8Bit().data();
    if(!in_file.is_open())
    {
        qDebug()<<"文件打开失败";
        return;
    }

    TRANS_DATA_BLOCK tdb;
    memset((void *) &tdb, 0, sizeof (tdb));
    tdb.msg = htons(1);
    memcpy(tdb.dataBlock,filename.toLocal8Bit().constData(), 255);
    tdb.len = htonl(file1.size());
    if(send(uploadSocket, (char *)&tdb, sizeof (tdb),0) < 0) //发送文件信息
    {
        qDebug("发送文件信息失败 ERROR: %d", WSAGetLastError());
        this->closeAll();
        return;
    }


    in_file.seekg(0, ios::beg);
    unsigned int t = 0;
    unsigned short last_progress = 0, now_progress;
    while(!in_file.eof())
    {
        mutexUpload.lock();
        if(bStopUpload == true)
        {
            mutexUpload.unlock();
            qDebug()<<"收到停止上传";
            // 发送取消传输消息
            tdb.msg = htons(0);
            tdb.len = htons(0);
            if(send(uploadSocket, (char *)&tdb, sizeof (tdb),0) < 0) //发送文件信息
            {
                qDebug("发送取消传输信息失败 ERROR: %d", WSAGetLastError());
                this->closeAll();
                in_file.close();
                return;
            }
            in_file.close();
            break;
        }else{
            mutexUpload.unlock();
        }
        //qDebug("111");
        in_file.read(tdb.dataBlock, UPLOAD_UNIT_LEN);
        tdb.msg = htons(2);
        tdb.len = htons(in_file.gcount());
        t += in_file.gcount();
        //qDebug()<<"11up";
        if(send(uploadSocket, (char *)&tdb, sizeof (tdb),0) < 0) //发送文件信息
        {
            qDebug("发送文件信息失败 ERROR: %d", WSAGetLastError());
            this->closeAll();
            in_file.close();
            return;
        }
        //qDebug()<<"22up";

        now_progress = (unsigned short)((t/double(file1.size()))*100);
        if(now_progress > last_progress)
        {
            emit signal_updateProgress(now_progress);
            last_progress = now_progress;
        }
    }
    if(bStopUpload == false)
    {
        tdb.msg = htons(3);
        if(send(uploadSocket, (char *)&tdb, sizeof (tdb),0) < 0)
        {
            qDebug("发送文件最后块失败 ERROR: %d", WSAGetLastError());
            this->closeAll();
        }
        in_file.close();
        emit signal_uploadFinished(1);
    }else
        emit signal_uploadFinished(0);
    mutexUpload.lock();
    bStopUpload = false;
    mutexUpload.unlock();

}

UploadUtil::~UploadUtil()
{
    this->closeAll();
}

void UploadUtil::closeAll()
{
    this->connState = false;
    if(this->uploadSocket == INVALID_SOCKET)
        return;
    if(closesocket(this->uploadSocket) == SOCKET_ERROR)
        qDebug()<<"closesocket错误 upload";
    this->uploadSocket = INVALID_SOCKET;
}


