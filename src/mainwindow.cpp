#include "mainwindow.h"

#include <memory>
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QMimeData>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QUndoGroup>
#include <QUndoStack>
#include <QTreeView>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

#include "api/yandex/yandexapi.h"
#include "customWidget/flowlayout.h"
#include "files/filesystem.h"
#include "customWidget/logpanel.h"
#include "files/snapshotcommand.h"
#include "files/yaml/yamlitemdelegate.h"
#include "files/yaml/yamlreader.h"
#include "files/yaml/yamltreemodel.h"
#include "controller/searchcontroller.h"
#include "controller/filewatchservice.h"
#include "customWidget/connectdialog.h"
#include "replacewindow.h"
#include "searchingwindow.h"

namespace {

QString YmlFolder()
{
    return QDir::currentPath() + "/ymlFiles";
}

bool IsYamlFile(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix();
    return suffix.compare("yml", Qt::CaseInsensitive) == 0
           || suffix.compare("yaml", Qt::CaseInsensitive) == 0;
}

const char *kScrollBarStyle = R"(
    QScrollBar:vertical {
        background: #262626; width: 11px; margin: 0; border: none;
    }
    QScrollBar::handle:vertical {
        background: #45596a; border-radius: 5px; min-height: 28px;
    }
    QScrollBar::handle:vertical:hover { background: #51b4d2; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0; background: none; border: none;
    }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    QScrollBar:horizontal {
        background: #262626; height: 11px; margin: 0; border: none;
    }
    QScrollBar::handle:horizontal {
        background: #45596a; border-radius: 5px; min-width: 28px;
    }
    QScrollBar::handle:horizontal:hover { background: #51b4d2; }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
        width: 0; background: none; border: none;
    }
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }
)";

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    undo_group_ = new QUndoGroup(this);

    InitTreeView();
    InitLogPanel();
    InitKeysLayout();

    search_controller = new SearchController(this);
    search_controller->setTargets(tree_model, tree_view);
    connect(search_controller, &SearchController::info, this,
            [this](const QString &msg) { QMessageBox::information(this, tr("Search"), msg); });
    connect(search_controller, &SearchController::warning, this,
            [this](const QString &msg) { QMessageBox::warning(this, tr("Search"), msg); });

    file_local_system = std::make_unique<FileSystem>(YmlFolder());
    yandex_api = std::make_unique<YandexApi>();
    yaml_reader = std::make_unique<YamlReader>();

    ConnectServices();
    ConnectShortCut();
    InitLanguageCmb();
    InitAccountMenu();
    PopulateFileList();

    yandex_api->GetFiles();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::InitTreeView()
{
    tree_model = new YamlTreeModel(this);
    tree_view = new QTreeView(this);
    tree_view->setModel(tree_model);

    auto *delegate = new YamlItemDelegate(this);
    tree_view->setItemDelegate(delegate);
    connect(delegate, &YamlItemDelegate::editRejected, this, [this](const QString &reason) {
        log_panel->logWarning(tr("Edit rejected: %1").arg(reason));
        statusBar()->showMessage(reason, 5000);
    });

    tree_view->setEditTriggers(QAbstractItemView::DoubleClicked
                               | QAbstractItemView::SelectedClicked
                               | QAbstractItemView::EditKeyPressed);

    tree_view->setColumnWidth(0, 280);
    tree_view->setAlternatingRowColors(true);
    tree_view->setRootIsDecorated(true);
    tree_view->setIndentation(20);
    tree_view->setUniformRowHeights(true);
    tree_view->setStyleSheet(QString(R"(
        QTreeView {
            background-color: #2f2f2f;
            border: none;
            outline: 0;
            font-size: 14px;
            alternate-background-color: #333333;
        }
        QTreeView::item {
            min-height: 30px;
            padding: 3px 6px;
            border-bottom: 1px solid #3a3a3a;
            color: #e6e6e6;
        }
        QTreeView::item:hover { background-color: #3f3f3f; }
        QTreeView::item:selected { background-color: #33405c; }
        QHeaderView::section {
            background-color: #262626;
            color: #9a9a93;
            padding: 8px 10px;
            border: none;
            font-weight: 500;
        }
    )") + kScrollBarStyle);

    tree_view->hide();

    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree_view, &QWidget::customContextMenuRequested,
            this, &MainWindow::ShowTreeContextMenu);

    // Cell edits (from the delegate or from Replace) become undoable steps.
    connect(tree_model, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &, const QModelIndex &, const QList<int> &) {
                if (applying_snapshot_ || current_file_.isEmpty())
                    return;
                CommitEdit(tr("Edit"), last_committed_tree_);
            });
}

void MainWindow::InitKeysLayout()
{
    QWidget *keys_container = ui->scrollAreaWidgetContents_2;
    if (!keys_container)
        return;

    if (QLayout *old_layout = keys_container->layout()) {
        QLayoutItem *item;
        while ((item = old_layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete old_layout;
    }
    flow_keys_layout = new FlowLayout(keys_container, 6, 8, 8);

    if (ui->scrollArea_2)
        ui->scrollArea_2->setStyleSheet(kScrollBarStyle);
}

void MainWindow::InitLogPanel()
{
    log_panel = new LogPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, log_panel);
    log_panel->logInfo(tr("Application started"));
}

// ---------------------------------------------------------------------------
// Undo
// ---------------------------------------------------------------------------

QUndoStack *MainWindow::StackFor(const QString &file_name)
{
    QUndoStack *&stack = undo_stacks_[file_name];
    if (!stack) {
        stack = new QUndoStack(undo_group_);
        connect(stack, &QUndoStack::cleanChanged, this, [this, file_name](bool clean) {
            const int tab = FindTabByName(file_name);
            if (tab != -1)
                ui->tabWidget->setTabToolTip(tab, clean ? QString() : tr("Unsaved changes"));
        });
    }
    return stack;
}

void MainWindow::CommitEdit(const QString &text, YamlNode before)
{
    const QString file_name = current_file_;
    const YamlNode after = tree_model->ToYamlNode();

    last_committed_tree_ = after;
    doc_store.set(file_name, after);
    Displaykeys(after);

    StackFor(file_name)->push(new YamlSnapshotCommand(
        text, std::move(before), after,
        [this, file_name](const YamlNode &snapshot) { ApplySnapshot(file_name, snapshot); }));
}

void MainWindow::ApplySnapshot(const QString &file_name, const YamlNode &snapshot)
{
    doc_store.set(file_name, snapshot);
    if (file_name != current_file_)
        return;

    applying_snapshot_ = true;
    SaveExpandedState();
    tree_model->SetRoot(snapshot);
    RestoreExpandedState();
    last_committed_tree_ = snapshot;
    Displaykeys(snapshot);
    applying_snapshot_ = false;
}

void MainWindow::SaveExpandedState()
{
    expanded_paths.clear();
    if (!tree_view || !tree_model)
        return;

    std::function<void(const QModelIndex &)> walk = [&](const QModelIndex &parent) {
        const int rows = tree_model->rowCount(parent);
        for (int r = 0; r < rows; ++r) {
            const QModelIndex idx = tree_model->index(r, 0, parent);
            if (tree_view->isExpanded(idx)) {
                const QString path = tree_model->data(idx, YamlTreeModel::PathRole).toString();
                if (!path.isEmpty())
                    expanded_paths.insert(path);
            }
            walk(idx);
        }
    };
    walk(QModelIndex());
}

void MainWindow::RestoreExpandedState()
{
    if (!tree_view || !tree_model)
        return;

    std::function<void(const QModelIndex &)> walk = [&](const QModelIndex &parent) {
        const int rows = tree_model->rowCount(parent);
        for (int r = 0; r < rows; ++r) {
            const QModelIndex idx = tree_model->index(r, 0, parent);
            const QString path = tree_model->data(idx, YamlTreeModel::PathRole).toString();
            if (expanded_paths.contains(path))
                tree_view->expand(idx);
            walk(idx);
        }
    };
    walk(QModelIndex());
}

// ---------------------------------------------------------------------------
// Services, shortcuts, menus
// ---------------------------------------------------------------------------

void MainWindow::ConnectServices()
{
    connect(yaml_reader.get(), &YamlReader::ErrorOccurred, this, [this](const QString &msg) {
        log_panel->logError(msg);
        QMessageBox::warning(this, tr("Error"), msg);
    });
    connect(yandex_api.get(), &YandexApi::ErrorOccurred, this, [this](const QString &msg) {
        log_panel->logWarning(tr("Network: %1").arg(msg));
    });
    connect(yandex_api.get(), &YandexApi::NewFile, this, [this](const QString &file_name) {
        log_panel->logInfo(tr("Downloaded from cloud: %1").arg(file_name));
        if (!file_local_system->Contains(file_name))
            file_local_system->AddFile(QDir(YmlFolder()).absoluteFilePath(file_name));
        AddToFileCombo(file_name);
    });

    file_watch = new FileWatchService(this);
    file_watch->watchFolder(YmlFolder());
    connect(file_watch, &FileWatchService::folderChanged, this, &MainWindow::OnFolderChanged);
    connect(file_watch, &FileWatchService::fileModified, this, &MainWindow::OnFileModified);
    connect(file_watch, &FileWatchService::fileRemoved, this,
            [this](const QString &file_name, const QString &) {
                log_panel->logWarning(tr("File removed on disk: %1").arg(file_name));
            });

    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::CloseTab);
    ui->fileNamecmb->setView(new QListView(ui->fileNamecmb));
}

void MainWindow::ConnectShortCut()
{
    connect(new QShortcut(Qt::Key_F11, this), &QShortcut::activated,
            this, &MainWindow::SlotShortcutF11);
    connect(new QShortcut(QKeySequence::Find, this), &QShortcut::activated,
            this, &MainWindow::SlotShortcutCtrlF);
    connect(new QShortcut(QKeySequence::Save, this), &QShortcut::activated,
            this, &MainWindow::SlotShortcutCtrlS);
    connect(new QShortcut(Qt::CTRL | Qt::Key_R, this), &QShortcut::activated,
            this, &MainWindow::SlotShortcutCtrlR);

    connect(new QShortcut(QKeySequence::Undo, this), &QShortcut::activated, this, [this]() {
        if (undo_group_->canUndo()) {
            log_panel->logInfo(tr("Undo: %1").arg(undo_group_->undoText()));
            undo_group_->undo();
        }
    });
    connect(new QShortcut(QKeySequence::Redo, this), &QShortcut::activated, this, [this]() {
        if (undo_group_->canRedo()) {
            log_panel->logInfo(tr("Redo: %1").arg(undo_group_->redoText()));
            undo_group_->redo();
        }
    });
}

void MainWindow::InitLanguageCmb()
{
    ui->langCombo->addItem("English", "en_GB");
    ui->langCombo->addItem("Русский", "ru_RU");
    connect(ui->langCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        switchLanguage(ui->langCombo->currentData().toString());
    });
}

