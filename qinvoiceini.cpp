#include "qinvoiceini.h"
#include <QDebug>
#include <QDomDocument>
#include <QFile>


QinvoiceINI::QinvoiceINI(QString XMLpath, QInvoiceSettingsStruct &XMLSettings, QObject *parent) : QObject(parent)
{
    parseXMLini(XMLpath,XMLSettings);
}

void QinvoiceINI::parseXMLini(QString XMLpath, QInvoiceSettingsStruct &XMLSettings)
{
    // Load xml file
    QDomDocument XMLDOMfile;
    QFile XMLfile(XMLpath);

    if (XMLfile.open(QIODevice::ReadOnly| QIODevice::Text))
    {
        if (XMLDOMfile.setContent(&XMLfile))
        {
            XMLfile.close();
            QDomElement root = XMLDOMfile.firstChildElement();
            QDomNode User    = root.namedItem("User");
            QDomNode Invoice = root.namedItem("Invoice");

            XMLSettings.UserName       = User.firstChildElement("UserName").text();
            XMLSettings.MailAddress    = User.firstChildElement("MailAddress").text();
            XMLSettings.EnterpriseName = Invoice.firstChildElement("EnterpriseName").text();
            XMLSettings.Footer         = Invoice.firstChildElement("Footer").text();
            XMLSettings.AddressLine1   = Invoice.firstChildElement("AddressLine1").text();
            XMLSettings.AddressLine2   = Invoice.firstChildElement("AddressLine2").text();
            XMLSettings.AddressLine3   = Invoice.firstChildElement("AddressLine3").text();
            XMLSettings.InvoiceLine1   = Invoice.firstChildElement("InvoiceLine1").text();
            XMLSettings.TVANr          = Invoice.firstChildElement("TVANr").text();
            XMLSettings.TelNr          = Invoice.firstChildElement("TelNr").text();

        }
    }
    else {
        qDebug() <<  "Cannot open xml file ";
    }

}
