#include "generictableviewdelegate.h"

genericTableViewDelegate::genericTableViewDelegate(QObject *parent)
   : QItemDelegate(parent)
{
}

void genericTableViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem myOption = option;
    myOption.displayAlignment =  Qt::AlignCenter | Qt::AlignVCenter;
    QItemDelegate::paint(painter, myOption, index);
}

QWidget *genericTableViewDelegate::createEditor(QWidget *parent,
const QStyleOptionViewItem &option,
const QModelIndex &index) const
{
    return QItemDelegate::createEditor(parent, option, index);
}

void genericTableViewDelegate::setEditorData(QWidget *editor,const QModelIndex &index) const
{
    QItemDelegate::setEditorData(editor, index);
}
void genericTableViewDelegate::setModelData(QWidget *editor,
QAbstractItemModel *model,
const QModelIndex &index) const
{
    QItemDelegate::setModelData(editor, model, index);
}

void genericTableViewDelegate::commitAndCloseEditor()
{

}
