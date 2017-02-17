#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtWidgets>
#include <qtrpt.h>
#include "qcustomplot.h"
#include "SmtpClientforQt/src/SmtpMime"
#include "mailcontent.h"
#include "invoicedb.h"
#include <QFileInfo>
#include "qinvoiceutil.h"


MainWindow::MainWindow(QString dbpath, QInvoiceSettingsStruct &XMLSettings, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    this->UserName       = XMLSettings.UserName       ;
    this->MailAddress    = XMLSettings.MailAddress    ;
    this->EnterpriseName = XMLSettings.EnterpriseName ;
    this->AddressLine1   = XMLSettings.AddressLine1   ;
    this->AddressLine2   = XMLSettings.AddressLine2   ;
    this->AddressLine3   = XMLSettings.AddressLine3   ;
    this->InvoiceLine1   = XMLSettings.InvoiceLine1   ;
    this->TVANr          = XMLSettings.TVANr          ;
    this->TelNr          = XMLSettings.TelNr          ;

    initialiseUI();

    DBLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(DBLabel);

    QFileInfo dbfile(dbpath);
    DBLabel->setText(dbfile.fileName());

    VERSION = "1.3.4";
    workingDirectory = QApplication::applicationDirPath() + "/dependencies";


    this->setWindowTitle(QString("V.L.K Invoice Tool - %1").arg(VERSION));
    qDebug() << QString("Starting QInvoice version %1").arg(VERSION);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initialiseUI()
{
    ReportType  = ReportTypeDefault;
    locale      = QLocale(QLocale::French, QLocale::France);
    FacturePath = QApplication::applicationDirPath() + "/FACTURES";
    /*
     * Shortcuts
     */
    QShortcut *shortcut_InvSave = new QShortcut(QKeySequence("Ctrl+S"), this);
    QObject::connect(shortcut_InvSave, SIGNAL(activated()), this, SLOT(on_saveInvoice_clicked()));

    QShortcut *shortcut_InvNew = new QShortcut(QKeySequence("Ctrl+N"), this);
    QObject::connect(shortcut_InvNew, SIGNAL(activated()), this, SLOT(on_newInvoice_clicked()));

    /*
     * Statusbar
     */
    statusBar()->showMessage(tr("ready"), 4000);
    /*
     * Invoice Model
     */
    invoiceModel = new QSqlRelationalTableModel(this);
    invoiceModel->setTable("Invoices");
    invoiceModel->setRelation(invoice_CustomerID, QSqlRelation("Customers", "CustomerID", "CompanyName"));
    invoiceModel->setSort(invoice_Id, Qt::AscendingOrder);
    if (!invoiceModel->select())
        qDebug() << invoiceModel->lastError();

    /*
     * Invoice Head
     */
    relationModel = invoiceModel->relationModel(invoice_CustomerID);
    ui->CustomerCombo->setModel(relationModel);
    ui->CustomerCombo->setModelColumn(relationModel->fieldIndex("CompanyName"));

    ui->CustomerLabel->setBuddy(ui->CustomerCombo);
    ui->DateLabel->setBuddy(ui->DateEdit);
    ui->StatusLabel->setBuddy(ui->StatusCombo);

    QStringList items;
    items << tr("Unpaid") << tr("Paid");
    typeModel = new QStringListModel(items);
    ui->StatusCombo->setModel(typeModel);

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setModel(invoiceModel);
    mapper->setItemDelegate(new QSqlRelationalDelegate(this));

    mapper->addMapping(ui->CustomerCombo, invoice_CustomerID);
    mapper->addMapping(ui->DateEdit, invoiceModel->fieldIndex("InvoiceDate"));
    mapper->addMapping(ui->StatusCombo, invoiceModel->fieldIndex("Status"), "currentIndex");
    mapper->addMapping(ui->NoteTextEdit, invoiceModel->fieldIndex("Notes"));
    mapper->addMapping(ui->InvoiceLabel, invoiceModel->fieldIndex("InvoiceNbr"));

    connect(ui->previousInvoice, SIGNAL(clicked()),this, SLOT(actualiseInvoiceTempInfos()));
    connect(ui->nextInvoice, SIGNAL(clicked()), this, SLOT(actualiseInvoiceTempInfos()));
    connect(ui->firstInvoice, SIGNAL(clicked()), this, SLOT(actualiseInvoiceTempInfos()));
    connect(ui->lastInvoice, SIGNAL(clicked()), this, SLOT(actualiseInvoiceTempInfos()));

    connect(ui->StatusCombo, SIGNAL(activated(QString)), this, SLOT(invoiceDataChangedPreCheck()));
    connect(ui->DateEdit, SIGNAL(editingFinished()), this, SLOT(invoiceDataChangedPreCheck()));
    connect(ui->CustomerCombo, SIGNAL(activated(QString)), this, SLOT(invoiceDataChangedPreCheck()));
    connect(ui->NoteTextEdit, SIGNAL(textChanged()), this, SLOT(NoteTextChanged()));

    connect(mapper, SIGNAL(currentIndexChanged(int)),this, SLOT(currentInvoiceChanged(int)));
    connect(ui->previousInvoice, SIGNAL(clicked()),mapper, SLOT(toPrevious()));
    connect(ui->nextInvoice, SIGNAL(clicked()),mapper, SLOT(toNext()));
    connect(ui->firstInvoice, SIGNAL(clicked()),mapper, SLOT(toFirst()));
    connect(ui->lastInvoice, SIGNAL(clicked()),mapper, SLOT(toLast()));

    /*
     * Courses Model
     */
    CoursesModel = new QSqlTableModel(this);
    CoursesModel->setTable("Courses");
    CoursesModel->setHeaderData(Courses_ID, Qt::Horizontal, tr("ID"));
    CoursesModel->setHeaderData(Courses_PickUpDate, Qt::Horizontal, tr("Date"));
    CoursesModel->setHeaderData(Courses_Reference , Qt::Horizontal, tr("Reference"));
    CoursesModel->setHeaderData(Courses_ShippingLocation , Qt::Horizontal, tr("Lieu d'expedition"));
    CoursesModel->setHeaderData(Courses_ShippedFrom , Qt::Horizontal, tr("Expediteur"));
    CoursesModel->setHeaderData(Courses_DeliveryLocation , Qt::Horizontal, tr("Lieu de Livraison"));
    CoursesModel->setHeaderData(Courses_TVA , Qt::Horizontal, tr("TVA"));
    CoursesModel->setHeaderData(Courses_PayementAmount , Qt::Horizontal, tr("Tarif H.T"));
    CoursesModel->setEditStrategy(QSqlTableModel::OnManualSubmit);


    ui->CoursesTableView->setModel(CoursesModel);
    ui->CoursesTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->CoursesTableView->resizeColumnsToContents();
    ui->CoursesTableView->setColumnHidden(Courses_ID, true);
    ui->CoursesTableView->setColumnHidden(coursesInvoiceID, true);
    ui->CoursesTableView->setAlternatingRowColors(true);
    ui->CoursesTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->CoursesTableView-> setItemDelegate(new MyDelegate(1,6));
    connect(ui->CoursesTableView->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onInvoiceDataChanged()));

    /*
     * Constat Model
     */
    ConstatModel = new QSqlTableModel(this);
    ConstatModel->setTable("Constat");
    ConstatModel->setHeaderData(Constat_ID, Qt::Horizontal, tr("ID"));
    ConstatModel->setHeaderData(Constat_ServiceDate, Qt::Horizontal, tr("Date"));
    ConstatModel->setHeaderData(Constat_ServiceLocation, Qt::Horizontal, tr("Lieu de Prestations"));
    ConstatModel->setHeaderData(Constat_Reference, Qt::Horizontal, tr("Reference"));
    ConstatModel->setHeaderData(Constat_TVA, Qt::Horizontal, tr("TVA"));
    ConstatModel->setHeaderData(Constat_PayementAmount, Qt::Horizontal, tr("Tarif H.T"));
    ConstatModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    ui->ConstatTableView->setModel(ConstatModel);
    ui->ConstatTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->ConstatTableView->resizeColumnsToContents();
    ui->ConstatTableView->setColumnHidden(Constat_ID, true);
    ui->ConstatTableView->setColumnHidden(constatInvoiceID, true);
    ui->ConstatTableView->setAlternatingRowColors(true);
    ui->ConstatTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->ConstatTableView-> setItemDelegate(new MyDelegate(1,4));

    connect(ui->ConstatTableView->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onInvoiceDataChanged()));

    mapper->toLast();

    actualiseInvoiceTempInfos();

    /*
     * Customer Model
     */
    CustomerModel = new QSqlRelationalTableModel(this);
    CustomerModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    CustomerModel->setTable("Customers");
    CustomerModel->setSort(CustomerModel->fieldIndex("CustomerID"), Qt::AscendingOrder);

    // Set the localized header captions
    CustomerModel->setHeaderData(CustomerModel->fieldIndex("CustomerID")      , Qt::Horizontal, tr("ID"));
    CustomerModel->setHeaderData(CustomerModel->fieldIndex("CompanyName")     , Qt::Horizontal, tr("Company Name"));
    CustomerModel->setHeaderData(CustomerModel->fieldIndex("Name")            , Qt::Horizontal, tr("Name"));
    CustomerModel->setHeaderData(CustomerModel->fieldIndex("Address")         , Qt::Horizontal, tr("Address"));
    CustomerModel->setHeaderData(CustomerModel->fieldIndex("PostalCode")      , Qt::Horizontal, tr("Postal Code"));
    CustomerModel->setHeaderData(CustomerModel->fieldIndex("City")            , Qt::Horizontal, tr("City"));
    CustomerModel->setHeaderData(CustomerModel->fieldIndex("CountryOrRegion") , Qt::Horizontal, tr("Country or Region"));
    CustomerModel->setHeaderData(CustomerModel->fieldIndex("EmailAddress")    , Qt::Horizontal, tr("Email"));
    CustomerModel->setHeaderData(CustomerModel->fieldIndex("Notes")           , Qt::Horizontal, tr("Notes"));

    // populate the model
    if (!CustomerModel->select()) {
        showError(CustomerModel->lastError());
        return;
    }

    ui->CustomerTableView->setModel(CustomerModel);
    ui->CustomerTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->CustomerTableView->setCurrentIndex(CustomerModel->index(0, 0));
    ui->CustomerTableView->resizeColumnsToContents();
    ui->CustomerTableView->setColumnHidden(CustomerModel->fieldIndex("CustomerID"), true);
    ui->CustomerTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->CustomerTableView->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onCustomerDataChanged()));

    /*
     * Search Tab
     */
    SearchInvoiceModel = new QSqlRelationalTableModel(this);
    SearchInvoiceModel->setTable("Invoices");
    SearchInvoiceModel->setRelation(invoice_CustomerID, QSqlRelation("Customers", "CustomerID", "CompanyName"));
    SearchInvoiceModel->setSort(invoice_Id, Qt::AscendingOrder);

    if (!SearchInvoiceModel->select())
        qDebug() << SearchInvoiceModel->lastError();

    /*
     * Invoice Head
     */
    SearchRelationModel = SearchInvoiceModel->relationModel(invoice_CustomerID);

    QDate today = QDate::currentDate();
    ui->StartDateEdit->setDate(today.addDays(-15));
    ui->EndDateEdit->setDate(today.addDays(15));

    ui->SStatusCombo->addItem("Unpaid");
    ui->SStatusCombo->addItem("Paid");

    connect(ui->CustomerCheckBox, SIGNAL(stateChanged(int)), this, SLOT(CustomerCheckBox_stateChanged(int)));
    connect(ui->StatusCheckBox, SIGNAL(stateChanged(int)), this, SLOT(StatusCheckBox_stateChanged(int)));

    connect(ui->SCustomerCombo,SIGNAL(currentIndexChanged(int)),this, SLOT(on_refreshSearch_clicked()));
    connect(ui->SStatusCombo,SIGNAL(currentIndexChanged(int)),this, SLOT(on_refreshSearch_clicked()));
    connect(ui->StartDateEdit,SIGNAL(dateChanged(QDate)),this, SLOT(on_refreshSearch_clicked()));
    connect(ui->EndDateEdit,SIGNAL(dateChanged(QDate)),this, SLOT(on_refreshSearch_clicked()));
    connect(ui->InvNbrLineEdit,SIGNAL(editingFinished()),this, SLOT(on_refreshSearch_clicked()));

    ui->SearchInvoiceView->setModel(SearchInvoiceModel);
    ui->SearchInvoiceView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->SearchInvoiceView->resizeColumnsToContents();
    ui->SearchInvoiceView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->SearchInvoiceView->setColumnHidden(SearchInvoiceModel->fieldIndex("InvoiceID"), true);
    ui->SearchInvoiceView->setColumnHidden(SearchInvoiceModel->fieldIndex("Notes"), true);
    ui->SearchInvoiceView->setItemDelegate(new SearchDelegate(SearchTab_invoiceDate, SearchTab_Satus));

    ui->SCustomerCombo->setModel(relationModel);
    ui->SCustomerCombo->setModelColumn(relationModel->fieldIndex("CompanyName"));
    plotInit();
}


