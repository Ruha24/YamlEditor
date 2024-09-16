#include "editwidget.h"
#include "customWidget/customlineedit.h"
#include "ui_editwidget.h"

editwidget::editwidget(CustomLineEdit *lineEdit, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editwidget)
    , lineEdit(lineEdit)
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
    QString currentText = lineEdit->text();
    QString path = lineEdit->getCurrentPath();
    if (currentText.isEmpty()) {
        return;
    }

    QString newValue = QInputDialog::getText(this, tr("Add Value"), tr("Enter the value:"));

    if (!newValue.isEmpty()) {
        emit addKeyValue(path, newValue);
    }

    this->close();
}

void editwidget::on_deletebtn_clicked()
{
    QString currentText = lineEdit->text();

    lineEdit->setKey(currentText);

    QString path = lineEdit->getCurrentPath();

    if (currentText.isEmpty()) {
        return;
    }

    emit deleteElement(path);

    this->close();
}