void MainWindow::InitAccountMenu()
{
    if (!menuBar())
        return;

    account_menu_ = menuBar()->addMenu(tr("Account"));
    connect_action_ = account_menu_->addAction(tr("Connect Yandex Disk..."));
    connect(connect_action_, &QAction::triggered, this, &MainWindow::openConnectDialog);
}

void MainWindow::openConnectDialog()
{
    ConnectDialog dialog(&token_store, this);
    connect(&dialog, &ConnectDialog::tokenSaved, this, [this]() {
        if (yandex_api) {
            yandex_api->reloadToken();
            log_panel->logInfo(tr("Yandex Disk token updated"));
        }
    });
    dialog.exec();
}

void MainWindow::switchLanguage(const QString &locale)
{
    qApp->removeTranslator(&translator_);

    const QString path = ":/i18n/editor_" + locale;
    if (translator_.load(path))
        qApp->installTranslator(&translator_);

    ui->retranslateUi(this);
    ui->fileNamecmb->setPlaceholderText(tr("Select a file..."));
    if (account_menu_)
        account_menu_->setTitle(tr("Account"));
    if (connect_action_)
        connect_action_->setText(tr("Connect Yandex Disk..."));
}

// ---------------------------------------------------------------------------
// File list and tabs
// ---------------------------------------------------------------------------

