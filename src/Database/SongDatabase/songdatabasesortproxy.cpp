#include "songdatabasesortproxy.h"

SongDatabaseSortProxy::SongDatabaseSortProxy(QObject *parent) :
    QSortFilterProxyModel( parent)
{
    // filter all columns
    setFilterKeyColumn(-1);
    setDynamicSortFilter( false );
    setFilterCaseSensitivity( Qt::CaseInsensitive );
}

void SongDatabaseSortProxy::setSourceModel(SongDatabase *sourceModel)
{
    QSortFilterProxyModel::setSourceModel(sourceModel);
}

bool SongDatabaseSortProxy::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    assert(!source_parent.isValid());
    if (sourceModel())
    {
        return sourceModel()->attributeVisible( source_column );
    }
    else
    {
        return false;
    }
}

void SongDatabaseSortProxy::setFilter(const QString &filter)
{
    m_filter = filter;
    invalidateFilter();
}

bool SongDatabaseSortProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QRegExp regexp( "*" + QRegExp::escape(m_filter) + "*", Qt::CaseInsensitive, QRegExp::WildcardUnix );
    assert( !source_parent.isValid() );
    Song* song = sourceModel() ? sourceModel()->songAtIndex( sourceModel()->index( source_row, 0, QModelIndex() ) ) : NULL;
    if (song)
    {
        QStringList tokens;
        for (int i = 0; i < song->attributes().length(); ++i)
        {
            if (song->database()->attributeVisible( i ))
            {
                tokens << song->attribute(i).toString();
            }
        }
        tokens << song->tags();
        for (const Attachment* a : song->attachments())
        {
            tokens << a->tags();
        }

        for (const QString& token : tokens)
        {
            if (regexp.exactMatch( token ))
            {
                return true;
            }
        }
        return false;
    }
    else
    {
        return true;
    }
}

SongDatabase* SongDatabaseSortProxy::sourceModel() const
{
     return qobject_assert_cast<SongDatabase*>( QSortFilterProxyModel::sourceModel() );
}
