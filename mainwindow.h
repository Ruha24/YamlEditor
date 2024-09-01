#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QDesktopServices>
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

    void on_prettychb_toggled(bool checked);

    void on_treechb_toggled(bool checked);

    void on_OpenFolderYmlFilebtn_clicked();

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

    QWidget *mainWidget;
    QVBoxLayout *mainLayout;
    QTreeWidget *treeWidget;

    QWidget *previousWidget = nullptr;
    QString previousWidgetOriginalStyleSheet;
    QString previousTextCmb;

    QSet<QString> displayedKeys;
    YamlNode root;
    QMap<QString, bool> checkBoxStates;
    QSet<QString> topLevelKeys;
    QSet<QString> subKeys;
    QList<QWidget *> foundWidgets;
    int currentFoundIndex;
    int startingIndex;
    Qt::CaseSensitivity cs;

    void updateValue(const QString &path, const QString &newValue, bool isKey);

    void displayNode(const YamlNode &node,
                     const QString &parentPath,
                     const QString &searchText,
                     bool useRegex);
    void displayTreeNode(const YamlNode &node,
                         const QString &parentPath,
                         const QString &searchText,
                         QTreeWidgetItem *parentItem,
                         QTreeWidget *treeWidget,
                         bool useRegex);

    void collectKeys(const YamlNode &node, QSet<QString> &topLevelKeys, QSet<QString> &subKeys);
    void readFile();

    void slotShortcutCtrlF();
    void slotShortcutF11();
    void slotShortcutCtrlS();
    void slotShortcutCtrlR();

    void highlightCurrentFound();
    void scrollIntoView(QWidget *widget);

    void createCheckBox(const QString &name, int row, int col, bool topLevelKey);

    void clearKeysArea();
    void clearScrollArea();

    void saveData(const QString &fileName);

    void displayYamlData();
    void displaykeys();
};
#endif // MAINWINDOW_H
