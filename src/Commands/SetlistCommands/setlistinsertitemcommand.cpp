#include "setlistinsertitemcommand.h"

SetlistInsertItemCommand::SetlistInsertItemCommand(Setlist *setlist, int position, SetlistItem *item) :
    SetlistCommand( setlist ),
    m_position(position),
    m_item(item)
{
    setText( QString(CommandTranslator::tr("Insert item to setlist %1").arg( item->description())) );

}

SetlistInsertItemCommand::~SetlistInsertItemCommand()
{
    if (m_ownsItem)
    {
        delete m_item;
    }
}

void SetlistInsertItemCommand::undo()
{
    setlist()->removeItem( m_item );
    m_ownsItem = true;
}

void SetlistInsertItemCommand::redo()
{
    setlist()->insertItem( m_position, m_item );
    m_ownsItem = false;
}
