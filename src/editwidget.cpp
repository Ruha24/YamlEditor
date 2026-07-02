#include "editwidget.h"

#include <QInputDialog>

#include "customWidget/customlineedit.h"
#include "ui_editwidget.h"

EditWidget::EditWidget(CustomLineEdit *lineEdit, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editwidget)
    , line_edit(lineEdit)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog);
}

EditWidget::~EditWidget()
{
    delete ui;
}

void EditWidget::on_addbtn_clicked()
{
    if (line_edit->text().isEmpty())
        return;

    const QString new_value =
        QInputDialog::getText(this, tr("Add Value"), tr("Enter the value:"));

    if (!new_value.isEmpty())
        emit AddKeyValue(new_value);

    close();
}

void EditWidget::on_deletebtn_clicked()
{
    if (line_edit->text().isEmpty())
        return;

    emit DeleteElement();
    close();
}
