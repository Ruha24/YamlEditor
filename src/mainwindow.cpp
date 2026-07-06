#include "mainwindow.h"

#include <memory>
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QCloseEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QMimeData>
#include <QTimer>
#include <QUndoStack>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

#include <algorithm>

#include "api/yandex/yandexapi.h"
#include "customWidget/customlineedit.h"
#include "customWidget/flowlayout.h"
#include "files/filesystem.h"
#include "customWidget/logpanel.h"
#include "files/snapshotcommand.h"
#include "files/yaml/yamlitemdelegate.h"
#include "files/yaml/yamlreader.h"
#include "files/yaml/yamltreemodel.h"
#include "files/yaml/yamlvalidator.h"
#include "controller/searchcontroller.h"
#include "controller/filewatchservice.h"
#include "customWidget/connectdialog.h"
#include "replacewindow.h"
#include "searchingwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    tree_model = new YamlTreeModel(this);
    tree_view  = new QTreeView(this);
    tree_view->setModel(tree_model);
    tree_view->setItemDelegate(new YamlItemDelegate(this));
    tree_view->setEditTriggers(QAbstractItemView::DoubleClicked
                               | QAbstractItemView::SelectedClicked);

    tree_view->setColumnWidth(0, 280);
    tree_view->setAlternatingRowColors(true);
    tree_view->setRootIsDecorated(true);
    tree_view->setIndentation(20);
    tree_view->setUniformRowHeights(true);
    tree_view->setStyleSheet(R"(
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
    )");

    tree_view->hide();

    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree_view, &QWidget::customContextMenuRequested,
            this, &MainWindow::ShowTreeContextMenu);

    connect(tree_model, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &, const QModelIndex &, const QList<int> &) {
                if (applying_snapshot_)
                    return;
                PushUndoCommand(tr("Edit"), last_committed_tree_);
                last_committed_tree_ = tree_model->ToYamlNode();
                doc_store.set(ui->fileNamecmb->currentText(), last_committed_tree_);
                Displaykeys(last_committed_tree_);
            });


    InitLogPanel();
    InitUndoStack();
    InitKeysLayout();

    search_controller = new SearchController(this);
    search_controller->setTargets(tree_model, tree_view);
    connect(search_controller, &SearchController::info, this,
            [this](const QString &msg) { QMessageBox::information(this, tr("Search"), msg); });
    connect(search_controller, &SearchController::warning, this,
            [this](const QString &msg) { QMessageBox::warning(this, tr("Search"), msg); });

    file_local_system = std::make_unique<FileSystem>(QDir::currentPath() + "/ymlFiles");
    yandex_api = std::make_unique<YandexApi>();
    yaml_reader = std::make_unique<YamlReader>();

    ConnectServices();
    ConnectShortCut();
    InitLanguageCmb();
    InitAccountMenu();

    yandex_api->GetFiles();
}

MainWindow::~MainWindow()
{
    delete ui;
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

    if (ui->scrollArea_2) {
        ui->scrollArea_2->setStyleSheet(R"(
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
        )");
    }
}

void MainWindow::InitLogPanel()
{
    log_panel = new LogPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, log_panel);
    log_panel->logInfo(tr("Application started"));
}

void MainWindow::InitUndoStack()
{
    undo_stack = new QUndoStack(this);
}

