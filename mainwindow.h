#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSpacerItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QWidget>

#include "searchingwindow.h"
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

    void searchingText(const QString &text, bool isSensitive);

    void on_prettychb_toggled(bool checked);

    void on_treechb_toggled(bool checked);

private:
    Ui::MainWindow *ui;

    QShortcut *keyF11;
    QShortcut *keyCtrlF;

    QString searching_text;

    YamlReader *yamlReader;
    YandexApi *yandexApi;

    QWidget *mainWidget;
    QVBoxLayout *mainLayout;
    QTreeWidget *treeWidget;

    QWidget *previousWidget = nullptr;
    QString previousWidgetOriginalStyleSheet;

    QSet<QString> displayedKeys;
    YamlNode root;
    QMap<QString, bool> checkBoxStates;
    QSet<QString> topLevelKeys;
    QSet<QString> subKeys;
    QList<QWidget *> foundWidgets;
    int currentFoundIndex;
    Qt::CaseSensitivity cs;

    void updateValue(const QString &path, const QString &newValue);

    void displayNode(const YamlNode &node, const QString &parentPath, const QString &searchText);
    void displayTreeNode(const YamlNode &node,
                         const QString &parentPath,
                         const QString &searchText,
                         QTreeWidgetItem *parentItem,
                         QTreeWidget *treeWidget);

    void collectKeys(const YamlNode &node, QSet<QString> &topLevelKeys, QSet<QString> &subKeys);

    void slotShortcutCtrlF();
    void slotShortcutF11();

    void highlightCurrentFound();
    void scrollIntoView(QWidget *widget);

    void createCheckBox(const QString &name, int row, int col, bool topLevelKey);
    void clearKeysArea();
    void clearScrollArea();
    void saveData();
    void displayYamlData();
    void displaykeys();
};
#endif // MAINWINDOW_H
