#ifndef ADDWINDOW_H
#define ADDWINDOW_H

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QPixmap>
#include <QPushButton>
#include <QSet>


struct WidgetData
{
    QComboBox *comboBox;
    QLineEdit *lineEdit;
    int id;

    WidgetData(QComboBox *comboBox, QLineEdit *lineEdit, int id)
        : comboBox(comboBox)
        , lineEdit(lineEdit)
        , id(id)
    {}
};

namespace Ui {
class AddWindow;
}

class AddWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AddWindow(QWidget *parent = nullptr, QSet<QString> keys = QSet<QString>());
    ~AddWindow();

private slots:
    void on_clearBtn_clicked();

    void on_savebtn_clicked();

signals:
    void dataAdded(int id, const QString &key, const QString &value);
    void dataChanged(int id, const QString &key, const QString &value);
    void saveData(const QList<WidgetData> &widgetDataList);

private:
    void createWidget();
    void deleteWidget(QWidget *widgetContainer);
    void clearLayout(QLayout *layout);

    int countWidget = 0;
    int nextId;

    QSet<QString> keys;
    QList<WidgetData> widgetDataList;

    QPixmap pixMapAdd;
    QPixmap pixMapDel;

    Ui::AddWindow *ui;
};

#endif // ADDWINDOW_H