void MainWindow:: plotInit(void)
{
    // prepare data for bar chart
    QVector<QString> AllCustomers;
    QVector<double> InvoiceTotal;
    QHash<QString, float> CImap;
    float TempSum = 0;
    QVector<double> ticks;
    QVector<QString> labels;
    QVector<double> barData;

    ticks << 1 << 2 << 3 << 4 << 5;

    AllCustomers = getAllCustomer();

    for (int i = 1; i <= AllCustomers.size(); i++)
    {
        QVector<int> InvoiceList;
        InvoiceList = getInvoiceList(AllCustomers[i-1]);
        for (int j = 1; j <= InvoiceList.size(); j++)
        {
            TempSum += getSubtotal(InvoiceList[j-1]);
        }
        InvoiceTotal.append(TempSum);
        TempSum = 0;
        CImap.insert(AllCustomers[i-1], InvoiceTotal[i-1]);
    }

    qSort(InvoiceTotal);
    std::reverse(InvoiceTotal.begin(), InvoiceTotal.end());

    if (AllCustomers.size() <= 5){
        foreach(float value, InvoiceTotal ){
            labels.append(CImap.key(value));
            barData.append(value);
        }
    }
    else{
        for (int i = 1; i <= 5; i++){
            labels.append(CImap.key(InvoiceTotal[i-1]));
            barData.append(InvoiceTotal[i-1]);
        }
    }

    // prepare data for curve chart
    QVector<double> MonthSums;
    QVector<double> ticks2;
    QVector<QString> labels2;
    QDate tmpDate;

    ticks2 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12;

    for (int i = 12; i >= 1; i--){
        MonthSums.append(getMonthSum(i));
        tmpDate = QDate::currentDate().addMonths(-i);
        labels2.append(tmpDate.toString("MM.yyyy"));
    }

    // bar chart
    fossil = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis2);
    ui->customPlot->addPlottable(fossil);

    // set names and colors:
    QPen pen;
    pen.setWidthF(1.2);
    fossil->setName("Top 5 Customers");
    fossil->setWidth(0.4);
    pen.setColor(QColor(255, 131, 0));
    fossil->setPen(pen);
    fossil->setBrush(QColor(255, 131, 0, 50));

    // prepare x axis:
    ui->customPlot->xAxis->setAutoTicks(false);
    ui->customPlot->xAxis->setAutoTickLabels(false);
    ui->customPlot->xAxis->setTickVector(ticks);
    ui->customPlot->xAxis->setTickVectorLabels(labels);
    ui->customPlot->xAxis->setSubTickCount(0);
    ui->customPlot->xAxis->setTickLength(0, 4);
    ui->customPlot->xAxis->setRange(0, 6);
    ui->customPlot->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->xAxis->setTickLabelColor(Qt::white);

    // prepare y2 axis:
    ui->customPlot->yAxis2->setVisible(true);
    ui->customPlot->yAxis2->setRange(0, barData[0]+100);
    ui->customPlot->yAxis2->setTicks(false);
    ui->customPlot->yAxis2->setTickLabels(false);
    ui->customPlot->yAxis2->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->yAxis2->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis2->setTickLabelColor(Qt::white);

    // add data:
    fossil->setData(ticks, barData);

    // set text item showed in bars
    for(int i = 1; i <= barData.size(); i++){
        textLabel = new QCPItemText(ui->customPlot);
        ui->customPlot->addItem(textLabel);
        textLabel->setClipToAxisRect(false);
        textLabel->position->setAxes(ui->customPlot->xAxis,ui->customPlot->yAxis2);
        textLabel->position->setType(QCPItemPosition::ptPlotCoords);
        textLabel->position->setCoords(ticks[i-1],(barData[i-1]/2));
        //Customizing the item
        textLabel->setText(QString::number(barData[i-1]));
        textLabel->setColor(QColor(255, 131, 0));
        textLabel->setPen(QPen(Qt::NoPen));
    }

    // curve chart
    ui->customPlot->addGraph(ui->customPlot->xAxis2, ui->customPlot->yAxis);

    // set color and style
    QPen CurvePen;
    CurvePen.setColor(QColor(120, 120, 120));
    CurvePen.setWidthF(3);
    ui->customPlot->graph(0)->setPen(CurvePen);
    ui->customPlot->graph(0)->setName("Monthly Sum");
    ui->customPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->customPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));

    // prepare x2 axis:
    ui->customPlot->xAxis2->setRange(0, 13);
    ui->customPlot->xAxis2->setVisible(true);
    ui->customPlot->xAxis2->setAutoTicks(false);
    ui->customPlot->xAxis2->setAutoTickLabels(false);
    ui->customPlot->xAxis2->setTickVector(ticks2);
    ui->customPlot->xAxis2->setTickVectorLabels(labels2);
    ui->customPlot->xAxis2->setSubTickCount(0);
    ui->customPlot->xAxis2->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->xAxis2->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->xAxis2->setTickLabelColor(Qt::white);

    // prepare y axis:
    QVector<double> axisData = MonthSums;
    qSort(axisData);
    ui->customPlot->yAxis->setRange(0, axisData[axisData.size()-1]+100);
    ui->customPlot->yAxis->setPadding(5); // a bit more space to the left border
    ui->customPlot->yAxis->setLabel("Chiffre d'affaires (en €)");
    ui->customPlot->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setTickLabelColor(Qt::white);
    ui->customPlot->yAxis->setLabelColor(Qt::white);
    ui->customPlot->yAxis->setSubTickPen(QPen(Qt::white, 1));

    // set grid
    ui->customPlot->xAxis->grid()->setVisible(false);
    ui->customPlot->xAxis2->grid()->setVisible(true);
    ui->customPlot->yAxis->grid()->setSubGridVisible(true);
    ui->customPlot->xAxis2->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->customPlot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));

    // add data:
    ui->customPlot->graph(0)->setData(ticks2, MonthSums);

    // add legend:
    ui->customPlot->legend->setVisible(true);
    ui->customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignHCenter);
    ui->customPlot->legend->setBrush(QColor(220, 220, 220, 220));
    QPen legendPen;
    legendPen.setColor(QColor(130, 130, 130, 200));
    ui->customPlot->legend->setBorderPen(legendPen);
    QFont legendFont = font();
    legendFont.setPointSize(10);
    ui->customPlot->legend->setFont(legendFont);
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor(80, 80, 80));
    plotGradient.setColorAt(1, QColor(50, 50, 50));
    ui->customPlot->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(80, 80, 80));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    ui->customPlot->axisRect()->setBackground(axisRectGradient);

}

void MainWindow:: plotRefresh(void){

    // prepare data for bar chart
    QVector<QString> AllCustomers;
    QVector<double> InvoiceTotal;
    QHash<QString, float> CImap;
    float TempSum = 0;
    QVector<double> ticks;
    QVector<QString> labels;
    QVector<double> barData;

    ticks << 1 << 2 << 3 << 4 << 5;

    AllCustomers = getAllCustomer();

    for (int i = 1; i <= AllCustomers.size(); i++)
    {
        QVector<int> InvoiceList;
        InvoiceList = getInvoiceList(AllCustomers[i-1]);
        for (int j = 1; j <= InvoiceList.size(); j++)
        {
            TempSum += getSubtotal(InvoiceList[j-1]);
        }
        InvoiceTotal.append(TempSum);
        TempSum = 0;
        CImap.insert(AllCustomers[i-1], InvoiceTotal[i-1]);
    }

    qSort(InvoiceTotal);
    std::reverse(InvoiceTotal.begin(), InvoiceTotal.end());

    if (AllCustomers.size() <= 5){
        foreach(float value, InvoiceTotal ){
            labels.append(CImap.key(value));
            barData.append(value);
        }
    }
    else{
        for (int i = 1; i <= 5; i++){
            labels.append(CImap.key(InvoiceTotal[i-1]));
            barData.append(InvoiceTotal[i-1]);
        }
    }

    // prepare data for curve chart
    QVector<double> MonthSums;
    QVector<double> ticks2;
    QVector<QString> labels2;
    QDate tmpDate;

    ticks2 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12;

    for (int i = 12; i >= 1; i--){
        MonthSums.append(getMonthSum(i));
        tmpDate = QDate::currentDate().addMonths(-i);
        labels2.append(tmpDate.toString("MM.yyyy"));
    }

    // prepare x axis:
    ui->customPlot->xAxis->setTickVectorLabels(labels);

    // prepare y2 axis:
    ui->customPlot->yAxis2->setRange(0, barData[0]+100);

    // add data:
    fossil->setData(ticks, barData);

    // set text item showed in bar
    ui->customPlot->clearItems();
    for(int i = 1; i <= barData.size(); i++){
        textLabel = new QCPItemText(ui->customPlot);
        ui->customPlot->addItem(textLabel);
        textLabel->setClipToAxisRect(false);
        textLabel->position->setAxes(ui->customPlot->xAxis,ui->customPlot->yAxis2);
        textLabel->position->setType(QCPItemPosition::ptPlotCoords);
        textLabel->position->setCoords(ticks[i-1],(barData[i-1]/2));
        //Customizing the item
        textLabel->setText(QString::number(barData[i-1]));
        textLabel->setColor(QColor(255, 131, 0));
        textLabel->setPen(QPen(Qt::NoPen));
    }

    // curve chart
    // prepare x2 axis:
    ui->customPlot->xAxis2->setTickVectorLabels(labels2);

    // prepare y axis:
    QVector<double> axisData = MonthSums;
    qSort(axisData);
    ui->customPlot->yAxis->setRange(0, axisData[axisData.size()-1]+100);

    // add data:
    ui->customPlot->graph(0)->setData(ticks2, MonthSums);
}

void MainWindow:: goToInvoice(int invoiceID)
{
    CoursesModel->setFilter(QString("InvoiceID = %1").arg(invoiceID));
    ConstatModel->setFilter(QString("InvoiceID = %1").arg(invoiceID));
    CoursesModel->select();
    ConstatModel->select();
}


void MainWindow::NoteTextChanged(void)
{

}
void MainWindow::invoiceDataChangedPreCheck(void)
{
    if (ui->StatusCombo->currentIndex())
    {
        ui->StatusCombo->setStyleSheet("QComboBox { color: #32CD32; }");
    }
    else
    {
        ui->StatusCombo->setStyleSheet("QComboBox { color: red; }");
    }

    /* Set actuNotes variable first if init phase*/
    if(initPhase)
    {
        actuNotes = ui->NoteTextEdit->toPlainText();
        initPhase = 0;
    }

    if (QString::compare(ui->CustomerCombo->currentText(), actuCustomer, Qt::CaseInsensitive) ||
            ui->DateEdit->date() != actuDate ||
            ui->StatusCombo->currentIndex() != actuStatus ||
            QString::compare(ui->NoteTextEdit->toPlainText() , actuNotes, Qt::CaseSensitive)
       )
    {
        onInvoiceDataChanged();
    }
}

