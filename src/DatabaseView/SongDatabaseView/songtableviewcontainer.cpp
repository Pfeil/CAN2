#include "songtableviewcontainer.h"
#include "ui_songtableviewcontainer.h"
#include <QLineEdit>
#include <QPushButton>
#include "global.h"

SongTableViewContainer::SongTableViewContainer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SongTableViewContainer)
{
    ui->setupUi(this);
}

SongTableViewContainer::~SongTableViewContainer()
{
    delete ui;
}

SongTableView* SongTableViewContainer::songTableView() const
{
    return ui->tableView;
}

void SongTableViewContainer::setModel(SongDatabaseSortProxy *model)
{
    ui->tableView->setModel(model);
    connect(ui->filterWidget, SIGNAL(filterChanged(QString)), model, SLOT(setFilter(QString)));
}

Song* SongTableViewContainer::currentSong() const
{
    QModelIndexList rows = ui->tableView->selectionModel()->selectedRows();
    if (rows.isEmpty())
    {
        return NULL;
    }
    else
    {
        QModelIndex index = ui->tableView->proxyModel()->mapToSource( rows.first() );
        return ui->tableView->model()->songAtIndex( index );
    }
}
