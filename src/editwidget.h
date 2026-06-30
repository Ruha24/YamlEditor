#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <QInputDialog>
#include <QLineEdit>
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
    ~EditWidget();

signals:
    void AddKeyValue(QString key, QString value);
    void DeleteElement(QString element);

private slots:
    void on_addbtn_clicked();
    void on_deletebtn_clicked();

private:
    Ui::editwidget *ui;
    CustomLineEdit *line_edit;
};

#endif // EDITWIDGET_H