void MainWindow::PopulateFileList()
{
    QStringList names = file_local_system->GetFiles().keys();
    std::sort(names.begin(), names.end(), [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });

    const QSignalBlocker blocker(ui->fileNamecmb);
    for (const QString &name : std::as_const(names))
        ui->fileNamecmb->addItem(name);
    ui->fileNamecmb->setPlaceholderText(tr("Select a file..."));
    ui->fileNamecmb->setCurrentIndex(-1);
}

void MainWindow::AddToFileCombo(const QString &file_name)
{
    if (ui->fileNamecmb->findText(file_name) != -1)
        return;
    const QSignalBlocker blocker(ui->fileNamecmb);
    ui->fileNamecmb->addItem(file_name);
    // addItem on an empty combo selects the new item; keep showing the active file.
    ui->fileNamecmb->setCurrentIndex(ui->fileNamecmb->findText(current_file_));
}

void MainWindow::SyncFileCombo(const QString &file_name)
{
    const QSignalBlocker blocker(ui->fileNamecmb);
    ui->fileNamecmb->setCurrentIndex(ui->fileNamecmb->findText(file_name));
}

int MainWindow::FindTabByName(const QString &file_name) const
{
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == file_name)
            return i;
    }
    return -1;
}

void MainWindow::ActivateFile(const QString &file_name)
{
    if (file_name.isEmpty())
        return;

    const int tab = FindTabByName(file_name);
    if (tab == -1) {
        if (!OpenInNewTab(file_name))
            SyncFileCombo(current_file_);
        return;
    }

    ui->tabWidget->setCurrentIndex(tab); // -> on_tabWidget_currentChanged -> ShowFile
}

