#include "mainwindow.h"
#include <QApplication>
#include "invoicedb.h"
#include <QSplashScreen>
#include <QTime>
#include "login.h"
#include "runGuard.h"
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include "qinvoiceutil.h"
#include "qinvoiceini.h"

void delay( int millisecondsToWait );


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString DBpath;
    QString iniFilePath = QString("%1/dependencies/Qinvoice.ini").arg(QApplication::applicationDirPath());

    /*
     *  Splashscreen
     */
    QSplashScreen *splash = new QSplashScreen;
    QFont splashFont;
    splashFont.setFamily("Arial");
    splashFont.setPixelSize(14);
    splash->setFont(splashFont);

    /*
     * Run guard to prevent the program to run several times
     */
    RunGuard guard( "QInvoice_random_key" );
     if ( !guard.tryToRun() )
         return 0;

   /*
    * Splash screen
    */
    splash->setPixmap(QPixmap(":/pictures/dependencies/pictures/splash.png"));
    splash->show();
    Qt::Alignment topRight = Qt::AlignRight | Qt::AlignTop;

    /* Establishing connections */
    splash->showMessage(QObject::tr("Establishing connections..."),topRight, Qt::white);

    /*
     * Load actual database.Take (actualyear-1) if actualyear not created yet
     */
    DBpath = QString("%1/dependencies/InvoiceToolDB_%2.db3").arg(QApplication::applicationDirPath()).arg(QDate::currentDate().year());
    bool createNewDBFlag = false;

    if(!QFileInfo(DBpath).exists())
    {
        QString DBpath_previous = QString("%1/dependencies/InvoiceToolDB_%2.db3").arg(QApplication::applicationDirPath()).arg(QDate::currentDate().year()-1);

        if(!QFileInfo(DBpath_previous).exists())
        {
            /* Create New Database */
            createNewDBFlag = true;
        }
        else
        {
            /* Load last year database */
            QString tmp = QString("Remember to archive the %1 Database and create the %2 Database. Use the archive function in menu file").arg(QDate::currentDate().year()-1).arg(QDate::currentDate().year());
            QMessageBox::warning(splash,"",tmp,QMessageBox::Ok);
        }
    }

    InvoiceDB invDB = InvoiceDB(DBpath);
    if(!invDB.createConnection()) return 1;

    /* Create necessary Tables when flag is set*/
    if (createNewDBFlag == true)
    {
        QSqlQuery query;
        query.exec(
                "CREATE TABLE Invoices ("
                "`InvoiceID`     INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                "`CustomerID`    INTEGER NOT NULL,"
                "`InvoiceDate`   TEXT NOT NULL,"
                "`InvoiceNbr`    TEXT,"
                "`Status`        INTEGER,"
                "`SentOn`	     TEXT,"
                "`Notes`         TEXT,"
                "FOREIGN KEY(`CustomerID`) REFERENCES `Customers` ON DELETE CASCADE ON UPDATE CASCADE)"
        );

        query.exec(
                  "CREATE TABLE Customers (CustomerID INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,CompanyName	TEXT,Name TEXT,Address	TEXT,PostalCode	NUMERIC,City	TEXT,CountryOrRegion	TEXT,EmailAddress	TEXT,Notes	TEXT)"
        );

        query.exec(
                    "CREATE TABLE Courses ("
                        "`CoursesID`	INTEGER NOT NULL,"
                        "`PickUpDate`	TEXT,"
                        " `Reference`	TEXT,"
                        "`SchippingLocation`	TEXT,"
                        " `ShippedFrom`	TEXT,"
                        "`DeliveryLocation`	TEXT,"
                        "`TVA`	INTEGER,"
                        "`PaymentAmount`	REAL,"
                        "`InvoiceID`	INTEGER NOT NULL,"
                        "PRIMARY KEY(CoursesID,InvoiceID),"
                        "FOREIGN KEY(`InvoiceID`) REFERENCES `Invoices` ON DELETE CASCADE ON UPDATE CASCADE)"
        );

        query.exec(
                    "CREATE TABLE Constat ("
                        "`ConstatID`	INTEGER NOT NULL,"
                        "`ServiceDate`	TEXT,"
                        "`ServiceLocation`	TEXT,"
                        " `Reference`	TEXT,"
                        "`TVA`	INTEGER,"
                        "`PaymentAmount`	REAL,"
                        " `InvoiceID`	INTEGER NOT NULL,"
                        "PRIMARY KEY(ConstatID,InvoiceID),"
                        "FOREIGN KEY(`InvoiceID`) REFERENCES `Invoices` ON DELETE CASCADE ON UPDATE CASCADE)"
        );
    }

    splash->showMessage(QObject::tr("Connected "),topRight, Qt::white);

    /*
     * Backing up database each 15th of each month
     */

    QString backupFullpath = QString("%1/dependencies/DatabaseBackups").arg(QApplication::applicationDirPath());
    QDir backupPath(backupFullpath);

    if(! backupPath.exists())
    {
        backupPath.mkpath(".");
    }

    QDate today = QDate::currentDate();
    QDate dayToBeComparedWith;
    dayToBeComparedWith.setDate(QDate::currentDate().year(),QDate::currentDate().month(),15);


    QStringList filters;
    filters << "*.db3" ;

    backupPath.setNameFilters(filters);
    QString DBfromThisMonth = QString("\\bBackup_[0-9]{1,2}_%1_%2\\.db3\\b").arg(QDate::currentDate().month()).arg(QDate::currentDate().year());
    QRegExp rx(DBfromThisMonth);
    rx.setCaseSensitivity(Qt::CaseSensitive);

    QStringList allFiles = backupPath.entryList(QDir::Files | QDir::NoSymLinks);
    QStringList matchingFiles;

    foreach (QString file, allFiles)
    {
       if (rx.exactMatch(file))
       {
           matchingFiles << file;
       }
    }

    if (today > dayToBeComparedWith && matchingFiles.count() == 0)
    {
        QString BackupFilename = QString("Backup_%1_%2_%3.db3").arg(QDate::currentDate().day()).arg(QDate::currentDate().month()).arg(QDate::currentDate().year());
        QString BackupFilenameFullpath = QString("%1/%2").arg(backupFullpath).arg(BackupFilename);

        if(!QFileInfo(BackupFilenameFullpath).exists())
        {
            splash->showMessage(QObject::tr("Backing up databes... "),topRight, Qt::white);
            QFile::copy(DBpath,BackupFilenameFullpath);
        }
    }

    splash->showMessage(QObject::tr("Setting up the main window..."),topRight, Qt::white);

    /* Loading ini settings */

    QInvoiceSettingsStruct XMLSettings;

    QinvoiceINI XMLiniSetting(QString("%1/dependencies/Qinvoiceini.xml").arg(QApplication::applicationDirPath()), XMLSettings);

    /* Loading modules*/
    splash->showMessage(QObject::tr("Login..."),topRight, Qt::white);

    login logIN(XMLSettings);
    if(!logIN.exec()) return 0;
/*
    do
    {
        if (!QInvoiceUtil::isEmailValid(logIN.userEmail)) logIN.setFocusToUserEmail();
        if(!logIN.exec()) return 0;
        if (!QInvoiceUtil::isEmailValid(logIN.userEmail)) QMessageBox::warning(splash,"","Please enter a valid email address",QMessageBox::Ok);
    }
    while(!QInvoiceUtil::isEmailValid(logIN.userEmail));
*/
    MainWindow w(DBpath, XMLSettings);

    w.UserPassword  = logIN.password;


    /* Loading modules*/
    splash->showMessage(QObject::tr("Loading modules..."),topRight, Qt::white);

    w.showMaximized();
    splash->finish(&w);
    delete splash;
    return a.exec();

}


void delay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}
