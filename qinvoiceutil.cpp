#include "qinvoiceutil.h"
#include <QDebug>
#include <QSettings>



QInvoiceUtil::QInvoiceUtil()
{

}
bool QInvoiceUtil::isEmailValid(QString email)
{
    if (email.length() == 0) return false;
    QString strPattern = "\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b";
    QRegExp regx(strPattern);
    regx.setCaseSensitivity(Qt::CaseInsensitive);
    return regx.exactMatch(email);
}