void MainWindow::PushUndoCommand(const QString &text, const YamlNode &before)
{
    const YamlNode after = tree_model->ToYamlNode();
    undo_stack->push(new YamlSnapshotCommand(text, before, after, [this](const YamlNode &snapshot) {
        applying_snapshot_ = true;
        SaveExpandedState();
        tree_model->SetRoot(snapshot);
        RestoreExpandedState();
        last_committed_tree_ = snapshot;
        doc_store.set(ui->fileNamecmb->currentText(), snapshot);
        Displaykeys(snapshot);
        applying_snapshot_ = false;
    }));
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

void MainWindow::SlotUndo()
{
    if (undo_stack && undo_stack->canUndo()) {
        log_panel->logInfo(tr("Undo: %1").arg(undo_stack->undoText()));
        undo_stack->undo();
    }
}

void MainWindow::SlotRedo()
{
    if (undo_stack && undo_stack->canRedo()) {
        log_panel->logInfo(tr("Redo: %1").arg(undo_stack->redoText()));
        undo_stack->redo();
    }
}

void MainWindow::ConnectServices()
{
    connect(yaml_reader.get(), &YamlReader::FileUploaded, this, [this](bool success) {
        if (success)
            log_panel->logInfo(tr("File saved"));
    });
    connect(yaml_reader.get(), &YamlReader::ErrorOccurred, this, [this](const QString &msg) {
        log_panel->logError(msg);
        QMessageBox::warning(this, tr("Error"), msg);
    });
    connect(yandex_api.get(), &YandexApi::ErrorOccurred, this, [this](const QString &msg) {
        log_panel->logWarning(tr("Network: %1").arg(msg));
    });
    connect(yandex_api.get(), &YandexApi::NewFile, this, [this](const QString &fileName) {
        log_panel->logInfo(tr("Downloaded from cloud: %1").arg(fileName));
    });
    connect(yandex_api.get(), &YandexApi::NewFile, this, &MainWindow::UploadFileOnCmb);

    file_watch = new FileWatchService(this);
    file_watch->watchFolder(QDir::currentPath() + "/ymlFiles");
    connect(file_watch, &FileWatchService::folderChanged, this, &MainWindow::OnFolderChanged);
    connect(file_watch, &FileWatchService::fileModified, this, &MainWindow::OnFileModified);
    connect(file_watch, &FileWatchService::fileRemoved, this,
            [this](const QString &fileName, const QString &) {
                log_panel->logWarning(tr("File removed on disk: %1").arg(fileName));
            });

    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::CloseTab);
    ui->fileNamecmb->setView(new QListView(ui->fileNamecmb));
}

void MainWindow::ConnectShortCut()
{
    key_F11 = new QShortcut(this);
    key_F11->setKey(Qt::Key_F11);
    connect(key_F11, &QShortcut::activated, this, &MainWindow::SlotShortcutF11);

    key_ctrl_f = new QShortcut(this);
    key_ctrl_f->setKey(Qt::CTRL | Qt::Key_F);
    connect(key_ctrl_f, &QShortcut::activated, this, &MainWindow::SlotShortcutCtrlF);

    key_ctrl_s = new QShortcut(this);
    key_ctrl_s->setKey(Qt::CTRL | Qt::Key_S);
    connect(key_ctrl_s, &QShortcut::activated, this, &MainWindow::SlotShortcutCtrlS);

    key_ctrl_r = new QShortcut(this);
    key_ctrl_r->setKey(Qt::CTRL | Qt::Key_R);
    connect(key_ctrl_r, &QShortcut::activated, this, &MainWindow::SlotShortcutCtrlR);

    auto *key_undo = new QShortcut(QKeySequence::Undo, this);
    connect(key_undo, &QShortcut::activated, this, &MainWindow::SlotUndo);

    auto *key_redo = new QShortcut(QKeySequence::Redo, this);
    connect(key_redo, &QShortcut::activated, this, &MainWindow::SlotRedo);
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

    QMenu *account_menu = menuBar()->addMenu(tr("Account"));
    QAction *connect_action = account_menu->addAction(tr("Connect Yandex Disk..."));
    connect(connect_action, &QAction::triggered, this, &MainWindow::openConnectDialog);
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

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
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

void MainWindow::on_fileNamecmb_currentIndexChanged(int index)
{
    ui->fileNamecmb->setCurrentIndex(index);

    if (undo_stack && !undo_stack->isClean() && tree_model->rowCount() > 0) {
        const auto reply = QMessageBox::question(this,
                                                 tr("Save File"),
                                                 tr("Do you want to save the file?"),
                                                 QMessageBox::Yes | QMessageBox::No,
                                                 QMessageBox::No);
        if (reply == QMessageBox::Yes)
            SaveData(previous_text_cmb);
    }

    previous_text_cmb = ui->fileNamecmb->currentText();
    ReadFile();
}

void MainWindow::SaveData(const QString &fileName)
{
    if (!undo_stack || undo_stack->isClean())
        return;

    if (HasValidationErrors()) {
        log_panel->logError(tr("Cannot save: fix the highlighted fields first"));
        QMessageBox::warning(this, tr("Invalid YAML"),
                             tr("Some fields contain invalid values (highlighted in red).\n"
                                "Please fix them before saving."));
        return;
    }

    const QString full_path = file_local_system->GetFilePath(fileName);
    const QString abs_path = QFileInfo(full_path).absoluteFilePath();

    file_watch->markSelfSaved(abs_path);

    yaml_reader->SaveValues(tree_model->ToYamlNode(), full_path);
    yandex_api->UploadFile(full_path, [](bool) {});
    undo_stack->setClean();

    file_watch->watchFile(abs_path);
}

void MainWindow::UploadFileOnCmb(const QString &file)
{
    if (ui->fileNamecmb->findText(file) != -1)
        return;

    const QString file_path = QDir::currentPath() + "/ymlFiles/" + file;
    if (!file_local_system->Contains(file))
        file_local_system->AddFile(file_path);
    ui->fileNamecmb->addItem(file);
}

bool MainWindow::CheckOpenTab(const QString &file)
{
    const int existing_tab_index = FindTabByName(file);
    if (existing_tab_index == -1)
        return false;

    if (doc_store.contains(file))
        tree_model->SetRoot(doc_store.get(file));
    last_committed_tree_ = tree_model->ToYamlNode();
    Displaykeys(tree_model->ToYamlNode());
    ShowTreeInTab(file);
    ui->tabWidget->setCurrentIndex(existing_tab_index);
    return true;
}

void MainWindow::CloseTab(int index)
{
    if (index < 0 || index >= ui->tabWidget->count())
        return;

    const QString file_name = ui->tabWidget->tabText(index);
    SaveData(file_name);

    file_watch->unwatchFile(file_local_system->GetFilePath(file_name));

    check_box_states_nodes.remove(file_name);
    doc_store.remove(file_name);
    check_box_states.clear();

    QWidget *tab = ui->tabWidget->widget(index);
    ui->tabWidget->removeTab(index);
    delete tab;
}

int MainWindow::FindTabByName(const QString &fileName) const
{
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == fileName)
            return i;
    }
    return -1;
}

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

