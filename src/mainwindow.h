#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QFileSystemWatcher>
#include <QHash>
#include <QMainWindow>
#include <QRegularExpression>
#include <memory>
#include <QSet>
#include <QShortcut>
#include <QStringList>
#include <QTranslator>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "files/yaml/yamlnode.h"

class YamlReader;
class YandexApi;
class FileSystem;
class FlowLayout;
class ReplaceWindow;
class SearchingWindow;
class QDragEnterEvent;
class QDropEvent;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void HandleAddKeyValue(const QString &path, const QString &newValue, bool isKey);
    void HandleDeleteElement(const QString &path, bool isKey);
    void CloseTab(int index);

    void on_fileNamecmb_currentIndexChanged(int index);
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void onCheckBoxStateChanged(int state);

    void SearchingText(const QString &text, bool isSensitive, bool isDownward, bool useRegex);
    void SearchReplaceText(const QString &text, bool isSensitive, bool useRegex);
    void ReplaceText(const QString &findText, const QString &replaceText, bool allText,
                     bool useRegex);

    void on_OpenFolderYmlFilebtn_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_OpenFilebtn_clicked();

private:
    void ConnectShortCut();
    void InitLanguageCmb();
    void InitKeysLayout();
    void ConnectServices();

    void switchLanguage(const QString &locale);

    void RefreshCurrentTree();
    void UpdateValue(const QString &path, const QString &newValue, bool isKey);
    void Displaykeys(const YamlNode &root);
    void SaveData(const QString &fileName);
    void DisplayYamlData();
    void DisplayTreeNode(const YamlNode &node, const QString &parentPath, const QString &searchText,
                         QTreeWidgetItem *parentItem, QTreeWidget *treeWidget,
                         const QRegularExpression &regex, bool useRegex);

    void CollectKeys(const YamlNode &node, QSet<QString> &keys) const;
    void ReadFile();

    void SlotShortcutCtrlF();
    void SlotShortcutF11();
    void SlotShortcutCtrlS();
    void SlotShortcutCtrlR();

    void HighlightCurrentFound();
    void ScrollIntoView(QWidget *widget);

    void CreateCheckBox(const QString &name);

    void ClearKeysArea();
    void ClearTreeWidget();
    void ClearTabWidget(QWidget *tab);

    void RemoveTab(const QString &tabText);

    void UploadFileOnCmb(const QString &file);
    bool CheckOpenTab(const QString &file);

    int FindTabByName(const QString &fileName) const;

    void OnFolderChanged(const QString &path);
    void OpenFileByPath(const QString &local_path);

    void SaveExpandedState();
    void RestoreExpandedState();

    void RunSearch(const QString &text, bool useRegex, bool resetSelection);
    bool BuildRegex(const QString &text, bool isSensitive, QRegularExpression &out) const;

    Ui::MainWindow *ui;

    QTranslator translator_;

    QShortcut *key_F11 = nullptr;
    QShortcut *key_ctrl_f = nullptr;
    QShortcut *key_ctrl_s = nullptr;
    QShortcut *key_ctrl_r = nullptr;

    FlowLayout *flow_keys_layout = nullptr;

    QRegularExpression searching_regex;

    ReplaceWindow *replace_wnd = nullptr;
    void ReplaceInWidget(QWidget *widget, const QString &findText, const QString &replaceText,
                         bool useRegex);

    SearchingWindow *search_wnd = nullptr;
    QString searching_text;

    std::unique_ptr<YamlReader> yaml_reader;
    std::unique_ptr<YandexApi> yandex_api;
    std::unique_ptr<FileSystem> file_local_system;

    QTreeWidget *tree_widget = nullptr;
    QHash<QString, YamlNode> nodes;
    QHash<QString, QHash<QString, bool>> check_box_states_nodes;

    QWidget *previous_widget = nullptr;
    QString previous_widget_original_style_sheet;
    QString previous_text_cmb;

    YamlNode root;
    QHash<QString, bool> check_box_states;
    QSet<QString> keys;
    QList<QWidget *> found_widgets;
    int current_found_index = -1;
    int starting_index = -1;
    bool is_update_file = false;
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;

    std::unique_ptr<QFileSystemWatcher> file_watcher;

    QSet<QString> expanded_paths;
};

#endif // MAINWINDOW_H
