#include "mydelegate.h"
#include <QDateEdit>
#include <QDebug>
#include <QComboBox>

MyDelegate::MyDelegate(int DateColumn, int tvaColumn, QObject *parent)
    : QItemDelegate(parent)
{
    this->datecolumn = DateColumn;
    this->tvacolumn  = tvaColumn;
}
void MyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == datecolumn)
    {
        QString text = index.model()->data(index, Qt::DisplayRole).toDate().toString("dd.MM.yyyy");
        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;

        drawDisplay(painter, myOption, myOption.rect, text);
        drawFocus(painter, myOption, myOption.rect);
    }
    else if(index.column() == tvacolumn)
    {
        QString text = index.model()->data(index, Qt::DisplayRole).toString();
        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment = Qt::AlignCenter | Qt::AlignVCenter;

        drawDisplay(painter, myOption, myOption.rect, text);
        drawFocus(painter, myOption, myOption.rect);

    }
    else
    {
        QItemDelegate::paint(painter, option, index);
    }

}

QWidget *MyDelegate::createEditor(QWidget *parent,
const QStyleOptionViewItem &option,
const QModelIndex &index) const
{
    if (index.column() == datecolumn)
    {
        QDateEdit *dateEdit = new QDateEdit(parent);
        dateEdit->setCalendarPopup(true);
        dateEdit->setDisplayFormat("dd.MM.yyyy");

        connect(dateEdit, SIGNAL(editingFinished()),
        this, SLOT(commitAndCloseEditor()));
        return dateEdit;

    }
    else if(index.column() == tvacolumn)
    {
        QComboBox *tva = new QComboBox(parent);
        //tva->setEditable(false);
        tva->addItem("1");
        tva->addItem("2");
        tva->setCurrentText("");
        return tva;
    }
    else
    {
        return QItemDelegate::createEditor(parent, option, index);
    }
}
void MyDelegate::setEditorData(QWidget *editor,const QModelIndex &index) const
{
    if (index.column() == datecolumn)
    {
        QDate date = index.model()->data(index, Qt::DisplayRole).toDate();
        QDateEdit *dateEdit = qobject_cast<QDateEdit *>(editor);
        if(date.isValid())
        {
            dateEdit->setDate(date);
        }
        else
        {
            dateEdit->setDate(QDate::currentDate());
        }
    }
    else if(index.column() == tvacolumn)
    {
        QString tvastr = index.model()->data(index, Qt::DisplayRole).toString();
        QComboBox *tva = qobject_cast<QComboBox *>(editor);
        if(tvastr == "0")
        {
            tvastr = "";
        }
        tva->setCurrentText(tvastr);
    }
    else
    {
        QItemDelegate::setEditorData(editor, index);
    }
}

void MyDelegate::setModelData(QWidget *editor,
QAbstractItemModel *model,
const QModelIndex &index) const
{
    if (index.column() == datecolumn)
    {
        QDateEdit *dateEdit = qobject_cast<QDateEdit *>(editor);
        QDate date = dateEdit->date();
        model->setData(index, date);

    }
    else if(index.column() == tvacolumn)
    {
        QComboBox *tva = qobject_cast<QComboBox *>(editor);
        model->setData(index,tva->currentText());
    }
    else
    {
        QItemDelegate::setModelData(editor, model, index);
    }
}

void MyDelegate::commitAndCloseEditor()
{
    QDateEdit *editor = qobject_cast<QDateEdit *>(sender());
    emit commitData(editor);
    //emit closeEditor(editor);
}
