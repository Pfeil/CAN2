#ifndef DATABASE_H
#define DATABASE_H

#include "persistentobject.h"
#include <QAbstractTableModel>

class Database : public PersistentObject, public QAbstractTableModel
{
public:
    Database( Project* project );
};

#endif // DATABASE_H
