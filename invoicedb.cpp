#include "invoicedb.h"
#include <QApplication>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDir>
#include <QDebug>
#include <QMessageBox>

InvoiceDB::InvoiceDB(QString path)
{
    DBpath = path; //QApplication::applicationDirPath() + "/dependencies/InvoiceToolDB_2016.db3";
}

bool InvoiceDB::createConnection(void)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(DBpath);
    if (!db.open()) {
        QMessageBox::critical(0, QObject::tr("Database Error"),
                             db.lastError().text());
        return false;
    }
    /* Enable foreign key (They are disabled by default)*/
    QSqlQuery query;
    if(!query.exec("PRAGMA foreign_keys = ON;")) showError(db.lastError());

    return true;
}

void InvoiceDB::showError(const QSqlError &err)
{
    QMessageBox::critical(0, "Database Error",err.text());
}

void InvoiceDB::CreateTestData(void)
{
    QStringList CompanyNames, Names,Address, PostalCode, City, CountryOrRegion, EmailAddress, Notes;
    CompanyNames    << "AIR SEA INTERNATIONAL" << "BALGUERIE" << "DIMOTRANS" ;
    Names           << "" << "" << "";
    Address	        << "BP 10317" << "BP 12148" << "73 Avenue Charles DE GAULLE";
    PostalCode	    << "95705" << "95732" << "95700";  
    City	        << "ROISSY" << "ROISSY" << "ROISSY";      
    CountryOrRegion << "FRANCE" << "FRANCE" << "FRANCE";   
    EmailAddress	<< "" << "" << "";
    Notes           << "" << "" << "";
    
    QSqlQuery query;
    query.exec("DROP TABLE Invoices");
    query.exec("DROP TABLE Customers");
    
    query.exec("CREATE TABLE Invoices ("
               "InvoiceID INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
               "CustomerID INTEGER NOT NULL,"
               "InvoiceDate TEXT NOT NULL,"
               "Status INTEGER,"
               "Notes TEXT, "
               "FOREIGN KEY(CustomerID) REFERENCES Customers ON DELETE CASCADE ON UPDATE CASCADE )");
    
    query.exec("CREATE TABLE Customers ("
               "CustomerID INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
               "CompanyName	TEXT,"
               "Name TEXT,"
               "Address	TEXT,"
               "PostalCode	NUMERIC,"
               "City	TEXT,"
               "CountryOrRegion	TEXT,"
               "EmailAddress	TEXT,"
               "Notes	TEXT)");
               
    /* Fill the Data in */           
    query.prepare("INSERT INTO Customers (CompanyName, Name, Address, PostalCode, "
                  "City, CountryOrRegion, EmailAddress, Notes)"
                  "VALUES (:CompanyName, :Name, :Address, :PostalCode,"
                  ":City, :CountryOrRegion, :EmailAddress, :Notes)");
                  
    for (int i = 0; i < CompanyNames.size(); ++i)
    {
        query.bindValue(":CompanyName",CompanyNames.at(i));
        query.bindValue(":Name ", Names.at(i));
        query.bindValue(":Address", Address.at(i));
        query.bindValue(":PostalCode", PostalCode.at(i));
        query.bindValue(":City", City.at(i));
        query.bindValue(":CountryOrRegion", CountryOrRegion.at(i));
        query.bindValue(":EmailAddress", EmailAddress.at(i).toLower().replace(" ", ".") + "@company.com");
        query.bindValue(":Notes", Notes.at(i));
        query.exec();    
    }
    
    query.prepare("INSERT INTO Invoices (CustomerID, InvoiceDate, Status, Notes )"
              "VALUES (:CustomerID, :InvoiceDate, :Status, :Notes)");   
    query.bindValue(":CustomerID",1);
    QDate a = QDate::currentDate().addDays(-(std::rand() % 3600));
    qDebug()<< a;
    query.bindValue(":InvoiceDate",a);
    query.bindValue(":Status",0);
    query.bindValue(":Notes", "" );
    query.exec();         

    query.bindValue(":CustomerID",2);
    a = QDate::currentDate().addDays(-(std::rand() % 3600));
    qDebug()<< a;
    query.bindValue(":InvoiceDate",a);
    query.bindValue(":Status",0);
    query.bindValue(":Notes", "" );
    query.exec(); 

    query.bindValue(":CustomerID",3);
    a = QDate::currentDate().addDays(-(std::rand() % 3600));
    qDebug()<< a;
    query.bindValue(":InvoiceDate",a);
    query.bindValue(":Status",0);
    query.bindValue(":Notes", "" );
    query.exec();         
    
}   

void InvoiceDB::CreateEmptyTable(void)
{
    QSqlQuery query;
    
    query.prepare("INSERT INTO Invoices (CustomerID, InvoiceDate, Status, Notes )"
              "VALUES (:CustomerID, :InvoiceDate, :Status, :Notes)");   
    query.bindValue(":CustomerID",1);
    QDate a = QDate::currentDate().addDays(-(std::rand() % 3600));
    qDebug()<< a;
    query.bindValue(":InvoiceDate",a);
    query.bindValue(":Status",0);
    query.bindValue(":Notes", "" );
    query.exec();         

    query.bindValue(":CustomerID",2);
    a = QDate::currentDate().addDays(-(std::rand() % 3600));
    qDebug()<< a;
    query.bindValue(":InvoiceDate",a);
    query.bindValue(":Status",0);
    query.bindValue(":Notes", "" );
    query.exec(); 

    query.bindValue(":CustomerID",3);
    a = QDate::currentDate().addDays(-(std::rand() % 3600));
    qDebug()<< a;
    query.bindValue(":InvoiceDate",a);
    query.bindValue(":Status",0);
    query.bindValue(":Notes", "" );
    query.exec();     
    
}  
    
    
    
