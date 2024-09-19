#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QFileSystemWatcher>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMimeData>
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

#include "api/yandex/yandexapi.h"
#include "customWidget/customlineedit.h"
#include "files/filesystem.h"
#include "files/yaml/yamlreader.h"
#include "replacewindow.h"
#include "searchingwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private slots:
    void HandleAddKeyValue(QString path, QString newValue, bool isKey);
    void HandleDeleteElement(QString path, bool isKey);
    void CloseTab(int index);

    void on_fileNamecmb_currentIndexChanged(int index);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void onCheckBoxStateChanged(int state);

    void SearchingText(const QString &text, bool isSensitive, bool is_downward, bool useRegex);

    void SearchReplaceText(const QString &text, bool isSensitive, bool useRegex);

    void ReplaceText(const QString &findText,
                     const QString &replaceText,
                     bool allText,
                     bool useRegex);

    void on_OpenFolderYmlFilebtn_clicked();

    void on_tabWidget_currentChanged(int index);

private:
    Ui::MainWindow *ui;

    QShortcut *key_F11;
    QShortcut *key_ctrl_f;
    QShortcut *key_ctrl_s;
    QShortcut *key_ctrl_r;

    QRegularExpression searching_regex;

    ReplaceWindow *replace_wnd;
    void ReplaceInWidget(QWidget *widget,
                         const QString &findText,
                         const QString &replaceText,
                         bool useRegex);

    searchingWindow *search_wnd;
    QString searching_text;

    YamlReader *yaml_reader;
    YandexApi *yandex_api;

    QTreeWidget *tree_widget;
    int FindTabByName(const QString &fileName);
    QMap<QString, YamlNode> nodes;

    QWidget *previous_widget;
    QString previous_widget_original_style_sheet;
    QString previous_text_cmb;

    YamlNode root;
    QMap<QString, bool> check_box_states;
    QSet<QString> keys;
    QList<QWidget *> found_widgets;
    int current_found_index;
    int starting_index;
    bool is_update_file;
    Qt::CaseSensitivity cs;

    FileSystem *file_local_system;
    QList<QString> local_files;

    QFileSystemWatcher *file_watcher;
    void OnFolderChanged(const QString &path);

    void UpdateValue(const QString &path, const QString &newValue, bool isKey);
    void Displaykeys(YamlNode root);
    void SaveData(const QString &fileName);
    void DisplayYamlData();
    void DisplayTreeNode(const YamlNode &node,
                         const QString &parentPath,
                         const QString &searchText,
                         QTreeWidgetItem *parentItem,
                         QTreeWidget *treeWidget,
                         bool useRegex);

    void CollectKeys(const YamlNode &node, QSet<QString> &keys);
    void ReadFile();

    void SlotShortcutCtrlF();
    void SlotShortcutF11();
    void SlotShortcutCtrlS();
    void SlotShortcutCtrlR();

    void HighlightCurrentFound();
    void ScrollIntoView(QWidget *widget);

    void CreateCheckBox(const QString &name, int row, int col);

    void ClearKeysArea();
    void ClearTreeWidget();
    void ClearTabWidget(QWidget *tab);

    void RemoveTab(const QString &tabText);

    void UploadFileOnCmb(const QString &file);
};
#endif // MAINWINDOW_H