bool MainWindow::OpenInNewTab(const QString &file_name)
{
    const QString path = file_local_system->GetFilePath(file_name);
    if (path.isEmpty()) {
        log_panel->logError(tr("Unknown file: %1").arg(file_name));
        return false;
    }
    if (!yaml_reader->ReadFile(path)) {
        log_panel->logError(tr("Failed to open %1").arg(file_name));
        return false;
    }

    doc_store.set(file_name, yaml_reader->GetRootNode());
    StackFor(file_name)->clear();
    file_watch->watchFile(path);

    QWidget *tab = new QWidget();
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);

    const int index = ui->tabWidget->addTab(tab, file_name);
    ui->tabWidget->setCurrentIndex(index); // the first tab is already current after addTab
    if (current_file_ != file_name)
        ShowFile(file_name);

    log_panel->logInfo(tr("Opened %1").arg(file_name));
    return true;
}

void MainWindow::ShowFile(const QString &file_name)
{
    if (!doc_store.contains(file_name))
        return;

    if (!current_file_.isEmpty())
        check_box_states_nodes[current_file_] = check_box_states;

    current_file_ = file_name;

    applying_snapshot_ = true;
    tree_model->SetRoot(doc_store.get(file_name));
    applying_snapshot_ = false;
    last_committed_tree_ = doc_store.get(file_name);

    check_box_states = check_box_states_nodes.value(file_name);
    Displaykeys(last_committed_tree_);

    ShowTreeInTab(file_name);
    undo_group_->setActiveStack(StackFor(file_name));
    SyncFileCombo(file_name);
    setWindowFilePath(file_local_system->GetFilePath(file_name));
}

