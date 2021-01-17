#include "downloadutil.h"
#include <QDebug>
#include <QDir>


DownloadUtil::DownloadUtil(QObject *parent) : QObject(parent)
{
    QString folderpath = "./Download";
    QDir dir;
    this->downloadSocket = INVALID_SOCKET;
    if(!dir.exists(folderpath))
        dir.mkpath(folderpath);
}


bool DownloadUtil::initSocket(ConnectParam &conn)
{

    this->connParam.bind_addr = conn.bind_addr;
    this->connParam.server_ip = conn.server_ip;
    this->connParam.server_port = conn.server_port;

    // 创建套接字
    if((downloadSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        emit signal_Error("downloadSocket创建失败",  WSAGetLastError());
        //qDebug("downloadSocket创建失败 -- ERROCODE:%d\n", WSAGetLastError());
        return false;
    }

    // 绑定ip地址
    sockaddr_in addr;
    memset((void *) &addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    addr.sin_addr.s_addr =this->connParam.bind_addr.s_addr;
    if(bind(downloadSocket, (sockaddr *)&addr, sizeof(addr)) != 0)
    {
        emit signal_Error("downloadSocket地址绑定失败",  WSAGetLastError());
        //qDebug("downloadSocket地址绑定失败 -- ERROCODE:%d\n", WSAGetLastError());
        closesocket(downloadSocket);
        return false;
    }

    // 与服务器连接
    memset((void *) &addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = this->connParam.server_ip.s_addr;
    addr.sin_port = htons(this->connParam.server_port);
    if(::connect(downloadSocket, (sockaddr *)&addr, sizeof(addr)) != 0)
    {
        emit signal_Error("downloadSocket与服务器连接失败",  WSAGetLastError());
        //qDebug("downloadSocket与服务器连接失败 -- ERROCODE:%d\n", WSAGetLastError());
        closesocket(downloadSocket);
        return false;
    }
    qDebug()<<"与服务器连接成功";
    this->connState = true;
    return true;
}


DownloadUtil::~DownloadUtil()
{
    this->closeAll();
}

void DownloadUtil::closeAll()
{
    this->connState = false;
    if(this->downloadSocket == INVALID_SOCKET)
        return;
    if(closesocket(this->downloadSocket) == SOCKET_ERROR)
        qDebug()<<"closesocket错误";
    this->downloadSocket = INVALID_SOCKET;
}

void DownloadUtil::slot_startDownload(QString filename, int size)
{

    TRANS_DATA_BLOCK tdb;
    memset((void *) &tdb, 0, sizeof (tdb));
    tdb.msg = htons(4);
    tdb.pos = htonl(0);
    memcpy(tdb.dataBlock,filename.toLocal8Bit().constData(), 255);
    if(send(downloadSocket, (char *)&tdb, sizeof (tdb),0) < 0) //发送文件信息
    {
        qDebug("发送询问文件信息失败 ERROR: %d", WSAGetLastError());
        this->closeAll();
        return;
    }
    //qDebug()<<sizeof(tdb);
    if(recv(downloadSocket, (char *)&tdb, sizeof (tdb),0) < 0) //接收文件信息
    {
        qDebug("接收文件信息失败 ERROR: %d", WSAGetLastError());
        this->closeAll();
        return;
    }
    tdb.msg = ntohs(tdb.msg);
    tdb.len = ntohs(tdb.len);

    qDebug()<<filename<<" "<<filename.size();
    ofstream o_file(("./Download/"+filename).toStdString().data(), ios::binary);
    if(!o_file.is_open())
    {
        qDebug()<<"文件打开失败";
        emit signal_Error("download文件打开失败", -1);
        return;
    }

    int nRecvByte, nSendByte;
    u_long pos = 0;
    TRANS_DATA_BLOCK tdb2;
    unsigned short last_progress = 0, curr_progress;
    if(tdb.len == 0)
    {
        qDebug()<<"服务器不存在该文件";
    }else{

        while(true)
        {
            //qDebug()<<"--00--";
            tdb2.pos = htonl(pos);
            //tdb2.pos = pos;
            tdb2.msg = htons(6);
            nSendByte = send(downloadSocket, (char *)&tdb2, sizeof (TRANS_DATA_BLOCK),0);
            if(nSendByte < 0) //发送文件信息
            {
                qDebug("发送询问文件信息失败 ERROR: %d", WSAGetLastError());
                this->closeAll();
                return;
            }

            if(nSendByte != sizeof (TRANS_DATA_BLOCK))
                qDebug()<<"发送不完全";
            L:
            //qDebug()<<"--11--";
            nRecvByte = recv(downloadSocket, (char *)&tdb, sizeof (tdb),0);
            //qDebug()<<"--22--";
            if(nRecvByte < 0)
            {
                qDebug("接收文件信息失败 ERROR: %d", WSAGetLastError());
                this->closeAll();
                return;
            }
            if(nRecvByte == sizeof (tdb))
            {
                tdb.len = ntohs(tdb.len);
                tdb.pos = ntohl(tdb.pos);
                //qDebug()<<tdb.pos;
                tdb.msg = ntohs(tdb.msg);
                o_file.seekp(tdb.pos);
                if(tdb.msg == 6)
                {
                    o_file.write(tdb.dataBlock, tdb.len);
                    o_file.flush();
                }else if(tdb.msg == 3)
                {
                    qDebug()<<"收到最后分片";
                    o_file.write(tdb.dataBlock, tdb.len);
                    o_file.flush();
                    o_file.close();
                    break;
                }else{
                    qDebug()<<"未知消息";
                }
            }else{
                qDebug()<<"接收不完全";
                break;
            }
            if(tdb.pos != pos)
            {
                qDebug()<<"重复"<<tdb.pos<<"  "<<pos;
                goto L;
            }
            else
            {
                //qDebug()<<pos;
                 pos += (u_long)tdb.len;
                 curr_progress = (pos/double(size*1024))*100;
                 if(curr_progress > last_progress || last_progress == 0)
                 {
                     last_progress = curr_progress;
                     emit signal_updateProgress(curr_progress);
                 }

            }

        }

    }
    //tdb2.pos = htonl(pos);
    tdb2.msg = htons(7);
    nSendByte = send(downloadSocket, (char *)&tdb2, sizeof (tdb2),0);
    if(nSendByte < 0) //发送文件信息
    {
        qDebug("发送结束传输信息失败 ERROR: %d", WSAGetLastError());
        this->closeAll();
        return;
    }
    QFile::rename(filename, "./Download/"+QString(filename));
    emit signal_downloadFinished(1);
}

