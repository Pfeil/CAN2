#ifndef SONGDATABASESORTPROXY_H
#define SONGDATABASESORTPROXY_H

#include <QSortFilterProxyModel>
#include "songdatabase.h"

class SongDatabaseSortProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SongDatabaseSortProxy(QObject* parent = 0);

    void setSourceModel(SongDatabase *sourceModel) { QSortFilterProxyModel::setSourceModel(sourceModel); }
    SongDatabase* sourceModel() const { return qobject_cast<SongDatabase*>( QSortFilterProxyModel::sourceModel() ); }

};

#endif // SONGDATABASESORTPROXY_H