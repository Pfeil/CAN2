#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include <QTimer>
#include <QToolButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QProcess>
#include <QClipboard>
#include "application.h"
#include "Dialogs/addfilestoindexdialog.h"
#include "Dialogs/stringdialog.h"
#include "Dialogs/configurationdialog.h"
#include <QProgressDialog>
#include <QScrollArea>
#include <QLabel>
#include "Attachments/ChordPatternAttachment/chordpatternproxyattachment.h"

#define HIDE_CLONE_SYNC // because it does not work.

DEFN_CONFIG( MainWindow, tr("Global") )

QString defaultStyleSheet()
{
    QFile file(":/style/styles/stylesheet.qss");

    assert( file.open(QIODevice::ReadOnly ));
    return file.readAll();
}

CONFIGURABLE_ADD_ITEM_HIDDEN( MainWindow, RecentProject, "");
CONFIGURABLE_ADD_ITEM_HIDDEN( MainWindow, RecentCloneURL, QDir::homePath());
CONFIGURABLE_ADD_ITEM_HIDDEN( MainWindow, FileIndexFilter, "");
CONFIGURABLE_ADD_ITEM_HIDDEN( MainWindow, locale, QLocale::system().name().toStdString().c_str());
CONFIGURABLE_ADD_ITEM( MainWindow,
                       Style,
                       QT_TRANSLATE_NOOP("ConfigurableItem", "Style"),
                       QT_TRANSLATE_NOOP("ConfigurableItem", "ConfigurableItem/MainWindow/Style"),
                       QVariant(),  // is set in MainWindow constructor since ::defaultStyleSheet() is not available in static context.
                       ConfigurableItemOptions::TextEditOptions( QT_TRANSLATE_NOOP( "ConfigurableItem", "Stylesheet") )
                       );



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    setWindowIcon( QIcon(":/oldIcons/oldIcons/01-elephant-icon.png") );
    app().setMainWindow( this );
    app().setProject( &m_project );
    createAttachmentActions();

    ui->setupUi(this);
    createDebugMenu();

    QSettings settings;
    restoreGeometry( settings.value("Geometry").toByteArray() );

    //     load stylesheet
    config.item("Style")->setDefaultValue( defaultStyleSheet() );
    connect( config.item("Style"), &ConfigurableItem::valueChanged, [this](QVariant value)
    {
        app().setStyleSheet( value.toString() );
    });
    app().setStyleSheet( config.value( "Style" ).toString() );

    //////////////////////////////////////////
    /// restore Configurable
    //////////////////////////////////////////
    /// is done implicitly when Configurable is loaded


    menuBar()->setNativeMenuBar(false);
    setupAttachmentMenu();

    //////////////////////////////////////////
    /// Undo/Redo
    //////////////////////////////////////////
    connect( &m_project, SIGNAL(canUndoChanged(bool)), ui->actionUndo, SLOT(setEnabled(bool)));
    connect( &m_project, SIGNAL(canRedoChanged(bool)), ui->actionRedo, SLOT(setEnabled(bool)));

    auto action = [this]()
    {
        const QUndoCommand* nextUndoCommand = m_project.command( m_project.QUndoStack::index() - 1);
        const QUndoCommand* nextRedoCommand = m_project.command( m_project.QUndoStack::index() );

        QString undoText = tr("&Undo");
        QString redoText = tr("&Redo");
        if (nextUndoCommand)
        {
            undoText += " \"" + nextUndoCommand->actionText() + "\"";
        }

        if (nextRedoCommand)
        {
            redoText += " \"" + nextRedoCommand->actionText() + "\"";
        }

        undoAction()->setText( undoText );
        redoAction()->setText( redoText );
    };

    connect( &m_project, &Project::commandPushed, action );
    connect( &m_project, &Project::undoStackCleared, [this]()
    {
        QString undoText = tr("&Undo");
        QString redoText = tr("&Redo");
        undoAction()->setText( undoText );
        redoAction()->setText( redoText );
    });
    ui->actionUndo->setEnabled( m_project.canUndo() );
    ui->actionRedo->setEnabled( m_project.canRedo() );


    //////////////////////////////////////////
    /// Splitter
    //////////////////////////////////////////
    connect( ui->splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(resizeSplitter()));
    QTimer::singleShot(0, this, SLOT(resizeSplitter()));


    //////////////////////////////////////////
    ///
    //////////////////////////////////////////
    connect( m_project.songDatabase(), &SongDatabase::attachmentAdded, [this](int i)
    {
        setCurrentAttachment( i );
    });
    connect( m_project.songDatabase(), &SongDatabase::attachmentRemoved, [this](int i)
    {
        setCurrentAttachment( i );
    });
    connect( m_project.songDatabase(), &SongDatabase::attachmentRenamed, [this](QString)
    {
        // just update the current attachment
        setCurrentAttachment( currentAttachmentIndex() );
    });
    connect( m_project.songDatabase(), &SongDatabase::dataChanged, [this]()
    {
        ui->songDatabaseWidget->attachmentChooser()->updateAttachmentView();
    });

    //////////////////////////////////////////
    ///
    //////////////////////////////////////////
    connect( &m_project, SIGNAL(canCloseChanged(bool)), this, SLOT(updateWindowTitle()) );
    updateWindowTitle();
    connect( ui->songDatabaseWidget->songTableView()->selectionModel(),
             &QItemSelectionModel::currentRowChanged,
             [this](){
        QTimer::singleShot(0, this, SLOT( updateWhichWidgetsAreEnabled() ));
    });
    updateWhichWidgetsAreEnabled();

    loadDefaultProject();

    connect( &m_project, SIGNAL(songDatabaseCommandPushed()), this, SLOT(gotoSongView()) );
    connect( &m_project, SIGNAL(eventDatabaseCommandPushed()), this, SLOT(gotoEventView()) );

    // very important to set associated widget. Else, shortcuts would be ambigous.
    initAction( actionNew_Song,      ui->songDatabaseWidget->songTableView(),    tr("&New Song"),       tr("Add a new song."),        "Ctrl+N",   ui->menuSongs,  "" )
    initAction( actionDelete_Song,   ui->songDatabaseWidget->songTableView(),    tr("&Remove Song"),    tr("Remove selected song."),  "Del",      ui->menuSongs,  "" )
    initAction( actionCopy_Song,     ui->songDatabaseWidget->songTableView(),    tr("&Copy Song"),      tr("Copy selected song."),    "Ctrl+C",   ui->menuSongs,  "" )
    initAction( actionPaste_Song,    ui->songDatabaseWidget->songTableView(),    tr("&Paste Song"),     tr("Paste song."),            "Ctrl+V",   ui->menuSongs,  "" )

    initAction( actionNew_Event,     ui->eventDatabaseWidget->eventTableView(),  tr("&New Event"),      tr("Add a new event."),       "Ctrl+N",   ui->menuEvents, "" )
    initAction( actionDelete_Event,  ui->eventDatabaseWidget->eventTableView(),  tr("&Remove Event"),   tr("Remove selected event."), "Del",      ui->menuEvents, "" )
    initAction( actionCopy_Event,    ui->eventDatabaseWidget->eventTableView(),  tr("&Copy Event"),     tr("Copy selected event."),   "Ctrl+C",   ui->menuEvents, "" )
    initAction( actionPaste_Event,   ui->eventDatabaseWidget->eventTableView(),  tr("&Paste Event"),    tr("Paste event."),           "Ctrl+V",   ui->menuEvents, "" )

    connect( ui->menuVisible_attributes, SIGNAL(aboutToShow()), this, SLOT(createAttributeVisibilityMenu() ));

    connect( m_project.songDatabase(), SIGNAL(songRemoved(int)), ui->songDatabaseWidget, SLOT(updateAttachmentChooser()) );

    connect( ui->menu_Language, SIGNAL(aboutToShow()), this, SLOT( createLanguageMenu() ));


    //////////////////////////////////////////
    ///
    //////////////////////////////////////////
    connect( ui->buttonSongDatabase, &QPushButton::clicked, [this]()
    {
        selectPage( SongDatabasePage );
    });
    connect( ui->buttonEventDatabase, &QPushButton::clicked, [this]()
    {
        selectPage( EventDatabasePage );
    });
    selectPage( SongDatabasePage );
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue( "Geometry", saveGeometry() );
    delete ui;
}

