#ifndef FILEITEM_H
#define FILEITEM_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

class FileItem : public QWidget
{
    Q_OBJECT
public:
    explicit FileItem(QWidget *parent = nullptr);
    void initAll(QString filename, int filesize);

    void reset(); //重置为初始状态，因为有其他文件在下载

signals:

    void signal_startDownload(int type, FileItem *fileItem, QString filename, int filesize); //type=1: 开始下载 type=2:取消下载

public slots:
    void slot_button_clicked();

    void slot_downloadFinished(); //下载完成

    void slot_updateProgress(unsigned short value); //更新进度条

private:
    QLabel *imgLabel;
    QLabel *filenameLabel;
    QLabel *filesizeLabel;
    QToolButton *toolButton;
    QProgressBar *progressBar;
    int state; //状态：state = 0: 未下载 state=1: 下载中 state=2: 下载完毕
    QString filename;
    int filesize;

};

#endif // FILEITEM_H
