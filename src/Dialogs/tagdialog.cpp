#include "tagdialog.h"
#include "ui_tagdialog.h"
#include <QMouseEvent>

TagDialog::TagDialog(const QStringList & tags, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TagDialog),
    m_tags( tags )
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

TagDialog::~TagDialog()
{
    delete ui;
}

void TagDialog::updateTextEdit()
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

void TagDialog::addTag(const QString &tag)
{
    if (!tag.isEmpty() && !m_tags.contains(tag))
    {
        m_tags.append( tag );
        updateTextEdit();
    }
}

void TagDialog::on_pushButton_clicked()
{
    addTag( ui->comboBox->currentText() );
}

// static convienece function
bool TagDialog::setTags(QWidget *parent, Taggable *taggable)
{
    TagDialog dialog( taggable->tags(), parent );
    if (dialog.exec() == QDialog::Accepted)
    {
        taggable->setTags( dialog.tags() );
        return true;
    }
    else
    {
        return false;
    }
}

void TagDialog::removeTag(QString tag)
{
    m_tags.removeOne( tag );
    updateTextEdit();
}