void MainWindow::resizeSplitter()
{
    if (ui->splitter->sizes()[0] != 0)
    {
        ui->splitter->setSizes(QList<int>() << 1 << 10000000);
    }
}

Song* MainWindow::currentSong() const
{
    return ui->songDatabaseWidget->currentSong();
}

Event* MainWindow::currentEvent() const
{
    return ui->eventDatabaseWidget->currentEvent();
}

#include "Commands/SongCommands/songaddattachmentcommand.h"

void MainWindow::createAttachmentActions()
{
    // gather attachment creators
    for (const QString & classname : Creatable::classnamesInCategory("Attachment"))
    {
        QAction* action = new QAction( this );
        action->setText( QString(tr("New %1")).arg( Creatable::translatedName(classname) ) );

        if (classname == "PDFAttachment")
        {
            action->setIcon(QIcon(":/icons/icons/pdf24.png"));
        }
        else if (classname == "ChordPatternAttachment")
        {
            action->setIcon(QIcon(":/icons/icons/write13.png"));
        }
        else if (classname == "AudioAttachment")
        {
            action->setIcon(QIcon(":/icons/icons/song1.png"));
        }
        else if (classname == "ChordPatternProxyAttachment")
        {
            action->setIcon( QIcon(":/icons/icons/link67.png") );
        }
        else
        {
            qWarning() << QString("action <create %1> has no icon.").arg( classname );
        }

        connect(action, &QAction::triggered, [this, classname]()
        {
            Song* song = currentSong();
            if (song)
            {
                Attachment* lastAttachment = currentAttachment();
                SongAddAttachmentCommand* command = new SongAddAttachmentCommand( song, classname );
                app().pushCommand( command );
                updateWhichWidgetsAreEnabled();

                if (lastAttachment && lastAttachment->inherits( "ChordPatternAttachment" )
                 && command->attachment()->inherits( "ChordPatternProxyAttachment" ) )
                {
                    qobject_assert_cast<ChordPatternProxyAttachment*>( command->attachment() )->setChordPatternAttachment(
                                qobject_assert_cast<ChordPatternAttachment*>(lastAttachment) );
                }
            }
        });
        m_newAttachmentActions.insert( classname, action );
    }
}