void MainWindow::ShowNoFile()
{
    current_file_.clear();
    check_box_states.clear();
    DetachTreeView();
    applying_snapshot_ = true;
    tree_model->SetRoot(YamlNode());
    applying_snapshot_ = false;
    last_committed_tree_ = YamlNode();
    ClearKeysArea();
    undo_group_->setActiveStack(nullptr);
    SyncFileCombo(QString());
    setWindowFilePath(QString());
}

void MainWindow::ShowTreeInTab(const QString &file_name)
{
    const int index = FindTabByName(file_name);
    if (index == -1)
        return;

    QWidget *tab = ui->tabWidget->widget(index);
    if (!tab)
        return;

    if (tree_view->parentWidget() != tab) {
        auto *layout = qobject_cast<QVBoxLayout *>(tab->layout());
        if (!layout)
            layout = new QVBoxLayout(tab);
        layout->addWidget(tree_view);
    }
    tree_view->show();
}

void MainWindow::DetachTreeView()
{
    // The shared tree view lives inside whichever tab is active. Take it out before a
    // tab is destroyed, otherwise the view would be deleted together with the tab.
    if (tree_view->parentWidget() != this) {
        tree_view->hide();
        tree_view->setParent(this);
        tree_view->hide();
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index < 0 || index >= ui->tabWidget->count()) {
        ShowNoFile();
        return;
    }
    const QString file_name = ui->tabWidget->tabText(index);
    if (file_name == current_file_) {
        ShowTreeInTab(file_name); // only the index shifted (a tab before it was closed)
        return;
    }
    ShowFile(file_name);
}

void MainWindow::on_fileNamecmb_currentIndexChanged(int index)
{
    if (index < 0)
        return;
    ActivateFile(ui->fileNamecmb->itemText(index));
}

void MainWindow::CloseTab(int index)
{
    if (index < 0 || index >= ui->tabWidget->count())
        return;

    const QString file_name = ui->tabWidget->tabText(index);

    if (IsFileDirty(file_name)) {
        const auto reply = QMessageBox::question(
            this, tr("Save File"),
            tr("\"%1\" has unsaved changes. Save them before closing?").arg(file_name),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (reply == QMessageBox::Cancel)
            return;
        if (reply == QMessageBox::Save && !WriteFile(file_name))
            return;
    }

    file_watch->unwatchFile(file_local_system->GetFilePath(file_name));
    check_box_states_nodes.remove(file_name);
    doc_store.remove(file_name);
    if (QUndoStack *stack = undo_stacks_.take(file_name)) {
        undo_group_->removeStack(stack);
        delete stack;
    }

    QWidget *tab = ui->tabWidget->widget(index);
    if (tree_view->parentWidget() == tab)
        DetachTreeView();
    if (file_name == current_file_)
        current_file_.clear();

    ui->tabWidget->removeTab(index); // switches to a neighbour tab, or to "no file"
    delete tab;
}

// ---------------------------------------------------------------------------
// Opening files
// ---------------------------------------------------------------------------

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mime_data = event->mimeData();
    if (!mime_data->hasUrls())
        return;
    for (const QUrl &url : mime_data->urls())
        OpenFileByPath(url.toLocalFile());
}

void MainWindow::OpenFileByPath(const QString &local_path)
{
    const QFileInfo info(local_path);
    if (!info.exists() || !info.isFile())
        return;

    if (!IsYamlFile(local_path)) {
        log_panel->logWarning(tr("Not a YAML file: %1").arg(info.fileName()));
        return;
    }

    const QString file_name = info.fileName();
    if (file_local_system->Contains(file_name)) {
        const QString known = QFileInfo(file_local_system->GetFilePath(file_name)).absoluteFilePath();
        if (known.compare(info.absoluteFilePath(), Qt::CaseInsensitive) != 0) {
            QMessageBox::information(
                this, tr("Open File"),
                tr("Another file named \"%1\" is already in the list:\n%2").arg(file_name, known));
            return;
        }
    } else {
        file_local_system->AddFile(info.absoluteFilePath());
    }

    AddToFileCombo(file_name);
    ActivateFile(file_name);
}

