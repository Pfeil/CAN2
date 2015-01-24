#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Commands/songdatabasenewsongcommand.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(testSlot1()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(testSlot2()));
    connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(testSlot3()));
    connect(ui->pushButton_4, SIGNAL(clicked()), this, SLOT(testSlot4()));
    connect(ui->pushButton_5, SIGNAL(clicked()), this, SLOT(testSlot5()));

    connect(&m_project, SIGNAL(canUndoChanged(bool)), ui->pushButton_2, SLOT(setEnabled(bool)));
    connect(&m_project, SIGNAL(canRedoChanged(bool)), ui->pushButton_3, SLOT(setEnabled(bool)));

    ui->tableView->setModel(m_project.songDatabase());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::testSlot1()
{
    m_project.pushCommand( new SongDatabaseNewSongCommand(m_project.songDatabase()) );
}

void MainWindow::testSlot2()
{
    m_project.undo();
}


void MainWindow::testSlot3()
{
    m_project.redo();
}

void MainWindow::testSlot4()
{

}

void MainWindow::testSlot5()
{

}
