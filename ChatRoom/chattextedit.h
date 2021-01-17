#ifndef CHATTEXTEDIT_H
#define CHATTEXTEDIT_H


#include <QWidget>
#include <QTextEdit>
#include <QMimeData>
#include <QFileInfo>
#include <QImageReader>
#include <QVector>
class ChatTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit ChatTextEdit(QWidget *parent = nullptr);
     //bool canInsertFromMimeData(const QMimeData *source) const override;
    //bool canInsertFromMimeData(const QMimeData* source) const override;

    void insertFromMimeData(const QMimeData* source) override;




signals:
     void signal_sendFile_ToOther(QString filepath); //信号发文件

//private:
//    QVector<QString> fileList;
//    QString msg; //需要发送的消息
};
#endif // CHATTEXTEDIT_H