void MainWindow::on_OpenFolderYmlFilebtn_clicked()
{
    const QString dir_path = QFileDialog::getExistingDirectory(
        this, tr("Select a folder with YAML files"), QDir::currentPath());

    if (dir_path.isEmpty())
        return;

    QDir dir(dir_path);
    const QStringList files = dir.entryList(QStringList{"*.yaml", "*.yml"}, QDir::Files);

    if (files.isEmpty()) {
        QMessageBox::information(this, tr("Open Folder"), tr("No YAML files in the folder"));
        return;
    }

    for (const QString &name : files)
        OpenFileByPath(dir.absoluteFilePath(name));
}

void MainWindow::on_OpenFilebtn_clicked()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Select YAML file(s)"), YmlFolder(),
        tr("YAML files (*.yml *.yaml);;All files (*)"));

    for (const QString &path : paths)
        OpenFileByPath(path);
}

void MainWindow::OnFolderChanged(const QString &path)
{
    const QDir dir(path);
    const QString folder = dir.absolutePath();
    const QStringList files = dir.entryList(QStringList{"*.yaml", "*.yml"}, QDir::Files);

    for (const QString &file : files) {
        if (!file_local_system->Contains(file))
            file_local_system->AddFile(dir.absoluteFilePath(file));
        AddToFileCombo(file);
    }

    // Drop entries that lived in this folder and are gone. Files opened from other
    // folders are not affected, and an open tab is kept so its content can be saved.
    for (int i = ui->fileNamecmb->count() - 1; i >= 0; --i) {
        const QString name = ui->fileNamecmb->itemText(i);
        const QFileInfo known(file_local_system->GetFilePath(name));
        if (known.absolutePath().compare(folder, Qt::CaseInsensitive) != 0 || known.exists())
            continue;
        if (FindTabByName(name) != -1)
            continue;

        const QSignalBlocker blocker(ui->fileNamecmb);
        ui->fileNamecmb->removeItem(i);
        file_local_system->RemoveFile(name);
    }
    SyncFileCombo(current_file_);
}

