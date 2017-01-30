#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include "qinvoiceutil.h"


class QString;
class QinvoiceINI;

namespace Ui {
class login;
}

class login : public QDialog
{
    Q_OBJECT
private slots:
    void actualiseUserpassword(QString text);
    void actualiseUsername(QString text);
    void actualiseUserEmail(QString text);
public:
    explicit login(QInvoiceSettingsStruct &XMLSettings, QWidget *parent = 0);
    ~login();
    QString username;
    QString password;
    QString userEmail;

private:
    Ui::login *ui;
    QString iniPath;


};

#endif // LOGIN_H
