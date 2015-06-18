#ifndef PERSISTENTOBJECT_H
#define PERSISTENTOBJECT_H

#include <QJsonObject>
#include <QJsonArray>
#include "global.h"


// check if object has a certain key of certain type. return false if not and print warning.
#define checkJsonObject(OBJECT, KEY, TYPE)                                          \
      (( OBJECT.contains(KEY)       || (WARNING << "Expected key " << KEY, false) )        \
    && ( OBJECT[KEY].type() == TYPE || (WARNING << "Expected type: " << TYPE, false)  ))

class Project;
class PersistentObject
{
protected:
    PersistentObject();
    virtual ~PersistentObject();

protected:
    // For each class deriving from Persistent, an key equal to its classname is reservated.
    virtual bool restoreFromJsonObject( const QJsonObject & object ) { Q_UNUSED(object); return true; }
    virtual QJsonObject toJsonObject() const { return QJsonObject(); }

public:
    virtual bool saveTo(const QString & path) const;
    virtual bool loadFrom(const QString & path);

private:
    static QString createRandomID();
protected:
    QString m_randomID;


public:
    QString randomID() const { return m_randomID; }
    static void seedRandomID();

    virtual QString filenameBase() const = 0;
    QString filename() const;
    QString absoluteFilename() const;
    Project* project() const { return m_project; }
    void setProject( Project* project ) { m_project = project; }

private:
    Project* m_project;

public:
    void markForDeletion() const;
    void unmarkForDeletion() const;

    void markForAddition() const;
    void unmarkForAddition() const;
};

#endif // PERSISTENTOBJECT_H
