#ifndef SONGDATABASESORTPROXY_H
#define SONGDATABASESORTPROXY_H

#include <QSortFilterProxyModel>
#include "songdatabase.h"

class SongDatabaseSortProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SongDatabaseSortProxy(QObject* parent = 0);

    void setSourceModel(SongDatabase *sourceModel);
    SongDatabase* sourceModel() const;

public slots:
    void setFilter( const QString& filter );

protected:
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    QString m_filter;

};

#endif // SONGDATABASESORTPROXY_H