void MainWindow::OnFileModified(const QString &file_name, const QString &abs_path)
{
    if (!doc_store.contains(file_name))
        return; // not open, nothing to reload

    const QString known = QFileInfo(file_local_system->GetFilePath(file_name)).absoluteFilePath();
    if (known.compare(abs_path, Qt::CaseInsensitive) != 0)
        return;

    log_panel->logWarning(tr("Changed on disk: %1").arg(file_name));

    if (reload_prompt_active_)
        return;

    QString question = tr("The file \"%1\" was changed on disk.").arg(file_name);
    if (IsFileDirty(file_name))
        question += "\n\n" + tr("You have unsaved changes. Reload and discard them?");
    else
        question += "\n\n" + tr("Reload it?");

    reload_prompt_active_ = true;
    const auto reply = QMessageBox::question(this, tr("File changed"), question,
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    reload_prompt_active_ = false;

    if (reply == QMessageBox::Yes)
        ReloadFileFromDisk(file_name);
}

void MainWindow::ReloadFileFromDisk(const QString &file_name)
{
    const QString path = file_local_system->GetFilePath(file_name);
    if (path.isEmpty() || !QFileInfo::exists(path))
        return;

    if (!yaml_reader->ReadFile(path)) {
        log_panel->logError(tr("Failed to reload %1").arg(file_name));
        return;
    }

    doc_store.set(file_name, yaml_reader->GetRootNode());
    StackFor(file_name)->clear();

    if (file_name == current_file_) {
        SaveExpandedState();
        ShowFile(file_name);
        RestoreExpandedState();
    }

    log_panel->logInfo(tr("Reloaded %1 from disk").arg(file_name));
}

// ---------------------------------------------------------------------------
// Saving
// ---------------------------------------------------------------------------

bool MainWindow::WriteFile(const QString &file_name)
{
    const QString full_path = file_local_system->GetFilePath(file_name);
    if (full_path.isEmpty() || !doc_store.contains(file_name)) {
        log_panel->logError(tr("Cannot save %1: unknown file").arg(file_name));
        return false;
    }

    if (!yaml_reader->SaveValues(doc_store.get(file_name), full_path))
        return false;

    file_watch->markSelfSaved(full_path);
    StackFor(file_name)->setClean();
    log_panel->logInfo(tr("Saved %1").arg(file_name));

    yandex_api->UploadFile(full_path, [this, file_name](bool ok) {
        if (ok)
            log_panel->logInfo(tr("Uploaded %1 to Yandex Disk").arg(file_name));
    });
    return true;
}

void MainWindow::SaveData(const QString &file_name)
{
    if (file_name.isEmpty() || !IsFileDirty(file_name))
        return;
    WriteFile(file_name);
}

bool MainWindow::IsFileDirty(const QString &file_name) const
{
    const QUndoStack *stack = undo_stacks_.value(file_name);
    return stack && !stack->isClean();
}

QStringList MainWindow::UnsavedFiles() const
{
    QStringList dirty;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        const QString file_name = ui->tabWidget->tabText(i);
        if (IsFileDirty(file_name))
            dirty << file_name;
    }
    return dirty;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    const QStringList dirty = UnsavedFiles();
    if (dirty.isEmpty()) {
        event->accept();
        return;
    }

    const QString list = dirty.join("\n  • ");
    const QString text =
        tr("The following files have unsaved changes:") + "\n\n  • " + list;

    QMessageBox box(this);
    box.setWindowTitle(tr("Unsaved changes"));
    box.setText(text);
    box.setInformativeText(tr("Do you want to save them before exiting?"));
    box.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::SaveAll);
    box.setIcon(QMessageBox::Warning);

    const int choice = box.exec();

    if (choice == QMessageBox::Cancel) {
        event->ignore();
        return;
    }

    if (choice == QMessageBox::SaveAll) {
        for (const QString &file_name : dirty) {
            if (!WriteFile(file_name)) {
                event->ignore(); // keep the window open so the data is not lost
                return;
            }
        }
    }

    event->accept();
}

// ---------------------------------------------------------------------------
// Shortcuts and buttons
// ---------------------------------------------------------------------------

void MainWindow::SlotShortcutCtrlF()
{
    if (replace_wnd)
        replace_wnd->close();

    if (search_wnd) {
        search_wnd->activateWindow();
        return;
    }

    search_wnd = new SearchingWindow(this);
    search_wnd->setWindowFlag(Qt::Window);
    search_wnd->setAttribute(Qt::WA_DeleteOnClose);
    connect(search_wnd, &QObject::destroyed, this, [this]() { search_wnd = nullptr; });
    connect(search_wnd, &SearchingWindow::searchingText, search_controller, &SearchController::search);
    search_wnd->show();
}

void MainWindow::SlotShortcutF11()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}

void MainWindow::SlotShortcutCtrlS()
{
    SaveData(current_file_);
}

void MainWindow::SlotShortcutCtrlR()
{
    if (search_wnd)
        search_wnd->close();

    if (replace_wnd) {
        replace_wnd->activateWindow();
        return;
    }

    replace_wnd = new ReplaceWindow(this);
    replace_wnd->setWindowFlag(Qt::Window);
    replace_wnd->setAttribute(Qt::WA_DeleteOnClose);
    connect(replace_wnd, &QObject::destroyed, this, [this]() { replace_wnd = nullptr; });
    connect(replace_wnd, &ReplaceWindow::searchReplaceText, search_controller,
            &SearchController::searchForReplace);
    connect(replace_wnd, &ReplaceWindow::replaceText, search_controller,
            &SearchController::replace);
    replace_wnd->show();
}

void MainWindow::on_pushButton_clicked()
{
    SaveData(current_file_);
}

void MainWindow::on_pushButton_2_clicked()
{
    Displaykeys(tree_model->ToYamlNode());
    tree_view->expandAll();
}

// ---------------------------------------------------------------------------
// Tree context menu
// ---------------------------------------------------------------------------

