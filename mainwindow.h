#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QSettings>
#include <QSpacerItem>
#include <QWidget>

#include "yamlreader.h"
#include "yandexapi.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_fileNamecmb_currentIndexChanged(int index);

    void setSpecialValue();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void onCheckBoxStateChanged(int state);

private:
    Ui::MainWindow *ui;

    QMap<QString, QMap<QString, QString>> updatedValues;
    QMap<QString, bool> checkBoxStates;
    QWidget *mainWidget = nullptr;
    QVBoxLayout *mainLayout = nullptr;

    void saveData();
    void displayYamlData();
    void displaykeys();

    YamlReader *yamlReader = new YamlReader();
    YandexApi *yandexApi = new YandexApi();
};
#endif // MAINWINDOW_H
