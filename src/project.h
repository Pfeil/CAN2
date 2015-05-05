#ifndef PROJECT_H
#define PROJECT_H

#include "../../ZipGit/src/zipgit.h"
#include "Database/SongDatabase/songdatabase.h"
#include "Database/EventDatabase/eventdatabase.h"

#include "Commands/command.h"
#include <QUndoStack>
#include "Database/SongDatabase/songdatabasesortproxy.h"
#include "Database/EventDatabase/eventdatabasesortproxy.h"
#include "configurable.h"

class Project : public QUndoStack, public GitRepository
{
    DECL_CONFIG(Project)
    Q_OBJECT
public:
    Project();
    ~Project();


    SongDatabase* songDatabase() const { return m_songDatabase; }
    SongDatabaseSortProxy* songDatabaseProxy() const { return m_songDatabaseProxy; }
    EventDatabase* eventDatabase() const { return m_eventDatabase; }
    EventDatabaseSortProxy* eventDatabaseProxy() const { return m_eventDatabaseProxy; }

public slots:
    void pushCommand(Command* command);
    void reset();
    void undo();
    void redo();

signals:
    void songDatabaseCommandPushed();
    void eventDatabaseCommandPushed();

public:
    bool loadFromTempDir();
    bool saveToTempDir();

private:
    SongDatabase* m_songDatabase;
    EventDatabase* m_eventDatabase;
    SongDatabaseSortProxy* m_songDatabaseProxy;
    EventDatabaseSortProxy* m_eventDatabaseProxy;

public:
    bool canClose() const;
private:
    void setCanClose( bool b );
    bool m_canClose = true;
    void resetUndoStack();
    void emitCommandPushedSignal(Command::Type type);
signals:
    void canCloseChanged( bool );
    void detachedSyncFinished( bool );


public:
    void syncDetached( const QString & message, const Identity& identity );
    bool syncFinished() const { return m_syncFinished; }
    bool syncResult() const { return m_syncResult; }
private:
    bool m_syncFinished = true;
    bool m_syncResult = false;

public:
    void cloneDetached( const QString & url );
    bool cloneFinished() const { return m_cloneFinished; }
    bool cloneResult() const { return m_cloneResult; }
private:
    bool m_cloneFinished = true;
    bool m_cloneResult = false;

};

#endif // PROJECT_H
