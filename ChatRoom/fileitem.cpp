#include "fileitem.h"
#include <QPixmap>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QDebug>

FileItem::FileItem(QWidget *parent) : QWidget(parent)
{
    state = 0;
}


void FileItem::initAll(QString filename, int filesize)
{
    this->filename = filename;
    this->filesize = filesize;

    QWidget *msgWidget = new QWidget(this);
    imgLabel = new QLabel(msgWidget);
    imgLabel->setObjectName("titleLabel");
    //filenameLabel->setMaximumHeight(30);
    //imgLabel->setTextInteractionFlags(Qt::TextSelectableByMouse); //设置标签可复制
    imgLabel->setText(this->filename);
    imgLabel->setScaledContents(true);
    imgLabel->setAlignment(Qt::AlignCenter);

    QImage* img=new QImage;
    if(!( img->load(":/imgs/fj.png") ) ) //加载图像
    {
        delete img;
        return;
    }
    imgLabel->setPixmap(QPixmap::fromImage(*img));
    imgLabel->setFixedSize(QSize(32,32));



    filenameLabel = new QLabel(msgWidget);
    filenameLabel->setObjectName("titleLabel");
    //filenameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse); //设置标签可复制
    filenameLabel->setText(this->filename);
    filenameLabel->setStyleSheet("QLabel{font-family: 微软雅黑;padding-left:4px}");
    filenameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    filesizeLabel = new QLabel(msgWidget);
    filesizeLabel->setObjectName("titleLabel");
    filesizeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    filesizeLabel->setStyleSheet("QLabel{font-family: 微软雅黑;padding-right:4px}");
    //filesizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse); //设置标签可复制
    filesizeLabel->setText(QString::number(this->filesize)+"KB");

    toolButton = new QToolButton(msgWidget);
    toolButton->setText("下载");
    toolButton->setStyleSheet("QToolButton{padding: 10px}");
    connect(toolButton, &QToolButton::clicked, this, &FileItem::slot_button_clicked);


    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setContentsMargins(0,0,0,0);
    hlayout->setMargin(3);
    hlayout->setSpacing(0);
    hlayout->addWidget(imgLabel);
    hlayout->addWidget(filenameLabel);
    hlayout->addWidget(filesizeLabel);
    hlayout->addWidget(toolButton);
    msgWidget->setLayout(hlayout);


    progressBar = new QProgressBar(this);
    progressBar->setMaximumHeight(20);
    progressBar->setValue(0);
    progressBar->hide();

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);
    vlayout->addWidget(msgWidget);
    vlayout->addWidget(progressBar);
    this->setLayout(vlayout);

}

void FileItem::slot_button_clicked()
{
    if(state == 0) //开始下载
    {
        emit signal_startDownload(1,this, this->filename, this->filesize);
        // 发送信号下载文件，信号参数：文件名
//        state = 1;
//        this->toolButton->setText("下载中");
//        this->progressBar->show();
//        this->progressBar->setValue(0);
    }else if(state == 1) //取消下载
    {
//        state = 0;
//        this->progressBar->hide();
//        this->progressBar->setValue(0);
//        this->toolButton->setText("下载");
//        emit signal_startDownload(2,this,this->filename, this->filesize);
    }else if(state == 2) //下载完毕
    {
        QDesktopServices::openUrl(QUrl("file:./Download/", QUrl::TolerantMode));
    }
}

void FileItem::slot_updateProgress(unsigned short value)
{
    state = 1;
    this->toolButton->setText("下载中");
    this->progressBar->show();
    this->progressBar->setValue(value);
}

void FileItem::slot_downloadFinished()
{
    this->state = 2;
    this->toolButton->setText("打开");
    this->progressBar->hide();
}

void FileItem::reset()
{
    qDebug()<<"reset";
    this->state = 0;
    this->progressBar->hide();
    this->progressBar->setValue(0);
    this->toolButton->setText("下载");
}

