#include "gitrepository.h"
#include <QThread>
#include "gitutil.h"

// loadFromTemp, saveToTemp must never be inside clone or sync call since those calls might be performed by other thread.

GitRepository::GitRepository( const QString ending ) :
    Zipped( ending ),
    m_repository( NULL ),
    m_index( NULL )
{
    git_libgit2_init();
}

GitRepository::~GitRepository()
{
    git_libgit2_shutdown();
    if (m_index)
    {
        qDebug() << "free index ~" << m_index;
        git_index_free( m_index );
        m_index = NULL;
    }
}

bool GitRepository::cloneAndInitialize( const QString& url, const Identity& identity )
{
    if (!clone( url ))
    {
        return false;
    }

    if (needsInitialCommit())
    {
        saveToTempDir();
        if (!initialize( identity ))
        {
            return false;
        }

    }

    return loadFromTempDir();
}


bool GitRepository::synchronize( const QString& message, const Identity& identity )
{
    commit( message, identity );

    if (!pullOriginMaster())
    {
        return false;
    }

    if (handleConflicts())
    {
        commit( "Merge", identity );
    }

    assert( !hasConflicts() );

    if (pushOriginMaster( identity ))
    {
        return false;
    }

    return loadFromTempDir();
}


bool GitRepository::loadZip(const QString &filename)
{
    if (!Zipped::loadZip(filename))
    {
        return false;
    }
    else
    {
        if (m_repository)
        {
            git_repository_free( m_repository );
            m_repository = NULL;
        }

        if (QFileInfo( makeAbsolute(".git") ).exists())
        {
            assert( CHECK_GIT( git_repository_open( &m_repository, CSTR(path() ) ) ) );
            if (m_index)
            {
                qDebug() << "free index load" << m_index;
                git_index_free( m_index );
                m_index = NULL;
            }
        }
        return true;
    }
}

bool GitRepository::saveZip(const QString &filename)
{
    // all files must be removed from index. imagine:
    // there are fileA, fileB.
    // user deletes fileA
    // user saves. so just fileB is there, so git thinks fileA was accidentaly deleted.
    // user commits (internally, git_rm_All deletes fileB from index, BUT NOT fileA, since it is not there!
    //               then git_add_All adds fileB again)
    // => commit, pull, merge, push will restore fileA!
    // so here is the last chance to tell git that a file _shall_ be removed. removing to many files is not
    // crucial since they get removed anyway from index (and added certainly)
    git_rm_all();
    return Zipped::saveZip( filename );
}

bool GitRepository::isGitRepository()
{
    return !!m_repository;
}

bool GitRepository::pushOriginMaster( const Identity& identity )
{
    CHECK_IS_REPOSITORY();

    Q_UNUSED( identity );
//    setCredentials( identity );
//    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
//    callbacks.credentials = credential_cb;
//    callbacks.payload = &m_payload;

    git_remote* remote = NULL;
    git_remote_lookup( &remote, m_repository, "origin" );


//    if (!CHECK_GIT( git_remote_set_callbacks( remote, &callbacks )))
//    {
//        qWarning() << "setting callbacks failed.";
//    }

    if (!checkGitCall( git_remote_connect( remote, GIT_DIRECTION_PUSH ) ))
    {
        qWarning() << "remote connection failed";
        return false;
    }

    if (!git_remote_connected( remote ))
    {
        qWarning() << "connection not established.";
        return false;
    }

    git_push_options options;
    git_push_init_options( &options, GIT_PUSH_OPTIONS_VERSION );

                           //"+refs/heads/*:refs/remotes/origin/*"
                           //"refs/heads/master:refs/heads/master"
    git_remote_add_push( remote,  "refs/heads/master:refs/heads/master" );

    if (!checkGitCall( git_remote_upload( remote, NULL, &options ) ))
    {
        qWarning() << "Pushing failed.";
        return false;
    }

//    repositoryStatusChanged( AllChangesPushed );
    return true;
}

