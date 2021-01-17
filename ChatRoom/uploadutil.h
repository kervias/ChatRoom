#ifndef UPLOADUTIL_H
#define UPLOADUTIL_H

#include <QObject>
#include <winsock2.h>
#include <QThread>
#include <fstream>
#include <QMutex>
#include "models.h"
using namespace std;


extern bool bStopUpload;
extern QMutex mutexUpload;


// 上传文件组件，单独线程传输
class UploadUtil : public QObject
{
    Q_OBJECT
public:
    explicit UploadUtil(QObject *parent = nullptr);
    ~UploadUtil();
    bool initSocket(ConnectParam &conn);
    void closeAll();

    QByteArray getMD5(QString filepath); //MD5值用于文件传输验证


public slots:
    void slot_startUpload(QString filepath); //文件传输函数



signals:
    void signal_uploadFinished(int type); //文件传输完毕信号
    void signal_updateProgress(unsigned short t);
    void signal_Error(QString errorMsg, int errorCode);

private:
    bool connState; //连接状态
    SOCKET uploadSocket;
    ConnectParam connParam;
    QString filePath;
    QByteArray md5;
};

#endif // UPLOADUTIL_H
