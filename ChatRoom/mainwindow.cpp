#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>

#include <QStringList>
#include <QFontDatabase>
#include <QFont>
#include <QDateTime>
#include <QTextCursor>
#include <QScrollBar>
#include <QListWidget>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->initWidget();
    this->initThread();

    this->stateUpload = false;
    this->stateDownload = false;
    this->state = false;
    this->refreshLAN();

}

MainWindow::~MainWindow()
{
    if(this->TextMsgThread.isRunning())
    {
        this->TextMsgThread.quit();
        this->TextMsgThread.wait();
    }
    if(this->UploadThread.isRunning())
    {
        this->UploadThread.quit();
        this->UploadThread.wait();
    }
    if(this->DownloadThread.isRunning())
    {
        this->DownloadThread.quit();
        this->DownloadThread.wait();
    }
    delete ui;
}

void MainWindow::initThread()
{
    textMsgUtil = new TextMsgUtil();
    textMsgUtil->setHWind(this->winId());
    textMsgUtil->moveToThread(&TextMsgThread);

    connect(&TextMsgThread, &QThread::finished, textMsgUtil, &QObject::deleteLater);
    connect(this->textMsgUtil, &TextMsgUtil::signal_RecvMsg_TOMain, this, &MainWindow::slot_RecvMsg);
    connect(this->textMsgUtil, &TextMsgUtil::signal_Error_TOMain, this, &MainWindow::slot_Error);
    connect(this->textMsgUtil, &TextMsgUtil::signal_curdPerson_TOMain, this, &MainWindow::slot_curdPerson);
    connect(this, &MainWindow::signal_CommonMsg, textMsgUtil, &TextMsgUtil::slot_CommonMsg);
    connect(this, &MainWindow::signal_RecvMsg, textMsgUtil, &TextMsgUtil::slot_RecvMsg);
    connect(this, &MainWindow::signal_RecvMsg_ASSIST, textMsgUtil, &TextMsgUtil::slot_RecvMsg_ASSIST);
    connect(this->textMsgUtil, &TextMsgUtil::signal_addFilemsg, this, &MainWindow::slot_addFileMsg);
    TextMsgThread.start();

    uploadUtil = new UploadUtil();
    uploadUtil->moveToThread(&UploadThread);
    connect(&UploadThread, &QThread::finished, uploadUtil, &QObject::deleteLater);
    connect(this, &MainWindow::signal_startUpload, uploadUtil, &UploadUtil::slot_startUpload); //开始下上传信号
    connect(uploadUtil, &UploadUtil::signal_uploadFinished, this, &MainWindow::slot_uploadFinished); //上传完毕信号
    connect(uploadUtil, &UploadUtil::signal_updateProgress, this, &MainWindow::slot_updateProgress);
    connect(uploadUtil, &UploadUtil::signal_Error, this, &MainWindow::slot_Error);
    UploadThread.start();

    downloadUtil = new DownloadUtil();
    downloadUtil->moveToThread(&DownloadThread);
    connect(&DownloadThread, &QThread::finished, downloadUtil, &QObject::deleteLater);
    connect(downloadUtil, &DownloadUtil::signal_downloadFinished, this, &MainWindow::slot_downloadFinished);
    connect(downloadUtil, &DownloadUtil::signal_Error, this, &MainWindow::slot_Error);
    connect(this, &MainWindow::signal_startDownload, downloadUtil, &DownloadUtil::slot_startDownload);
    DownloadThread.start();
}



