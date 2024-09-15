#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QDesktopServices>
#include <QFileSystemWatcher>
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
#include <QtAlgorithms>

#include "customlineedit.h"
#include "filesystem.h"
#include "replacewindow.h"
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
    void handleAddKeyValue(QString path, QString newValue, bool isKey);
    void handleDeleteElement(QString path, bool isKey);
    void closeTab(int index);

    void on_fileNamecmb_currentIndexChanged(int index);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void onCheckBoxStateChanged(int state);

    void searchingText(const QString &text, bool isSensitive, bool is_downward, bool useRegex);

    void searchReplaceText(const QString &text, bool isSensitive, bool useRegex);

    void replaceText(const QString &findText,
                     const QString &replaceText,
                     bool allText,
                     bool useRegex);

    void on_OpenFolderYmlFilebtn_clicked();

    void on_tabWidget_currentChanged(int index);

private:
    Ui::MainWindow *ui;

    QShortcut *keyF11;
    QShortcut *keyCtrlF;
    QShortcut *keyCtrlS;
    QShortcut *keyCtrlR;

    QRegularExpression searchingRegex;

    ReplaceWindow *replaceWnd = nullptr;
    void replaceInWidget(QWidget *widget,
                         const QString &findText,
                         const QString &replaceText,
                         bool useRegex);

    searchingWindow *searchWnd = nullptr;
    QString searching_text;

    YamlReader *yamlReader;
    YandexApi *yandexApi;

    QTreeWidget *treeWidget = nullptr;
    int findTabByName(const QString &fileName);
    QMap<QString, YamlNode> nodes;

    QWidget *previousWidget = nullptr;
    QString previousWidgetOriginalStyleSheet;
    QString previousTextCmb;

    YamlNode root;
    QMap<QString, bool> checkBoxStates;
    QSet<QString> keys;
    QList<QWidget *> foundWidgets;
    int currentFoundIndex;
    int startingIndex;
    bool isUpdateFile = false;
    Qt::CaseSensitivity cs;

    FileSystem *fileLocalSystem;
    QList<QString> localFiles;

    QFileSystemWatcher *fileWatcher;
    void onFolderChanged(const QString &path);

    void updateValue(const QString &path, const QString &newValue, bool isKey);
    void displaykeys(YamlNode root);
    void saveData(const QString &fileName);
    void displayYamlData();
    void displayTreeNode(const YamlNode &node,
                         const QString &parentPath,
                         const QString &searchText,
                         QTreeWidgetItem *parentItem,
                         QTreeWidget *treeWidget,
                         bool useRegex);

    void collectKeys(const YamlNode &node, QSet<QString> &keys);
    void readFile();

    void slotShortcutCtrlF();
    void slotShortcutF11();
    void slotShortcutCtrlS();
    void slotShortcutCtrlR();

    void highlightCurrentFound();
    void scrollIntoView(QWidget *widget);

    void createCheckBox(const QString &name, int row, int col);

    void clearKeysArea();
    void clearTreeWidget();

    void uploadFileOnCmb(const QString &file);
};
#endif // MAINWINDOW_H
