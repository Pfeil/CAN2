#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "project.h"
#include "configurable.h"
#include "DatabaseView/SongDatabaseView/songtableview.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    enum Page { SongDatabasePage, EventDatabasePage };
    Q_OBJECT
    DECL_CONFIG( MainWindow )
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QAction* undoAction() const;
    QAction* redoAction() const;
    /**
     * @brief newAttachmentAction returns the action that is used to create Attachment with given classname.
     * @param classname
     * @return QAction to create the Attachment or NULL if no action is known to create it.
     */
    QAction* newAttachment_Action(const QString &classname);

    SongTableView* songTableView();


private slots:
    void resizeSplitter();  // left column should be as small as possible.
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionDelete_Attachment_triggered();

    void on_actionNew_Project_triggered();

private:
    Ui::MainWindow *ui;

    Project m_project;

    // saving and loading related stuff
    bool canProjectClose();
    QString projectName() const;
    void setCurrentPath( const QString & path );
    QString filter() const { return tr("All files (*);;CAN files (*.can)"); }
    QString proposedPath() const;
    QString m_currentPath;

    // may archivate or save project
    bool save();
    bool open(const QString &filename);
    bool saveProjectAs();
    bool saveArchiveAs();
    bool saveProjectAs( const QString& filename );
    bool saveArchiveAs( const QString& filename );
    bool openProject( const QString& filename );
    bool openArchive( const QString& filename );




    bool newProject();
    void loadDefaultProject();
    void createDebugMenu();


//    void updateToolBar();


protected:
    void closeEvent(QCloseEvent *e);

public slots:
    void gotoSongView();
    void gotoEventView();

private slots:
    void selectPage( Page page );
    void updateWindowTitle();
    void on_actionSave_triggered();
    void on_actionSave_As_triggered();
    void on_actionOpen_triggered();
    void on_actionAdd_Folder_triggered();
    void on_actionClear_Index_triggered();
    void on_actionRename_Attachment_triggered();
    void on_actionDuplicate_Attachment_triggered();
    void updateWhichWidgetsAreEnabled();
    void on_actionOpen_Terminal_here_triggered();
    void on_actionSettings_triggered();
    void on_action_Index_Info_triggered();

    void my_on_actionNew_Song_triggered();
    void my_on_actionDelete_Song_triggered();
    void my_on_actionCopy_Song_triggered();
    void my_on_actionPaste_Song_triggered();

    void my_on_actionNew_Event_triggered();
    void my_on_actionDelete_Event_triggered();
    void my_on_actionCopy_Event_triggered();
    void my_on_actionPaste_Event_triggered();

    void createAttributeVisibilityMenu();
    void createLanguageMenu();

    void on_actionExport_all_songs_triggered();
    void on_actionArchivate_triggered();

private:
    void createAttachmentActions();
    void setupAttachmentMenu();
    QMap<QString, QAction*> m_newAttachmentActions;
    void setCurrentAttachment( int index );
    Song* currentSong() const;
    Event* currentEvent() const;

    int currentAttachmentIndex() const;
    Attachment* currentAttachment() const;
    Page currentPage() const;
    bool canRemoveSong( Song* song );
    bool canRemoveAttachment(const Attachment *attachment );


    QAction* m_actionNew_Song;
    QAction* m_actionDelete_Song;
    QAction* m_actionCopy_Song;
    QAction* m_actionPaste_Song;

    QAction* m_actionNew_Event;
    QAction* m_actionDelete_Event;
    QAction* m_actionCopy_Event;
    QAction* m_actionPaste_Event;
};

#endif // MAINWINDOW_H
