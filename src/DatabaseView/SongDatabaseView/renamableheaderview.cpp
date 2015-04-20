#include "renamableheaderview.h"
#include <QMouseEvent>
#include <QLineEdit>
#include "global.h"
#include "project.h"
#include "Commands/SongDatabaseCommands/songdatabaserenameheadercommand.h"
#include "Commands/SongDatabaseCommands/songdatabaseremovecolumncommand.h"
#include "util.h"
#include "global.h"
#include "application.h"

RenamableHeaderView::RenamableHeaderView(Qt::Orientation orientation, SongTableView *parent) :
    QHeaderView(orientation, parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(this, SIGNAL(sectionResized(int,int,int)), parent, SLOT(fakeFocusOutEvent()));
}

void RenamableHeaderView::mouseDoubleClickEvent(QMouseEvent *event)
{
    // leave a space around the resize handles
    int sectionLeft  = parent()->columnAt(event->pos().x() - 5);
    int sectionRight = parent()->columnAt(event->pos().x() + 5);

    if (sectionLeft == sectionRight)
        editHeader( sectionRight );

    QHeaderView::mouseDoubleClickEvent( event );
}

void RenamableHeaderView::editHeader(int section, bool endMacroOnFinish)
{
    if (section < 0)
        return;

    int x = parent()->columnViewportPosition(section);
    int width = parent()->columnWidth(section);

    QLineEdit* lineEdit = new QLineEdit(this);
    lineEdit->setPlaceholderText(tr("Attribute Name"));
    lineEdit->setFixedSize(width, height() );
    lineEdit->move( x, 0 );
    lineEdit->setFocus();
    lineEdit->setText( parent()->model()->headerData( section, orientation(), Qt::EditRole ).toString() );
    lineEdit->show();

    connect(lineEdit, &QLineEdit::editingFinished, [this, endMacroOnFinish, lineEdit, section](){
        SongDatabase* database = parent()->model();
        app().pushCommand( new SongDatabaseRenameHeaderCommand( database, lineEdit->text(), section, orientation() ) );

        if (endMacroOnFinish)
        {
            database->project()->endMacro();
        }
        lineEdit->deleteLater();
    });
}

int RenamableHeaderView::sectionUnderCursor() const
{
    QPoint pos = viewport()->mapFromGlobal( QCursor::pos() );
    return logicalIndexAt(pos.x());
}

void RenamableHeaderView::showContextMenu(QPoint pos)
{
    QMenu* menu = new QMenu(this);

    setUpContextMenu(menu);

    menu->popup(viewport()->mapToGlobal(pos));
    connect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));
}

void RenamableHeaderView::setUpContextMenu(QMenu *menu)
{
    int section = sectionUnderCursor();
    if (section < 0)
        return;


    SongDatabase* database =  parent()->model();
    QString key = database->attributeKeys()[section];

    Util::addAction(menu, QString(tr("Delete Attribute %1")).arg(key), [this, database, section](){
        app().pushCommand( new SongDatabaseRemoveColumnCommand( database, section ) );
    });
}