void MainWindow::actualiseInvoiceTempInfos(void)
{
    int actualInvoiceNbr = mapper->currentIndex();
    QSqlRecord record = invoiceModel->record(actualInvoiceNbr);
    int id = record.value("InvoiceID").toInt();

    actuInvoiceID = id;
    actuCustomer = ui->CustomerCombo->currentText();
    actuDate = ui->DateEdit->date();
    actuStatus = ui->StatusCombo->currentIndex();
    actuNotes = ui->NoteTextEdit->toPlainText();

    if (ui->StatusCombo->currentIndex())
        ui->StatusCombo->setStyleSheet("QComboBox {color: #32CD32; }");
    else
        ui->StatusCombo->setStyleSheet("QComboBox {color: red; }");

}

void MainWindow::currentInvoiceChanged(int index)
{
    updateButtons(index);
    QSqlRecord record = invoiceModel->record(index);
    int id = record.value("InvoiceID").toInt();

    CoursesModel->setFilter(QString("InvoiceID = %1").arg(id));
    CoursesModel->setSort(Courses_PickUpDate, Qt::AscendingOrder);

    ConstatModel->setFilter(QString("InvoiceID = %1").arg(id));
    ConstatModel->setSort(Constat_ServiceDate, Qt::AscendingOrder);

    CoursesModel->select();
    ConstatModel->select();
    refreshCoursesView();
    refreshConstatView();
    actualiseAmountField(getSubtotal(id), getTotal(id));
    actualiseInvoiceTempInfos();

    if(id==0)
    {
        actuInvoiceTitle =  "-";
        ui->InvoiceLabel->setText(actuInvoiceTitle); // New included invoice
        actuInvoiceID = -1;

    }
    else
    {
        actuInvoiceTitle =  QString("%1/%2%3").arg(actuDate.year())\
                .arg(locale.toString(actuDate,"MMM")) \
                .arg(actuInvoiceID).remove(".");

        ui->InvoiceLabel->setText(actuInvoiceTitle);
        actuInvoiceID = id;
    }

    actuInvoicePath = actuInvoiceTitle;
    actuInvoicePath.replace("/","_");
    actuInvoicePath = FacturePath + "/" + actuInvoicePath + ".pdf";

}

void MainWindow::refreshCustomerView(void){

    ui->CustomerTableView->resizeColumnsToContents();
}

void MainWindow::refreshCoursesView(void){

    ui->CoursesTableView->resizeColumnsToContents();
}

void MainWindow::refreshConstatView(){

    ui->ConstatTableView->resizeColumnsToContents();
}

void MainWindow::updateButtons(int row){

    // Update buttons
    ui->previousInvoice->setEnabled(row > 0);
    ui->nextInvoice->setEnabled(row < invoiceModel->rowCount() - 1);
    ui->firstInvoice->setEnabled(row > 0);
    ui->lastInvoice->setEnabled(row < invoiceModel->rowCount() - 1);

}

void MainWindow::on_editInvoice_clicked()
{
    ui->CustomerCombo->setEnabled(true);
    ui->DateEdit->setEnabled(true);
    ui->StatusCombo->setEnabled(true);
    ui->removeCourse->setEnabled(true);
    ui->addCourse->setEnabled(true);
    ui->removeConstat->setEnabled(true);
    ui->addConstat->setEnabled(true);
    ui->duplicateConstat->setEnabled(true);
    ui->duplicateCourses->setEnabled(true);
    ui->NoteTextEdit->setEnabled(true);
    ui->CoursesTableView->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->ConstatTableView->setEditTriggers(QAbstractItemView::DoubleClicked);

}

void MainWindow::on_saveInvoice_clicked()
{
    int error = 0;

    int maxConstatID = 0;
    int maxCoursesID = 0;

    QSqlQuery query;

    int actualInvoiceNbr = mapper->currentIndex();


    mapper->submit();
    QSqlRecord record = invoiceModel->record(actualInvoiceNbr);
    int id = record.value("InvoiceID").toInt();


    query.exec(QString("select max(ConstatID) from Constat where InvoiceID=%1").arg(id));

    while (query.next()) {
        maxConstatID = query.value(0).toInt()+1;
    }


    query.exec(QString("select max(CoursesID) from Courses where InvoiceID=%1").arg(id));

    while (query.next()) {
        maxCoursesID = query.value(0).toInt()+1;
    }


    for(int i = 0; i < CoursesModel->rowCount(); i++)
    {

        if(CoursesModel->data(CoursesModel->index(i, 8)).toInt() != id) // avoid doubles write
        {
            CoursesModel->setData(CoursesModel->index(i, 8), id);
        }
        if (CoursesModel->isDirty(CoursesModel->index(i, 0)))
        {
            CoursesModel->setData(CoursesModel->index(i, 0), maxCoursesID++);
        }
    }

    for(int i = 0; i < ConstatModel->rowCount(); i++)
    {
        if(ConstatModel->data(ConstatModel->index(i, 6)).toInt() != id){
            ConstatModel->setData(ConstatModel->index(i, 6), id);
        }
        if (ConstatModel->isDirty(ConstatModel->index(i, 0)))
        {
            ConstatModel->setData(ConstatModel->index(i, 0), maxConstatID++);
        }
    }

    if(!CoursesModel->submitAll())
    {
        showError(CoursesModel->lastError());
        error = 1;
    }

    if(!ConstatModel->submitAll())
    {
        showError(ConstatModel->lastError());
        error = 1;
    }

    if (!error)
    {
        ui->CustomerCombo->setEnabled(false);
        ui->DateEdit->setEnabled(false);
        ui->StatusCombo->setEnabled(false);
        ui->removeCourse->setEnabled(false);
        ui->addCourse->setEnabled(false);
        ui->removeConstat->setEnabled(false);
        ui->addConstat->setEnabled(false);
        ui->duplicateConstat->setEnabled(false);
        ui->duplicateCourses->setEnabled(false);
        ui->saveInvoice->setEnabled(false);
        ui->newInvoice->setEnabled(true);
        ui->cancelInvoice->setEnabled(false);
        ui->deleteInvoice->setEnabled(true);
        ui->NoteTextEdit->setEnabled(false);
        ui->CoursesTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->ConstatTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        actualiseInvoiceTempInfos();
        statusBar()->showMessage(tr("Data saved"), 4000);
        if (unsavedElemList.contains("InvoiceData",Qt::CaseSensitive))unsavedElemList.removeOne("InvoiceData");

        int row = mapper->currentIndex();
        mapper->setCurrentIndex(qMin(row, invoiceModel->rowCount() - 1));
        mapper->submit(); // Call to update invoiceNbr in invoices when new invoice is created
    }


}

void MainWindow::showError(const QSqlError &err)
{
    QMessageBox::critical(this, tr("Database Error"),
                          err.text());
}

void MainWindow::onInvoiceDataChanged()
{
    if (!unsavedElemList.contains("InvoiceData",Qt::CaseSensitive))unsavedElemList << "InvoiceData";
    ui->saveInvoice->setEnabled(true);
    refreshCoursesView();
    refreshConstatView();
}

void MainWindow::onCustomerDataChanged(void)
{
    if (!unsavedElemList.contains("CustomerData",Qt::CaseSensitive))unsavedElemList << "CustomerData";
    ui->SaveCustomer->setEnabled(true);
    refreshCustomerView();
}

void MainWindow::actualiseAmountField(float subtotal, float total)
{
    ui->SubtotalValue->setText(QString("%1 €").arg(subtotal));
    ui->TotalValue->setText(QString("%1 €").arg(total));
}

bool MainWindow::okayToClose(void)
{
    if(!unsavedElemList.isEmpty()){

        QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                                  QApplication::applicationName(),
                                                                  tr("Do you want to save the changes "
                                                                     "made to the database file %1?").arg("InvoiceDB"),
                                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if(reply == QMessageBox::Yes){
            if (unsavedElemList.contains("InvoiceData",Qt::CaseSensitive))
            {
                if(!CoursesModel->submitAll())  qDebug() << CoursesModel->lastError();
                if(!ConstatModel->submitAll())  qDebug() << ConstatModel->lastError();
                if(!invoiceModel->submitAll())  qDebug() << invoiceModel->lastError();
                mapper->submit();
            }
            if (unsavedElemList.contains("CustomerData",Qt::CaseSensitive))
            {
                if(!CustomerModel->submitAll())  qDebug() << CustomerModel->lastError();
            }
            return true;
        } else if(reply == QMessageBox::No) {
            CoursesModel->revertAll();
            ConstatModel->revertAll();
            return true;
        } else return false;


    } else return true;

}

void MainWindow::closeEvent( QCloseEvent* event )
{
    if(okayToClose())
    {
        qDebug() << "closing";
        QMainWindow::closeEvent(event);
    } else {
        event->ignore();
    }
}


void MainWindow::on_newInvoice_clicked()
{
    ui->newInvoice->setEnabled(false);
    ui->deleteInvoice->setEnabled(false);
    ui->cancelInvoice->setEnabled(true);
    ui->CustomerCombo->setEnabled(true);
    ui->DateEdit->setEnabled(true);
    ui->StatusCombo->setEnabled(true);
    ui->removeCourse->setEnabled(true);
    ui->addCourse->setEnabled(true);
    ui->removeConstat->setEnabled(true);
    ui->addConstat->setEnabled(true);
    ui->duplicateConstat->setEnabled(true);
    ui->duplicateCourses->setEnabled(true);
    ui->saveInvoice->setEnabled(true);

    int row = invoiceModel->rowCount();
    invoiceModel->insertRow(row);
    mapper->setCurrentIndex(row);
    ui->DateEdit->setDate(QDate::currentDate());
    ui->CustomerCombo->setFocus();

    ui->CoursesTableView->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->ConstatTableView->setEditTriggers(QAbstractItemView::DoubleClicked);


}

void MainWindow::on_cancelInvoice_clicked()
{
    ui->addCourse->setEnabled(false);
    ui->removeCourse->setEnabled(false);
    ui->addConstat->setEnabled(false);
    ui->duplicateConstat->setEnabled(false);
    ui->duplicateCourses->setEnabled(false);
    ui->removeConstat->setEnabled(false);
    ui->CustomerCombo->setEnabled(false);
    ui->DateEdit->setEnabled(false);

    ui->deleteInvoice->setEnabled(true);
    ui->newInvoice->setEnabled(true);
    ui->cancelInvoice->setEnabled(false);
    int row = invoiceModel->rowCount()-1;
    invoiceModel->removeRow(row);
    invoiceModel->submit();
    mapper->toLast();
    ui->CoursesTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->ConstatTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

}

void MainWindow::on_deleteInvoice_clicked()
{
    int row = mapper->currentIndex();
    QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                              QApplication::applicationName(),
                                                              tr("Do you want to delete this Invoice"),
                                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if(reply == QMessageBox::Yes){
        if (invoiceModel->removeRow(row))
            statusBar()->showMessage(tr("Invoice deleted"), 4000);
        else
            statusBar()->showMessage(tr("Cannot delete invoice"), 4000);
        mapper->submit();
        invoiceModel->submitAll();
        invoiceModel->select();
        mapper->setCurrentIndex(qMin(row, invoiceModel->rowCount() - 1));

    }
}

void MainWindow::on_previewInvoice_clicked()
{
    generateReport(previewReport,reportGroup0);
}

