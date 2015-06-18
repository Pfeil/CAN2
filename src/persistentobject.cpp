#include "persistentobject.h"
#include <QFile>
#include <QJsonDocument>
#include "global.h"
#include <QDateTime>
#include "project.h"

PersistentObject::PersistentObject() :
    m_project( NULL )
{
    m_randomID = createRandomID();
}

PersistentObject::~PersistentObject()
{
}

bool PersistentObject::loadFrom(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        WARNING << "Cannot open file " << path << " for reading.";
        return false;
    }

    QByteArray fileContent = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(fileContent, &error);
    if (error.error != QJsonParseError::NoError)
    {
        WARNING << "Cannot parse " << path << ": " << error.errorString();
        return false;
    }

    if (!doc.isObject())
    {
        WARNING << "Expected JsonObject\n" << QString(doc.toJson());
        return false;
    }

    if (!restoreFromJsonObject(doc.object()))
    {
        WARNING << "Cannot restore object\n" << QString(doc.toJson());
        return false;
    }
    return true;
}

bool PersistentObject::saveTo(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        WARNING << "Cannot open file " << path << " for writing.";
        return false;
    }

    QJsonDocument doc(toJsonObject());
    file.write(doc.toJson());

    return true;
}

void PersistentObject::seedRandomID()
{
    qsrand( QDateTime::currentMSecsSinceEpoch() );
}

QString PersistentObject::createRandomID()
{
    QString s;


    for (int i = 0; i < 32; ++i)
    {
        char c = qrand() % 36;
        if (c < 10)
        {
            s += QChar( char(c + '0') );
        }
        else
        {
            c -= 10;
            s += QChar( char( c + 'a' ) );
        }
    }
    return s;
}

QString PersistentObject::filename() const
{
    return QString("%1%2").arg( filenameBase() ).arg( randomID() );
}

QString PersistentObject::absoluteFilename() const
{
    return project()->makeAbsolute( filename() );
}

void PersistentObject::markForDeletion() const
{
    project()->markFileForDeletion( absoluteFilename() );
}
void PersistentObject::unmarkForDeletion() const
{
    project()->unmarkFileForDeletion( absoluteFilename() );
}

void PersistentObject::markForAddition() const
{
    project()->markFileForAddition( absoluteFilename() );
}
void PersistentObject::unmarkForAddition() const
{
    project()->unmarkFileForAddition( absoluteFilename() );
}
