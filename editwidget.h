#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <QInputDialog>
#include <QLineEdit>
#include <QWidget>

namespace Ui {
class editwidget;
}

class CustomLineEdit;

class editwidget : public QWidget
{
    Q_OBJECT

public:
    explicit editwidget(CustomLineEdit *lineEdit, QWidget *parent = nullptr);
    ~editwidget();

signals:
    void addKeyValue(QString key, QString value);
    void deleteElement(QString element);

private slots:
    void on_addbtn_clicked();
    void on_deletebtn_clicked();

private:
    Ui::editwidget *ui;
    CustomLineEdit *lineEdit;
};

#endif // EDITWIDGET_H
