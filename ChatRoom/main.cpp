#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    WSADATA wsaData;
    WORD wVersionReq = MAKEWORD(2, 2);
    if (WSAStartup(wVersionReq, &wsaData) != 0)
    {
        int error = WSAGetLastError(); //错误代码
        qDebug("WSAStartUp失败! ERRORCODE %d",error);
        WSACleanup();
        return 0;
    }
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    a.exec();
    WSACleanup();
    return 0;
}
