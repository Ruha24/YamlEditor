#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QMainWindow>
#include <QModelIndex>
#include <memory>
#include <QSet>
#include <QStringList>
#include <QTranslator>

#include "files/yaml/yamlnode.h"
#include "controller/documentstore.h"
#include "security/tokenstore.h"

class YamlReader;
class YandexApi;
class FileSystem;
class SearchController;
class FileWatchService;
class LogPanel;
class FindBar;
class FileListPanel;
class KeyFilterPanel;
class QAction;
class QActionGroup;
class QDockWidget;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QMenu;
class QPushButton;
class QStackedWidget;
class QTabBar;
class QToolBar;
class QToolButton;
class QTreeView;
class QUndoGroup;
class QUndoStack;
class YamlTreeModel;

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
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // --- UI construction --------------------------------------------------
    void CreateActions();
    void CreateMenus();
    void CreateToolBar();
    void CreateCentralArea();
    QWidget *CreateEmptyState();
    void CreateDocks();
    void CreateStatusBar();
    void ConnectServices();
    void PopulateFileList();
    void RetranslateUi();
    void RefreshIcons();
    void RestoreSettings();
    void SaveSettings() const;

    void SwitchLanguage(const QString &locale);
    void OpenConnectDialog();
    void UpdateCloudStatus();
    void UpdateActions();
    void UpdateStatus();
    void UpdateCenter();

    // --- Documents --------------------------------------------------------
    // One tree view/model is shared by all files; each open file has a tab, a
    // tree in doc_store and an undo stack. current_file_ is the one shown.
    void ActivateFile(const QString &file_name);
    bool OpenInNewTab(const QString &file_name);
    void ShowFile(const QString &file_name);
    void ShowNoFile();
    void AddKnownFile(const QString &file_name);
    QString FileAt(int tab) const;
    int FindTabByName(const QString &file_name) const;
    void OnTabChanged(int index);
    void CloseTab(int index);
    void UpdateTabTitle(const QString &file_name);

    void OpenFileDialog();
    void OpenFolderDialog();
    void OpenFileByPath(const QString &local_path);
    void OnFolderChanged(const QString &path);
    void OnFileModified(const QString &file_name, const QString &abs_path);
    void ReloadFileFromDisk(const QString &file_name);

    // --- Saving -----------------------------------------------------------
    bool WriteFile(const QString &file_name);
    void SaveCurrent();
    void SaveAll();
    bool IsFileDirty(const QString &file_name) const;
    QStringList UnsavedFiles() const;

    // --- Editing / undo ---------------------------------------------------
    QUndoStack *StackFor(const QString &file_name);
    void CommitEdit(const QString &text, YamlNode before); // by value: callers pass last_committed_tree_
    void ApplySnapshot(const QString &file_name, const YamlNode &snapshot);
    void AddChildAt(const QModelIndex &node);
    void DeleteAt(const QModelIndex &node);
    void ShowTreeContextMenu(const QPoint &pos);
    QSet<QString> ExpandedPaths() const;
    void RestoreExpanded(const QSet<QString> &paths);

    // --- Key filter -------------------------------------------------------
    void RefreshKeyFilter();
    void ApplyKeyFilter();

    // --- Find -------------------------------------------------------------
    void ShowFind(bool replace);

    // --- Widgets ----------------------------------------------------------
    QToolBar *toolbar_ = nullptr;
    QStackedWidget *center_stack_ = nullptr;
    QWidget *empty_state_ = nullptr;
    QLabel *empty_icon_ = nullptr;
    QLabel *empty_title_ = nullptr;
    QLabel *empty_text_ = nullptr;
    QLabel *empty_hint_ = nullptr;
    QPushButton *empty_open_btn_ = nullptr;
    QPushButton *empty_folder_btn_ = nullptr;
    QTabBar *tab_bar_ = nullptr;
    FindBar *find_bar_ = nullptr;
    QTreeView *tree_view = nullptr;
    YamlTreeModel *tree_model = nullptr;

    QDockWidget *files_dock_ = nullptr;
    QDockWidget *filter_dock_ = nullptr;
    FileListPanel *file_panel_ = nullptr;
    KeyFilterPanel *filter_panel_ = nullptr;
    LogPanel *log_panel = nullptr;

    QLabel *status_file_ = nullptr;
    QLabel *status_stats_ = nullptr;
    QToolButton *status_cloud_ = nullptr;
    QToolButton *status_log_ = nullptr;
    int unseen_errors_ = 0;

    // --- Menus and actions -------------------------------------------------
    QMenu *menu_file_ = nullptr;
    QMenu *menu_edit_ = nullptr;
    QMenu *menu_view_ = nullptr;
    QMenu *menu_theme_ = nullptr;
    QMenu *menu_language_ = nullptr;
    QMenu *menu_account_ = nullptr;

    QAction *act_open_ = nullptr;
    QAction *act_open_folder_ = nullptr;
    QAction *act_save_ = nullptr;
    QAction *act_save_all_ = nullptr;
    QAction *act_close_tab_ = nullptr;
    QAction *act_exit_ = nullptr;
    QAction *act_undo_ = nullptr;
    QAction *act_redo_ = nullptr;
    QAction *act_find_ = nullptr;
    QAction *act_replace_ = nullptr;
    QAction *act_add_child_ = nullptr;
    QAction *act_delete_ = nullptr;
    QAction *act_expand_all_ = nullptr;
    QAction *act_collapse_all_ = nullptr;
    QAction *act_fullscreen_ = nullptr;
    QAction *act_theme_system_ = nullptr;
    QAction *act_theme_light_ = nullptr;
    QAction *act_theme_dark_ = nullptr;
    QAction *act_lang_en_ = nullptr;
    QAction *act_lang_ru_ = nullptr;
    QAction *act_connect_ = nullptr;

    // --- Services and state -----------------------------------------------
    QTranslator translator_;
    QString language_;

    SearchController *search_controller = nullptr;
    FileWatchService *file_watch = nullptr;
    TokenStore token_store;

    std::unique_ptr<YamlReader> yaml_reader;
    std::unique_ptr<YandexApi> yandex_api;
    std::unique_ptr<FileSystem> file_local_system;

    DocumentStore doc_store;
    QString current_file_;

    QUndoGroup *undo_group_ = nullptr;
    QHash<QString, QUndoStack *> undo_stacks_;

    YamlNode last_committed_tree_;
    bool applying_snapshot_ = false;
    bool suppress_commit_ = false;
    bool reload_prompt_active_ = false;

    QHash<QString, QSet<QString>> hidden_keys_;    // per file
    QHash<QString, QSet<QString>> expanded_paths_; // per file
};

#endif
