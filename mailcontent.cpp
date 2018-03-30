#include "mailcontent.h"
#include "ui_mailcontent.h"
#include <QDebug>
#include <QDesktopServices>

MailContent::MailContent(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MailContent)
{
    ui->setupUi(this);
    connect(ui->mailTextEdit, SIGNAL(textChanged()),this, SLOT(updateMailText()));
    connect(ui->SubjectContent, SIGNAL(textChanged(const QString &)),this, SLOT(updateSubjectContent()));
}

MailContent::~MailContent()
{
    delete ui;
}

void MailContent::setMailInfos(const QString &text, QString receiver, QString Subject, QString AttachmentPath)
{
    Mailtext = text;
    SubjectContent = Subject;
    PathToAttachment = AttachmentPath;

    ui->mailTextEdit->setPlainText(Mailtext);
    ui->toCustomer->setText(receiver);
    ui->SubjectContent->setText(Subject);
    ui->AttachmentPathEdit->setText(QFileInfo(AttachmentPath).fileName());
    // Set Cursor to the end of line
    ui->mailTextEdit->setFocus();
    ui->mailTextEdit->moveCursor ( QTextCursor::End );
}

QString MailContent::getMailText(void)
{
    return Mailtext;
}

QString MailContent::getSubjectContent(void)
{
    return SubjectContent;
}

void MailContent::updateMailText(void)
{
    Mailtext = ui->mailTextEdit->toPlainText();
}

void MailContent::updateSubjectContent(void)
{
    SubjectContent = ui->SubjectContent->text();
}

void MailContent::on_OpenAttachment_clicked()
{
    qDebug() << "Opening " << PathToAttachment;
    QDesktopServices::openUrl(QUrl::fromLocalFile(PathToAttachment));
}
