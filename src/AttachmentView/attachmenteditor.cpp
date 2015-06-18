#include "attachmenteditor.h"
#include <QHBoxLayout>
#include "Database/SongDatabase/song.h"
#include "attachmentchooser.h"

AttachmentEditor::AttachmentEditor(QWidget *parent) :
    QWidget(parent)
{
    m_scrollArea = new QScrollArea(this);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );
    layout->addWidget( m_scrollArea );
    setAttachment( NULL );

    assert( parent->inherits( "AttachmentChooser" ));
}

AttachmentView* createAttachmentView(Attachment* attachment)
{
    QString classname = attachment->classname();
    classname = classname.append("View");

    assert( Creatable::category(classname) == "AttachmentView" );

    AttachmentView* attachmentView = static_cast<AttachmentView*>( Creatable::create( classname ) );
    attachmentView->setAttachment( attachment );

    return attachmentView;

}

void AttachmentEditor::setAttachment(Attachment *attachment)
{
    if (m_currentView)
    {
        m_attachmentViewOptions[m_currentView->attachment()] = m_currentView->options();
    }

    delete m_currentView;
    delete m_scrollArea->layout();
    QVBoxLayout* layout = new QVBoxLayout(m_scrollArea);
    layout->setSpacing( 0 );
    layout->setMargin( 0 );


    if (attachment)
    {
        m_currentView = createAttachmentView( attachment );
        connect( m_currentView, &AttachmentView::focus, [this](const Attachment* f)
        {
            // why not setAttachment(f)?
            // 1. f is const
            // 2. ensure that a is an attachment of this song.
            int i = 0;
            for (Attachment* a : f->song()->attachments())
            {
                if (a == f)
                {
                    qobject_assert_cast<AttachmentChooser*>( parent() )->setAttachment( i );
                    break;
                }
                ++i;
            }
        });

        if (m_attachmentViewOptions.contains(attachment))
        {
            m_currentView->restoreOptions(m_attachmentViewOptions[attachment]);
        }

        m_currentView->setParent( m_scrollArea );
        layout->addWidget( m_currentView );

        delete m_scrollArea->widget();
    }
    else
    {
        m_currentView = NULL;
    }

    m_scrollArea->setLayout( layout );
}

void AttachmentEditor::updateAttachmentView()
{
    if (m_currentView)
    {
        m_currentView->updateAttachmentView();
    }
}
