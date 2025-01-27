#include "eventdatabaseediteventcommand.h"

EventDatabaseEditEventCommand::EventDatabaseEditEventCommand(EventDatabase*         eventDatabase,
                                                             const QModelIndex &    index,
                                                             const QVariant &       newData,
                                                             const int              role) :
    EventDatabaseCommand(eventDatabase),
    m_index(index),
    m_newData(newData),
    m_oldData( eventDatabase->data( index, role ) ),
    m_role(role)
{
    setText( QString(CommandTranslator::tr("Edit event %1 -> %2").arg( m_oldData.toString(), m_newData.toString() )) );
}

void EventDatabaseEditEventCommand::redo()
{
    eventDatabase()->setData_( m_index, m_newData, m_role );
}

void EventDatabaseEditEventCommand::undo()
{
    eventDatabase()->setData_( m_index, m_oldData, m_role );
}