void MainWindow::generateReport(int type, int group)
{
    QString fileName;
    QtRPT *report = new QtRPT(this);

    if( group == reportGroup0)
    {
        int CoursesCount = CoursesModel->rowCount();
        int ConstatCount = ConstatModel->rowCount();


        if(CoursesCount > 0 && ConstatCount > 0)
        {
            fileName = QApplication::applicationDirPath() + "/dependencies/report_files/ReportTypeDefault.xml";
            ReportType = ReportTypeDefault;
            reportGroup = reportGroup0;
        }
        else if (CoursesCount == 0 && ConstatCount > 0)
        {
            fileName = QApplication::applicationDirPath() + "/dependencies/report_files/ReportTypeConstatOnly.xml";
            ReportType = ReportTypeConstatOnly;
            reportGroup = reportGroup0;
        }
        else if (CoursesCount > 0 && ConstatCount == 0)
        {
            fileName = QApplication::applicationDirPath() + "/dependencies/report_files/ReportTypeCoursesOnly.xml";
            ReportType = ReportTypeCoursesOnly;
            reportGroup = reportGroup0;
        }
        else
        {
            qDebug()<<"Cannot generate Report. Courses and Constat are empty";
            statusBar()->showMessage(tr("Cannot generate Report. Courses and Constat are empty"), 4000);
            return;
        }


        if (report->loadReport(fileName) == false) {
            QString warnStr = QString("Report file %1 not found ").arg(fileName);
            QMessageBox::warning(this, "", warnStr,QMessageBox::Ok);
            statusBar()->showMessage(warnStr, 4000);
            return;
        }

        QObject::connect(report, SIGNAL(setValue(const int, const QString, QVariant&, const int)),
                         this, SLOT(setValue(const int, const QString, QVariant&, const int)));

        switch (ReportType) {
        case ReportTypeDefault:
            report->recordCount << 1;
            report->recordCount << CoursesCount;
            report->recordCount << ConstatCount;
            break;
        case ReportTypeCoursesOnly:
            report->recordCount << 1;
            report->recordCount << CoursesCount;
            break;
        case ReportTypeConstatOnly:
            report->recordCount << 1;
            report->recordCount << ConstatCount;
            break;
        default:
            report->recordCount << 1;
            break;
        }

        if(!unsavedElemList.isEmpty())
        {
            QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                                      QApplication::applicationName(),
                                                                      tr("Do you want to save the changes before preview ?"),
                                                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if(reply == QMessageBox::Yes){
                on_saveInvoice_clicked();
            } else
            {
                on_revert_clicked();
                ui->CoursesTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
                ui->ConstatTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
            }
        }
    }
    else // if (group ==reportGroup1)
    {
        fileName = QApplication::applicationDirPath() + "/dependencies/report_files/Report_Cumul.xml";
        reportGroup = reportGroup1;

        if (report->loadReport(fileName) == false) {
            QString warnStr = QString("Report file %1 not found ").arg(fileName);
            QMessageBox::warning(this, "", warnStr,QMessageBox::Ok);
            statusBar()->showMessage(warnStr, 4000);
            return;
        }

        QObject::connect(report, SIGNAL(setValue(const int, const QString, QVariant&, const int)),
                         this, SLOT(setValue(const int, const QString, QVariant&, const int)));
        report->recordCount << SearchInvoiceModel->rowCount();
    }

    QString tmpPath;
    switch (type)
    {
    case previewReport:
        report->printExec();
        break;
    case printReport:
        if(QDir(FacturePath).exists())
        {
            tmpPath = actuInvoiceTitle;
            tmpPath.remove("FACTURE N° ");
            tmpPath.replace("/","_");
            actuInvoicePath = FacturePath+ "/" + tmpPath + ".pdf";
        }
        else
        {
            QDir().mkdir(FacturePath);
        }
        report->printPDF((actuInvoicePath), true);
        //report->printExec(true,true,"Kyocera_FS-C5150DN");
        break;
    case saveReport:
        if(QDir(FacturePath).exists())
        {
            tmpPath = actuInvoiceTitle;
            tmpPath.remove("FACTURE N° ");
            tmpPath.replace("/","_");
            actuInvoicePath = FacturePath+ "/" + tmpPath + ".pdf";
        }
        else
        {
            QDir().mkdir(FacturePath);
        }
        report->printPDF((actuInvoicePath), false);
        break;
    default:

        break;
    }

}
float MainWindow::getMontantHTCumulReport(int tva, const QString& companyname)
{
    float returnvalue = 0;
    QSqlQuery query;

    query.exec(QString("select sum(PaymentAmount) from ("
                       " select  CustomerID, InvoiceID, InvoiceDate, TVA, PaymentAmount from Customers"
                       " inner join ("
                       " select * from Invoices "
                       " inner join Constat using (InvoiceID)"
                       " where Status=0)"
                       " using (CustomerID) where CompanyName='%1'"
                       " union all"
                       " select  CustomerID, InvoiceID, InvoiceDate, TVA, PaymentAmount from Customers"
                       " inner join ("
                       " select * from Invoices"
                       " inner join Courses using (InvoiceID)"
                       " where Status=0)"
                       " using (CustomerID) where CompanyName='%2') where TVA= %3"
                       " group by TVA").arg(companyname).arg(companyname).arg(tva));
    while (query.next())
    {
        returnvalue = query.value(0).toFloat();
    }

    return returnvalue;
}

