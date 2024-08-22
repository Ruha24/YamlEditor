#include "replacewindow.h"
#include "ui_replacewindow.h"

ReplaceWindow::ReplaceWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ReplaceWindow)
{
    ui->setupUi(this);
}

ReplaceWindow::~ReplaceWindow()
{
    delete ui;
}