void MainWindow::ShowTreeContextMenu(const QPoint &pos)
{
    if (current_file_.isEmpty())
        return;

    const QModelIndex index = tree_view->indexAt(pos);
    const QModelIndex node = index.isValid() ? index.siblingAtColumn(0) : QModelIndex();

    QMenu menu(this);
    QAction *add_child = menu.addAction(tr("Add child"));
    add_child->setEnabled(!node.isValid() || tree_model->CanHaveChildren(node));
    QAction *remove = node.isValid() ? menu.addAction(tr("Delete")) : nullptr;

    QAction *chosen = menu.exec(tree_view->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    const YamlNode before = last_committed_tree_;
    QModelIndex to_edit;

    if (chosen == add_child) {
        const QModelIndex added = tree_model->AddChild(node, tr("new_key"));
        if (!added.isValid())
            return;
        if (node.isValid())
            tree_view->expand(node);
        const bool list_item = added.data(YamlTreeModel::IsSequenceItemRole).toBool();
        to_edit = added.siblingAtColumn(list_item ? YamlTreeModel::ValueColumn
                                                  : YamlTreeModel::KeyColumn);
    } else if (remove && chosen == remove) {
        tree_model->RemoveNode(node);
    } else {
        return;
    }

    CommitEdit(tr("Structure change"), before);

    if (to_edit.isValid()) {
        tree_view->setCurrentIndex(to_edit);
        tree_view->edit(to_edit);
    }
}

// ---------------------------------------------------------------------------
// Keys panel
// ---------------------------------------------------------------------------

void MainWindow::CollectKeys(const YamlNode &node, QSet<QString> &keys) const
{
    QString key = node.key;
    if (key.isEmpty())
        key = "-";

    if (node.children.isEmpty())
        keys.insert(key);

    for (const YamlNode &sub_node : node.children)
        CollectKeys(sub_node, keys);
}

void MainWindow::Displaykeys(const YamlNode &root)
{
    ClearKeysArea();
    keys.clear();

    for (const YamlNode &node : root.children)
        CollectKeys(node, keys);

    QStringList sorted_keys = keys.values();
    std::sort(sorted_keys.begin(), sorted_keys.end());

    for (const QString &key : std::as_const(sorted_keys))
        CreateCheckBox(key);
}

void MainWindow::CreateCheckBox(const QString &name)
{
    QCheckBox *check_box = new QCheckBox(name, this);
    check_box->setCursor(Qt::PointingHandCursor);
    check_box->setStyleSheet(R"(
        QCheckBox {
            font-size: 14px;
            color: #cfeefa;
            padding: 6px 12px 6px 10px;
            spacing: 7px;
            background-color: #33414a;
            border: 1px solid #51b4d2;
            border-radius: 8px;
        }
        QCheckBox:hover { background-color: #3a4b55; }
        QCheckBox::indicator {
            width: 14px; height: 14px;
            border-radius: 4px;
            background-color: #51b4d2;
            border: 1px solid #51b4d2;
        }
        QCheckBox:!checked {
            color: #7c7c75;
            background-color: #363636;
            border: 1px solid #474747;
        }
        QCheckBox:!checked:hover { background-color: #3d3d3d; color: #9a9a93; }
        QCheckBox::indicator:unchecked {
            background-color: transparent;
            border: 1px solid #5a5a5a;
        }
    )");

    if (flow_keys_layout)
        flow_keys_layout->addWidget(check_box);

    if (check_box_states.contains(name)) {
        check_box->setChecked(check_box_states[name]);
    } else {
        check_box->setChecked(true);
        check_box_states[name] = true;
    }

    connect(check_box, &QCheckBox::checkStateChanged, this,
            [this, check_box](Qt::CheckState state) {
                check_box_states[check_box->text()] = (state == Qt::Checked);
            });
}

void MainWindow::ClearKeysArea()
{
    if (!flow_keys_layout)
        return;

    QLayoutItem *item;
    while ((item = flow_keys_layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}
