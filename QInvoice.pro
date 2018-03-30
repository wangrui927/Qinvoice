#-------------------------------------------------
#
# Project created by QtCreator 2016-04-27T22:32:01
#
#-------------------------------------------------

QT       += core gui\
            sql \
            xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = QInvoice
TEMPLATE = app

# Location of SMTP Library
SMTP_LIBRARY_LOCATION = $$PWD/lib

INCLUDEPATH += $$SMTP_LIBRARY_LOCATION
DEPENDPATH += $$SMTP_LIBRARY_LOCATION

#win32:CONFIG(release, debug|release): LIBS += -L$$SMTP_LIBRARY_LOCATION -lSMTPEmail
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$SMTP_LIBRARY_LOCATION -lSMTPEmail
#else:unix: LIBS += -L$$SMTP_LIBRARY_LOCATION -lSMTPEmail



LIBS += -L$$SMTP_LIBRARY_LOCATION -lSMTPEmail

SOURCES +=  main.cpp\
            mainwindow.cpp \
            invoicedb.cpp \
            qcustomplot.cpp \
            mydelegate.cpp \
            login.cpp \
            mailcontent.cpp \
            searchdelegate.cpp \
            runGuard.cpp \
            qinvoiceutil.cpp \
            qinvoiceini.cpp \
    generictableviewdelegate.cpp \
    myerror.cpp


HEADERS  += mainwindow.h \
            invoicedb.h \
            qcustomplot.h \
            mydelegate.h \
            login.h \
            mailcontent.h \
            searchdelegate.h \
            runGuard.h \
            qinvoiceutil.h \
            qinvoiceini.h \
    generictableviewdelegate.h \
    myerror.h


FORMS    += mainwindow.ui \
            login.ui \
            mailcontent.ui

RESOURCES += \
            invoiceressources.qrc
include($$PWD/QtRptProject/QtRPT/QtRPT.pri)

# run target after build prozess
!win32
{
    linkDependenciesFolder.commands = ln -s -f $$PWD/dependencies $$OUT_PWD/$$dependencies
    #copyLibContent.commands = $(COPY_DIR) $$PWD/lib/* $$OUT_PWD
    QMAKE_EXTRA_TARGETS += linkDependenciesFolder #copyLibContent
    POST_TARGETDEPS += linkDependenciesFolder #copyLibContent
}
