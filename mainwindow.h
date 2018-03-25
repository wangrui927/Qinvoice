#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QLabel>
#include "mydelegate.h"
#include "searchdelegate.h"
#include "qinvoiceutil.h"


namespace Ui {
class MainWindow;
}

class QDataWidgetMapper;
class QCPBars;
class QCPItemText;
class QtRPT;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString dbpath, QInvoiceSettingsStruct &XMLSettings, QWidget *parent = 0);
    ~MainWindow();
    float getSubtotal(int record);
    float getTotal(int record);
    void actualiseAmountField(float subtotal, float total);
    QVector<int> getInvoiceList(QString record);
    QVector<QString> getAllCustomer(void);
    float getMonthSum(int record);
    QString UserPassword;

protected:
    void closeEvent(QCloseEvent *);

private slots:
    void updateButtons(int row);
    void on_editInvoice_clicked();
    void on_saveInvoice_clicked();
    void currentInvoiceChanged(int index);
    void onInvoiceDataChanged();
    void on_newInvoice_clicked();
    void on_cancelInvoice_clicked();
    void on_deleteInvoice_clicked();
    void on_previewInvoice_clicked();
    void on_addCourse_clicked();
    void on_removeCourse_clicked();
    void on_openFolder_clicked();
    void on_addConstat_clicked();
    void on_removeConstat_clicked();
    void onCustomerDataChanged(void);
    void test_slot();
    void invoiceDataChangedPreCheck(void);
    void on_revert_clicked();
    void actualiseInvoiceTempInfos(void);
    void setValue(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage);
    void NoteTextChanged(void);

    void on_addNewCustomer_clicked();

    void on_deleteCustomer_clicked();

    void on_SaveCustomer_clicked();

    void on_cancelAddNewCustomer_clicked();

    void on_pushButton_clicked();

    void on_revertCustomer_clicked();

    void CustomerCheckBox_stateChanged(int state);

    void StatusCheckBox_stateChanged(int state);

    void on_refreshSearch_clicked();

    void on_refreshPlot_clicked();

    void on_openInvoiceview_clicked();

    void on_sendInvoice_clicked();

    void on_printInvoice_clicked();

    void on_duplicateConstat_clicked();

    void on_duplicateCourses_clicked();

    void on_InvoiveView_tabBarClicked(int index);

    void on_invNbrCheckbox_stateChanged(int state);

    void on_DategroupBox_clicked();

    void on_actionLoad_Database_triggered();

    void on_actionCumul_Report_triggered();

    void on_previousInvoice_clicked();

    void on_nextInvoice_clicked();

    void on_firstInvoice_clicked();

    void on_lastInvoice_clicked();

    void on_actionArchive_triggered();

    void on_sendCumul_clicked();

    void on_actionRelease_Notes_triggered();

    void on_actionAbout_triggered();



private:
    Ui::MainWindow *ui;
    QStringList unsavedElemList;
    QSqlRelationalTableModel *invoiceModel,*CustomerModel, *SearchInvoiceModel, *RelaunchInvoiceModel;
    QDataWidgetMapper *mapper;
    QStringListModel *typeModel;
    QSqlTableModel *CoursesModel;
    QSqlTableModel *ConstatModel;
    QString actuCustomer;
    QDate actuDate;
    int actuStatus;
    int actuInvoiceID;
    int initPhase = 1;
    QString actuNotes;
    QString actuInvoiceTitle;
    QString actuInvoicePath;
    QString FacturePath;
    QLocale locale;
    QSqlTableModel *relationModel, *SearchRelationModel;
    MyDelegate *myDelegate;
    QCPBars *fossil;
    QCPItemText *textLabel;
    QString VERSION;
    QString workingDirectory, iniFileDirectory;
    QSettings qInvoiceSettings;
    QLabel *DBLabel;

    enum {
        SearchTab_invoiceDate = 2,
        SearchTab_InvoiceNbr  = 3,
        SearchTab_Satus    = 4
    };


    enum {
        previewReport = 0,
        printReport   = 1,
        saveReport    = 2
    };

    enum {
        reportGroup0 = 0, /* INVOICE REPORT*/
        reportGroup1 = 1  /* CUMUL REPORT*/
    };
    enum {
        ReportTypeDefault = 0,
        ReportTypeCoursesOnly = 1,
        ReportTypeConstatOnly = 2
    };

    enum {
        invoice_Id = 0,
        invoice_CustomerID = 1,
        invoice_Date = 2,
        invoice_Status = 3,
        invoice_Notes = 4
    };

    enum{
        Constat_ID = 0,
        Constat_ServiceDate = 1,
        Constat_ServiceLocation = 2,
        Constat_Reference = 3,
        Constat_TVA = 4,
        Constat_PayementAmount= 5,
        constatInvoiceID = 6
    };

    enum{
        Courses_ID = 0,
        Courses_PickUpDate = 1,
        Courses_Reference = 2,
        Courses_ShippingLocation = 3,
        Courses_ShippedFrom = 4,
        Courses_DeliveryLocation = 5,
        Courses_TVA = 6,
        Courses_PayementAmount = 7,
        coursesInvoiceID = 8
    };

    QString ToolVersion;
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





    int ReportType;
    int reportGroup;
    void initialiseUI(void);
    void goToInvoice(int invoiceID);
    void showError(const QSqlError &err);
    bool okayToClose(void);
    float getCoursesTVA1Sum(int record);
    float getCoursesTVA2Sum(int record);
    float getConstatTVA1Sum(int record);
    float getConstatTVA2Sum(int record);
    int getCount(int record);
    QString getCompanyName(int record);
    QString getAddress1(int record);
    QString getAddress2(int record);
    QString getAddress3(int record);
    QString generateReport(int type, int group);
    void plotInit(void);
    void plotRefresh(void);
    void ApplicationShowInfos(QString text, int time_ms);
    int sendMail(void);
    int sendMailWithParam(QString AttachmentPath);
    void loadDatabase(QString pathToFile);
    void resetALL(void);
    int getInvoiceIDfromInvoiceNbr(const QString& InvoiceNbr);
    float getMontantHTCumulReport(int tva, const QString& companyname);
    void resetGuiElementsAfterInvoiceSaved(void);
    void actualiseAllViews();
};
#endif // MAINWINDOW_H
