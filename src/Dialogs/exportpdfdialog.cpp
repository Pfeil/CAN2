#include "exportpdfdialog.h"
#include "ui_exportpdfdialog.h"

ExportPDFDialog::ExportPDFDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportPDFDialog)
{
    ui->setupUi(this);
}

ExportPDFDialog::~ExportPDFDialog()
{
    delete ui;
}
