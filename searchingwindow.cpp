#include "searchingwindow.h"
#include "ui_searchingwindow.h"

searchingWindow::searchingWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::searchingWindow)
{
    ui->setupUi(this);
}

searchingWindow::~searchingWindow()
{
    delete ui;
}

void searchingWindow::on_searchingBtn_clicked()
{
    emit searchingText(ui->lineEdit->text());
}

void searchingWindow::on_cancelBtn_clicked()
{
    close();
}
