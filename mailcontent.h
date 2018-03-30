#ifndef MAILCONTENT_H
#define MAILCONTENT_H

#include <QDialog>
#include <QFileInfo>


namespace Ui {
class MailContent;
}

class MailContent : public QDialog
{
    Q_OBJECT

public:
    explicit MailContent(QWidget *parent = 0);
    ~MailContent();
    void setMailInfos(const QString &text, QString receiver, QString Subject, QString AttachmentPath);
    QString getMailText(void);
    QString getSubjectContent(void);

private:
    Ui::MailContent *ui;
    QString Mailtext, SubjectContent, PathToAttachment;

private slots:
    void updateMailText(void);
    void updateSubjectContent(void);

    void on_OpenAttachment_clicked();
};

#endif // MAILCONTENT_H
