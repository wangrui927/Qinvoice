#ifndef QINVOICEINI_H
#define QINVOICEINI_H

#include <QObject>
#include <QString>
#include <qinvoiceutil.h>

class QinvoiceINI : public QObject
{
    Q_OBJECT
public:
    explicit QinvoiceINI(QString XMLpath, QInvoiceSettingsStruct &XMLSettings, QObject *parent = 0);
    void parseXMLini(QString XMLpath, QInvoiceSettingsStruct &XMLSettings);

};

#endif // QINVOICEINI_H