void MainWindow::setValue(const int recNo, const QString paramName, QVariant &paramValue, const int reportPage)
{
    Q_UNUSED(reportPage);

    if(reportGroup == reportGroup0)
    {
        switch (ReportType)
        {
        case ReportTypeDefault:
            float tmpfloat;
            if (reportPage == 0) {
                if (paramName == "InvoiceDateTitle")
                {
                    QString tmpString = locale.toString(actuDate,"dd MMMM yyyy");
                    if (tmpString == "") return;
                    else paramValue = tmpString;
                }
                if (paramName == "InvoiceDate")
                {
                    paramValue = actuDate.toString("dd.MM.yyyy");
                }
                if (paramName == "InvoiceTitle")
                {
                    paramValue = QString("FACTURE N° %1/%2%3").arg(actuDate.year())\
                            .arg(locale.toString(actuDate,"MMM")) \
                            .arg(actuInvoiceID).remove(".");
                }
                if (paramName == "CoursesTVA1")
                {
                    tmpfloat  = getCoursesTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CoursesTVA2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "ConstatTVA1")
                {
                    tmpfloat = getConstatTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "ConstatTVA2")
                {
                    tmpfloat = getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "MontantHTTVA1")
                {
                    tmpfloat = getCoursesTVA1Sum(actuInvoiceID)+ getConstatTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "MontantHTTVA2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "MontantHTSum")
                {
                    tmpfloat = getCoursesTVA1Sum(actuInvoiceID)+getConstatTVA1Sum(actuInvoiceID)+getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");

                }
                if (paramName == "TVA")
                {
                    tmpfloat = (getCoursesTVA1Sum(actuInvoiceID)+getConstatTVA1Sum(actuInvoiceID))*0.2;
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "TotalTTC1")
                {
                    tmpfloat = (getCoursesTVA1Sum(actuInvoiceID)+getConstatTVA1Sum(actuInvoiceID))*1.2;
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "TotalTTC2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "TotalTTCSum")
                {
                    tmpfloat = (getCoursesTVA1Sum(actuInvoiceID)+ getConstatTVA1Sum(actuInvoiceID))*1.2 + getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "Count")
                {
                    paramValue = getCount(actuInvoiceID);
                }
                if (paramName == "CompanyName")
                {
                    paramValue = getCompanyName(actuInvoiceID);
                }
                if (paramName == "Address")
                {
                    paramValue = getAddress1(actuInvoiceID);
                }
                if (paramName == "PostalCode")
                {
                    paramValue = getAddress2(actuInvoiceID);
                }
                if (paramName == "CountryOrRegion")
                {
                    paramValue = getAddress3(actuInvoiceID);
                }
                if(paramName == "MailAddress")
                {
                    paramValue = this->MailAddress;
                }
                if(paramName == "EnterpriseName")
                {
                    paramValue = this->EnterpriseName;
                }
                if(paramName == "AddressLine1")
                {
                    paramValue = this->AddressLine1;
                }
                if(paramName == "AddressLine2")
                {
                    paramValue = this->AddressLine2;
                }
                if(paramName == "AddressLine3")
                {
                    paramValue = this->AddressLine3;
                }
                if(paramName == "TVANr")
                {
                    paramValue = this->TVANr;
                }
                if(paramName == "TelNr")
                {
                    paramValue = this->TelNr;
                }
                if(paramName == "InvoiceLine1")
                {
                    paramValue = this->InvoiceLine1;
                }
            }
            if (reportPage == 1) {
                if (paramName == "InvoiceDateTitle")
                {
                    QString tmpString = locale.toString(actuDate,"dd MMMM yyyy");
                    if (tmpString == "") return;
                    else paramValue = tmpString;
                }
                if (paramName == "PickUpDate")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,1), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,1), Qt::DisplayRole).toDate().toString("dd.MM.yyyy");
                }
                if (paramName == "Reference")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,2), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,2), Qt::DisplayRole).toString();
                }
                if (paramName == "ShippingLocation")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,3), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,3), Qt::DisplayRole).toString();
                }
                if (paramName == "ShippedFrom")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,4), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,4), Qt::DisplayRole).toString();
                }
                if (paramName == "DeliveryLocation")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,5), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,5), Qt::DisplayRole).toString();
                }
                if (paramName == "TVA")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,6), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,6), Qt::DisplayRole).toString();
                }
                if (paramName == "PaymentAmount")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,7), Qt::DisplayRole).toString() == 0) return;
                    tmpfloat   = CoursesModel->data(CoursesModel->index(recNo,7), Qt::DisplayRole).toFloat();
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CoursesTVA1")
                {
                    tmpfloat = getCoursesTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CoursesTVA2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CompanyName")
                {
                    paramValue = getCompanyName(actuInvoiceID);
                }
                if (paramName == "Address")
                {
                    paramValue = getAddress1(actuInvoiceID);
                }
                if (paramName == "PostalCode")
                {
                    paramValue = getAddress2(actuInvoiceID);
                }
                if (paramName == "CountryOrRegion")
                {
                    paramValue = getAddress3(actuInvoiceID);
                }
                if(paramName == "MailAddress")
                {
                    paramValue = this->MailAddress;
                }
                if(paramName == "EnterpriseName")
                {
                    paramValue = this->EnterpriseName;
                }
                if(paramName == "AddressLine1")
                {
                    paramValue = this->AddressLine1;
                }
                if(paramName == "AddressLine2")
                {
                    paramValue = this->AddressLine2;
                }
                if(paramName == "AddressLine3")
                {
                    paramValue = this->AddressLine3;
                }
                if(paramName == "TVANr")
                {
                    paramValue = this->TVANr;
                }
                if(paramName == "TelNr")
                {
                    paramValue = this->TelNr;
                }
            }
            if (reportPage == 2) {
                if (paramName == "InvoiceDateTitle")
                {
                    QString tmpString = locale.toString(actuDate,"dd MMMM yyyy");
                    if (tmpString == "") return;
                    else paramValue = tmpString;
                }
                if (paramName == "ServiceDate")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,1), Qt::DisplayRole).toString() == 0) return;
                    paramValue = ConstatModel->data(ConstatModel->index(recNo,1), Qt::DisplayRole).toDate().toString("dd.MM.yyyy");
                }

                if (paramName == "ServiceLocation")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,2), Qt::DisplayRole).toString() == 0) return;
                    paramValue = ConstatModel->data(ConstatModel->index(recNo,2), Qt::DisplayRole).toString();
                }
                if (paramName == "Reference")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,3), Qt::DisplayRole).toString() == 0) return;
                    paramValue = ConstatModel->data(ConstatModel->index(recNo,3), Qt::DisplayRole).toString();
                }
                if (paramName == "TVA")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,4), Qt::DisplayRole).toString() == 0) return;
                    paramValue = ConstatModel->data(ConstatModel->index(recNo,4), Qt::DisplayRole).toString();
                }
                if (paramName == "PaymentAmount")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,5), Qt::DisplayRole).toString() == 0) return;
                    tmpfloat = ConstatModel->data(ConstatModel->index(recNo,5), Qt::DisplayRole).toFloat();
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "ConstatTVA1")
                {
                    tmpfloat = getConstatTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");

                }
                if (paramName == "ConstatTVA2")
                {
                    tmpfloat = getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CompanyName")
                {
                    paramValue = getCompanyName(actuInvoiceID);
                }
                if (paramName == "Address")
                {
                    paramValue = getAddress1(actuInvoiceID);
                }
                if (paramName == "PostalCode")
                {
                    paramValue = getAddress2(actuInvoiceID);
                }
                if (paramName == "CountryOrRegion")
                {
                    paramValue = getAddress3(actuInvoiceID);
                }
                if(paramName == "MailAddress")
                {
                    paramValue = this->MailAddress;
                }
                if(paramName == "EnterpriseName")
                {
                    paramValue = this->EnterpriseName;
                }
                if(paramName == "AddressLine1")
                {
                    paramValue = this->AddressLine1;
                }
                if(paramName == "AddressLine2")
                {
                    paramValue = this->AddressLine2;
                }
                if(paramName == "AddressLine3")
                {
                    paramValue = this->AddressLine3;
                }
                if(paramName == "TVANr")
                {
                    paramValue = this->TVANr;
                }
                if(paramName == "TelNr")
                {
                    paramValue = this->TelNr;
                }
            }
            break;
        case ReportTypeCoursesOnly:
            if (reportPage == 0) {
                if (paramName == "InvoiceDateTitle")
                {
                    QString tmpString = locale.toString(actuDate,"dd MMMM yyyy");
                    if (tmpString == "") return;
                    else paramValue = tmpString;
                }
                if (paramName == "InvoiceDate")
                {
                    paramValue = actuDate.toString("dd.MM.yyyy");
                }
                if (paramName == "InvoiceTitle")
                {
                    paramValue = QString("FACTURE N° %1/%2%3").arg(actuDate.year())\
                            .arg(locale.toString(actuDate,"MMM")) \
                            .arg(actuInvoiceID).remove(".");
                }
                if (paramName == "CoursesTVA1")
                {
                    tmpfloat  = getCoursesTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CoursesTVA2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "ConstatTVA1")
                {
                    tmpfloat = getConstatTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "ConstatTVA2")
                {
                    tmpfloat = getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "MontantHTTVA1")
                {
                    tmpfloat = getCoursesTVA1Sum(actuInvoiceID)+ getConstatTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "MontantHTTVA2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "MontantHTSum")
                {
                    tmpfloat = getCoursesTVA1Sum(actuInvoiceID)+getConstatTVA1Sum(actuInvoiceID)+getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");

                }
                if (paramName == "TVA")
                {
                    tmpfloat = (getCoursesTVA1Sum(actuInvoiceID)+getConstatTVA1Sum(actuInvoiceID))*0.2;
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "TotalTTC1")
                {
                    tmpfloat = (getCoursesTVA1Sum(actuInvoiceID)+getConstatTVA1Sum(actuInvoiceID))*1.2;
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "TotalTTC2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "TotalTTCSum")
                {
                    tmpfloat = (getCoursesTVA1Sum(actuInvoiceID)+ getConstatTVA1Sum(actuInvoiceID))*1.2 + getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "Count")
                {
                    paramValue = getCount(actuInvoiceID);
                }
                if (paramName == "CompanyName")
                {
                    paramValue = getCompanyName(actuInvoiceID);
                }
                if (paramName == "Address")
                {
                    paramValue = getAddress1(actuInvoiceID);
                }
                if (paramName == "PostalCode")
                {
                    paramValue = getAddress2(actuInvoiceID);
                }
                if (paramName == "CountryOrRegion")
                {
                    paramValue = getAddress3(actuInvoiceID);
                }
                if(paramName == "MailAddress")
                {
                    paramValue = this->MailAddress;
                }
                if(paramName == "EnterpriseName")
                {
                    paramValue = this->EnterpriseName;
                }
                if(paramName == "AddressLine1")
                {
                    paramValue = this->AddressLine1;
                }
                if(paramName == "AddressLine2")
                {
                    paramValue = this->AddressLine2;
                }
                if(paramName == "AddressLine3")
                {
                    paramValue = this->AddressLine3;
                }
                if(paramName == "InvoiceLine1")
                {
                    paramValue = this->InvoiceLine1;
                }
                if(paramName == "TVANr")
                {
                    paramValue = this->TVANr;
                }
                if(paramName == "TelNr")
                {
                    paramValue = this->TelNr;
                }
            }
            if (reportPage == 1) {
                if (paramName == "InvoiceDateTitle")
                {
                    QString tmpString = locale.toString(actuDate,"dd MMMM yyyy");
                    if (tmpString == "") return;
                    else paramValue = tmpString;
                }
                if (paramName == "PickUpDate")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,1), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,1), Qt::DisplayRole).toDate().toString("dd.MM.yyyy");
                }
                if (paramName == "Reference")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,2), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,2), Qt::DisplayRole).toString();
                }
                if (paramName == "ShippingLocation")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,3), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,3), Qt::DisplayRole).toString();
                }
                if (paramName == "ShippedFrom")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,4), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,4), Qt::DisplayRole).toString();
                }
                if (paramName == "DeliveryLocation")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,5), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,5), Qt::DisplayRole).toString();
                }
                if (paramName == "TVA")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,6), Qt::DisplayRole).toString() == 0) return;
                    paramValue = CoursesModel->data(CoursesModel->index(recNo,6), Qt::DisplayRole).toString();
                }
                if (paramName == "PaymentAmount")
                {
                    if (CoursesModel->data(CoursesModel->index(recNo,7), Qt::DisplayRole).toString() == 0) return;
                    tmpfloat   = CoursesModel->data(CoursesModel->index(recNo,7), Qt::DisplayRole).toFloat();
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CoursesTVA1")
                {
                    tmpfloat = getCoursesTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CoursesTVA2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CompanyName")
                {
                    paramValue = getCompanyName(actuInvoiceID);
                }
                if (paramName == "Address")
                {
                    paramValue = getAddress1(actuInvoiceID);
                }
                if (paramName == "PostalCode")
                {
                    paramValue = getAddress2(actuInvoiceID);
                }
                if (paramName == "CountryOrRegion")
                {
                    paramValue = getAddress3(actuInvoiceID);
                }
                if(paramName == "MailAddress")
                {
                    paramValue = this->MailAddress;
                }
                if(paramName == "EnterpriseName")
                {
                    paramValue = this->EnterpriseName;
                }
                if(paramName == "AddressLine1")
                {
                    paramValue = this->AddressLine1;
                }
                if(paramName == "AddressLine2")
                {
                    paramValue = this->AddressLine2;
                }
                if(paramName == "AddressLine3")
                {
                    paramValue = this->AddressLine3;
                }
                if(paramName == "TVANr")
                {
                    paramValue = this->TVANr;
                }
                if(paramName == "TelNr")
                {
                    paramValue = this->TelNr;
                }
            }
            break;
        case ReportTypeConstatOnly:
            if (reportPage == 0) {
                if (paramName == "InvoiceDateTitle")
                {
                    QString tmpString = locale.toString(actuDate,"dd MMMM yyyy");
                    if (tmpString == "") return;
                    else paramValue = tmpString;
                }
                if (paramName == "InvoiceDate")
                {
                    paramValue = actuDate.toString("dd.MM.yyyy");
                }
                if (paramName == "InvoiceTitle")
                {
                    paramValue = QString("FACTURE N° %1/%2%3").arg(actuDate.year())\
                            .arg(locale.toString(actuDate,"MMM")) \
                            .arg(actuInvoiceID).remove(".");
                }
                if (paramName == "CoursesTVA1")
                {
                    tmpfloat  = getCoursesTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CoursesTVA2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "ConstatTVA1")
                {
                    tmpfloat = getConstatTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "ConstatTVA2")
                {
                    tmpfloat = getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "MontantHTTVA1")
                {
                    tmpfloat = getCoursesTVA1Sum(actuInvoiceID)+ getConstatTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "MontantHTTVA2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "MontantHTSum")
                {
                    tmpfloat = getCoursesTVA1Sum(actuInvoiceID)+getConstatTVA1Sum(actuInvoiceID)+getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");

                }
                if (paramName == "TVA")
                {
                    tmpfloat = (getCoursesTVA1Sum(actuInvoiceID)+getConstatTVA1Sum(actuInvoiceID))*0.2;
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "TotalTTC1")
                {
                    tmpfloat = (getCoursesTVA1Sum(actuInvoiceID)+getConstatTVA1Sum(actuInvoiceID))*1.2;
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "TotalTTC2")
                {
                    tmpfloat = getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "TotalTTCSum")
                {
                    tmpfloat = (getCoursesTVA1Sum(actuInvoiceID)+ getConstatTVA1Sum(actuInvoiceID))*1.2 + getCoursesTVA2Sum(actuInvoiceID)+getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "Count")
                {
                    paramValue = getCount(actuInvoiceID);
                }
                if (paramName == "CompanyName")
                {
                    paramValue = getCompanyName(actuInvoiceID);
                }
                if (paramName == "Address")
                {
                    paramValue = getAddress1(actuInvoiceID);
                }
                if (paramName == "PostalCode")
                {
                    paramValue = getAddress2(actuInvoiceID);
                }
                if (paramName == "CountryOrRegion")
                {
                    paramValue = getAddress3(actuInvoiceID);
                }
                if (paramName == "MailAddress")
                {
                    paramValue = this->MailAddress;
                }
                if (paramName == "EnterpriseName")
                {
                    paramValue = this->EnterpriseName;
                }
                if (paramName == "AddressLine1")
                {
                    paramValue = this->AddressLine1;
                }
                if (paramName == "AddressLine2")
                {
                    paramValue = this->AddressLine2;
                }
                if (paramName == "AddressLine3")
                {
                    paramValue = this->AddressLine3;
                }
                if (paramName == "InvoiceLine1")
                {
                    paramValue = this->InvoiceLine1;
                }
                if (paramName == "TVANr")
                {
                    paramValue = this->TVANr;
                }
                if (paramName == "TelNr")
                {
                    paramValue = this->TelNr;
                }
            }
            if (reportPage == 1) {
                if (paramName == "InvoiceDateTitle")
                {
                    QString tmpString = locale.toString(actuDate,"dd MMMM yyyy");
                    if (tmpString == "") return;
                    else paramValue = tmpString;
                }
                if (paramName == "ServiceDate")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,1), Qt::DisplayRole).toString() == 0) return;
                    paramValue = ConstatModel->data(ConstatModel->index(recNo,1), Qt::DisplayRole).toDate().toString("dd.MM.yyyy");
                }

                if (paramName == "ServiceLocation")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,2), Qt::DisplayRole).toString() == 0) return;
                    paramValue = ConstatModel->data(ConstatModel->index(recNo,2), Qt::DisplayRole).toString();
                }
                if (paramName == "Reference")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,3), Qt::DisplayRole).toString() == 0) return;
                    paramValue = ConstatModel->data(ConstatModel->index(recNo,3), Qt::DisplayRole).toString();
                }
                if (paramName == "TVA")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,4), Qt::DisplayRole).toString() == 0) return;
                    paramValue = ConstatModel->data(ConstatModel->index(recNo,4), Qt::DisplayRole).toString();
                }
                if (paramName == "PaymentAmount")
                {
                    if (ConstatModel->data(ConstatModel->index(recNo,5), Qt::DisplayRole).toString() == 0) return;
                    tmpfloat = ConstatModel->data(ConstatModel->index(recNo,5), Qt::DisplayRole).toFloat();
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "ConstatTVA1")
                {
                    tmpfloat = getConstatTVA1Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");

                }
                if (paramName == "ConstatTVA2")
                {
                    tmpfloat = getConstatTVA2Sum(actuInvoiceID);
                    paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
                }
                if (paramName == "CompanyName")
                {
                    paramValue = getCompanyName(actuInvoiceID);
                }
                if (paramName == "Address")
                {
                    paramValue = getAddress1(actuInvoiceID);
                }
                if (paramName == "PostalCode")
                {
                    paramValue = getAddress2(actuInvoiceID);
                }
                if (paramName == "CountryOrRegion")
                {
                    paramValue = getAddress3(actuInvoiceID);
                }
                if(paramName == "MailAddress")
                {
                    paramValue = this->MailAddress;
                }
                if(paramName == "EnterpriseName")
                {
                    paramValue = this->EnterpriseName;
                }
                if(paramName == "AddressLine1")
                {
                    paramValue = this->AddressLine1;
                }
                if(paramName == "AddressLine2")
                {
                    paramValue = this->AddressLine2;
                }
                if(paramName == "AddressLine3")
                {
                    paramValue = this->AddressLine3;
                }
                if(paramName == "TVANr")
                {
                    paramValue = this->TVANr;
                }
                if(paramName == "TelNr")
                {
                    paramValue = this->TelNr;
                }
            }
            break;

        default:
            break;
        }
    }
    else /* REPORTGROUP1*/
    {

        if (reportPage == 0)
        {   // HACK: Customer already chosen. So all invoices are from one customer
            float tmpfloat;
            int tmpActuInvoiceID = getInvoiceIDfromInvoiceNbr(SearchInvoiceModel->data(SearchInvoiceModel->index(recNo,3), Qt::DisplayRole).toString());

            if (paramName == "CompanyName")
            {
                paramValue = getCompanyName(tmpActuInvoiceID);
            }
            if (paramName == "Address")
            {
                paramValue = getAddress1(tmpActuInvoiceID);
            }
            if (paramName == "PostalCode")
            {
                paramValue = getAddress2(tmpActuInvoiceID);
            }
            if (paramName == "CountryOrRegion")
            {
                paramValue = getAddress3(tmpActuInvoiceID);
            }
            if(paramName == "InvoiceID")
            {
                paramValue = SearchInvoiceModel->data(SearchInvoiceModel->index(recNo,3), Qt::DisplayRole).toString();
            }
            if (paramName == "MontantHTTVA1")
            {
                tmpfloat = getCoursesTVA1Sum(tmpActuInvoiceID)+ getConstatTVA1Sum(tmpActuInvoiceID);
                paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
            }
            if (paramName == "MontantHTTVA2")
            {
                tmpfloat = getCoursesTVA2Sum(tmpActuInvoiceID)+getConstatTVA2Sum(tmpActuInvoiceID);
                paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
            }
            if (paramName == "TVA")
            {
                tmpfloat = (getCoursesTVA1Sum(tmpActuInvoiceID)+getConstatTVA1Sum(tmpActuInvoiceID))*0.2;
                paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
            }

            if (paramName == "TotalTTCSum")
            {
                tmpfloat = (getCoursesTVA1Sum(tmpActuInvoiceID)+ getConstatTVA1Sum(tmpActuInvoiceID))*1.2 + getCoursesTVA2Sum(tmpActuInvoiceID)+getConstatTVA2Sum(tmpActuInvoiceID);
                paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
            }
            if(paramName == "Status")
            {
                if(SearchInvoiceModel->data(SearchInvoiceModel->index(recNo,4), Qt::DisplayRole).toInt())
                    paramValue = "payé";
                else
                    paramValue = " pas payé";
            }

            if(paramName == "Cumul_MontantHTTVA1") {
                tmpfloat = getMontantHTCumulReport(1, getCompanyName(tmpActuInvoiceID));
                paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
            }
            if(paramName == "Cumul_MontantHTTVA2") {
                tmpfloat = getMontantHTCumulReport(2, getCompanyName(tmpActuInvoiceID));
                paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
            }
            if(paramName == "Cumul_tva") {
                tmpfloat = getMontantHTCumulReport(1, getCompanyName(tmpActuInvoiceID))*0.2;
                paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
            }

            if(paramName == "Cumul_MontantHTTVA1_incluTVA") {
                tmpfloat = getMontantHTCumulReport(1, getCompanyName(tmpActuInvoiceID))*1.2;
                paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
            }

            if(paramName == "Cumul_TotalTTC") {
                tmpfloat = getMontantHTCumulReport(1, getCompanyName(tmpActuInvoiceID))*1.2 + getMontantHTCumulReport(2, getCompanyName(tmpActuInvoiceID));
                paramValue = QString::number(tmpfloat, 'f',2).replace(".",",");
            }
            if(paramName == "Year")
            {
                paramValue = QDate::currentDate().year();
            }

            if(paramName == "ReportDate")
            {
                paramValue = locale.toString(QDate::currentDate(),"dd MMMM yyyy");
            }

            if(paramName == "UserName")
            {
                paramValue = this->UserName;
            }
            if(paramName == "MailAddress")
            {
                paramValue = this->MailAddress;
            }
            if(paramName == "EnterpriseName")
            {
                paramValue = this->EnterpriseName;
            }
            if(paramName == "AddressLine1")
            {
                paramValue = this->AddressLine1;
            }
            if(paramName == "AddressLine2")
            {
                paramValue = this->AddressLine2;
            }
            if(paramName == "AddressLine3")
            {
                paramValue = this->AddressLine3;
            }
            if(paramName == "InvoiceLine1")
            {
                paramValue = this->InvoiceLine1;
            }
            if(paramName == "TVANr")
            {
                paramValue = this->TVANr;
            }
            if(paramName == "TelNr")
            {
                paramValue = this->TelNr;
            }
        }

    }
}

