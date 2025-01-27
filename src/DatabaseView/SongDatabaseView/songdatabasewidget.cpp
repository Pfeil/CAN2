#include "songdatabasewidget.h"
#include "ui_songdatabasewidget.h"
#include <QTimer>
#include "application.h"
#include "project.h"

SongDatabaseWidget::SongDatabaseWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SongDatabaseWidget)
{
    ui->setupUi(this);
    ui->songTableViewContainer->setModel( app().project()->songDatabaseProxy() );
    connect( ui->songTableViewContainer->songTableView()->selectionModel(),
             SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this,
             SLOT(updateAttachmentChooser()) );
}

SongDatabaseWidget::~SongDatabaseWidget()
{
    delete ui;
}

Song* SongDatabaseWidget::currentSong() const
{
    return ui->songTableViewContainer->currentSong();
}

void SongDatabaseWidget::updateAttachmentChooser()
{
    QModelIndexList list = ui->songTableViewContainer->songTableView()->selectionModel()->selectedRows();

    if (list.isEmpty())
    {
        ui->attachmentChooser->setSong( NULL );
    }
    else
    {
        if (list.first() != m_currentIndex)
        {
            m_currentIndex = ui->songTableViewContainer->songTableView()->proxyModel()->mapToSource( list.first() );
            ui->attachmentChooser->setSong(
                            ui->songTableViewContainer->songTableView()->model()->songAtIndex( m_currentIndex ) );
        }
    }
}

AttachmentChooser* SongDatabaseWidget::attachmentChooser() const
{
    return ui->attachmentChooser;
}

SongTableView* SongDatabaseWidget::songTableView() const
{
    return ui->songTableViewContainer->songTableView();
}