bool GitRepository::pullOriginMaster()
{
    // someone should commit all changes before calling this.

    CHECK_IS_REPOSITORY();

    // fetch...
    git_remote* remote;
    git_remote_lookup( &remote, m_repository, "origin" );
    assert( remote );

    assert( CHECK_GIT( git_remote_add_fetch(remote, "+refs/heads/*:refs/remotes/origin/*") ) );

    if (!checkGitCall(git_remote_fetch( remote, NULL, NULL, NULL )))
    {
        qWarning() << "fetching failed.";
        return false;
    }


    // merge fetch head
    git_oid fetch_head_oid;
    assert( CHECK_GIT( git_reference_name_to_id( &fetch_head_oid, m_repository, "FETCH_HEAD" ) ) );

    git_annotated_commit* their_head = NULL;
    const char* url = git_remote_url( remote );
    assert( CHECK_GIT( git_annotated_commit_from_fetchhead( &their_head, m_repository, "master", url, &fetch_head_oid ) ) );

    git_merge_options merge_options;
    merge_options.version = GIT_MERGE_OPTIONS_VERSION;
    merge_options.file_favor = GIT_MERGE_FILE_FAVOR_NORMAL;
    merge_options.flags = GIT_MERGE_TREE_FIND_RENAMES;
    merge_options.metric = NULL;    // Default metric
    merge_options.rename_threshold = 50;
    merge_options.target_limit = 200;

    git_checkout_options checkout_options;
    assert( CHECK_GIT( git_checkout_init_options( &checkout_options, GIT_CHECKOUT_OPTIONS_VERSION ) ) );
    checkout_options.checkout_strategy = GIT_CHECKOUT_FORCE;

    // this call succeedes, even if there are conflicts. If it fails, something went very wrong and there is no chance for this to become consistent.
    assert( CHECK_GIT( git_merge( m_repository, (const git_annotated_commit**) &their_head, 1, &merge_options, &checkout_options ) ) );

    git_repository_state_cleanup( m_repository );
    git_remote_free( remote );
    remote = NULL;

    return true;
}

bool GitRepository::needsInitialCommit() const
{
    //try to resolve HEAD. If HEAD does not exist, the repo needs initial commit.
    git_oid unused_oid;
    if ( git_reference_name_to_id( &unused_oid, m_repository, "HEAD" ) == 0 )
    {
        // HEAD found, no initialization needed
        return false;
    }
    else
    {
        // HEAD not found, initialization needed.
        return true;
    }

}

git_commit *GitRepository::getCommit ( const QString & ref )
{
    assert( isGitRepository() );

    git_commit * commit = NULL; /* the result */
    git_oid oid_parent_commit;  /* the SHA1 for last commit */

    /* resolve HEAD into a SHA1 */
    assert( CHECK_GIT( git_reference_name_to_id( &oid_parent_commit, m_repository, CSTR( ref ) ) ) );
    assert( CHECK_GIT( git_commit_lookup( &commit, m_repository, &oid_parent_commit )) );
    return commit;
}

void GitRepository::commit( const QString& message, const Identity& identity )
{
    if (!isGitRepository())
    {
        qWarning() << "cannot sync non-gitrepository";
        return;
    }

    git_rm_all();
    saveToTempDir();
    git_add_all();

    if ( m_index == NULL )
    {
        qWarning() << "nothing to commit.";
        return;
    }

    // look up parent
    git_commit* parent = getCommit("HEAD");
    assert( parent );

    // create tree
    //create a tree from m_index
    git_tree* tree = 0;
    git_oid tree_id;

    assert( CHECK_GIT( git_index_write_tree(&tree_id, index()) ) );
    assert( CHECK_GIT( git_tree_lookup(&tree, m_repository, &tree_id) ) );

    git_signature *sig = identity.signatureNow();
    assert( sig );

    const git_commit *parents[] = {parent};

    git_oid new_commit_id;
    assert( CHECK_GIT( git_commit_create( &new_commit_id,
                                          m_repository,
                                          "HEAD",                      /* name of ref to update */
                                          sig,                         /* author */
                                          sig,                         /* committer */
                                          "UTF-8",                     /* message encoding */
                                          CSTR(message),               /* message */
                                          tree,                        /* root tree */
                                          1,                           /* parent count */
                                          parents                      /* parents */   ) ) );

    assert( CHECK_GIT( git_index_write(m_index) ) );

    qDebug() << "free index commit" << m_index;
    git_index_free( m_index );
    m_index = NULL;

    git_signature_free(sig);
    git_commit_free(parent);
    git_tree_free(tree);
}

bool GitRepository::checkGitCall(int errorCode)
{
    if (errorCode != 0)
    {
        const git_error* error = giterr_last();
        if (error)
        {
            m_errorSender.sendError( QString("Error %1.%2: %3")
                                        .arg(errorCode)
                                        .arg(error->klass)
                                        .arg(error->message) );
        }
        else
        {
            m_errorSender.sendError( ErrorSender::tr("Invalid Error") );
        }
        return false;
    }
    else
    {
        return true;
    }
}

