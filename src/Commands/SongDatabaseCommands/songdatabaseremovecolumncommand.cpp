#include "songdatabaseremovecolumncommand.h"

SongDatabaseRemoveColumnCommand::SongDatabaseRemoveColumnCommand(SongDatabase *songDatabase, const int section) :
    SongDatabaseCommand(songDatabase),
    m_section(section)
{
    setText(QString(CommandTranslator::tr("Remove column %1")).arg( songDatabase->attributeKeys()[section] ));
}

void SongDatabaseRemoveColumnCommand::undo()
{
    database()->markForAddition();
    database()->restoreColumn( m_section, m_label, m_attributes );
    m_attributes.clear();
}

void SongDatabaseRemoveColumnCommand::redo()
{
    database()->markForAddition();
    m_attributes.clear();
    m_label = database()->attributeKeys()[m_section];
    for (Song* s : database()->songs())
    {
        m_attributes.append(s->attribute( m_section ));
    }

    database()->removeColumns( m_section, 1, QModelIndex() );
}
