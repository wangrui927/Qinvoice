#ifndef MYDELEGATE_H
#define MYDELEGATE_H

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QSpinBox>
#include <QCalendarWidget>


class MyDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    MyDelegate(int DateColumn, int tvaColumn, QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;

private slots:
    void commitAndCloseEditor();

private:
    int datecolumn,tvacolumn;
};

#endif // MYDELEGATE_H
