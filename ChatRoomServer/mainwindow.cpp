#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QThread>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    chatServer = nullptr;
    assistServer = nullptr;
    state_chatServer = false;
    state_assistServer = false;
    ui->lineEdit->setValidator(new QIntValidator(0,65535,this));
    ui->lineEdit_2->setValidator(new QIntValidator(0,65535,this));
    this->setWindowTitle("局域网聊天室服务器");

    refreshLAN();

    qRegisterMetaType<WPARAM>("WPARAM");
    qRegisterMetaType<WPARAM>("WPARAM&");
    qRegisterMetaType<LPARAM>("LPARAM");
    qRegisterMetaType<LPARAM>("LPARAM&");
    qRegisterMetaType<FileMsg>("FileMsg");
    qRegisterMetaType<FileMsg>("FileMsg&");
    qDebug()<<sizeof(TRANS_DATA_BLOCK);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_startButton_released()
{
    if(state_chatServer == true || state_assistServer == true)
    {
        QMessageBox::information(this, "信息","服务器已启动");
        return;
    }
    if(ui->lineEdit->text().toInt() > 65535 || ui->lineEdit_2->text().toInt() > 65535)
    {
        QMessageBox::warning(this, "警告","端口输入不合法");
        return;
    }

    this->getConnParam();
    if(chatServer == nullptr)
    {
        chatServer = new ChatServer();
        chatServer->initConnParam(this->connParam);
        chatServer->moveToThread(&FileThread);
        connect(&FileThread, &QThread::finished, chatServer, &QObject::deleteLater);
        connect(this, &MainWindow::signal_startServer, chatServer, &ChatServer::slot_startServer);
        connect(chatServer, &ChatServer::signal_ErrorMsg, this, &MainWindow::slot_ErrorMsg);
        //connect(this, &MainWindow::signal_stopServer, chatServer, &ChatServer::slot_stopServer);
        FileThread.start();
    }
    // 处理startServer();
    chatServer->initConnParam(this->connParam);
    emit signal_startServer();
}


void MainWindow::on_stopButton_released()
{
    if(state_chatServer == false || state_chatServer == false)
    {
        QMessageBox::information(this, "信息","服务器已经关闭");
        return;
    }

    SOCKET TEMP;
    TEMP = this->chatServer->socketListen;
    this->chatServer->socketListen = INVALID_SOCKET;
    closesocket(TEMP);
    state_chatServer = false;
    state_assistServer = false;

    this->assistServer->closeAll();
    QMessageBox::information(this, "信息","服务器关闭成功");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(state_chatServer == true || state_assistServer == true)
        this->on_stopButton_released();
    if(FileThread.isRunning())
    {
        FileThread.quit();
        FileThread.wait();
    }
    if(AssistThread.isRunning())
    {
        AssistThread.quit();
        AssistThread.wait();
    }
}


void MainWindow::getLANInfo(QVector<IP_MASK> &ip_maskVec)
{
    IP_ADAPTER_INFO  *pai = new IP_ADAPTER_INFO();
    unsigned long paiSize = sizeof(IP_ADAPTER_INFO);
    if(GetAdaptersInfo(pai, &paiSize) == ERROR_BUFFER_OVERFLOW) //防止申请的内存不够
    {
        pai = (IP_ADAPTER_INFO *)new byte[paiSize];
        GetAdaptersInfo(pai, &paiSize);
    }
    while(pai)  //遍历所有网卡
    {
        IP_ADDR_STRING *ipList = &pai->IpAddressList;
        while(ipList)  //遍历一个网卡中的IP
        {
            IP_MASK ip_mask;
            IP_ADDRESS_STRING ip = ipList->IpAddress;
            IP_MASK_STRING mask = ipList->IpMask;
            ipList = ipList->Next;
            ip_mask.addr.S_un.S_addr = inet_addr(ip.String);
            ip_mask.subnet_mask.S_un.S_addr = inet_addr(mask.String);
            if(ip_mask.addr.S_un.S_addr == 0)
                 continue;
            ip_maskVec.push_back(ip_mask);
        }
        pai = pai->Next;
    }

//    // 添加本地回环地址 127.0.0.1
//    IP_MASK ip_mask;
//    ip_mask.addr.S_un.S_addr = inet_addr("127.0.0.1");
//    ip_mask.subnet_mask.S_un.S_addr = inet_addr("255.0.0.0");
//    ip_maskVec.push_back(ip_mask);
}


bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG* msg = (MSG*)message;
    switch (msg->message)
    {
        case MSG_01:
        {
            emit signal_RecvMsg(msg->wParam, msg->lParam);
            break;
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::slot_ErrorMsg(int type)
{
    switch (type) {
        case 0: //ChatServer启动报错
        {
            QMessageBox::information(this, "信息","服务器启动失败");
            state_chatServer = false;
            this->refreshLAN();
            break;
        }
        case 1: //ChatServer成功
        {
            qDebug()<<"消息1";
            state_chatServer = true;
            if(assistServer == nullptr)
            {
                assistServer = new AssistServer();
                assistServer->setHWind(this->winId());
                assistServer->moveToThread(&AssistThread);
                connect(&AssistThread, &QThread::finished, assistServer, &QObject::deleteLater);
                connect(assistServer, &AssistServer::signal_ErrorMsg, this, &MainWindow::slot_ErrorMsg);
                connect(this, &MainWindow::signal_startAssistServer, assistServer,&AssistServer::slot_startAssistServer);
                connect(this, &MainWindow::signal_RecvMsg, assistServer, &AssistServer::slot_RecvMsg);
                connect(chatServer, &ChatServer::signal_addFileMsg,assistServer, &AssistServer::slot_addFileMsg);
                AssistThread.start();
            }
            assistServer->initConnParam(this->connParam);
            emit signal_startAssistServer();
            break;
        }
        case 2: //AssistServer启动报错
        {
            QMessageBox::information(this, "信息","服务器启动失败");
            this->refreshLAN();
            qDebug()<<"消息2";
            // 关闭ChatServer
            SOCKET TEMP;
            TEMP = this->chatServer->socketListen;
            this->chatServer->socketListen = INVALID_SOCKET;
            closesocket(TEMP);
            state_chatServer = false;
            state_assistServer = false;
            break;
        }
        case 3://AssistServer启动成功
        {
            qDebug()<<"消息3";
            state_assistServer = true;
            QMessageBox::information(this, "信息","服务器启动成功");
            break;
        }
    }
}

void MainWindow::refreshLAN()
{
    this->ip_maskVec.clear();
    this->getLANInfo(this->ip_maskVec);
    ui->comboBox->clear();
    for(int i = this->ip_maskVec.size()-1; i >= 0; i--)
    {
        ui->comboBox->addItem(QString(inet_ntoa(this->ip_maskVec[i].addr)));
    }
}

void MainWindow::getConnParam()
{
    if(ui->comboBox->currentText() == "")
    {
        QMessageBox::warning(this, "无网络连接","当前计算机并未在任何局域网内");
        return;
    }

    connParam.bind_addr.S_un.S_addr = inet_addr(ui->comboBox->currentText().toStdString().data());
    connParam.port = ui->lineEdit->text().toUShort();
    connParam.broadcast_port = ui->lineEdit_2->text().toUShort();

    for(int i = 0; i < this->ip_maskVec.size(); i++)
    {
        if(ip_maskVec[i].addr.S_un.S_addr == connParam.bind_addr.S_un.S_addr)
        {
            connParam.broadcast_addr.S_un.S_addr = ip_maskVec[i].addr.S_un.S_addr & ip_maskVec[i].subnet_mask.S_un.S_addr;
            connParam.broadcast_addr.S_un.S_addr = connParam.broadcast_addr.S_un.S_addr | (ip_maskVec[i].subnet_mask.S_un.S_addr ^ 0xFFFFFFFF);
        }
    }
}
