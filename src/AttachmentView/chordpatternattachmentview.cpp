#include "chordpatternattachmentview.h"
#include "ui_chordpatternattachmentview.h"
#include "global.h"
#include <QScrollBar>
#include "util.h"
#include "Database/SongDatabase/song.h"
#include "Database/SongDatabase/songdatabase.h"
#include "project.h"
#include "Commands/AttachmentCommands/abstractchordpatternattachmenttransposecommand.h"
#include "application.h"
#include "Dialogs/chordpatternviewer.h"

DEFN_CREATABLE(ChordPatternAttachmentView, AttachmentView)
DEFN_CONFIG( ChordPatternAttachmentView, "Chord Pattern Attachment" )


ChordPatternAttachmentView::ChordPatternAttachmentView(QWidget *parent) :
    AttachmentView(parent),
    ui(new Ui::ChordPatternAttachmentView)
{
    ui->setupUi(this);
}

ChordPatternAttachmentView::~ChordPatternAttachmentView()
{
    delete ui;
}

void ChordPatternAttachmentView::highlightTextEdit(ChordPatternEdit *edit, const QString& text)
{
    edit->blockSignals(true);
    int cursorPosition = edit->textCursor().position();
    int scrollbarPosition = edit->verticalScrollBar()->value();
    edit->setText( text );
    QTextCursor cursor(edit->document());
    cursor.setPosition(cursorPosition);
    edit->setTextCursor(cursor);
    edit->verticalScrollBar()->setValue(scrollbarPosition);

    QList<QTextEdit::ExtraSelection> highlights;
    int i = 0;
    for (QString line : text.split("\n")) {
        QStringList chords, tokens;
        bool isChordLine = Chord::parseLine( line, chords, tokens );
        for ( const QString & token : tokens )
        {
            if (Chord(token).isValid() && isChordLine) {
                QTextCursor cursor(edit->document());
                cursor.setPosition(i);
                cursor.setPosition(i + token.length(), QTextCursor::KeepAnchor);
                QTextEdit::ExtraSelection highlight;
                highlight.cursor = cursor;
                highlight.format.setFontUnderline(true);
                highlight.format.setUnderlineColor( QColor(255, 128, 0) );
                highlights << highlight;
            }
            i += token.length() + 1;
        }
    }
    edit->setExternalExtraSelections(highlights);
    edit->blockSignals(false);
}

void ChordPatternAttachmentView::updateText()
{
    highlightTextEdit( ui->textEdit, attachment<ChordPatternAttachment>()->chordPattern() );
}

#include "Commands/AttachmentCommands/chordpatternattachmenteditpatterncommand.h"
void ChordPatternAttachmentView::textEdited()
{
    app().pushCommand( new ChordPatternAttachmentEditPatternCommand( attachment<ChordPatternAttachment>(), ui->textEdit->toPlainText() ) );
}

void ChordPatternAttachmentView::polish()
{
    updateText();
    connect(ui->textEdit, SIGNAL(textChanged()), this, SLOT(textEdited()));
    connect(attachment<ChordPatternAttachment>(), SIGNAL(changed()), this, SLOT(updateText()));
    connect( ui->textEdit, SIGNAL(pasted()), attachment<ChordPatternAttachment>(), SLOT(transpose()) );

}

void ChordPatternAttachmentView::on_buttonUp_clicked()
{
    app().pushCommand( new AbstractChordPatternAttachmentTransposeCommand( attachment<ChordPatternAttachment>(), 1 ) );
}

void ChordPatternAttachmentView::on_buttonDown_clicked()
{
    app().pushCommand( new AbstractChordPatternAttachmentTransposeCommand( attachment<ChordPatternAttachment>(), -1 ) );
}

void ChordPatternAttachmentView::on_buttonView_clicked()
{
    ChordPatternViewer::displayChordPatternAttachment( attachment<AbstractChordPatternAttachment>(), this );
}
