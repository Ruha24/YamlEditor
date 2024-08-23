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

void ReplaceWindow::on_searchbtn_clicked()
{
    QString text = ui->findtxt->text();

    if (text != "")
        emit searchReplaceText(text, ui->casechb->checkState());
}

void ReplaceWindow::on_replacebtn_clicked()
{
    QString text = ui->findtxt->text();

    if (text != "")
        emit replaceText(text, ui->replacetxt->text(), false);
}

void ReplaceWindow::on_replaceAllbtn_clicked()
{
    QString text = ui->findtxt->text();

    if (text != "")
        emit replaceText(text, ui->replacetxt->text(), true);
}

void ReplaceWindow::on_cancelbtn_clicked()
{
    close();
}
