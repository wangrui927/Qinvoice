#include "mailcontent.h"
#include "ui_mailcontent.h"

MailContent::MailContent(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MailContent)
{
    ui->setupUi(this);
    connect(ui->mailTextEdit, SIGNAL(textChanged()),this, SLOT(updateMailText()));
}

MailContent::~MailContent()
{
    delete ui;
}

void MailContent::setMailInfos(const QString &text, QString receiver)
{
    Mailtext = text;
    ui->mailTextEdit->setPlainText(Mailtext);
    ui->toCustomer->setText(receiver);
}

QString MailContent::getMailText(void)
{
    return Mailtext;
}

void MailContent::updateMailText(void)
{
    Mailtext = ui->mailTextEdit->toPlainText();
}