bool GitRepository::clone(const QString& url)
{
    reset();    // remove all files (includes .git)

    if (m_repository)
    {
        git_repository_free( m_repository );
        m_repository = NULL;
    }

    if (!checkGitCall( git_clone( &m_repository, CSTR(url), CSTR(path()), NULL ) ) )
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool GitRepository::initialize(const Identity &identity)
{
    CHECK_IS_REPOSITORY();

    git_add_all();

    git_oid tree_id, commit_id;
    git_tree* tree;
    assert( CHECK_GIT( git_index_write_tree(&tree_id, index()) ) );

    assert( CHECK_GIT( git_tree_lookup( &tree, m_repository, &tree_id )) );

    git_signature* sig = identity.signatureNow();
    assert( CHECK_GIT( git_commit_create_v(  &commit_id,
                                             m_repository,
                                             "HEAD",
                                             sig,
                                             sig,
                                             NULL,
                                             "Initial Commit",
                                             tree,
                                             0                          )            ) );
    git_tree_free( tree );
    tree = NULL;
    git_signature_free( sig );
    sig = NULL;

    qDebug() << "free index init" << m_index;
    git_index_free( m_index );
    m_index = NULL;

    // push the initialization. Else, sync will not work.
    return pushOriginMaster( identity );
}

void GitRepository::mergeCommits(const QString &message, const Identity &identity)
{
    assert( isGitRepository() );

    // look up parent
    git_commit* parentLocal = getCommit("HEAD");
    assert( parentLocal );

    // look up parent
    git_commit* parentRemote = getCommit("refs/remotes/origin/master");
    assert( parentRemote );

    // create tree
    // create a tree from m_index
    git_tree* tree = 0;
    git_oid tree_id;

    assert( CHECK_GIT( git_index_write_tree(&tree_id, index()) ) );
    assert( CHECK_GIT( git_tree_lookup(&tree, m_repository, &tree_id) ) );

    git_signature *sig = identity.signatureNow();
    assert( sig );

    const git_commit *parents[] = { parentLocal, parentRemote };
    git_oid new_commit_id;
    assert( CHECK_GIT( git_commit_create(   &new_commit_id,
                                            m_repository,
                                            "HEAD",                      /* name of ref to update */
                                            sig,                         /* author */
                                            sig,                         /* committer */
                                            "UTF-8",                     /* message encoding */
                                            CSTR(message),               /* message */
                                            tree,                        /* root tree */
                                            2,                           /* parent count */
                                            parents                      /* parents */      ) ) );
    assert( CHECK_GIT( git_index_write(index()) ) );

    qDebug() << "free index merge commits " << m_index;
    git_index_free( m_index );
    m_index = NULL;
    git_signature_free(sig);
    git_commit_free(parentLocal);
    git_commit_free(parentRemote);
    git_tree_free(tree);
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////
////
///  remove and add files from hdd
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

git_index* GitRepository::index()
{
    assert( m_repository );
    if (!m_index)
    {
        git_repository_index(&m_index, m_repository);
        qDebug() << "create index " << m_index;
    }
    return m_index;
}


void GitRepository::git_add_all()
{
    addDirRecursively( path() );
}

void GitRepository::git_rm_all()
{
    removeDirRecursively( path() );
}

void GitRepository::git_add(const QString & absoluteFilename)
{
    QString relativeFilename = QDir(path()).relativeFilePath( absoluteFilename );
    if (isGitRepository())
    {
        qDebug() << "add " << absoluteFilename;
        git_index_add_bypath(index(), CSTR(relativeFilename));
    }
}

void GitRepository::git_rm(const QString & absoluteFilename)
{
    QString relativeFilename = QDir(path()).relativeFilePath( absoluteFilename );
    if (isGitRepository())
    {
        qDebug() << "rm " << absoluteFilename;
        git_index_remove_bypath(index(), CSTR(relativeFilename));
    }
    QFile(absoluteFilename).remove();
}

void GitRepository::removeDirRecursively(const QString& path)
{
    for (const QString& filename : QDir(path).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot))
    {
        git_rm( QDir(path).absoluteFilePath( filename ) );
    }

    for (const QString& dirname : QDir(path).entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot))
    {
        if (dirname != ".git")
        {
            removeDirRecursively( QDir(path).absoluteFilePath( dirname ) );
        }
    }
}