void MainWindow::setupAttachmentMenu()
{
    // gather attachment creators
    QAction* actionBefore = ui->menuAttachments->actions().first();
    for (const QString & classname : Creatable::classnamesInCategory("Attachment"))
    {
        QAction* action = newAttachment_Action( classname );
        ui->menuAttachments->insertAction(actionBefore, action );
    }
}

void MainWindow::setCurrentAttachment( int index )
{
    ui->songDatabaseWidget->attachmentChooser()->setAttachment( index );
}




//////////////////////////////////////////////////
////
///     Saving and loading stuff
//
////////////////////////////////////////////////


QString MainWindow::projectName() const
{
    QString filename = QFileInfo(m_currentPath).fileName();
    if (filename.isEmpty())
    {
        return tr("Unnamed");
    }
    else
    {
        return filename;
    }
}

void MainWindow::setCurrentPath(const QString &path)
{
    m_currentPath = path;
    config.set( "RecentProject", m_currentPath );
}

QString MainWindow::proposedPath() const
{
    QString filename = projectName();
    QString ending = QString(".%1").arg(m_project.ending());

    if (!filename.endsWith(ending))
    {
        filename = filename.append(ending);
    }

    if (m_currentPath.isEmpty())
    {
        return QDir::home().absoluteFilePath( filename );
    }
    else
    {
        return m_currentPath;
    }
}


bool MainWindow::newProject()
{
    if (!canProjectClose())
    {
        return false;
    }

    m_project.reset();
    setCurrentPath( QString() );
    updateWindowTitle();
    return true;
}

bool MainWindow::canProjectClose()
{
    if (m_project.canClose())
    {
        return true;
    }
    else
    {
        switch (
        QMessageBox::question( this,
                               tr("Really close %1").arg( projectName() ),
                               tr("There are unsaved changes."),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Abort,
                               QMessageBox::Abort ) )
        {
        case QMessageBox::Save:
            return save();
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Abort:
            return false;
        default:
            WARNING << "Illegal case in switch statement";
            return false;
        }
    }
}

void MainWindow::on_actionSettings_triggered()
{
    ConfigurationDialog dialog(this);
    dialog.exec();
}

