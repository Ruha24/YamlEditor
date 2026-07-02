#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <QWidget>

namespace Ui {
class editwidget;
}

class CustomLineEdit;

class EditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditWidget(CustomLineEdit *lineEdit, QWidget *parent = nullptr);
    ~EditWidget() override;

signals:
    void AddKeyValue(const QString &value);
    void DeleteElement();

private slots:
    void on_addbtn_clicked();
    void on_deletebtn_clicked();

private:
    Ui::editwidget *ui;
    CustomLineEdit *line_edit;
};

#endif // EDITWIDGET_H