void MainWindow::on_sendButton_released()
{
    QString msg = ui->textEdit->toPlainText();

    if (this->state == true && msg.size() > 0){
        ui->textEdit->setText("");
        ui->textBrowser->append(QString("<p style='font-family: 楷体;font-size: 20px; text-align:left'><span style='color:deepskyblue'>☆&nbsp;</span>%2&nbsp;[%1] <span style='color:black'>&nbsp;%3</span></p>").arg(inet_ntoa(this->textMsgUtil->getParam().bind_addr), this->textMsgUtil->getParam().username,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm")));
        // ui->textBrowser->append(QString("<p style='font-family: 楷体;font-size: 20px; text-align:left'><span style='color:deepskyblue'>☆&nbsp;</span>%2&nbsp;[%1]</p>").arg(inet_ntoa(this->textMsgUtil->getParam().bind_addr), this->textMsgUtil->getParam().username));
        ui->textBrowser->append(msg);
        ui->textBrowser->append("");
        emit signal_CommonMsg(0, msg);
    }else if(this->state == false){
        QMessageBox::critical(this, "ERROR", "当前未连接, 请先连接进入聊天室",QMessageBox::Ok);
    }else{
        QMessageBox::critical(this, "ERROR", "聊天内容为空, 请输入聊天内容",QMessageBox::Ok);
    }
}

void MainWindow::on_enterButton_released()
{
    if(this->state == true)  //关闭
    {
        this->closeAllConnect();
        return;
    }

    // 连接
    // 检查用户输入合法性
    if(ui->comboBox->currentText() == "")
    {
        QMessageBox::warning(this, "无网络连接","当前计算机并未在任何局域网内");
        return;
    }


    // 1. 检查端口范围是否在0-65534之内
    if(ui->lineEdit_2->text().toInt() > 65534 || ui->lineEdit_5->text().toInt() > 65535)
    {
        QMessageBox::warning(this, "端口输入错误", "请输入范围在0-65534范围内的端口号", QMessageBox::Ok);
        return;
    }
    //qDebug("端口%d", c->port);
    // 2. 检查用户名字节范围
    ConnectParam *c = new ConnectParam;
    c->bind_addr.S_un.S_addr = inet_addr(ui->comboBox->currentText().toStdString().data());

    c->port = ui->lineEdit_2->text().toUShort();
    c->username = ui->lineEdit_4->text();
    if(c->username.toUtf8().size() > 30 || c->username == "")
    {
        QMessageBox::warning(this, "用户名长度限制", "请确保用户名长度在1-30字节之内", QMessageBox::Ok);
        return;
    }
     //qDebug("范围 %d", c->username.toUtf8().size());
    // 3. 检查房间密钥字节范围

    for(int i = 0; i < this->ip_maskVec.size(); i++)
    {
        if(ip_maskVec[i].addr.S_un.S_addr == c->bind_addr.S_un.S_addr)
        {
            c->broadcast_addr.S_un.S_addr = ip_maskVec[i].addr.S_un.S_addr & ip_maskVec[i].subnet_mask.S_un.S_addr;
            c->broadcast_addr.S_un.S_addr = c->broadcast_addr.S_un.S_addr | (ip_maskVec[i].subnet_mask.S_un.S_addr ^ 0xFFFFFFFF);
        }
    }
    c->server_port = ui->lineEdit_5->text().toUShort(); //服务器UDP端口
    c->server_ip.S_un.S_addr = inet_addr(ui->lineEdit->text().toStdString().data()); //服务器IP地址
    if(this->textMsgUtil->initAll(c))
    {
        this->state = true;
        ui->enterButton->setText("退出");
        ui->statusBar->showMessage("已进入聊天室");

        Person per;
        per.ip.S_un.S_addr = c->bind_addr.S_un.S_addr;
        per.username = c->username;
        this->slot_curdPerson(0,per);
        ConnectParam connParam;

        connParam.server_ip.S_un.S_addr = inet_addr(ui->lineEdit->text().toStdString().data());//服务器ip地址
        connParam.server_port = ui->lineEdit_5->text().toUShort(); //服务器端口
        connParam.bind_addr.S_un.S_addr =  c->bind_addr.s_addr; //本地地址

        if(this->uploadUtil->initSocket(connParam) && this->downloadUtil->initSocket(connParam))
        {            
            ui->tabWidget->setCurrentIndex(1);
            this->textMsgUtil->sendInitAssitMsg();
            QMessageBox::information(this, "消息", "连接成功");

        }else{
            this->closeAllConnect();
        }

    }else{
        this->closeAllConnect();
    }
}


void MainWindow::closeAllConnect()
{
    if(stateUpload == true)
    {
        mutexUpload.lock();
        bStopUpload = true;
        mutexUpload.unlock();
        stateUpload = false;
    }

    this->uploadUtil->closeAll();
    this->downloadUtil->closeAll();
    this->textMsgUtil->closeAll();

    this->stateUpload = false;
    this->stateDownload = false;
    this->state = false;
    ui->enterButton->setText("连接");
    ui->statusBar->showMessage("未进入聊天室");
    ui->listWidget->clear();
    ui->fileList->clear();
    ui->textBrowser->clear();
    ui->textBrowser->clearHistory();
    ui->progressBar->setValue(0);
    this->refreshLAN();

}

void MainWindow::slot_RecvMsg(int type, MYMSG &mymsg)
{
    qDebug()<<"收到消息";
    if(type == 1)
    {
         ui->textBrowser->append(QString("<p style='font-family: 楷体;font-size: 20px; text-align:left'><span style='color:orange'>☆&nbsp;</span>%2&nbsp;[%1] &nbsp; %3</p>").arg(inet_ntoa(mymsg.ip), mymsg.username,QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm")));
         ui->textBrowser->append(mymsg.msg);
         ui->textBrowser->append("");
    }
}


void MainWindow::slot_Error(QString errorMsg, int errorCode)
{
    QMessageBox::critical(this, QString("Error: %1").arg(errorCode), errorMsg,QMessageBox::Ok);
    this->closeAllConnect();
}


void MainWindow::refreshLAN()
{
    this->ip_maskVec.clear();
    textMsgUtil->getLANInfo(this->ip_maskVec);
    ui->comboBox->clear();
    for(int i = this->ip_maskVec.size()-1; i >= 0; i--)
    {
        ui->comboBox->addItem(QString(inet_ntoa(this->ip_maskVec[i].addr)));
    }
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if(this->state == true)
    {
        if(QMessageBox::question(this, "关闭软件", "当前仍处于连接状态，您确认要关闭软件吗？") == QMessageBox::Yes)
        {
            this->closeAllConnect();
            Sleep(500);
            event->accept();
        }
        else {
            event->ignore();
        }
    }
}



void MainWindow::on_refreshButton_released()
{
    this->refreshLAN();
}

void MainWindow::slot_curdPerson(int type, Person &per)
{
    switch (type)
    {
        case 0:
        {
            QListWidgetItem *item = new QListWidgetItem(QIcon(":/imgs/3.png"),per.username);
            item->setToolTip(inet_ntoa(per.ip));
            ui->listWidget->addItem(item);
            //ui->statusBar->showMessage(QString("已进入聊天室 - Info: 用户[username: %1, ip: %2]上线 ").arg(per.username, inet_ntoa(per.ip)));
            break;
        }
        case 1:
        {
            int flag = -1;
            for(int i = 0; i < ui->listWidget->count(); i++)
            {
                if(ui->listWidget->item(i)->toolTip() == QString(inet_ntoa(per.ip)))
                {
                    flag = i;
                    break;
                }
            }
            if(flag != -1)
            {
                ui->listWidget->takeItem(flag);
                //ui->statusBar->showMessage(QString("已进入聊天室 - Info: 用户[username: %1, ip: %2]下线 ").arg(per.username, inet_ntoa(per.ip)));
            }
            break;
        }
    }
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
    //qDebug()<<"键盘start";
   if(event->key() == Qt::Key_Return && event->modifiers() == Qt::ControlModifier)
   {
       this->on_sendButton_released();
   }
   //qDebug()<<"键盘stop";
}


bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{

    MSG* msg = (MSG*)message;

    switch (msg->message)
    {

        case MSG_01:
        {
            //qDebug()<<"MSG01";
            emit signal_RecvMsg(msg->wParam, msg->lParam);
            break;
        }
        case MSG_02:
        {
            //qDebug()<<"MSG02";
            emit signal_RecvMsg_ASSIST(msg->wParam, msg->lParam);
            break;
        }
        default:
           return QMainWindow::nativeEvent(eventType, message, result);

    }

    return true;
}


void MainWindow::initWidget()
{
    ui->splitter_1->setStretchFactor(0,8);
    ui->splitter_1->setStretchFactor(1,2);
    ui->splitter_2->setStretchFactor(0,8);
    ui->splitter_2->setStretchFactor(1,3);

    this->setWindowTitle("局域网聊天室");
    this->setStyleSheet("QMainWindow#MainWindow,QMessageBox{background-image:url(:/imgs/bg02.jpg)}");
    this->setWindowIcon(QIcon(":/imgs/chat.png"));

    ui->tabWidget->setTabText(0, "设置");
    ui->tabWidget->setTabText(1,"在线好友");
    ui->statusBar->showMessage("未进入聊天室");
    ui->enterButton->setText("连接");
    //ui->textEdit->setAcceptRichText(true);
    ui->textBrowser->verticalScrollBar()->setStyleSheet("QScrollBar:vertical{width:11px;background:rgba(0,0,0,0%);margin:0px,0px,0px,0px;padding-top:9px;padding-bottom:9px;}"
                                                    "QScrollBar::handle:vertical{width:8px;background:rgba(0,0,0,25%);border-radius:4px;}"
                                                    );
    ui->textEdit->verticalScrollBar()->setStyleSheet("QScrollBar:vertical{width:11px;background:rgba(0,0,0,0%);margin:0px,0px,0px,0px;padding-top:9px;padding-bottom:9px;}"
                                                     "QScrollBar::handle:vertical{width:8px;background:rgba(0,0,0,25%);border-radius:4px;}"
                                                     );

    ui->comboBox->setStyleSheet("border: 1px solid black");
    ui->listWidget->verticalScrollBar()->setStyleSheet("QScrollBar:vertical{width:11px;background:rgba(0,0,0,0%);margin:0px,0px,0px,0px;padding-top:9px;padding-bottom:9px;}"
                                                        "QScrollBar::handle:vertical{width:8px;background:rgba(0,0,0,25%);border-radius:4px;}"
                                                        );
    ui->listWidget->setStyleSheet("QListWidget:Item{padding: 10px 20px;}");

    ui->tabWidget->setCurrentIndex(0);
    ui->tabWidget->setTabIcon(0,QIcon(":/imgs/2.png"));
    ui->tabWidget->setTabIcon(1,QIcon(":/imgs/1.png"));
    ui->tabWidget->setTabIcon(2, QIcon(":/imgs/folder.png"));

    ui->lineEdit_2->setValidator(new QIntValidator(0,65534,this));
    ui->lineEdit_5->setValidator(new QIntValidator(0,65535,this));
    ui->lineEdit->setValidator(new QRegExpValidator(QRegExp("(((\\d{1,2})|(1\\d{2})|(2[0-4]\\d)|(25[0-5]))\\.){3}((\\d{1,2})|(1\\d{2})|(2[0-4]\\d)|(25[0-5]))"), this));
    ui->lineEdit_4->setMaxLength(30);
    ui->textEdit->setPlaceholderText("请在此输入消息");
    ui->lineEdit_2->setPlaceholderText("请输入端口号");
    ui->lineEdit_4->setPlaceholderText("请输入用户名");
    ui->lineEdit_4->setText("");

    ui->progressBar->setValue(0);
    ui->progressBar->hide();
    ui->pushButton->setText("上传文件");

    qRegisterMetaType<WPARAM>("WPARAM");
    qRegisterMetaType<WPARAM>("WPARAM&");
    qRegisterMetaType<LPARAM>("LPARAM");
    qRegisterMetaType<LPARAM>("LPARAM&");

    qRegisterMetaType<Person>("Person");
    qRegisterMetaType<Person>("Person&");
    qRegisterMetaType<MYMSG>("MYMSG");
    qRegisterMetaType<MYMSG>("MYMSG&");

    qRegisterMetaType<FileMsg>("FileMsg");
    qRegisterMetaType<FileMsg>("FileMsg&");
}


void MainWindow::slot_updateProgress(unsigned short t)
{
    ui->progressBar->setValue(t);
}

void MainWindow::slot_uploadFinished(int type)
{
    if(type == 1)
    {
        ui->progressBar->setValue(0);
        ui->progressBar->hide();
        ui->pushButton->setText("上传文件");
        this->stateUpload = false;
        QMessageBox msg;
        msg.information(this,"消息","文件上传成功");
    }

}

void MainWindow::on_pushButton_released()  //上传按钮
{
    if(state == false)
    {
        QMessageBox::warning(this, "警告", "请先连接再上传文件");
        return;
    }
    if(this->stateUpload == false)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"),"./", tr("All files (*.*)"));
        if(fileName.isEmpty())
            return;
        emit signal_startUpload(fileName);
        ui->pushButton->setText("取消上传");
        ui->progressBar->show();
        this->stateUpload = true;
    }else{  //取消上传
        mutexUpload.lock();
        bStopUpload = true;
        mutexUpload.unlock();
        stateUpload = false;
        ui->progressBar->hide();
        ui->progressBar->setValue(0);
        ui->pushButton->setText("上传文件");
    }

}

void MainWindow::slot_addFileMsg(FileMsg fileMsg) //添加文件
{
    QListWidgetItem *ITEM = new QListWidgetItem();
    FileItem* widget1 = new FileItem(ui->fileList);
    widget1->initAll(fileMsg.filename, fileMsg.filesize);
    ui->fileList->addItem(ITEM);
    ui->fileList->setItemWidget(ITEM, widget1);
    ITEM->setSizeHint(QSize(100,65));
    connect(widget1, &FileItem::signal_startDownload, this, &MainWindow::slot_startDownload);
}

void MainWindow::slot_startDownload(int type, FileItem *fileItem, QString filename, int filesize)
{
    if(this->state == false)
    {
        QMessageBox::warning(this, "警告", "请连接后再下载文件");
        return;
    }

    if(this->stateDownload == false)
    {
        if(type == 1) //下载
        {
            this->currDownloadItem = fileItem;
            this->stateDownload = true;
            connect(this->downloadUtil, &DownloadUtil::signal_updateProgress, fileItem, &FileItem::slot_updateProgress);
            emit signal_startDownload(filename, filesize);
        }
    }else{
        if(fileItem == this->currDownloadItem)
        {
            if(type == 2) //取消
            {
                        //待定
            }

        }else
        {
            QMessageBox::warning(this,"警告","请先等待其他文件下载完成");
//            fileItem->reset();
        }

    }
}


void MainWindow::slot_downloadFinished(int type)
{
    if(this->state == false || this->stateDownload == false)
        return;

    if(type == 1)
    {
        this->currDownloadItem->slot_downloadFinished();
        disconnect(this->downloadUtil, &DownloadUtil::signal_updateProgress, this->currDownloadItem, &FileItem::slot_updateProgress);
        disconnect(this->currDownloadItem, &FileItem::signal_startDownload, this, &MainWindow::slot_startDownload);
        this->currDownloadItem = nullptr;
        this->stateDownload = false;
    }
}
