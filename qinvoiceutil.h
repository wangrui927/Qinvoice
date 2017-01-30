#ifndef QINVOICEUTIL_H
#define QINVOICEUTIL_H

#include <QString>

class QSettings;

struct QInvoiceSettingsStruct
{
    QString UserName      ;
    QString MailAddress   ;
    QString EnterpriseName;
    QString Footer        ;
    QString AddressLine1  ;
    QString AddressLine2  ;
    QString AddressLine3  ;
    QString InvoiceLine1  ;
    QString TVANr         ;
    QString TelNr         ;
};

class QInvoiceUtil
{
public:
     QInvoiceUtil();
     static bool isEmailValid(QString email);


};

#endif // QINVOICEUTIL_H
