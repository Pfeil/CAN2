#include "songaddattachmentcommand.h"

SongAddAttachmentCommand::SongAddAttachmentCommand(Song *song, const QString &classname) :
    SongCommand(song),
    m_classname(classname)
{
    assert( Creatable::category(m_classname) == "Attachment" );
}

SongAddAttachmentCommand::~SongAddAttachmentCommand()
{
    if (m_ownsAttachment)
    {
        delete m_attachment;
        m_attachment = 0;
    }
}

void SongAddAttachmentCommand::redo()
{
    if (!m_attachment)
    {
        m_attachment = static_cast<Attachment*>( Creatable::create(m_classname) );
        m_attachment->setSong( song() );
        m_attachment->makeNameUnique();
    }

    setText( QString(CommandTranslator::tr("Add attachment %1").arg( m_attachment->description())) );
    song()->addAttachment( m_attachment );
    m_ownsAttachment = false;
}

void SongAddAttachmentCommand::undo()
{
    assert( m_attachment );

    song()->removeAttachment( m_attachment );
    m_ownsAttachment = true;
}
