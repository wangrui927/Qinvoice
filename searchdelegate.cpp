#include "searchdelegate.h"
#include <QDateEdit>
#include <QDebug>
#include <QPainter>
#include <QPointF>

SearchDelegate::SearchDelegate(int DateColumn, int StatusColumn)

{
    this->dateCol = DateColumn;
    this->statusCol = StatusColumn;
}

void SearchDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,const QModelIndex &index) const
{
    if (index.column() == dateCol)
    {
        QString text = index.model()->data(index, Qt::DisplayRole).toDate().toString("dd.MM.yyyy");
        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment =  Qt::AlignCenter | Qt::AlignVCenter;

        drawDisplay(painter, myOption, myOption.rect, text);
        drawFocus(painter, myOption, myOption.rect);
    }
    else if(index.column() == statusCol)
    {
        int val = index.model()->data(index, Qt::DisplayRole).toInt();
        if(val)
        {
            //painter->fillRect(option.rect.x(),option.rect.y(),option.rect.width()/2,option.rect.height()/2, QBrush(QColor(0, 255, 0, 200)));
            int x = option.rect.x() + option.rect.width()/2;
            int y = option.rect.y() + option.rect.height()/2;
            int r = option.rect.height()/5;
            painter->setBrush(Qt::green);
            painter->drawEllipse(QPointF(x,y), r,r);
        }
        else
        {
            int x = option.rect.x() + option.rect.width()/2;
            int y = option.rect.y() + option.rect.height()/2;
            int r = option.rect.height()/5;
            painter->setBrush(Qt::red);
            painter->drawEllipse(QPointF(x,y), r,r);
            //painter->fillRect(option.rect, QBrush(QColor(255, 0, 0, 200)));
        }
    }
    else
    {
        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment =Qt::AlignCenter | Qt::AlignVCenter;
        QItemDelegate::paint(painter, myOption, index);
    }
}
