#include "eventtableviewdelegates.h"

TypeComboBoxDelegate::TypeComboBoxDelegate( QObject* parent ) :
    QItemDelegate( parent )
{
}

QWidget* TypeComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );
    return new QComboBox( parent );
}

void TypeComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const EventDatabaseSortProxy* database = qobject_assert_cast<const EventDatabaseSortProxy*>(index.model());
    QComboBox* comboBox = qobject_assert_cast<QComboBox*>(editor);
    assert( comboBox );

    // first column:
    if (index.column() == 0)
    {
        comboBox->addItem( tr("Reherasal") );
        comboBox->addItem( tr("Gig") );
        comboBox->addItem( tr("Other") );

    }
    comboBox->setCurrentIndex( database->data( index, Qt::EditRole ).toInt() );

    // other columns should not have combobox delegate.
}

void TypeComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox* comboBox = qobject_assert_cast<QComboBox*>(editor);
    EventDatabaseSortProxy* database = qobject_assert_cast<EventDatabaseSortProxy*>(model);    // index->model() is const

    assert( database );
    assert( comboBox );

    if (index.column() == 0)
    {
        database->setData( index, comboBox->currentIndex(), Qt::EditRole );
    }
}

void TypeComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED( index );
    editor->setGeometry( option.rect );
}





DateTimeDelegate::DateTimeDelegate( QObject* parent ) :
    QItemDelegate( parent )
{

}

QWidget* DateTimeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED( option );
    Q_UNUSED( index  );
    Q_UNUSED( parent );
    return new DateTimeDialog( NULL );
}

void DateTimeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const EventDatabase* database = qobject_assert_cast<const EventDatabase*>(index.model());
    DateTimeDialog* dialog = qobject_assert_cast<DateTimeDialog*>(editor);
    assert( dialog );

    dialog->setTimeSpan( database->data( index, Qt::EditRole ).value<TimeSpan>() );
}

void DateTimeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    DateTimeDialog* dialog = qobject_assert_cast<DateTimeDialog*>(editor);
    EventDatabase* database = qobject_assert_cast<EventDatabase*>(model);    // index->model() is const

    assert( database );
    assert( dialog );

    if (index.column() == 0)
    {
        database->setData( index, QVariant::fromValue(dialog->timeSpan()), Qt::EditRole );
    }
}

void DateTimeDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED( index );
    editor->setGeometry( option.rect );
}
