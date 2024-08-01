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

#include "addwindow.h"
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

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void onCheckBoxStateChanged(int state);

    void on_pushButton_3_clicked();

    void handleDataAdded(int id, const QString &key, const QString &value);
    void handleDataChanged(int id, const QString &key, const QString &value);
    void handleSaveData(const QList<WidgetData> &widgetDataList);

private:
    Ui::MainWindow *ui;

    YamlNode root;
    QMap<QString, bool> checkBoxStates;
    QWidget *mainWidget = nullptr;
    QVBoxLayout *mainLayout = nullptr;
    QSet<QString> displayedKeys;

    QSet<QString> topLevelKeys;
    QSet<QString> subKeys;

    void addValue();
    void updateValue(const QString &path, const QString &newValue);
    void displayNode(const YamlNode &node, const QString &parentPath);
    void collectKeys(const YamlNode &node, QSet<QString> &topLevelKeys, QSet<QString> &subKeys);

    void createCheckBox(const QString &name, int row, int col, bool topLevelKey);
    void clearKeysArea();
    void clearScrollArea();
    void saveData();
    void displayYamlData();
    void displaykeys();

    YamlReader *yamlReader = new YamlReader();
    YandexApi *yandexApi = new YandexApi();
};
#endif // MAINWINDOW_H
