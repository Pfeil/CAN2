#include "chordpatternproxyattachmentview.h"
#include "ui_chordpatternproxyattachmentview.h"
#include "Dialogs/chordpatternviewer.h"
#include "application.h"
#include "Attachments/ChordPatternAttachment/chordpatternproxyattachment.h"
#include "chordpatternattachmentview.h"


DEFN_CREATABLE(ChordPatternProxyAttachmentView, AttachmentView)

ChordPatternProxyAttachmentView::ChordPatternProxyAttachmentView(QWidget *parent) :
    AttachmentView(parent),
    ui(new Ui::ChordPatternProxyAttachmentView)
{
    ui->setupUi(this);
}

ChordPatternProxyAttachmentView::~ChordPatternProxyAttachmentView()
{
    delete ui;
}

void ChordPatternProxyAttachmentView::polish()
{
    connect(attachment<ChordPatternProxyAttachment>(), SIGNAL(changed()), this, SLOT(updateText()));
    updateText();
}

#include "Commands/AttachmentCommands/abstractchordpatternattachmenttransposecommand.h"
void ChordPatternProxyAttachmentView::on_buttonUp_clicked()
{
    app().pushCommand( new AbstractChordPatternAttachmentTransposeCommand( attachment<ChordPatternProxyAttachment>(),  1 ) );
}

void ChordPatternProxyAttachmentView::on_buttonDown_clicked()
{
    app().pushCommand( new AbstractChordPatternAttachmentTransposeCommand( attachment<ChordPatternProxyAttachment>(), -1 ) );
}

void ChordPatternProxyAttachmentView::on_buttonView_clicked()
{
    ChordPatternViewer::displayChordPatternAttachment( attachment<AbstractChordPatternAttachment>(), this );
}

void ChordPatternProxyAttachmentView::updateText()
{
    ChordPatternProxyAttachment* a = attachment<ChordPatternProxyAttachment>();
    const ChordPatternAttachment* source = a ? a->source() : NULL;
    if (a)
    {
        ChordPatternAttachmentView::highlightTextEdit( ui->textEdit, a->chordPattern() );
    }
    else
    {
        ui->textEdit->clear();
    }

    if (source)
    {
        ui->buttonOriginal->setText( "< " + source->name() + " >");
        ui->buttonOriginal->setEnabled( true );
    }
    else
    {
        ui->buttonOriginal->setText( "" );
        ui->buttonOriginal->setEnabled( false );
    }


}

void ChordPatternProxyAttachmentView::on_buttonOriginal_clicked()
{
    emit focus( attachment<ChordPatternProxyAttachment>()->source() );
}
