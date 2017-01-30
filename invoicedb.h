#ifndef INVOICEDB_H
#define INVOICEDB_H

#include <QObject>
#include <QWidget>
#include <QSqlDatabase>
#include "mainwindow.h"

class InvoiceDB
{
public:
    InvoiceDB(QString path);
    bool createConnection(void);
    void CreateTestData(void);
    void CreateEmptyTable(void);
    void showError(const QSqlError &err);
signals:

public slots:

private:
    QSqlDatabase *db;
    QString DBpath;
};

#endif // INVOICEDB_H
