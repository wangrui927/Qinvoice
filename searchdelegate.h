#ifndef SEARCHDELEGATE_H
#define SEARCHDELEGATE_H

#include <QItemDelegate>
#include <QObject>
#include <QCalendarWidget>

class SearchDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    SearchDelegate(int DateColumn, int StatusColumn);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,const QModelIndex &index) const;

private:
    int dateCol,statusCol,invoiceNbrCol;
};

#endif // SEARCHDELEGATE_H
