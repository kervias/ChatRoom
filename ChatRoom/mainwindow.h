#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTextEdit>
#include <QScrollBar>
#include "textmsgutil.h"
#include <QMessageBox>
#include <QEvent>
#include <QMutex>
#include "models.h"
#include <QDateTime>
#include "uploadutil.h"
#include "fileitem.h"
#include "downloadutil.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


extern bool bStopUpload;
extern QMutex mutexUpload;



class MainWindow : public QMainWindow
{
    Q_OBJECT
    QThread TextMsgThread;   //消息通信 UDP
    QThread UploadThread;   //上传文件 TCP
    QThread DownloadThread; //下载文件 TCP
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
    void closeAllConnect(); //关闭所有连接

    void initWidget();  //初始化窗口样式
    void initThread();

    void refreshLAN();

private slots:
    void on_sendButton_released();

    void on_enterButton_released();

    void on_refreshButton_released();


    void slot_RecvMsg(int type, MYMSG &mymsg);
    void slot_Error(QString errorMsg, int errorCode);
    void slot_curdPerson(int type, Person &per);
    void slot_addFileMsg(FileMsg fileMsg);

    void slot_uploadFinished(int type); //文件传输完毕信号
    void slot_updateProgress(unsigned short t);

    void on_pushButton_released();

    void slot_startDownload(int type, FileItem *fileItem, QString filename, int filesize); //下载文件
    void slot_downloadFinished(int type); //下载完毕


signals:
    void signal_CommonMsg(int type, QString msg);
    void signal_RecvMsg(WPARAM w, LPARAM l);
    void signal_RecvMsg_ASSIST(WPARAM w, LPARAM l);

    // UploadUtil
    void signal_startUpload(QString filepath);
    void signal_startDownload(QString filepath, int filesize);


private:
    Ui::MainWindow *ui;
    TextMsgUtil *textMsgUtil;
    UploadUtil *uploadUtil;
    DownloadUtil *downloadUtil;


    bool state; //连接状态
    QVector<IP_MASK> ip_maskVec;
    QDateTime lastTime;

    bool stateUpload;
    bool stateDownload;


    FileItem* currDownloadItem;

};
#endif // MAINWINDOW_H