int MainWindow::getInvoiceIDfromInvoiceNbr(const QString& InvoiceNbr)
{
    QSqlQuery query;
    int returnValue = 0;
    query.exec(QString("SELECT InvoiceID FROM Invoices WHERE InvoiceNbr= '%1'").arg(InvoiceNbr));
    while (query.next())
    {
        returnValue = query.value(0).toInt();
    }
    return returnValue;
}

void MainWindow::on_addCourse_clicked()
{
    CoursesModel->insertRow(CoursesModel->rowCount());
    QModelIndex index = ui->CoursesTableView->model()->index(CoursesModel->rowCount()-1, 2);
    ui->CoursesTableView->scrollTo(index);

}

void MainWindow::on_removeCourse_clicked()
{
    QModelIndexList indexes =  ui->CoursesTableView->selectionModel()->selectedRows();
    int countRow = indexes.count();
    for( int i = countRow; i > 0; i--){
        CoursesModel->removeRow( indexes.at(i-1).row(), QModelIndex());
    }

    onInvoiceDataChanged();
}

void MainWindow::on_openFolder_clicked()
{
    QString folderPath = QApplication::applicationDirPath() + "/FACTURES";

    if(QDir(folderPath).exists())
    {
        QDesktopServices::openUrl(folderPath);
    }
    else
    {
        QDir().mkdir(folderPath);
        QDesktopServices::openUrl(folderPath);
    }

}

void MainWindow::on_addConstat_clicked()
{
    ConstatModel->insertRow(ConstatModel->rowCount());
    QModelIndex index = ui->ConstatTableView->model()->index(ConstatModel->rowCount()-1, 2);
    ui->ConstatTableView->scrollTo(index);
}

void MainWindow::on_removeConstat_clicked()
{
    QModelIndexList indexes =  ui->ConstatTableView->selectionModel()->selectedRows();
    int countRow = indexes.count();

    for( int i = countRow; i > 0; i--){
        ConstatModel->removeRow( indexes.at(i-1).row(), QModelIndex());
    }
    onInvoiceDataChanged();
}

void MainWindow::test_slot()
{

}

void MainWindow::on_revert_clicked()
{
    /*CoursesModel->revertAll();
    ConstatModel->revertAll();
    invoiceModel->revertAll();
    mapper->revert();

    if(unsavedElemList.contains("InvoiceData",Qt::CaseSensitive))unsavedElemList.removeOne("InvoiceData");
    if(ui->saveInvoice->isEnabled()) ui->saveInvoice->setEnabled(false);*/
}

float MainWindow::getSubtotal(int record)
{
    QSqlQuery query;
    float subtotal = 0;
    query.exec(QString("SELECT SUM(PaymentAmount) FROM"
                       " (SELECT PaymentAmount FROM Constat"
                       " WHERE InvoiceID=%1"
                       " UNION ALL"
                       " SELECT PaymentAmount FROM Courses"
                       " WHERE InvoiceID=%2)").arg(record).arg(record));

    while (query.next()) {
        subtotal = query.value(0).toFloat();
    }
    return subtotal;
}

float MainWindow::getTotal(int record)
{
    QSqlQuery query;
    float total = 0;
    query.exec(QString(
                   "SELECT SUM(x) FROM"
                   " (SELECT SUM(PaymentAmount)*1.2 AS x FROM Constat"
                   " WHERE TVA=1 and InvoiceID=%1"
                   " UNION ALL"
                   " SELECT SUM(PaymentAmount) AS x FROM Constat"
                   " WHERE TVA=2 and InvoiceID=%2"
                   " UNION ALL"
                   " SELECT SUM(PaymentAmount)*1.2 AS x FROM Courses"
                   " WHERE TVA=1 and InvoiceID=%3"
                   " UNION ALL"
                   " SELECT SUM(PaymentAmount) AS x FROM Courses"
                   " WHERE TVA=2 and InvoiceID=%4)").arg(record).arg(record).arg(record).arg(record));

    while (query.next()) {
        total = query.value(0).toFloat();
    }
    return total;
}

float MainWindow::getCoursesTVA1Sum(int record)
{
    QSqlQuery query;
    float returnValue = 0;
    query.exec(QString("SELECT SUM(PaymentAmount) FROM Courses"
                       " WHERE TVA=1"
                       " AND InvoiceID=%1").arg(record));

    while (query.next()) {
        returnValue = query.value(0).toFloat();
    }
    return returnValue;
}

float MainWindow::getCoursesTVA2Sum(int record)
{
    QSqlQuery query;
    float returnValue = 0;
    query.exec(QString("SELECT SUM(PaymentAmount) FROM Courses"
                       " WHERE TVA=2"
                       " AND InvoiceID=%1").arg(record));

    while (query.next()) {
        returnValue = query.value(0).toFloat();
    }
    return returnValue;
}

float MainWindow::getConstatTVA1Sum(int record)
{
    QSqlQuery query;
    float returnValue = 0;
    query.exec(QString("SELECT SUM(PaymentAmount) FROM Constat"
                       " WHERE TVA=1"
                       " AND InvoiceID=%1").arg(record));

    while (query.next()) {
        returnValue = query.value(0).toFloat();
    }
    return returnValue;
}

float MainWindow::getConstatTVA2Sum(int record)
{
    QSqlQuery query;
    float returnValue = 0;
    query.exec(QString("SELECT SUM(PaymentAmount) FROM Constat"
                       " WHERE TVA=2"
                       " AND InvoiceID=%1").arg(record));

    while (query.next()) {
        returnValue = query.value(0).toFloat();
    }
    return returnValue;
}

int MainWindow::getCount(int record)
{
    QSqlQuery query;
    int returnValue = 0;
    query.exec(QString("SELECT COUNT(PaymentAmount) FROM"
                       " (SELECT PaymentAmount FROM Constat"
                       " WHERE InvoiceID=%1"
                       " UNION ALL"
                       " SELECT PaymentAmount FROM Courses"
                       " WHERE InvoiceID=%2)").arg(record).arg(record));
    while (query.next())
    {
        returnValue = query.value(0).toInt();
    }
    return returnValue;
}

QString MainWindow::getCompanyName(int record)
{
    QSqlQuery query;
    QString returnValue = "";
    query.exec(QString("SELECT CompanyName FROM Invoices JOIN Customers USING(CustomerID) WHERE InvoiceID=%1").arg(record));
    while (query.next())
    {
        returnValue = query.value(0).toString();
    }
    return returnValue;
}


