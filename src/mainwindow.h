#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QHash>
#include <QMainWindow>
#include <QModelIndex>
#include <QRegularExpression>
#include <memory>
#include <QSet>
#include <QShortcut>
#include <QStringList>
#include <QTranslator>

#include "files/yaml/yamlnode.h"
#include "controller/documentstore.h"
#include "security/tokenstore.h"

class YamlReader;
class YandexApi;
class FileSystem;
class FlowLayout;
class ReplaceWindow;
class SearchingWindow;
class SearchController;
class FileWatchService;
class QDragEnterEvent;
class QDropEvent;
class LogPanel;
class QUndoStack;
class YamlTreeModel;
class QTreeView;

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
    void closeEvent(QCloseEvent *event) override;

private slots:
    void HandleAddKeyValue(const QString &path, const QString &newValue, bool isKey);
    void HandleDeleteElement(const QString &path, bool isKey);
    void CloseTab(int index);

    void on_fileNamecmb_currentIndexChanged(int index);
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void onCheckBoxStateChanged(int state);


    void on_OpenFolderYmlFilebtn_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_OpenFilebtn_clicked();

private:
    void ConnectShortCut();
    void InitLanguageCmb();
    void InitKeysLayout();
    void ConnectServices();

    void switchLanguage(const QString &locale);

    void Displaykeys(const YamlNode &root);
    void SaveData(const QString &fileName);
    void DisplayYamlData();
    void ShowTreeInTab(const QString &fileName);


    void CollectKeys(const YamlNode &node, QSet<QString> &keys) const;
    void ReadFile();

    void SlotShortcutCtrlF();
    void SlotShortcutF11();
    void SlotShortcutCtrlS();
    void SlotShortcutCtrlR();

    void CreateCheckBox(const QString &name);

    void ClearKeysArea();


    void RemoveTab(const QString &tabText);

    void UploadFileOnCmb(const QString &file);
    bool CheckOpenTab(const QString &file);

    int FindTabByName(const QString &fileName) const;

    void OnFolderChanged(const QString &path);
    void OnFileModified(const QString &fileName, const QString &absPath);
    void OpenFileByPath(const QString &local_path);

    void ReloadFileFromDisk(const QString &file_name);

    bool IsFileDirty(const QString &file_name) const;
    QStringList UnsavedFiles() const;

    QSet<QWidget *> invalid_fields_;
    bool HasValidationErrors() const { return !invalid_fields_.isEmpty(); }

    void ShowTreeContextMenu(const QPoint &pos);
    void SaveExpandedState();
    void RestoreExpandedState();

    Ui::MainWindow *ui;

    QTranslator translator_;

    QShortcut *key_F11 = nullptr;
    QShortcut *key_ctrl_f = nullptr;
    QShortcut *key_ctrl_s = nullptr;
    QShortcut *key_ctrl_r = nullptr;

    FlowLayout *flow_keys_layout = nullptr;


    SearchingWindow *search_wnd = nullptr;

    SearchController *search_controller = nullptr;
    FileWatchService *file_watch = nullptr;

    void InitAccountMenu();
    void openConnectDialog();

    TokenStore token_store;

    ReplaceWindow *replace_wnd = nullptr;

    std::unique_ptr<YamlReader> yaml_reader;
    std::unique_ptr<YandexApi> yandex_api;
    std::unique_ptr<FileSystem> file_local_system;


    QTreeView *tree_view = nullptr;
    YamlTreeModel *tree_model = nullptr;

    DocumentStore doc_store;
    QHash<QString, QHash<QString, bool>> check_box_states_nodes;

    QString previous_text_cmb;

    LogPanel *log_panel = nullptr;
    void InitLogPanel();

    QUndoStack *undo_stack = nullptr;
    void InitUndoStack();

    void PushUndoCommand(const QString &text, const YamlNode &before);
    void ApplySnapshot(const YamlNode &snapshot);
    void SlotUndo();
    void SlotRedo();

    YamlNode last_committed_tree_;
    bool value_edit_active_ = false;
    bool applying_snapshot_ = false;
    bool rebuild_scheduled_ = false;

    QHash<QString, bool> check_box_states;
    QSet<QString> keys;

    bool reload_prompt_active_ = false;

    QSet<QString> expanded_paths;
};

#endif
