#include "chattextedit.h"

#include <QDebug>
#include <QMimeData>
#include <QMimeDatabase>

ChatTextEdit::ChatTextEdit(QWidget *parent) : QTextEdit(parent)
{
//    this->setAcceptRichText(true);
//    this->setAutoFormatting(QTextEdit::AutoAll);
}


void ChatTextEdit::insertFromMimeData(const QMimeData* source)
{
   if (source->hasUrls())
   {
       foreach (QUrl url, source->urls())
       {
           qDebug()<<url;
           qDebug()<<url.toLocalFile();
           emit signal_sendFile_ToOther(url.toLocalFile());
       }
   }
   else
   {
       QTextEdit::insertFromMimeData(source);
   }
}


