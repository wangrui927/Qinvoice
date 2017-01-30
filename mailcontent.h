#ifndef MAILCONTENT_H
#define MAILCONTENT_H

#include <QDialog>

namespace Ui {
class MailContent;
}

class MailContent : public QDialog
{
    Q_OBJECT

public:
    explicit MailContent(QWidget *parent = 0);
    ~MailContent();
    void setMailInfos(const QString &text,QString receiver);
    QString getMailText(void);

private:
    Ui::MailContent *ui;
    QString Mailtext;

private slots:
    void updateMailText(void);
};

#endif // MAILCONTENT_H