QString MainWindow::getAddress1(int record)
{
    QSqlQuery query;
    QString returnValue = "";
    query.exec(QString("SELECT Address FROM Invoices JOIN Customers USING(CustomerID) WHERE InvoiceID=%1").arg(record));
    while (query.next())
    {
        returnValue = query.value(0).toString();
    }
    return returnValue;
}

QString MainWindow::getAddress2(int record)
{
    QSqlQuery query;
    QString returnValue = "";
    query.exec(QString("SELECT PostalCode ||' '|| City FROM Invoices JOIN Customers USING(CustomerID) WHERE InvoiceID=%1").arg(record));
    while (query.next())
    {
        returnValue = query.value(0).toString();
    }
    return returnValue;
}

QString MainWindow::getAddress3(int record)
{
    QSqlQuery query;
    QString returnValue = "";
    query.exec(QString("SELECT CountryOrRegion FROM Invoices JOIN Customers USING(CustomerID) WHERE InvoiceID=%1").arg(record));
    while (query.next())
    {
        returnValue = query.value(0).toString();
    }
    return returnValue;
}

void MainWindow::on_addNewCustomer_clicked()
{
    ui->SaveCustomer->setEnabled(true);
    ui->cancelAddNewCustomer->setEnabled(true);
    CustomerModel->insertRow(CustomerModel->rowCount());
    ui->CustomerTableView->setEditTriggers(QAbstractItemView::DoubleClicked);
}

void MainWindow::on_deleteCustomer_clicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                              QApplication::applicationName(),
                                                              tr("Do you want to delete this Customer and all its invoices?"),
                                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if(reply == QMessageBox::Yes)
    {
        QModelIndexList indexes =  ui->CustomerTableView->selectionModel()->selectedRows();
        int countRow = indexes.count();
        for( int i = countRow; i > 0; i--)
        {
            CustomerModel->removeRow( indexes.at(i-1).row(), QModelIndex());
        }
        CustomerModel->submitAll();
        CustomerModel->select();
        statusBar()->showMessage(tr("Invoice deleted"), 4000);

        invoiceModel->select();
        relationModel->select(); // update customer combobox
        mapper->toLast();
    }


}

void MainWindow::on_SaveCustomer_clicked()
{
    int error = 0;

    if(!CustomerModel->submitAll())
    {
        showError(CustomerModel->lastError());
        error = 1;
    }

    if (!error)
    {
        ui->CustomerTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        statusBar()->showMessage(tr("Customer saved"), 4000);
        if (unsavedElemList.contains("CustomerData",Qt::CaseSensitive))unsavedElemList.removeOne("CustomerData");

        ui->SaveCustomer->setEnabled(false);
        ui->cancelAddNewCustomer->setEnabled(false);

        relationModel->select(); // update customer combobox
        invoiceModel->select();
        mapper->toLast();
    }
}

void MainWindow::on_cancelAddNewCustomer_clicked()
{
    int row = CustomerModel->rowCount()-1;
    CustomerModel->removeRow(row);
    CustomerModel->submit();
    ui->SaveCustomer->setEnabled(false);
    ui->cancelAddNewCustomer->setEnabled(false);

}

void MainWindow::on_pushButton_clicked()
{
    ui->CustomerTableView->setEditTriggers(QAbstractItemView::DoubleClicked);
}

void MainWindow::on_revertCustomer_clicked()
{

}

void MainWindow::CustomerCheckBox_stateChanged(int state)
{
    if(state == 2) //Checked
    {
        ui->SCustomerCombo->setEnabled(true);
        if(ui->invNbrCheckbox->isChecked()) ui->invNbrCheckbox->setChecked(false);
    }
    else //unchecked
    {
        ui->SCustomerCombo->setEnabled(false);
    }
    on_refreshSearch_clicked();
}
void MainWindow::on_invNbrCheckbox_stateChanged(int state)
{
    if(state == 2) //Checked
    {
        ui->InvNbrLineEdit->setEnabled(true);
        // disable all others
        ui->CustomerCheckBox->setChecked(false);
        ui->StatusCheckBox->setChecked(false);
        ui->DategroupBox->setChecked(false);
    }
    else //unchecked
    {
        ui->InvNbrLineEdit->setEnabled(false);
    }
    on_refreshSearch_clicked();
}

void MainWindow::StatusCheckBox_stateChanged(int state)
{
    if(state == 2) //Checked
    {
        ui->SStatusCombo->setEnabled(true);
        if(ui->invNbrCheckbox->isChecked()) ui->invNbrCheckbox->setChecked(false);
    }
    else //unchecked
    {
        ui->SStatusCombo->setEnabled(false);
    }
    on_refreshSearch_clicked();
}
void MainWindow::on_DategroupBox_clicked()
{
    if(ui->DategroupBox->isChecked())
    {
        if(ui->invNbrCheckbox->isChecked()) ui->invNbrCheckbox->setChecked(false);
    }
 on_refreshSearch_clicked();
}

void MainWindow::on_refreshSearch_clicked()
{
    QStringList filterList;
    QString filter;

    if(ui->invNbrCheckbox->isChecked())
    {
        QString invnbr = ui->InvNbrLineEdit->text();
        filterList.append(QString("InvoiceNbr = '%1'").arg(invnbr));
    }
    else
    {
        if(ui->DategroupBox->isChecked())
        {
            QDate startDate = ui->StartDateEdit->date();
            QDate endDate   = ui->EndDateEdit->date();
            filterList.append(QString("InvoiceDate <= Date('%2')"
                                      " and"
                                      " InvoiceDate >= date('%1')").arg(startDate.toString("yyyy-MM-dd")).arg(endDate.toString("yyyy-MM-dd")));
        }

        if(ui->CustomerCheckBox->isChecked())
        {
            QString customer = ui->SCustomerCombo->currentText();
            filterList.append(QString("CompanyName = '%1'").arg(customer));
        }

        if(ui->StatusCheckBox->isChecked())
        {
            if ( ui->SStatusCombo->currentIndex() == 0) /*unpaid*/
            {
                filterList.append(QString("Status = '%1'").arg(0));
            }
            else
            {
                filterList.append(QString("Status = '%1'").arg(1));
            }
        }
    }
    int listLength = filterList.length();
    for (int i = 0; i < listLength; i++)
    {
        filter += filterList[i] ;
        if (i != listLength-1)filter += " and ";
    }

    SearchInvoiceModel->setFilter(filter);
    SearchInvoiceModel->select();

    ui->openInvoiceview->setEnabled(SearchInvoiceModel->rowCount()>0);
}

QVector<int> MainWindow::getInvoiceList(QString record)
{
    QSqlQuery query;
    QVector<int> total;
    query.exec(QString("SELECT InvoiceID FROM Invoices"
                       " INNER JOIN Customers ON Invoices.CustomerID=Customers.CustomerID"
                       " WHERE CompanyName='%1'").arg(record));
    while (query.next()) {
        total.append(query.value(0).toInt());
    }
    return total;
}

QVector<QString> MainWindow::getAllCustomer(void)
{
    QSqlQuery query;
    QVector<QString> total;
    query.exec(QString("SELECT DISTINCT CompanyName FROM Customers"
                       " INNER JOIN Invoices on Customers.CustomerID=Invoices.CustomerID"));
    while (query.next()) {
        total.append(query.value(0).toString());
    }
    return total;
}

float MainWindow::getMonthSum(int record)
{
    QSqlQuery query;
    QVector<int> invList;
    float returnValue = 0;
    query.exec(QString("SELECT InvoiceID FROM Invoices"
                       " WHERE InvoiceDate"
                       " BETWEEN date('now','start of month','-%1 month')"
                       " AND date('now','start of month','-%2 month','-1 day')").arg(record).arg(record-1));
    while (query.next()) {
        invList.append(query.value(0).toInt());
    }

    foreach (int tmp, invList)
    {
        returnValue += getSubtotal(tmp);
    }
    return returnValue;
}

void MainWindow::on_refreshPlot_clicked()
{
    plotRefresh();
    ui->customPlot->replot();
}

void MainWindow::on_openInvoiceview_clicked()
{
    int mapperindex=0;
    QSqlQuery query;

    QModelIndex index  =  ui->SearchInvoiceView->selectionModel()->currentIndex();

    if (index.isValid())
    {
        QSqlRecord  record =  SearchInvoiceModel->record(index.row());
        int invoiceID = record.value("InvoiceID").toInt();

        query.exec(QString("SELECT count(InvoiceID) FROM Invoices WHERE InvoiceID<%1").arg(invoiceID));

        while (query.next()) {
            mapperindex = query.value(0).toInt();
        }

        mapper->setCurrentIndex(mapperindex);

        ui->InvoiveView->setCurrentIndex(1);
    }
    else
    {
        statusBar()->showMessage(tr("Please select an item"), 4000);
    }
}

void MainWindow::on_sendInvoice_clicked()
{
    int error = 1;

    /*
     * smtp client
     */
    if(!QFile(actuInvoicePath).exists())
    {

        QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                                  QApplication::applicationName(),
                                                                  tr("Cannot send mail. Attachement is missing!\n Do you want to generate the attachement first?"),
                                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if(reply == QMessageBox::Yes)
        {
            generateReport(saveReport, reportGroup0);
            ApplicationShowInfos("Attachement generated.",2000);
        }
        else
        {
            return;
        }

        qDebug() << "Cannot send mail. Generate attachment first";
        statusBar() -> showMessage(tr("Cannot send mail. Generate attachment first"), 2000);

    }

    try {
        error = sendMail();
    }
    catch(...)
    {
        error = 1;
    }

    if (error)
    {
        ApplicationShowInfos("Mail not sent. Error during sending!",4000);
    }
    else
    {
        ApplicationShowInfos("Mail successfully sent.",4000);
        return;
    }
}

void MainWindow::on_printInvoice_clicked()
{
    generateReport(printReport, reportGroup0);
}

void MainWindow::ApplicationShowInfos(QString text, int time_ms)
{
    statusBar() -> showMessage(text, time_ms);
}

int MainWindow::sendMail(void)
{
    int error = 1;
    int customerid = 0;
    /* Gather informations*/
    QString CustomerName,CustomerMail;
    QDate InvDate;

    QSqlQuery query;
    query.exec(QString("SELECT CustomerID, InvoiceDate FROM Invoices WHERE InvoiceID=%1").arg(actuInvoiceID));
    while (query.next())
    {
        customerid = query.value(0).toInt();
        InvDate    = query.value(1).toDate();
    }

    query.exec(QString("SELECT Name, EmailAddress FROM Customers WHERE CustomerID=%1").arg(customerid));
    while (query.next())
    {
        CustomerName = query.value(0).toString();
        CustomerMail = query.value(1).toString();
    }

    QString myMail = this->MailAddress;

    /*
     * Get mail provider
     */
    QString mailProvider;
    QString strPattern = ".*@([A-Za-z0-9.-]+\\.[A-Za-z]{2,4})";
    QRegExp rxlen(strPattern);
    int pos = rxlen.indexIn(this->MailAddress);
    if (pos > -1) {
        mailProvider = rxlen.cap(1);
    }


    QString smtpProvider = "";

    if (QString::compare(mailProvider, "yahoo.fr", Qt::CaseInsensitive) == 0)
        smtpProvider = "smtp.mail.yahoo.com";
    else
    {
        qDebug() << "ERROR: please provide a smtp provider";
        return 1;
    }
    SmtpClient smtp(smtpProvider, 465, SmtpClient::SslConnection);
    smtp.setUser(myMail);
    smtp.setPassword(UserPassword);

    MimeMessage message;
    message.setSender(new EmailAddress(myMail,this->UserName));
    message.addRecipient(new EmailAddress(CustomerMail, CustomerName));
    message.addBcc(new EmailAddress(this->MailAddress, this->UserName));
    message.setSubject(QString("Facture V.L.K du mois de %1").arg(locale.toString(InvDate,"MMMM")));

    MimeText text;
    QString MailText;
    if (actuDate.month()== 4 || actuDate.month()== 8 ||  actuDate.month()== 10)
    {
        MailText = QString("Bonjour %1\n"
                           "vous trouverez ci-joint la facture V.L.K. Express du mois d' %2 %3.\n"
                           "sincères salutations et bonne journée.\n"
                           "%4\n"
                           "V.L.K. Express\n").arg(CustomerName).arg(locale.toString(InvDate,"MMMM")).arg(InvDate.year()).arg(this->UserName);
    }
    else
    {
        MailText = QString("Bonjour %1\n"
                           "vous trouverez ci-joint la facture V.L.K. Express du mois de %2 %3.\n"
                           "sincères salutations et bonne journée.\n"
                           "%4\n"
                           "V.L.K. Express\n").arg(CustomerName).arg(locale.toString(InvDate,"MMMM")).arg(InvDate.year()).arg(this->UserName);
    }


    MailContent MContent;
    MContent.setMailInfos(MailText,CustomerName);
    if(!MContent.exec())
    {
        return 1;
    }
    else
    {
        MailText = MContent.getMailText();
        text.setText(MailText);
        message.addPart(&text);
        message.addPart(new MimeAttachment(new QFile(actuInvoicePath)));
        smtp.connectToHost();
        smtp.login();

        if(smtp.sendMail(message))
        {

            error = 0;
        }
        else
        {

            error = 1;
        }
        smtp.quit();
        return error;
    }
}