void MainWindow::updateWindowTitle()
{
    QString star = m_project.canClose() ? "" : "*";

    QString title = QString("%1%2 - %4")
            .arg( projectName() )
            .arg( star )
            .arg( qApp->applicationName() );

    setWindowTitle( title );
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (canProjectClose())
    {
        QMainWindow::closeEvent(e);
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

void MainWindow::loadDefaultProject()
{
    m_currentPath = config.value( "RecentProject" ).toString();
    if (!m_currentPath.isEmpty())
    {
        open( m_currentPath );
    }
}

int MainWindow::currentAttachmentIndex() const
{
    return ui->songDatabaseWidget->attachmentChooser()->currentAttachmentIndex();
}

Attachment* MainWindow::currentAttachment() const
{
    return ui->songDatabaseWidget->attachmentChooser()->currentAttachment();
}

void setEnabled( QObject* o, bool enable )
{
    QAction* a = qobject_cast<QAction*>(o);
    if (a) a->setEnabled(enable);

    QWidget* w = qobject_cast<QWidget*>(o);
    if (w) w->setEnabled(enable);
}

void MainWindow::updateWhichWidgetsAreEnabled()
{
    Song* cSong = currentSong();
    Attachment* cAttachment = currentAttachment();

    QObjectList attachmentObjects, songObects;

    for (QAction* action : m_newAttachmentActions)
    {
        songObects << action;
    }


    attachmentObjects   << ui->actionDelete_Attachment;
    attachmentObjects   << ui->actionRename_Attachment;
    attachmentObjects   << ui->actionDuplicate_Attachment;

    for (QObject* o : songObects )          ::setEnabled( o, !!cSong        );
    for (QObject* o : attachmentObjects)    ::setEnabled( o, !!cAttachment  );

    bool chordPatternProxyAttachmentEnabled = false;
    if (cAttachment && cAttachment->inherits( "ChordPatternAttachment" ))
    {
        chordPatternProxyAttachmentEnabled = true;
    }
    m_newAttachmentActions["ChordPatternProxyAttachment"]->setEnabled( chordPatternProxyAttachmentEnabled );

}

MainWindow::Page MainWindow::currentPage() const
{
    switch (ui->stackedWidget->currentIndex())
    {
    case 0:
        return SongDatabasePage;
    case 1:
        return EventDatabasePage;
    default:
        assert( false );
        return (Page) -1;
    }
}

void MainWindow::gotoSongView()
{
    selectPage( SongDatabasePage );
}

void MainWindow::gotoEventView()
{
    selectPage( EventDatabasePage );
}












////////////////////////////////////////////////
////
///     Connections to Actions
//
///////////////////////////////////////////////


#include "Commands/SongDatabaseCommands/songdatabasenewsongcommand.h"
void MainWindow::my_on_actionNew_Song_triggered()
{
    app().pushCommand( new SongDatabaseNewSongCommand( m_project.songDatabase() ) );
    updateWhichWidgetsAreEnabled();
}

void MainWindow::on_actionUndo_triggered()
{
    m_project.undo();
    updateWhichWidgetsAreEnabled();
}

void MainWindow::on_actionRedo_triggered()
{
    m_project.redo();
    updateWhichWidgetsAreEnabled();
}



#include "Commands/SongCommands/songremoveattachmentcommand.h"
void MainWindow::on_actionDelete_Attachment_triggered()
{
    Song* song = currentSong();
    int index = ui->songDatabaseWidget->attachmentChooser()->currentAttachmentIndex();

    if (song && index >= 0)
    {
        if (canRemoveAttachment( song->attachment(index) ))
        {
            app().pushCommand( new SongRemoveAttachmentCommand( song, index ) );
            updateWhichWidgetsAreEnabled();
        }
        else
        {
            QMessageBox::warning( this,
                                  tr("Attachment cannot be removed"),
                                  tr("This attachment is currently in use and can thus not be removed."),
                                  QMessageBox::Ok,
                                  QMessageBox::NoButton );
        }
    }
}

void MainWindow::on_actionNew_Project_triggered()
{
    newProject();
    updateWhichWidgetsAreEnabled();
}


bool MainWindow::canRemoveSong(Song *song)
{
    // check if song is used in setlist
    for (Event* event: m_project.eventDatabase()->events())
    {
        for (SetlistItem* item : event->setlist()->items())
        {
            if (item->song() == song)
            {
                return false;
            }
        }
    }

    return true;
}

bool MainWindow::canRemoveAttachment( const Attachment* attachment )
{
    for ( const Song* song : m_project.songDatabase()->songs() )
    {
        for (const Attachment* a : song->attachments())
        {
            if (a->inherits("ChordPatternProxyAttachment"))
            {
                const ChordPatternProxyAttachment* cppa = qobject_assert_cast<const ChordPatternProxyAttachment*>( a );
                if (cppa->source() == qobject_cast<const ChordPatternAttachment*>(attachment))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

#include "Commands/SongDatabaseCommands/songdatabaseremovesongcommand.h"
void MainWindow::my_on_actionDelete_Song_triggered()
{
    Song* song = currentSong();
    if (song)
    {
        if (canRemoveSong( song ))
        {
            app().pushCommand( new SongDatabaseRemoveSongCommand( m_project.songDatabase(), song ) );
        }
        else
        {
            QMessageBox::warning( this,
                                  tr("Song cannot be removed"),
                                  tr("This song is currently in use and can thus not be removed."),
                                  QMessageBox::Ok,
                                  QMessageBox::NoButton );
        }
        updateWhichWidgetsAreEnabled();
    }
}

void MainWindow::on_actionAdd_Folder_triggered()
{
    AddFilesToIndexDialog dialog( this );

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    QProgressDialog pd( "Task in Progress", "Cancel", 0, -1, this );
    pd.setWindowModality( Qt::WindowModal );

    app().fileIndex().addSource( dialog.path(),
                                 dialog.includePDF(),
                                 dialog.includeMP3(),
                                 dialog.includeOgg() );

    QLabel* label = new QLabel(&pd);
    label->setWordWrap(true);
    pd.setLabel(label);
    pd.show();

    while (!app().fileIndex().operationIsFinished())
    {
        pd.setValue( (pd.value() + 1) % 100 );
        label->setText( QString("%1\n%2").arg(app().fileIndex().currentFilename())
                                         .arg(app().fileIndex().size())            );
        qApp->processEvents();
        QThread::msleep( 10 );
        if (pd.wasCanceled())
        {
            app().fileIndex().abortOperations();
        }
    }

}

void MainWindow::on_actionClear_Index_triggered()
{
    app().fileIndex().clear();
}

#include "Commands/AttachmentCommands/attachmentrenamecommand.h"
void MainWindow::on_actionRename_Attachment_triggered()
{

    Attachment* attachment = currentAttachment();
    if (attachment)
    {
        QString name = attachment->name();
        QString newName = StringDialog::getString( tr("Rename Attachment"), name, QString(tr("New Name for %1")).arg(name) );
        if (newName.isEmpty())
        {
            return;
        }

        app().pushCommand( new AttachmentRenameCommand( attachment, newName ) );
        updateWhichWidgetsAreEnabled();
    }
}

#include "Commands/SongCommands/songduplicateattachmentcommand.h"
void MainWindow::on_actionDuplicate_Attachment_triggered()
{
    Song* cs = currentSong();
    assert( cs );

    int index = ui->songDatabaseWidget->attachmentChooser()->currentAttachmentIndex();
    assert( index >= 0 );

    Attachment* attachment = cs->attachments()[index];
    assert( attachment );

    app().pushCommand( new SongDuplicateAttachmentCommand( attachment ) );
    updateWhichWidgetsAreEnabled();
}

void MainWindow::on_actionOpen_Terminal_here_triggered()
{
    QProcess::startDetached( "gnome-terminal", QStringList(), m_project.path() );
}

void MainWindow::on_action_Index_Info_triggered()
{
    QMessageBox::information( this,
                              tr("File index information"),
                              QString(tr("Files: %1")).arg(app().fileIndex().size()) );
}

#include "Commands/EventDatabaseCommands/eventdatabaseneweventcommand.h"
void MainWindow::my_on_actionNew_Event_triggered()
{
    app().pushCommand( new EventDatabaseNewEventCommand( m_project.eventDatabase()) );
}

#include "Commands/EventDatabaseCommands/eventdatabaseremoveeventcommand.h"
void MainWindow::my_on_actionDelete_Event_triggered()
{
    Event* event = currentEvent();
    if (event)
    {
        app().pushCommand( new EventDatabaseRemoveEventCommand( m_project.eventDatabase(), event ));
        updateWhichWidgetsAreEnabled();
    }
}


QAction* MainWindow::undoAction() const
{
    return ui->actionUndo;
}

QAction* MainWindow::redoAction() const
{
    return ui->actionRedo;
}

QAction* MainWindow::newAttachment_Action( const QString& classname )
{
    if (m_newAttachmentActions.contains(classname))
    {
        return m_newAttachmentActions[classname];
    }
    else
    {
        qWarning() << "Attachment " << classname << "not known." << m_newAttachmentActions.keys();
        return NULL;
    }
}

void MainWindow::my_on_actionCopy_Song_triggered()
{
    QModelIndexList selectedSongs = ui->songDatabaseWidget->songTableView()->selectionModel()->selectedIndexes();
    app().clipboard()->setMimeData( m_project.songDatabase()->mimeData( selectedSongs ) );
}

void MainWindow::my_on_actionPaste_Song_triggered()
{
    ui->songDatabaseWidget->songTableView()->pasteSongs( app().clipboard()->mimeData(), m_project.songDatabase()->rowCount(), Qt::CopyAction );
}

void MainWindow::my_on_actionCopy_Event_triggered()
{
    QModelIndexList selectedSongs = ui->eventDatabaseWidget->eventTableView()->selectionModel()->selectedIndexes();
    app().clipboard()->setMimeData( m_project.eventDatabase()->mimeData( selectedSongs ) );
}

void MainWindow::my_on_actionPaste_Event_triggered()
{
    ui->eventDatabaseWidget->eventTableView()->pasteEvents( app().clipboard()->mimeData(), m_project.eventDatabase()->rowCount(), Qt::CopyAction );
}

#include "Commands/SongDatabaseCommands/songdatabasetoggleattributevisibility.h"
void MainWindow::createAttributeVisibilityMenu()
{
    QMenu* menu = ui->menuVisible_attributes;

    for (int i = 0; i < m_project.songDatabase()->attributeKeys().length(); ++i )
    {
        QString name = m_project.songDatabase()->attributeKeys()[i];
        QString editorType;
        SongDatabase::editorTypeAndHeaderLabel( name, editorType, name );
        QAction* action = menu->addAction( name );
        action->setCheckable( true );
        action->setChecked( m_project.songDatabase()->attributeVisible(i) );

        connect( action, &QAction::triggered, [this, i](bool visible)
        {
            app().pushCommand( new SongDatabaseToggleAttributeVisibility( m_project.songDatabase(), i, visible ) );
        });

        connect( menu, SIGNAL(aboutToHide()), action, SLOT(deleteLater()) );
    }

}

void MainWindow::createLanguageMenu()
{
    for (const QFileInfo& fileInfo : QDir(":/translations/").entryInfoList(QStringList() << "can2*.qm", QDir::Files, QDir::Name ))
    {
        QString locale = fileInfo.baseName().mid(5); // skip ending, skip can2_
        QAction* action = new QAction( ui->menu_Language );
        action->setText( QLocale(locale).nativeLanguageName() );
        ui->menu_Language->addAction(action);
        action->setCheckable( true );
        connect(ui->menu_Language, SIGNAL(aboutToHide()), action, SLOT(deleteLater()));
        if (config["locale"] == locale)
        {
            action->setChecked(true);
        }

        connect( action, &QAction::triggered, [this, locale]()
        {
            QMessageBox::information( this,
                                      tr("Information"),
                                      tr("Language changes will apply on next start."),
                                      QMessageBox::Ok,
                                      QMessageBox::NoButton );
            config.set("locale", locale);
        });
    }
}


SongTableView* MainWindow::songTableView()
{
    return ui->songDatabaseWidget->songTableView();
}


#include "PDFCreator/pdfcreator.h"
#include "PDFCreator/orphantsetlist.h"
void MainWindow::on_actionExport_all_songs_triggered()
{
    OrphantSetlist setlist( tr("All songs"));

    for( Song* song : m_project.songDatabase()->songs() )
    {
        setlist.appendItem( new SetlistItem( song ) );
    }

    PDFCreator::exportSetlist( &setlist, this );
}

void MainWindow::selectPage(Page page)
{
    switch (page)
    {
    case SongDatabasePage:
        if (ui->stackedWidget->currentIndex() != 0)
        {
            ui->stackedWidget->setCurrentIndex( 0 );
        }
        break;
    case EventDatabasePage:
        if (ui->stackedWidget->currentIndex() != 1)
        {
            ui->stackedWidget->setCurrentIndex( 1 );
        }
        break;
    }
}

void MainWindow::createDebugMenu()
{
#ifdef QT_DEBUG
    QMenu* menu = new QMenu( this );
    QAction* action = new QAction( menu );
    menu->addAction( action );
    menu->setTitle( "Debug" );
    action->setText("open terminal");
    ui->menuBar->addMenu( menu );
    connect( action, &QAction::triggered, [this]()
    {
        QProcess::startDetached( "gnome-terminal", QStringList(), m_project.path() );
    });
#endif
}


void MainWindow::on_actionSave_triggered()
{
    save();
}

void MainWindow::on_actionSave_As_triggered()
{
    saveProjectAs();
}

void MainWindow::on_actionOpen_triggered()
{

    if (!canProjectClose())
    {
        return;
    }

    QString filename = config["RecentProject"].toString();
    if (filename.isEmpty())
    {
        filename = QDir::homePath();
    }

    filename =
    QFileDialog::getOpenFileName( this,
                                  tr("Open project or archive ..."),
                                  filename
                                   );
    if (!filename.isEmpty())
    {
        open( filename );
    }
}

void MainWindow::on_actionArchivate_triggered()
{
    saveArchiveAs();
}

bool MainWindow::open( const QString& filename )
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning( this,
                              QString(tr("Opening %1")).arg(filename),
                              QString(tr("File %1 not found.")).arg(filename),
                              QMessageBox::Ok
                              );
        return false;
    }

    if (m_project.isProjectFile( filename ) && openProject( filename ))
    {
        // all right, project opened
    }
    else if (openArchive( filename ))
    {
        // all right, archive opened
    }
    else
    {
        // fail;
        QMessageBox::warning( this,
                              QString(tr("Opening %1")).arg(filename),
                              QString(tr("Cannot open %1. Unknown file format.")).arg(filename),
                              QMessageBox::Ok
                              );
        newProject();
        return false;
    }

    updateWindowTitle();
    updateWhichWidgetsAreEnabled();
    return true;
}

bool MainWindow::save()
{
    if (m_currentPath.isEmpty())
    {
        QMessageBox box( this );
        box.setStandardButtons( QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
        box.setButtonText( QMessageBox::Yes, tr("Save project as ...") );
        box.setButtonText( QMessageBox::No, tr("Archive project as ...") );
        box.setIcon( QMessageBox::Question );
        switch (box.exec())
        {
        case QMessageBox::Yes:
            return saveProjectAs();
        case QMessageBox::No:
            return saveArchiveAs();
        case QMessageBox::Cancel:
        default:
            return false;
        }
    }
    else if ( m_project.isProjectFile( m_currentPath ))
    {
        return saveProjectAs( m_currentPath );
    }
    else
    {
        return saveArchiveAs( m_currentPath );

    }
}

bool MainWindow::openArchive(const QString &filename)
{
    // just call this from open()
    if ( m_project.loadZip( filename ) )
    {
        setCurrentPath( filename );
        updateWindowTitle();
        updateWhichWidgetsAreEnabled();
        return true;
    }
    else
    {
        return false;
    }
}

bool MainWindow::openProject(const QString &filename)
{
    // just call this from open()
    if ( m_project.loadProject( filename ) )
    {
        setCurrentPath( filename );
        updateWindowTitle();
        updateWhichWidgetsAreEnabled();
        return true;
    }
    else
    {
        return false;
    }
}

bool MainWindow::saveProjectAs()
{
    QString filename =
    QFileDialog::getSaveFileName( this,
                                  tr("Save project ..."),
                                  config["RecentProject"].toString() );
    if (filename.isEmpty())
    {
        return false;
    }
    else
    {
        return saveProjectAs( filename );
    }
}

bool MainWindow::saveProjectAs(const QString &filename)
{
    if (m_project.saveProject( filename ))
    {
        setCurrentPath( filename );
        updateWindowTitle();
        return true;
    }
    else
    {
        return false;
    }
}

bool MainWindow::saveArchiveAs()
{
    QString filename =
    QFileDialog::getSaveFileName( this,
                                  tr("Save archive ..."),
                                  config["RecentProject"].toString() );
    if (filename.isEmpty())
    {
        return false;
    }
    else
    {
        return saveArchiveAs( filename );
    }
}

bool MainWindow::saveArchiveAs(const QString &filename)
{
    if (m_project.saveZip( filename ))
    {
        setCurrentPath( filename );
        updateWindowTitle();
        return true;
    }
    else
    {
        return false;
    }
}
























