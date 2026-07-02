#include "replacewindow.h"
#include "ui_replacewindow.h"

ReplaceWindow::ReplaceWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ReplaceWindow)
{
    ui->setupUi(this);
}

ReplaceWindow::~ReplaceWindow()
{
    delete ui;
}

void ReplaceWindow::on_searchbtn_clicked()
{
    const QString text = ui->findtxt->text();
    if (text.isEmpty())
        return;

    emit searchReplaceText(text, ui->casechb->isChecked(), ui->casechb_2->isChecked());
}

void ReplaceWindow::on_replacebtn_clicked()
{
    const QString text = ui->findtxt->text();
    if (text.isEmpty())
        return;

    emit replaceText(text, ui->replacetxt->text(), false, ui->casechb_2->isChecked());
}

void ReplaceWindow::on_replaceAllbtn_clicked()
{
    const QString text = ui->findtxt->text();
    if (text.isEmpty())
        return;

    emit replaceText(text, ui->replacetxt->text(), true, ui->casechb_2->isChecked());
}

void ReplaceWindow::on_cancelbtn_clicked()
{
    close();
}
