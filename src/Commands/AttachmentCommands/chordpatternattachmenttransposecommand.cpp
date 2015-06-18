#include "chordpatternattachmenttransposecommand.h"

ChordPatternAttachmentTransposeCommand::ChordPatternAttachmentTransposeCommand(AbstractChordPatternAttachment* attachment , int transpose) :
    AttachmentCommand(attachment),
    m_transpose( transpose )
{
    setText( QString(CommandTranslator::tr("Transpose chord pattern %1").arg(m_transpose)) );
}

void ChordPatternAttachmentTransposeCommand::undo()
{
    attachment<AbstractChordPatternAttachment>()->transpose( -m_transpose );
}

void ChordPatternAttachmentTransposeCommand::redo()
{

    attachment<AbstractChordPatternAttachment>()->transpose( m_transpose );
}