void MainWindow::on_duplicateConstat_clicked()
{
    QModelIndex index  =  ui->ConstatTableView->selectionModel()->currentIndex();
    int lastIndex = ConstatModel->rowCount();
    int idx = 0;

    ConstatModel->insertRow(lastIndex);

    if (index.isValid() && ui->ConstatTableView->selectionModel()->isSelected(index))
    {
        idx = index.row();
        ConstatModel->setData(ConstatModel->index(lastIndex, 1), ConstatModel->index(idx , 1).data().toDate());
        ConstatModel->setData(ConstatModel->index(lastIndex, 2), ConstatModel->index(idx , 2).data().toString());
        ConstatModel->setData(ConstatModel->index(lastIndex, 3), ConstatModel->index(idx , 3).data().toString());
        ConstatModel->setData(ConstatModel->index(lastIndex, 4), ConstatModel->index(idx , 4).data().toInt());
        ConstatModel->setData(ConstatModel->index(lastIndex, 5), ConstatModel->index(idx , 5).data().toFloat());
    }
    else
    {
        idx = lastIndex;
        ConstatModel->setData(ConstatModel->index(lastIndex, 1), ConstatModel->index(lastIndex-1 , 1).data().toDate());
        ConstatModel->setData(ConstatModel->index(lastIndex, 2), ConstatModel->index(lastIndex-1 , 2).data().toString());
        ConstatModel->setData(ConstatModel->index(lastIndex, 3), ConstatModel->index(lastIndex-1 , 3).data().toString());
        ConstatModel->setData(ConstatModel->index(lastIndex, 4), ConstatModel->index(lastIndex-1 , 4).data().toInt());
        ConstatModel->setData(ConstatModel->index(lastIndex, 5), ConstatModel->index(lastIndex-1 , 5).data().toFloat());

    }

    QModelIndex Qindex = ui->ConstatTableView->model()->index(ConstatModel->rowCount()-1, 2);
    ui->ConstatTableView->scrollTo(Qindex);
}

void MainWindow::on_duplicateCourses_clicked()
{
    QModelIndex index  =  ui->CoursesTableView->selectionModel()->currentIndex();
    int lastIndex = CoursesModel->rowCount();
    int idx = 0;

    CoursesModel->insertRow(lastIndex);

    if (index.isValid() && ui->CoursesTableView->selectionModel()->isSelected(index))
    {
        idx = index.row();
        CoursesModel->setData(CoursesModel->index(lastIndex, 1), CoursesModel->index(idx , 1).data().toDate());
        CoursesModel->setData(CoursesModel->index(lastIndex, 2), CoursesModel->index(idx , 2).data().toString());
        CoursesModel->setData(CoursesModel->index(lastIndex, 3), CoursesModel->index(idx , 3).data().toString());
        CoursesModel->setData(CoursesModel->index(lastIndex, 4), CoursesModel->index(idx , 4).data().toString());
        CoursesModel->setData(CoursesModel->index(lastIndex, 5), CoursesModel->index(idx , 5).data().toString());
        CoursesModel->setData(CoursesModel->index(lastIndex, 6), CoursesModel->index(idx , 6).data().toInt());
        CoursesModel->setData(CoursesModel->index(lastIndex, 7), CoursesModel->index(idx , 7).data().toFloat());
    }
    else
    {
        idx = lastIndex;
        CoursesModel->setData(CoursesModel->index(lastIndex, 1), CoursesModel->index(lastIndex-1 , 1).data().toDate());
        CoursesModel->setData(CoursesModel->index(lastIndex, 2), CoursesModel->index(lastIndex-1 , 2).data().toString());
        CoursesModel->setData(CoursesModel->index(lastIndex, 3), CoursesModel->index(lastIndex-1 , 3).data().toString());
        CoursesModel->setData(CoursesModel->index(lastIndex, 4), CoursesModel->index(lastIndex-1 , 4).data().toString());
        CoursesModel->setData(CoursesModel->index(lastIndex, 5), CoursesModel->index(lastIndex-1 , 5).data().toString());
        CoursesModel->setData(CoursesModel->index(lastIndex, 6), CoursesModel->index(lastIndex-1 , 6).data().toInt());
        CoursesModel->setData(CoursesModel->index(lastIndex, 7), CoursesModel->index(lastIndex-1 , 7).data().toFloat());

    }
    QModelIndex Qindex = ui->CoursesTableView->model()->index(CoursesModel->rowCount()-1, 2);
    ui->CoursesTableView->scrollTo(Qindex);
}

void MainWindow::on_InvoiveView_tabBarClicked(int index)
{
    if(index == 2)
    {
        on_refreshSearch_clicked();
    }
    else if(index == 3)
    {
        on_refreshPlot_clicked();
    }
    else {}
}


void MainWindow::on_actionLoad_Database_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Database"), workingDirectory, tr("Image Files (*.db3)"));
    if(QFileInfo::exists(fileName))
    {
        loadDatabase(fileName);
    }
}

void MainWindow::loadDatabase(QString pathToFile)
{
    InvoiceDB invDB = InvoiceDB(pathToFile);
    if(invDB.createConnection())
    {
        resetALL();
        /* actualise Statusbar text */
        QFileInfo fi(pathToFile);
        QString filenamename = fi.fileName();
        DBLabel->setText(filenamename);
    }
    else
    {
        qDebug() << "Coudn't load the file";
    }
}

void MainWindow::on_actionArchive_triggered()
{
    int actualYear = QDate::currentDate().year();
    QString newFileName = QString("InvoiceToolDB_%1.db3").arg(actualYear);
    QString newfilepath = QString("%1/%2").arg(workingDirectory).arg(newFileName);
    if(QFileInfo::exists(newfilepath))
    {
        QMessageBox::critical(0, QObject::tr("Archive Error"),
                             QString("Cannot archive file. %1 already exits").arg(newFileName));
    }
    else{
        QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                                  QApplication::applicationName(),
                                                                  QString("Do you want to archive %1?").arg(newFileName),
                                                                  QMessageBox::Yes | QMessageBox::Cancel);
        if(reply == QMessageBox::Yes){
            if(okayToClose())
            {
             QString oldfilepath = QString("%1/%2").arg(workingDirectory).arg(QString("InvoiceToolDB_%1.db3").arg(actualYear-1));

             QFile::copy(oldfilepath,newfilepath);
             loadDatabase(newfilepath);

             /* Clean all except Customer table*/
             QSqlQuery query;
             query.exec("delete from Courses;");
             query.exec("Delete from sqlite_sequence where name='Courses';");

             query.exec("delete from Constat;");
             query.exec("Delete from sqlite_sequence where name='Constat';");

             query.exec("delete from Invoices;");
             query.exec("delete from sqlite_sequence where name='Invoices';");

             resetALL();

             ui->SubtotalValue->setText("-");
             ui->TotalValue->setText("-");
             ui->InvoiceLabel->setText("-");
            }
        }

    }
}
void MainWindow::resetALL(void)
{
    invoiceModel->select();
    CoursesModel->select();
    ConstatModel->select();
    SearchInvoiceModel->select();
    on_refreshPlot_clicked();
    CustomerModel->select();
    mapper->toLast();
}

void MainWindow::on_actionCumul_Report_triggered()
{
    ui->InvoiveView->setCurrentIndex(2);

    on_refreshSearch_clicked();  /* This have to be done since this is not handled by a tab change*/

    if(ui->CustomerCheckBox->isChecked())
    {
        /* Set global parameter needed for the report*/
        generateReport(previewReport, reportGroup1);

    }
    else
    {
        QMessageBox::warning(this, "", tr("Please select a customer first."),
                                       QMessageBox::Ok);
    }
}


void MainWindow::resetGuiElementsAfterInvoiceSaved(void)
{
    ui->CustomerCombo->setEnabled(false);
    ui->DateEdit->setEnabled(false);
    ui->StatusCombo->setEnabled(false);
    ui->removeCourse->setEnabled(false);
    ui->addCourse->setEnabled(false);
    ui->removeConstat->setEnabled(false);
    ui->addConstat->setEnabled(false);
    ui->duplicateConstat->setEnabled(false);
    ui->duplicateCourses->setEnabled(false);
    ui->saveInvoice->setEnabled(false);
    ui->newInvoice->setEnabled(true);
    ui->cancelInvoice->setEnabled(false);
    ui->deleteInvoice->setEnabled(true);
    ui->NoteTextEdit->setEnabled(false);
    ui->CoursesTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->ConstatTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    unsavedElemList.removeOne("InvoiceData");
}

void MainWindow::on_previousInvoice_clicked()
{
    if (unsavedElemList.contains("InvoiceData",Qt::CaseSensitive))
    {
        QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                                  QApplication::applicationName(),
                                                                  tr("Do you want to save the changes "
                                                                     "made to the database file %1?").arg("InvoiceDB"),
                                                                  QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            on_saveInvoice_clicked();
        }
        else
        {
            resetGuiElementsAfterInvoiceSaved();
        }
    }
}

void MainWindow::on_nextInvoice_clicked()
{
    if (unsavedElemList.contains("InvoiceData",Qt::CaseSensitive))
    {
        QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                                  QApplication::applicationName(),
                                                                  tr("Do you want to save the changes "
                                                                     "made to the database file %1?").arg("InvoiceDB"),
                                                                  QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            on_saveInvoice_clicked();
        }
        else
        {
            resetGuiElementsAfterInvoiceSaved();
        }
    }
}

void MainWindow::on_firstInvoice_clicked(void)
{
    if (unsavedElemList.contains("InvoiceData",Qt::CaseSensitive))
    {
        QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                                  QApplication::applicationName(),
                                                                  tr("Do you want to save the changes "
                                                                     "made to the database file %1?").arg("InvoiceDB"),
                                                                  QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            on_saveInvoice_clicked();
        }
        else
        {
            resetGuiElementsAfterInvoiceSaved();
        }
    }
}

void MainWindow::on_lastInvoice_clicked(void)
{
    if (unsavedElemList.contains("InvoiceData",Qt::CaseSensitive))
    {
        QMessageBox::StandardButton reply = QMessageBox::question(0,
                                                                  QApplication::applicationName(),
                                                                  tr("Do you want to save the changes "
                                                                     "made to the database file %1?").arg("InvoiceDB"),
                                                                  QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            on_saveInvoice_clicked();
        }
        else
        {
            resetGuiElementsAfterInvoiceSaved();
        }
    }
}



/* -------------- This should be the end lines --------------*/
void MainWindow::on_actionDebug_triggered()
{
    qDebug() <<  this->TelNr << this->TVANr;

}
