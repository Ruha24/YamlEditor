#include "searchingwindow.h"
#include "ui_searchingwindow.h"

SearchingWindow::SearchingWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::searchingWindow)
{
    ui->setupUi(this);
    is_downward = false;
}

SearchingWindow::~SearchingWindow()
{
    delete ui;
}

void SearchingWindow::on_searchingBtn_clicked()
{
    emit searchingText(ui->lineEdit->text(),
                       ui->casechb->checkState(),
                       is_downward,
                       ui->casechb_2->checkState());
}

void SearchingWindow::on_cancelBtn_clicked()
{
    close();
}

void SearchingWindow::on_upchb_toggled(bool checked)
{
    if (checked) {
        is_downward = false;
        ui->downchb->setChecked(false);
    }
}

void SearchingWindow::on_downchb_toggled(bool checked)
{
    if (checked) {
        is_downward = true;
        ui->upchb->setChecked(false);
    }
}
