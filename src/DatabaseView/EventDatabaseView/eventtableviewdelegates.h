#ifndef EVENTTABLEVIEWDELEGATES_H
#define EVENTTABLEVIEWDELEGATES_H

#include <QItemDelegate>
#include <QComboBox>
#include "Database/EventDatabase/eventdatabasesortproxy.h"
#include "Dialogs/datetimedialog.h"


class TypeComboBoxDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    TypeComboBoxDelegate( QObject* parent = NULL );
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class DateTimeDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    DateTimeDelegate( QObject* parent = NULL );
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // EVENTTABLEVIEWDELEGATES_H