void MainWindow::ReadFile()
{
    const QString file_name = ui->fileNamecmb->currentText();
    const QString file_path = file_local_system->GetFilePath(file_name);

    if (!CheckOpenTab(file_name)) {
        if (yaml_reader->ReadFile(file_path)) {
            tree_model->SetRoot(yaml_reader->GetRootNode());
            DisplayYamlData();
            Displaykeys(tree_model->ToYamlNode());
        }
    }

    doc_store.set(file_name, tree_model->ToYamlNode());
    check_box_states_nodes.insert(file_name, check_box_states);

    last_committed_tree_ = tree_model->ToYamlNode();

    if (undo_stack) {
        undo_stack->clear();
        undo_stack->setClean();
    }
    value_edit_active_ = false;
    if (search_controller)
        search_controller->reset();
}

void MainWindow::SlotShortcutCtrlF()
{
    if (replace_wnd) {
        replace_wnd->close();
        replace_wnd = nullptr;
    }

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
    SaveData(ui->fileNamecmb->currentText());
}

void MainWindow::SlotShortcutCtrlR()
{
    if (search_wnd) {
        search_wnd->close();
        search_wnd = nullptr;
    }

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

void MainWindow::DisplayYamlData()
{
    const QString file_name = ui->fileNamecmb->currentText();

    QWidget *tab = new QWidget();
    new QVBoxLayout(tab);
    ui->tabWidget->addTab(tab, file_name);
    ui->tabWidget->setCurrentWidget(tab);

    ShowTreeInTab(file_name);
}

void MainWindow::ShowTreeInTab(const QString &fileName)
{
    const int index = FindTabByName(fileName);
    if (index == -1)
        return;

    QWidget *tab = ui->tabWidget->widget(index);
    if (!tab)
        return;

    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(tab->layout());
    if (!layout)
        layout = new QVBoxLayout(tab);

    if (tree_view->parentWidget() != tab) {
        layout->addWidget(tree_view);
        tree_view->show();
    }
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

void MainWindow::on_pushButton_clicked()
{
    SaveData(ui->fileNamecmb->currentText());
}

void MainWindow::on_pushButton_2_clicked()
{
    Displaykeys(tree_model->ToYamlNode());
    tree_view->expandAll();
}

void MainWindow::onCheckBoxStateChanged(int state)
{
    if (QCheckBox *check_box = qobject_cast<QCheckBox *>(sender()))
        check_box_states[check_box->text()] = (state == Qt::Checked);
}

void MainWindow::HandleAddKeyValue(const QString &path, const QString &newValue, bool isKey)
{
    Q_UNUSED(path);
    Q_UNUSED(newValue);
    Q_UNUSED(isKey);
}

void MainWindow::HandleDeleteElement(const QString &path, bool isKey)
{
    Q_UNUSED(path);
    Q_UNUSED(isKey);
}

void MainWindow::ShowTreeContextMenu(const QPoint &pos)
{
    const QModelIndex index = tree_view->indexAt(pos);

    QMenu menu(this);
    QAction *add_child = menu.addAction(tr("Add child"));
    QAction *remove = index.isValid() ? menu.addAction(tr("Delete")) : nullptr;

    QAction *chosen = menu.exec(tree_view->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    const YamlNode before = tree_model->ToYamlNode();

    if (chosen == add_child) {
        const QModelIndex parent = index.isValid() ? index.siblingAtColumn(0) : QModelIndex();
        tree_model->AddChild(parent, tr("new_key"), QString());
        if (parent.isValid())
            tree_view->expand(parent);
    } else if (remove && chosen == remove) {
        tree_model->RemoveNode(index.siblingAtColumn(0));
    } else {
        return;
    }

    PushUndoCommand(tr("Structure change"), before);
    last_committed_tree_ = tree_model->ToYamlNode();
    doc_store.set(ui->fileNamecmb->currentText(), last_committed_tree_);
    Displaykeys(last_committed_tree_);
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



void MainWindow::RemoveTab(const QString &tabText)
{
    const int index = FindTabByName(tabText);
    if (index != -1)
        ui->tabWidget->removeTab(index);
    else
        qWarning() << "Tab with text" << tabText << "not found!";
}

void MainWindow::OnFileModified(const QString &fileName, const QString &absPath)
{
    Q_UNUSED(absPath);

    const bool has_unsaved =
        (fileName == ui->fileNamecmb->currentText()) && undo_stack && !undo_stack->isClean();

    QString question = tr("The file \"%1\" was changed on disk.").arg(fileName);
    if (has_unsaved)
        question += "\n\n" + tr("You have unsaved changes. Reload and discard them?");
    else
        question += "\n\n" + tr("Reload it?");

    log_panel->logWarning(tr("Changed on disk: %1").arg(fileName));

    if (reload_prompt_active_)
        return;
    reload_prompt_active_ = true;

    const auto reply = QMessageBox::question(this, tr("File changed"), question,
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    reload_prompt_active_ = false;

    if (reply == QMessageBox::Yes)
        ReloadFileFromDisk(fileName);
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

    const YamlNode reloaded = yaml_reader->GetRootNode();
    doc_store.set(file_name, reloaded);

    if (file_name == ui->fileNamecmb->currentText()) {
        tree_model->SetRoot(reloaded);
        last_committed_tree_ = reloaded;
        Displaykeys(reloaded);
        if (undo_stack) {
            undo_stack->clear();
            undo_stack->setClean();
        }
    }

    log_panel->logInfo(tr("Reloaded %1 from disk").arg(file_name));
}

bool MainWindow::IsFileDirty(const QString &file_name) const
{
    if (!doc_store.contains(file_name))
        return false;

    const QString path = file_local_system->GetFilePath(file_name);

    const YamlNode current = (file_name == ui->fileNamecmb->currentText())
                                 ? tree_model->ToYamlNode()
                                 : doc_store.get(file_name);
    return DocumentStore::differsFromDisk(current, path);
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

    const QString list = dirty.join("\n  \u2022 ");
    const QString text =
        tr("The following files have unsaved changes:") + "\n\n  \u2022 " + list;

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

        const QString active = ui->fileNamecmb->currentText();
        if (HasValidationErrors()) {
            const auto proceed = QMessageBox::warning(
                this, tr("Invalid YAML"),
                tr("The current file has invalid fields and cannot be saved.\n"
                   "Exit without saving it?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (proceed != QMessageBox::Yes) {
                event->ignore();
                return;
            }
        }

        for (const QString &file_name : dirty) {

            if (file_name == active && HasValidationErrors()) {
                log_panel->logWarning(tr("Skipped %1 (invalid)").arg(file_name));
                continue;
            }
            const YamlNode tree =
                (file_name == active) ? tree_model->ToYamlNode() : doc_store.get(file_name);
            const QString full_path = file_local_system->GetFilePath(file_name);
            const QString abs_path = QFileInfo(full_path).absoluteFilePath();
            file_watch->markSelfSaved(abs_path);
            yaml_reader->SaveValues(tree, full_path);
            yandex_api->UploadFile(full_path, [](bool) {});
            log_panel->logInfo(tr("Saved %1 on exit").arg(file_name));
        }
    }

    event->accept();
}

void MainWindow::OnFolderChanged(const QString &path)
{
    QDir dir(path);
    const QStringList files =
        dir.entryList(QStringList{"*.yaml", "*.yml"}, QDir::Files);

    for (const QString &file : files) {
        if (ui->fileNamecmb->findText(file) == -1)
            ui->fileNamecmb->addItem(file);
    }

    for (int i = 0; i < ui->fileNamecmb->count();) {
        const QString combo_file = ui->fileNamecmb->itemText(i);
        if (!files.contains(combo_file)) {
            ui->fileNamecmb->removeItem(i);
            RemoveTab(combo_file);
        } else {
            ++i;
        }
    }
}

void MainWindow::OpenFileByPath(const QString &local_path)
{
    const QFileInfo info(local_path);
    if (!info.exists() || !info.isFile())
        return;

    const QString file_name = info.fileName();
    if (ui->fileNamecmb->findText(file_name) != -1) {
        QMessageBox::information(this, tr("Error"), tr("This file is already open"));
        return;
    }

    file_local_system->AddFile(local_path);
    ui->fileNamecmb->addItem(file_name);
    ui->fileNamecmb->setCurrentIndex(ui->fileNamecmb->findText(file_name));
    ReadFile();

    file_watch->watchFile(info.absoluteFilePath());
    log_panel->logInfo(tr("Opened %1").arg(file_name));
}

void MainWindow::on_OpenFolderYmlFilebtn_clicked()
{
    const QString dir_path = QFileDialog::getExistingDirectory(
        this, tr("Select a folder with YAML files"), QDir::currentPath(),
        QFileDialog::DontUseNativeDialog);

    if (dir_path.isEmpty())
        return;

    QDir dir(dir_path);
    const QStringList files =
        dir.entryList(QStringList{"*.yaml", "*.yml"}, QDir::Files);

    if (files.isEmpty()) {
        QMessageBox::information(this, tr("Open Folder"), tr("No YAML files in the folder"));
        return;
    }

    for (const QString &name : files)
        OpenFileByPath(dir.absoluteFilePath(name));
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index < 0 || index >= ui->tabWidget->count())
        return;

    const QString previousFileName = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
    check_box_states_nodes[previousFileName] = check_box_states;

    const QString file_name = ui->tabWidget->tabText(index);
    ui->fileNamecmb->setCurrentIndex(ui->fileNamecmb->findText(file_name));

    if (doc_store.contains(file_name)) {
        tree_model->SetRoot(doc_store.get(file_name));
        last_committed_tree_ = tree_model->ToYamlNode();
        check_box_states = check_box_states_nodes.value(file_name);
        Displaykeys(tree_model->ToYamlNode());
        ShowTreeInTab(file_name);

        if (undo_stack) {
            undo_stack->clear();
            undo_stack->setClean();
        }
        value_edit_active_ = false;
        if (search_controller)
            search_controller->reset();
    }
}

void MainWindow::on_OpenFilebtn_clicked()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Select YAML file(s)"), QDir::currentPath() + "/ymlFiles",
        tr("YAML files (*.yml *.yaml);;All files (*)"));

    for (const QString &path : paths)
        OpenFileByPath(path);
}

void MainWindow::switchLanguage(const QString &locale)
{
    qApp->removeTranslator(&translator_);

    const QString path = ":/i18n/editor_" + locale;
    if (translator_.load(path))
        qApp->installTranslator(&translator_);

    ui->retranslateUi(this);
}