#ifndef DOWNLOADUTIL_H
#define DOWNLOADUTIL_H

#include <QObject>
#include <winsock2.h>
#include <QThread>
#include <fstream>
#include "models.h"
using namespace std;

//#include <QMutex>
//extern bool bStopUpload;
//extern QMutex mutexUpload;

class DownloadUtil : public QObject
{
    Q_OBJECT
public:
    explicit DownloadUtil(QObject *parent = nullptr);
    ~DownloadUtil();
    bool initSocket(ConnectParam &conn);
    void closeAll();

public slots:
    void slot_startDownload(QString filename, int size); //文件传输函数


signals:
    void signal_downloadFinished(int type); //文件传输完毕信号
    void signal_updateProgress(unsigned short t); //更新进度条信号
    void signal_Error(QString errorMsg, int errorCode);

private:
    SOCKET downloadSocket;
    bool connState;
    ConnectParam connParam;
};

#endif // DOWNLOADUTIL_H
