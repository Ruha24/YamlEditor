#include "editwidget.h"
#include "customWidget/customlineedit.h"
#include "ui_editwidget.h"

editwidget::editwidget(CustomLineEdit *lineEdit, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editwidget)
    , line_edit(lineEdit)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog);
}

editwidget::~editwidget()
{
    delete ui;
}

void editwidget::on_addbtn_clicked()
{
    QString current_text = line_edit->text();
    QString path = line_edit->GetCurrentPath();
    if (current_text.isEmpty()) {
        return;
    }

    QString new_value = QInputDialog::getText(this, tr("Add Value"), tr("Enter the value:"));

    if (!new_value.isEmpty()) {
        emit AddKeyValue(path, new_value);
    }

    this->close();
}

void editwidget::on_deletebtn_clicked()
{
    QString current_text = line_edit->text();

    line_edit->SetKey(current_text);

    QString path = line_edit->GetCurrentPath();

    if (current_text.isEmpty()) {
        return;
    }

    emit DeleteElement(path);

    this->close();
}
