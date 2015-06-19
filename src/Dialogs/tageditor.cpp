#include "tageditor.h"
#include "ui_tageditor.h"
#include <QMouseEvent>

TagEditor::TagEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TagEditor)
{
    ui->setupUi(this);
    updateTextEdit();
    ui->comboBox->addItems( Taggable::allTags() );
    ui->label->setTextInteractionFlags( Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse );
    ui->comboBox->setInsertPolicy( QComboBox::InsertAtTop );
    connect( ui->label, SIGNAL(linkActivated(QString)), this, SLOT(removeTag(QString)) );
    ui->label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->label->setWordWrap(true);
    connect(ui->comboBox, &QComboBox::currentTextChanged, [this](QString text)
    {
        ui->pushButton->setEnabled( !text.isEmpty() );
    });
    ui->pushButton->setEnabled( false );

    ui->comboBox->setCurrentIndex( -1 );
}

TagEditor::~TagEditor()
{
    delete ui;
}

void TagEditor::setTags( const QStringList& tags )
{
    m_tags = tags;
    updateTextEdit();
}

void TagEditor::updateTextEdit()
{
    ui->label->clear();

    QStringList tokens;
    QChar x = QChar(0x2717);

    for ( const QString & tag : m_tags)
    {
        tokens << QString("<a href=\"%1\" style=\"text-decoration: none\"><u>%1</u><sup>%2</sup></a>").arg(tag).arg(x);
    }

    ui->label->setText( QString("<html>%1</html>").arg(tokens.join(" ") ));
}

void TagEditor::addTag(const QString &tag)
{
    if (!tag.isEmpty() && !m_tags.contains(tag))
    {
        m_tags.append( tag );
        updateTextEdit();
    }
}

void TagEditor::on_pushButton_clicked()
{
    addTag( ui->comboBox->currentText() );
}

void TagEditor::removeTag(QString tag)
{
    m_tags.removeOne( tag );
    updateTextEdit();
}

