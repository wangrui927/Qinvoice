#include "login.h"
#include "ui_login.h"
#include <QDebug>



login::login(QInvoiceSettingsStruct &XMLSettings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::login)
{
    ui->setupUi(this);

    QString strEmail  = XMLSettings.MailAddress;
    QString strName   = XMLSettings.UserName;

    ui->editUsername->setText(strName);

    if(QInvoiceUtil::isEmailValid(strEmail))
    {
        userEmail = strEmail;
        ui->editUserEmail->setText(userEmail);
        ui->editPassword->setFocus();
    }

    password  = ui->editPassword->text();
    username  = ui->editUsername->text();

    //connect(ui->editUserEmail,SIGNAL(textChanged(QString)), this, SLOT(actualiseUserEmail(QString)));
    connect(ui->editPassword ,SIGNAL(textChanged(QString)), this, SLOT(actualiseUserpassword(QString)));
    //connect(ui->editUsername ,SIGNAL(textChanged(QString)), this, SLOT(actualiseUsername(QString)));

}

login::~login()
{
    delete ui;
}

void login::actualiseUsername(QString text)
{
    username = text ; //ui->editUsername->text();
}

void login::actualiseUserEmail(QString text)
{
    userEmail = text ;
}

void login::actualiseUserpassword(QString text)
{
    password = text; //ui->editPassword->text();
}
