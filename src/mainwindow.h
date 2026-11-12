#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QMainWindow>
#include <QModelIndex>
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
class QMenu;
class QAction;
class QUndoGroup;
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
    void CloseTab(int index);

    void on_fileNamecmb_currentIndexChanged(int index);
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

    void on_OpenFolderYmlFilebtn_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_OpenFilebtn_clicked();

private:
    void ConnectShortCut();
    void InitLanguageCmb();
    void InitKeysLayout();
    void InitTreeView();
    void InitLogPanel();
    void InitAccountMenu();
    void ConnectServices();
    void PopulateFileList();

    void switchLanguage(const QString &locale);
    void openConnectDialog();

    // --- Documents --------------------------------------------------------
    // The tree view and model are shared; every open file has a tab, a tree in
    // doc_store and its own undo stack. current_file_ is the file shown now.
    void ActivateFile(const QString &file_name);
    bool OpenInNewTab(const QString &file_name);
    void ShowFile(const QString &file_name);
    void ShowNoFile();
    void ShowTreeInTab(const QString &file_name);
    void DetachTreeView();
    void SyncFileCombo(const QString &file_name);
    void AddToFileCombo(const QString &file_name);

    void OpenFileByPath(const QString &local_path);
    void OnFolderChanged(const QString &path);
    void OnFileModified(const QString &file_name, const QString &abs_path);
    void ReloadFileFromDisk(const QString &file_name);

    int FindTabByName(const QString &file_name) const;

    // --- Saving -----------------------------------------------------------
    bool WriteFile(const QString &file_name);
    void SaveData(const QString &file_name);
    bool IsFileDirty(const QString &file_name) const;
    QStringList UnsavedFiles() const;

    // --- Editing / undo ---------------------------------------------------
    QUndoStack *StackFor(const QString &file_name);
    void CommitEdit(const QString &text, YamlNode before); // by value: callers pass last_committed_tree_
    void ApplySnapshot(const QString &file_name, const YamlNode &snapshot);
    void ShowTreeContextMenu(const QPoint &pos);
    void SaveExpandedState();
    void RestoreExpandedState();

    // --- Keys panel -------------------------------------------------------
    void Displaykeys(const YamlNode &root);
    void CollectKeys(const YamlNode &node, QSet<QString> &keys) const;
    void CreateCheckBox(const QString &name);
    void ClearKeysArea();

    void SlotShortcutCtrlF();
    void SlotShortcutF11();
    void SlotShortcutCtrlS();
    void SlotShortcutCtrlR();

    Ui::MainWindow *ui;

    QTranslator translator_;

    FlowLayout *flow_keys_layout = nullptr;

    SearchingWindow *search_wnd = nullptr;
    ReplaceWindow *replace_wnd = nullptr;

    SearchController *search_controller = nullptr;
    FileWatchService *file_watch = nullptr;
    LogPanel *log_panel = nullptr;

    QMenu *account_menu_ = nullptr;
    QAction *connect_action_ = nullptr;

    TokenStore token_store;

    std::unique_ptr<YamlReader> yaml_reader;
    std::unique_ptr<YandexApi> yandex_api;
    std::unique_ptr<FileSystem> file_local_system;

    QTreeView *tree_view = nullptr;
    YamlTreeModel *tree_model = nullptr;

    DocumentStore doc_store;
    QString current_file_;

    QUndoGroup *undo_group_ = nullptr;
    QHash<QString, QUndoStack *> undo_stacks_;

    YamlNode last_committed_tree_;
    bool applying_snapshot_ = false;
    bool reload_prompt_active_ = false;

    QHash<QString, QHash<QString, bool>> check_box_states_nodes;
    QHash<QString, bool> check_box_states;
    QSet<QString> keys;

    QSet<QString> expanded_paths;
};

#endif
