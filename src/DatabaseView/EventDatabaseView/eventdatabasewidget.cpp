#include "eventdatabasewidget.h"
#include "ui_eventdatabasewidget.h"
#include "application.h"
#include "project.h"

EventDatabaseWidget::EventDatabaseWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EventDatabaseWidget)
{
    ui->setupUi(this);
    ui->eventTableViewContainer->setModel( app().project()->eventDatabaseProxy() );
    connect( ui->eventTableViewContainer->eventTableView()->selectionModel(),
             SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this,
             SLOT(updateSetlistView()) );
    connect( ui->eventTableViewContainer->eventTableView()->model(), SIGNAL(modelReset()), this, SLOT(updateSetlistView()) );
}

EventDatabaseWidget::~EventDatabaseWidget()
{
    delete ui;
}

Event* EventDatabaseWidget::currentEvent() const
{
    QModelIndexList rows = ui->eventTableViewContainer->eventTableView()->selectionModel()->selectedRows();
    if (rows.isEmpty())
    {
        return NULL;
    }
    else
    {
        return ui->eventTableViewContainer->eventTableView()->model()->eventAtIndex( rows.first() );
    }
}

void EventDatabaseWidget::updateSetlistView()
{
    Event* e = currentEvent();
    if (e)
    {
        ui->setlistWidget->setSetlist( e->setlist() );
    }
    else
    {
        ui->setlistWidget->setSetlist( NULL );
    }
}

SetlistView* EventDatabaseWidget::setlistView() const
{
    return ui->setlistWidget->listView();
}

EventTableView* EventDatabaseWidget::eventTableView() const
{
    return ui->eventTableViewContainer->eventTableView();
}

Setlist* EventDatabaseWidget::currentSetlist() const
{
    return qobject_assert_cast<Setlist*>( ui->setlistWidget->listView()->model() );
}