void GitRepository::addDirRecursively(const QString& path)
{
    for (const QString& filename : QDir(path).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot))
    {
        git_add( QDir(path).absoluteFilePath( filename ) );
    }

    for (const QString& dirname : QDir(path).entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot))
    {
        if ( dirname != ".git" )
        {
            addDirRecursively( QDir(path).absoluteFilePath( dirname ) );
        }
    }
}

class CloneThread : public QThread
{
public:
    CloneThread( GitRepository* repository, const QString& url ) :
        m_repository( repository ), m_url( url )   {    }
private:
    GitRepository* m_repository;
    QString m_url;
    Identity m_identity;
protected:
    void run()
    {
        setProperty( "success",  m_repository->clone( m_url ) );
    }
};

void GitRepository::cloneDetached(const QString &url)
{
    m_succeeded = false;
    assert( !m_thread );
    m_thread = new CloneThread( this, url );
    QThread::connect( m_thread, &QThread::finished, [this]()
    {
        m_succeeded = m_thread->property("success").toBool();
        m_thread->deleteLater();
        m_thread = NULL;
    });
    m_thread->start();
}


class InitializeThread : public QThread
{
public:
    InitializeThread( GitRepository* repository, const Identity& identity ) :
        m_repository( repository ), m_identity( identity )   {    }
private:
    GitRepository* m_repository;
    QString m_url;
    Identity m_identity;
protected:
    void run()
    {
        setProperty( "success",  m_repository->initialize( m_identity ) );
    }
};

void GitRepository::initializeDetached(const Identity &identity)
{
    m_succeeded = false;
    assert( !m_thread );
    m_thread = new InitializeThread( this, identity );
    QThread::connect( m_thread, &QThread::finished, [this]()
    {
        m_succeeded = m_thread->property("success").toBool();
        m_thread->deleteLater();
        m_thread = NULL;
    });
    m_thread->start();
}



class PullThread : public QThread
{
public:
    PullThread( GitRepository* repository ) :
        m_repository( repository )  {    }
private:
    GitRepository* m_repository;
protected:
    void run()
    {
        setProperty( "success",  m_repository->pullOriginMaster() );
    }
};

void GitRepository::pullOriginMasterDetached()
{
    m_succeeded = false;
    assert( !m_thread );
    m_thread = new PullThread( this );
    QThread::connect( m_thread, &QThread::finished, [this]()
    {
        m_succeeded = m_thread->property("success").toBool();
        m_thread->deleteLater();
        m_thread = NULL;
    });
    m_thread->start();
}



class PushThread : public QThread
{
public:
    PushThread( GitRepository* repository, const Identity& identity ) :
        m_repository( repository ), m_identity( identity )    {    }
private:
    GitRepository* m_repository;
    Identity m_identity;
protected:
    void run()
    {
        setProperty( "success",  m_repository->pushOriginMaster( m_identity ) );
    }
};

void GitRepository::pushOriginMasterDetached(const Identity& identity)
{
    m_succeeded = false;
    assert( !m_thread );
    m_thread = new PushThread( this, identity );
    QThread::connect( m_thread, &QThread::finished, [this]()
    {
        m_succeeded = m_thread->property("success").toBool();
        m_thread->deleteLater();
        m_thread = NULL;
    });
    m_thread->start();
}

bool GitRepository::detachedTaskFinished() const
{
    if (!m_thread)
    {
        return true;
    }
    else
    {
        return m_thread->isFinished();
    }
}


QList<ConflictFile> GitRepository::conflictingFiles( ) const
{
    QStringList filenames = dir().entryList( QDir::Files );
    qDebug() << "dir = " << dir().absolutePath();
    qDebug() << "all files: " << filenames << dir().entryList();
    QList<ConflictFile> conflictFiles;
    for ( const QString & filename : filenames )
    {
        QString absoluteFilename = dir().absoluteFilePath( filename );
        if (filename == ".git")
        {
            continue;
        }

        qDebug() << "file " << absoluteFilename << "has ...";
        ConflictFile file( absoluteFilename );
        if ( !file.conflicts().isEmpty() )
        {
            qDebug() << "...conflicts";
            conflictFiles << file;
        }
        else
        {
            qDebug() << "...no conflicts";
        }
    }

    qDebug() << conflictFiles.length() << "conflicts";
    return conflictFiles;
}



