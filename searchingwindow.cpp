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
    emit searchingText(ui->lineEdit->text(),
                       ui->casechb->checkState(),
                       is_downward,
                       ui->casechb_2->checkState());
}

void searchingWindow::on_cancelBtn_clicked()
{
    close();
}

void searchingWindow::on_upchb_toggled(bool checked)
{
    if (checked) {
        is_downward = false;
        ui->downchb->setChecked(false);
    }
}

void searchingWindow::on_downchb_toggled(bool checked)
{
    if (checked) {
        is_downward = true;
        ui->upchb->setChecked(false);
    }
}
