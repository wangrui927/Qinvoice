#ifndef GENERICTABLEVIEWDELEGATE_H
#define GENERICTABLEVIEWDELEGATE_H

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>

class genericTableViewDelegate : public QItemDelegate
{
        Q_OBJECT
public:
    genericTableViewDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;

private slots:
    void commitAndCloseEditor();
};

#endif // GENERICTABLEVIEWDELEGATE_H
