#include "mainwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabBar>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QUndoGroup>
#include <QUndoStack>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

#include "api/yandex/yandexapi.h"
#include "controller/filewatchservice.h"
#include "controller/searchcontroller.h"
#include "customWidget/connectdialog.h"
#include "customWidget/filelistpanel.h"
#include "customWidget/findbar.h"
#include "customWidget/keyfilterpanel.h"
#include "customWidget/logpanel.h"
#include "files/filesystem.h"
#include "files/snapshotcommand.h"
#include "files/yaml/yamlitemdelegate.h"
#include "files/yaml/yamlreader.h"
#include "files/yaml/yamltreemodel.h"
#include "theme/theme.h"

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

// Depth-first walk over every row of the model.
void ForEachRow(const QAbstractItemModel *model, const QModelIndex &parent,
                const std::function<void(const QModelIndex &)> &fn)
{
    const int rows = model->rowCount(parent);
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = model->index(r, 0, parent);
        fn(idx);
        ForEachRow(model, idx, fn);
    }
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);
    setDocumentMode(true);
    resize(1280, 800);

    undo_group_ = new QUndoGroup(this);
    tree_model = new YamlTreeModel(this);
    search_controller = new SearchController(this);

    CreateActions();
    CreateMenus();
    CreateToolBar();
    CreateCentralArea();
    CreateDocks();
    CreateStatusBar();

    search_controller->setTargets(tree_model, tree_view);
    connect(search_controller, &SearchController::resultChanged, find_bar_, &FindBar::setResult);
    connect(search_controller, &SearchController::warning, this,
            [this](const QString &msg) { statusBar()->showMessage(msg, 4000); });

    file_local_system = std::make_unique<FileSystem>(YmlFolder());
    yandex_api = std::make_unique<YandexApi>();
    yaml_reader = std::make_unique<YamlReader>();

    ConnectServices();
    PopulateFileList();
    RestoreSettings();
    RetranslateUi();
    RefreshIcons();
    UpdateCenter();
    UpdateActions();
    UpdateStatus();

    connect(&Theme::instance(), &Theme::changed, this, [this]() {
        RefreshIcons();
        UpdateStatus();
        tree_view->viewport()->update();
    });

    log_panel->logInfo(tr("Application started"));
    yandex_api->GetFiles();
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void MainWindow::CreateActions()
{
    auto make = [this](const QKeySequence &shortcut = {}) {
        auto *action = new QAction(this);
        if (!shortcut.isEmpty())
            action->setShortcut(shortcut);
        return action;
    };

    act_open_ = make(QKeySequence::Open);
    act_open_folder_ = make(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    act_save_ = make(QKeySequence::Save);
    act_save_all_ = make(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    act_close_tab_ = make(QKeySequence(Qt::CTRL | Qt::Key_W));
    act_exit_ = make(QKeySequence::Quit);
    act_undo_ = make(QKeySequence::Undo);
    act_redo_ = make(QKeySequence::Redo);
    act_find_ = make(QKeySequence::Find);
    act_replace_ = make();
    act_replace_->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_H), QKeySequence(Qt::CTRL | Qt::Key_R)});
    act_add_child_ = make(QKeySequence(Qt::Key_Insert));
    act_delete_ = make(QKeySequence::Delete);
    act_expand_all_ = make();
    act_collapse_all_ = make();
    act_fullscreen_ = make(QKeySequence(Qt::Key_F11));
    act_fullscreen_->setCheckable(true);
    act_connect_ = make();

    // Insert/Delete must not fire while typing in the find bar or a cell editor.
    act_add_child_->setShortcutContext(Qt::WidgetShortcut);
    act_delete_->setShortcutContext(Qt::WidgetShortcut);

    auto *theme_group = new QActionGroup(this);
    act_theme_system_ = make();
    act_theme_light_ = make();
    act_theme_dark_ = make();
    for (QAction *a : {act_theme_system_, act_theme_light_, act_theme_dark_}) {
        a->setCheckable(true);
        theme_group->addAction(a);
    }

    auto *lang_group = new QActionGroup(this);
    act_lang_en_ = make();
    act_lang_ru_ = make();
    act_lang_en_->setText(QStringLiteral("English"));
    act_lang_ru_->setText(QStringLiteral("Русский"));
    for (QAction *a : {act_lang_en_, act_lang_ru_}) {
        a->setCheckable(true);
        lang_group->addAction(a);
    }

    connect(act_open_, &QAction::triggered, this, &MainWindow::OpenFileDialog);
    connect(act_open_folder_, &QAction::triggered, this, &MainWindow::OpenFolderDialog);
    connect(act_save_, &QAction::triggered, this, &MainWindow::SaveCurrent);
    connect(act_save_all_, &QAction::triggered, this, &MainWindow::SaveAll);
    connect(act_close_tab_, &QAction::triggered, this, [this]() { CloseTab(tab_bar_->currentIndex()); });
    connect(act_exit_, &QAction::triggered, this, &QWidget::close);
    connect(act_undo_, &QAction::triggered, undo_group_, &QUndoGroup::undo);
    connect(act_redo_, &QAction::triggered, undo_group_, &QUndoGroup::redo);
    connect(undo_group_, &QUndoGroup::canUndoChanged, this, &MainWindow::UpdateActions);
    connect(undo_group_, &QUndoGroup::canRedoChanged, this, &MainWindow::UpdateActions);
    connect(undo_group_, &QUndoGroup::undoTextChanged, this, &MainWindow::UpdateActions);
    connect(undo_group_, &QUndoGroup::redoTextChanged, this, &MainWindow::UpdateActions);
    connect(act_find_, &QAction::triggered, this, [this]() { ShowFind(false); });
    connect(act_replace_, &QAction::triggered, this, [this]() { ShowFind(true); });
    connect(act_add_child_, &QAction::triggered, this, [this]() {
        AddChildAt(tree_view->currentIndex());
    });
    connect(act_delete_, &QAction::triggered, this, [this]() { DeleteAt(tree_view->currentIndex()); });
    connect(act_expand_all_, &QAction::triggered, this, [this]() { tree_view->expandAll(); });
    connect(act_collapse_all_, &QAction::triggered, this, [this]() { tree_view->collapseAll(); });
    connect(act_fullscreen_, &QAction::toggled, this, [this](bool on) {
        on ? showFullScreen() : showNormal();
    });
    connect(act_connect_, &QAction::triggered, this, &MainWindow::OpenConnectDialog);

    connect(act_theme_system_, &QAction::triggered, this,
            []() { Theme::instance().setMode(Theme::Mode::System); });
    connect(act_theme_light_, &QAction::triggered, this,
            []() { Theme::instance().setMode(Theme::Mode::Light); });
    connect(act_theme_dark_, &QAction::triggered, this,
            []() { Theme::instance().setMode(Theme::Mode::Dark); });
    connect(act_lang_en_, &QAction::triggered, this, [this]() { SwitchLanguage("en_GB"); });
    connect(act_lang_ru_, &QAction::triggered, this, [this]() { SwitchLanguage("ru_RU"); });
}

void MainWindow::CreateMenus()
{
    menu_file_ = menuBar()->addMenu(QString());
    menu_file_->addAction(act_open_);
    menu_file_->addAction(act_open_folder_);
    menu_file_->addSeparator();
    menu_file_->addAction(act_save_);
    menu_file_->addAction(act_save_all_);
    menu_file_->addSeparator();
    menu_file_->addAction(act_close_tab_);
    menu_file_->addAction(act_exit_);

    menu_edit_ = menuBar()->addMenu(QString());
    menu_edit_->addAction(act_undo_);
    menu_edit_->addAction(act_redo_);
    menu_edit_->addSeparator();
    menu_edit_->addAction(act_find_);
    menu_edit_->addAction(act_replace_);
    menu_edit_->addSeparator();
    menu_edit_->addAction(act_add_child_);
    menu_edit_->addAction(act_delete_);

    menu_view_ = menuBar()->addMenu(QString());
    // Dock toggles are inserted at the top in CreateDocks().
    menu_view_->addAction(act_expand_all_);
    menu_view_->addAction(act_collapse_all_);
    menu_view_->addSeparator();
    menu_theme_ = menu_view_->addMenu(QString());
    menu_theme_->addAction(act_theme_system_);
    menu_theme_->addAction(act_theme_light_);
    menu_theme_->addAction(act_theme_dark_);
    menu_language_ = menu_view_->addMenu(QString());
    menu_language_->addAction(act_lang_en_);
    menu_language_->addAction(act_lang_ru_);
    menu_view_->addSeparator();
    menu_view_->addAction(act_fullscreen_);

    menu_account_ = menuBar()->addMenu(QString());
    menu_account_->addAction(act_connect_);
}

void MainWindow::CreateToolBar()
{
    toolbar_ = addToolBar(QString());
    toolbar_->setObjectName("mainToolBar");
    toolbar_->setMovable(false);
    toolbar_->setFloatable(false);
    toolbar_->setIconSize(QSize(18, 18));
    toolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar_->toggleViewAction()->setVisible(false);

    toolbar_->addAction(act_open_);
    toolbar_->addAction(act_open_folder_);
    toolbar_->addSeparator();
    toolbar_->addAction(act_save_);
    toolbar_->addSeparator();
    toolbar_->addAction(act_undo_);
    toolbar_->addAction(act_redo_);
    toolbar_->addSeparator();
    toolbar_->addAction(act_find_);
    toolbar_->addAction(act_replace_);
    toolbar_->addSeparator();
    toolbar_->addAction(act_add_child_);
    toolbar_->addAction(act_delete_);
}

QWidget *MainWindow::CreateEmptyState()
{
    auto *page = new QWidget(this);
    page->setObjectName("emptyState");
    page->setAttribute(Qt::WA_StyledBackground);

    empty_icon_ = new QLabel(page);
    empty_icon_->setAlignment(Qt::AlignCenter);
    empty_title_ = new QLabel(page);
    empty_title_->setProperty("role", "title");
    empty_title_->setAlignment(Qt::AlignCenter);
    empty_text_ = new QLabel(page);
    empty_text_->setProperty("role", "muted");
    empty_text_->setAlignment(Qt::AlignCenter);
    empty_text_->setWordWrap(true);
    empty_hint_ = new QLabel(page);
    empty_hint_->setProperty("role", "muted");
    empty_hint_->setAlignment(Qt::AlignCenter);

    empty_open_btn_ = new QPushButton(page);
    empty_open_btn_->setProperty("primary", true);
    empty_folder_btn_ = new QPushButton(page);
    for (QPushButton *btn : {empty_open_btn_, empty_folder_btn_}) {
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumWidth(150);
    }
    connect(empty_open_btn_, &QPushButton::clicked, this, &MainWindow::OpenFileDialog);
    connect(empty_folder_btn_, &QPushButton::clicked, this, &MainWindow::OpenFolderDialog);

    auto *buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    buttons->addStretch();
    buttons->addWidget(empty_open_btn_);
    buttons->addWidget(empty_folder_btn_);
    buttons->addStretch();

    auto *column = new QVBoxLayout();
    column->setSpacing(10);
    column->addWidget(empty_icon_);
    column->addSpacing(4);
    column->addWidget(empty_title_);
    column->addWidget(empty_text_);
    column->addSpacing(10);
    column->addLayout(buttons);
    column->addSpacing(6);
    column->addWidget(empty_hint_);

    auto *layout = new QVBoxLayout(page);
    layout->addStretch(2);
    layout->addLayout(column);
    layout->addStretch(3);
    return page;
}

void MainWindow::CreateCentralArea()
{
    tab_bar_ = new QTabBar(this);
    tab_bar_->setDocumentMode(true);
    tab_bar_->setMovable(true);
    tab_bar_->setExpanding(false);
    tab_bar_->setElideMode(Qt::ElideMiddle);
    tab_bar_->setUsesScrollButtons(true);
    tab_bar_->installEventFilter(this); // middle click closes a tab
    connect(tab_bar_, &QTabBar::currentChanged, this, &MainWindow::OnTabChanged);

    find_bar_ = new FindBar(this);
    find_bar_->hide();
    connect(find_bar_, &FindBar::findRequested, search_controller, &SearchController::find);
    connect(find_bar_, &FindBar::replaceRequested, search_controller,
            &SearchController::replaceCurrent);
    connect(find_bar_, &FindBar::replaceAllRequested, this,
            [this](const QString &text, const QString &replacement, bool cs, bool regex) {
                const YamlNode before = last_committed_tree_;
                suppress_commit_ = true;
                const int changed = search_controller->replaceAll(text, replacement, cs, regex);
                suppress_commit_ = false;
                if (changed > 0)
                    CommitEdit(tr("Replace all"), before);
                statusBar()->showMessage(tr("Replaced in %n cell(s)", nullptr, changed), 4000);
            });
    connect(find_bar_, &FindBar::closed, this, [this]() { tree_view->setFocus(); });

    tree_view = new QTreeView(this);
    tree_view->setModel(tree_model);
    tree_view->setFrameShape(QFrame::NoFrame);
    tree_view->setAlternatingRowColors(true);
    tree_view->setUniformRowHeights(true);
    tree_view->setAnimated(true);
    tree_view->setIndentation(18);
    tree_view->setEditTriggers(QAbstractItemView::DoubleClicked
                               | QAbstractItemView::SelectedClicked
                               | QAbstractItemView::EditKeyPressed);
    tree_view->header()->setSectionResizeMode(QHeaderView::Interactive);
    tree_view->header()->setStretchLastSection(true);
    tree_view->setColumnWidth(YamlTreeModel::KeyColumn, 320);
    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view->addAction(act_add_child_);
    tree_view->addAction(act_delete_);
    connect(tree_view, &QWidget::customContextMenuRequested, this, &MainWindow::ShowTreeContextMenu);
    connect(tree_view->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &MainWindow::UpdateActions);

    auto *delegate = new YamlItemDelegate(this);
    tree_view->setItemDelegate(delegate);
    connect(delegate, &YamlItemDelegate::editRejected, this, [this](const QString &reason) {
        log_panel->logWarning(tr("Edit rejected: %1").arg(reason));
        statusBar()->showMessage(reason, 5000);
    });

    // Cell edits (from the delegate or from Replace) become undoable steps.
    connect(tree_model, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &, const QModelIndex &, const QList<int> &) {
                if (applying_snapshot_ || suppress_commit_ || current_file_.isEmpty())
                    return;
                CommitEdit(tr("Edit"), last_committed_tree_);
            });

    auto *doc_page = new QWidget(this);
    auto *doc_layout = new QVBoxLayout(doc_page);
    doc_layout->setContentsMargins(0, 0, 0, 0);
    doc_layout->setSpacing(0);
    doc_layout->addWidget(tab_bar_);
    doc_layout->addWidget(find_bar_);
    doc_layout->addWidget(tree_view, 1);

    empty_state_ = CreateEmptyState();

    center_stack_ = new QStackedWidget(this);
    center_stack_->addWidget(empty_state_);
    center_stack_->addWidget(doc_page);
    setCentralWidget(center_stack_);
}

void MainWindow::CreateDocks()
{
    file_panel_ = new FileListPanel(this);
    connect(file_panel_, &FileListPanel::fileActivated, this, &MainWindow::ActivateFile);
    files_dock_ = new QDockWidget(this);
    files_dock_->setObjectName("filesDock");
    files_dock_->setWidget(file_panel_);
    files_dock_->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, files_dock_);

    filter_panel_ = new KeyFilterPanel(this);
    connect(filter_panel_, &KeyFilterPanel::hiddenKeysChanged, this,
            [this](const QSet<QString> &hidden) {
                if (current_file_.isEmpty())
                    return;
                hidden_keys_[current_file_] = hidden;
                ApplyKeyFilter();
                search_controller->reset();
            });
    filter_dock_ = new QDockWidget(this);
    filter_dock_->setObjectName("filterDock");
    filter_dock_->setWidget(filter_panel_);
    filter_dock_->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::RightDockWidgetArea, filter_dock_);

    log_panel = new LogPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, log_panel);
    log_panel->hide();
    connect(log_panel, &LogPanel::entryAdded, this, [this](LogPanel::Level level, const QString &) {
        if (level == LogPanel::Level::Error && !log_panel->isVisible()) {
            ++unseen_errors_;
            UpdateStatus();
        }
    });
    connect(log_panel, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (visible) {
            unseen_errors_ = 0;
            UpdateStatus();
        }
    });

    resizeDocks({files_dock_, filter_dock_}, {220, 240}, Qt::Horizontal);
    resizeDocks({log_panel}, {180}, Qt::Vertical);

    QAction *first = menu_view_->actions().value(0);
    menu_view_->insertAction(first, files_dock_->toggleViewAction());
    menu_view_->insertAction(first, filter_dock_->toggleViewAction());
    menu_view_->insertAction(first, log_panel->toggleViewAction());
    menu_view_->insertSeparator(first);
    log_panel->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft));
}

void MainWindow::CreateStatusBar()
{
    status_file_ = new QLabel(this);
    status_file_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    status_stats_ = new QLabel(this);
    statusBar()->addWidget(status_file_, 1);
    statusBar()->addWidget(status_stats_);
    statusBar()->setSizeGripEnabled(false);

    status_cloud_ = new QToolButton(this);
    status_cloud_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    status_cloud_->setAutoRaise(true);
    status_cloud_->setIconSize(QSize(14, 14));
    status_cloud_->setCursor(Qt::PointingHandCursor);
    connect(status_cloud_, &QToolButton::clicked, this, &MainWindow::OpenConnectDialog);

    status_log_ = new QToolButton(this);
    status_log_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    status_log_->setAutoRaise(true);
    status_log_->setIconSize(QSize(14, 14));
    status_log_->setCursor(Qt::PointingHandCursor);
    connect(status_log_, &QToolButton::clicked, this,
            [this]() { log_panel->setVisible(!log_panel->isVisible()); });

    statusBar()->addPermanentWidget(status_cloud_);
    statusBar()->addPermanentWidget(status_log_);
}

void MainWindow::RetranslateUi()
{
    menu_file_->setTitle(tr("&File"));
    menu_edit_->setTitle(tr("&Edit"));
    menu_view_->setTitle(tr("&View"));
    menu_theme_->setTitle(tr("Theme"));
    menu_language_->setTitle(tr("Language"));
    menu_account_->setTitle(tr("&Account"));

    act_open_->setText(tr("Open File..."));
    act_open_folder_->setText(tr("Open Folder..."));
    act_save_->setText(tr("Save"));
    act_save_all_->setText(tr("Save All"));
    act_close_tab_->setText(tr("Close Tab"));
    act_exit_->setText(tr("Exit"));
    act_undo_->setText(tr("Undo"));
    act_redo_->setText(tr("Redo"));
    act_find_->setText(tr("Find..."));
    act_replace_->setText(tr("Replace..."));
    act_add_child_->setText(tr("Add Child"));
    act_delete_->setText(tr("Delete"));
    act_expand_all_->setText(tr("Expand All"));
    act_collapse_all_->setText(tr("Collapse All"));
    act_fullscreen_->setText(tr("Full Screen"));
    act_theme_system_->setText(tr("System"));
    act_theme_light_->setText(tr("Light"));
    act_theme_dark_->setText(tr("Dark"));
    act_connect_->setText(tr("Connect Yandex Disk..."));

    // Tooltips name the shortcut, since the toolbar shows icons only.
    for (QAction *a : {act_open_, act_open_folder_, act_save_, act_find_, act_replace_,
                       act_add_child_, act_delete_}) {
        const QString shortcut = a->shortcut().toString(QKeySequence::NativeText);
        a->setToolTip(shortcut.isEmpty() ? a->text()
                                         : QStringLiteral("%1 (%2)").arg(a->text(), shortcut));
    }

    toolbar_->setWindowTitle(tr("Toolbar"));
    files_dock_->setWindowTitle(tr("Files"));
    filter_dock_->setWindowTitle(tr("Key filter"));

    empty_title_->setText(tr("Open a YAML file"));
    empty_text_->setText(tr("Pick a file from the list on the left, or open one from disk."));
    empty_open_btn_->setText(tr("Open File..."));
    empty_folder_btn_->setText(tr("Open Folder..."));
    empty_hint_->setText(tr("You can also drag .yml / .yaml files into this window"));

    emit tree_model->headerDataChanged(Qt::Horizontal, 0, YamlTreeModel::ColumnCount - 1);
    for (int i = 0; i < tab_bar_->count(); ++i)
        UpdateTabTitle(FileAt(i));
    UpdateActions();
    UpdateStatus();
    UpdateCloudStatus();
}

void MainWindow::RefreshIcons()
{
    const Theme &theme = Theme::instance();
    act_open_->setIcon(theme.icon("file"));
    act_open_folder_->setIcon(theme.icon("folder"));
    act_save_->setIcon(theme.icon("save"));
    act_undo_->setIcon(theme.icon("undo"));
    act_redo_->setIcon(theme.icon("redo"));
    act_find_->setIcon(theme.icon("search"));
    act_replace_->setIcon(theme.icon("replace"));
    act_add_child_->setIcon(theme.icon("plus"));
    act_delete_->setIcon(theme.icon("trash"));
    empty_icon_->setPixmap(theme.icon("files", theme.colors().text_muted).pixmap(56, 56));
    setWindowIcon(theme.icon("files", theme.colors().accent));

    for (int i = 0; i < tab_bar_->count(); ++i) {
        if (auto *btn = qobject_cast<QToolButton *>(tab_bar_->tabButton(i, QTabBar::RightSide)))
            btn->setIcon(theme.icon("close", theme.colors().text_muted));
    }
    UpdateCloudStatus();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange && menu_file_)
        RetranslateUi();
    QMainWindow::changeEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == tab_bar_ && event->type() == QEvent::MouseButtonRelease) {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::MiddleButton) {
            const int tab = tab_bar_->tabAt(mouse->position().toPoint());
            if (tab != -1) {
                CloseTab(tab);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::RestoreSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("ui/geometry").toByteArray());
    restoreState(settings.value("ui/state").toByteArray());

    switch (Theme::instance().mode()) {
    case Theme::Mode::System:
        act_theme_system_->setChecked(true);
        break;
    case Theme::Mode::Light:
        act_theme_light_->setChecked(true);
        break;
    case Theme::Mode::Dark:
        act_theme_dark_->setChecked(true);
        break;
    }

    QString language = settings.value("ui/language").toString();
    if (language.isEmpty())
        language = QLocale::system().language() == QLocale::Russian ? "ru_RU" : "en_GB";
    SwitchLanguage(language);
}

void MainWindow::SaveSettings() const
{
    QSettings settings;
    settings.setValue("ui/geometry", saveGeometry());
    settings.setValue("ui/state", saveState());
    settings.setValue("ui/theme", Theme::modeToString(Theme::instance().mode()));
    settings.setValue("ui/language", language_);
}

void MainWindow::SwitchLanguage(const QString &locale)
{
    language_ = locale;
    (locale == "ru_RU" ? act_lang_ru_ : act_lang_en_)->setChecked(true);

    qApp->removeTranslator(&translator_);
    if (translator_.load(":/i18n/editor_" + locale))
        qApp->installTranslator(&translator_); // sends LanguageChange to every widget
    RetranslateUi();
}

// ---------------------------------------------------------------------------
// Status, actions
// ---------------------------------------------------------------------------

void MainWindow::UpdateCenter()
{
    center_stack_->setCurrentIndex(tab_bar_->count() > 0 ? 1 : 0);
}

void MainWindow::UpdateActions()
{
    const bool has_file = !current_file_.isEmpty();
    const QModelIndex current = tree_view->currentIndex();
    const QModelIndex node = current.isValid() ? current.siblingAtColumn(0) : QModelIndex();

    act_save_->setEnabled(has_file && IsFileDirty(current_file_));
    act_save_all_->setEnabled(!UnsavedFiles().isEmpty());
    act_close_tab_->setEnabled(has_file);
    act_find_->setEnabled(has_file);
    act_replace_->setEnabled(has_file);
    act_expand_all_->setEnabled(has_file);
    act_collapse_all_->setEnabled(has_file);
    act_add_child_->setEnabled(has_file && (!node.isValid() || tree_model->CanHaveChildren(node)));
    act_delete_->setEnabled(has_file && node.isValid());

    act_undo_->setEnabled(undo_group_->canUndo());
    act_redo_->setEnabled(undo_group_->canRedo());
    const QString undo_text = undo_group_->undoText();
    const QString redo_text = undo_group_->redoText();
    act_undo_->setToolTip(undo_text.isEmpty() ? tr("Undo") : tr("Undo: %1").arg(undo_text));
    act_redo_->setToolTip(redo_text.isEmpty() ? tr("Redo") : tr("Redo: %1").arg(redo_text));
}

void MainWindow::UpdateStatus()
{
    if (!status_file_)
        return;

    if (current_file_.isEmpty()) {
        status_file_->setText(tr("No file open"));
        status_stats_->clear();
        setWindowTitle(QStringLiteral("YAML Editor"));
        setWindowModified(false);
    } else {
        const QString path =
            file_local_system ? file_local_system->GetFilePath(current_file_) : QString();
        status_file_->setText(QDir::toNativeSeparators(path));
        int nodes = 0;
        ForEachRow(tree_model, QModelIndex(), [&nodes](const QModelIndex &) { ++nodes; });
        QString stats = tr("%n node(s)", nullptr, nodes);
        if (IsFileDirty(current_file_))
            stats += QStringLiteral("  ·  ") + tr("modified");
        status_stats_->setText(stats);
        setWindowTitle(QStringLiteral("%1[*] — YAML Editor").arg(current_file_));
        setWindowModified(IsFileDirty(current_file_));
    }

    status_log_->setText(unseen_errors_ > 0 ? tr("Log · %n error(s)", nullptr, unseen_errors_)
                                            : tr("Log"));
    status_log_->setToolTip(tr("Show or hide the log (Ctrl+`)"));
    const Theme &theme = Theme::instance();
    status_log_->setIcon(theme.icon("terminal", unseen_errors_ > 0 ? theme.colors().danger
                                                                   : theme.colors().text_muted));
}

void MainWindow::UpdateCloudStatus()
{
    if (!status_cloud_)
        return;
    const bool connected = yandex_api && yandex_api->hasToken();
    const Theme &theme = Theme::instance();
    status_cloud_->setIcon(theme.icon("cloud", connected ? theme.colors().success
                                                         : theme.colors().text_muted));
    status_cloud_->setText(connected ? tr("Yandex Disk") : tr("Cloud sync off"));
    status_cloud_->setToolTip(connected ? tr("Files are uploaded to Yandex Disk on save")
                                        : tr("Connect Yandex Disk to sync files"));
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
            UpdateTabTitle(file_name);
            file_panel_->setDirty(file_name, !clean);
            UpdateActions();
            UpdateStatus();
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
    RefreshKeyFilter();

    StackFor(file_name)->push(new YamlSnapshotCommand(
        text, std::move(before), after,
        [this, file_name](const YamlNode &snapshot) { ApplySnapshot(file_name, snapshot); }));
    UpdateActions();
    UpdateStatus();
}

void MainWindow::ApplySnapshot(const QString &file_name, const YamlNode &snapshot)
{
    doc_store.set(file_name, snapshot);
    if (file_name != current_file_)
        return;

    applying_snapshot_ = true;
    const QSet<QString> expanded = ExpandedPaths();
    tree_model->SetRoot(snapshot);
    RestoreExpanded(expanded);
    last_committed_tree_ = snapshot;
    RefreshKeyFilter();
    applying_snapshot_ = false;
    UpdateActions();
    UpdateStatus();
}

QSet<QString> MainWindow::ExpandedPaths() const
{
    QSet<QString> paths;
    ForEachRow(tree_model, QModelIndex(), [&](const QModelIndex &idx) {
        if (tree_view->isExpanded(idx))
            paths.insert(idx.data(YamlTreeModel::PathRole).toString());
    });
    return paths;
}

void MainWindow::RestoreExpanded(const QSet<QString> &paths)
{
    ForEachRow(tree_model, QModelIndex(), [&](const QModelIndex &idx) {
        if (paths.contains(idx.data(YamlTreeModel::PathRole).toString()))
            tree_view->expand(idx);
    });
}

// ---------------------------------------------------------------------------
// Services
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
        AddKnownFile(file_name);
    });

    file_watch = new FileWatchService(this);
    file_watch->watchFolder(YmlFolder());
    connect(file_watch, &FileWatchService::folderChanged, this, &MainWindow::OnFolderChanged);
    connect(file_watch, &FileWatchService::fileModified, this, &MainWindow::OnFileModified);
    connect(file_watch, &FileWatchService::fileRemoved, this,
            [this](const QString &file_name, const QString &) {
                log_panel->logWarning(tr("File removed on disk: %1").arg(file_name));
            });
}

void MainWindow::OpenConnectDialog()
{
    ConnectDialog dialog(&token_store, this);
    connect(&dialog, &ConnectDialog::tokenSaved, this, [this]() {
        if (yandex_api) {
            yandex_api->reloadToken();
            log_panel->logInfo(tr("Yandex Disk token updated"));
            UpdateCloudStatus();
        }
    });
    dialog.exec();
}

// ---------------------------------------------------------------------------
// File list and tabs
// ---------------------------------------------------------------------------

void MainWindow::PopulateFileList()
{
    const auto &files = file_local_system->GetFiles();
    for (auto it = files.cbegin(); it != files.cend(); ++it)
        file_panel_->addFile(it.key(), it.value());
}

void MainWindow::AddKnownFile(const QString &file_name)
{
    file_panel_->addFile(file_name, file_local_system->GetFilePath(file_name));
}

QString MainWindow::FileAt(int tab) const
{
    return tab_bar_->tabData(tab).toString();
}

int MainWindow::FindTabByName(const QString &file_name) const
{
    for (int i = 0; i < tab_bar_->count(); ++i) {
        if (FileAt(i) == file_name)
            return i;
    }
    return -1;
}

void MainWindow::UpdateTabTitle(const QString &file_name)
{
    const int tab = FindTabByName(file_name);
    if (tab == -1)
        return;
    const bool dirty = IsFileDirty(file_name);
    tab_bar_->setTabText(tab, dirty ? file_name + QStringLiteral(" ●") : file_name);
    QString tip = QDir::toNativeSeparators(file_local_system->GetFilePath(file_name));
    if (dirty)
        tip += QStringLiteral("\n") + tr("Unsaved changes");
    tab_bar_->setTabToolTip(tab, tip);
}

void MainWindow::ActivateFile(const QString &file_name)
{
    if (file_name.isEmpty())
        return;

    const int tab = FindTabByName(file_name);
    if (tab == -1) {
        OpenInNewTab(file_name);
        file_panel_->setCurrent(current_file_);
        return;
    }
    tab_bar_->setCurrentIndex(tab); // -> OnTabChanged -> ShowFile
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

    const QSignalBlocker blocker(tab_bar_); // ShowFile below needs the tab data set first
    const int index = tab_bar_->addTab(file_name);
    tab_bar_->setTabData(index, file_name);

    auto *close = new QToolButton(tab_bar_);
    close->setAutoRaise(true);
    close->setIconSize(QSize(14, 14));
    close->setIcon(Theme::instance().icon("close", Theme::instance().colors().text_muted));
    close->setCursor(Qt::PointingHandCursor);
    close->setToolTip(tr("Close (Ctrl+W)"));
    connect(close, &QToolButton::clicked, this, [this, close]() {
        for (int i = 0; i < tab_bar_->count(); ++i) {
            if (tab_bar_->tabButton(i, QTabBar::RightSide) == close) {
                CloseTab(i);
                return;
            }
        }
    });
    tab_bar_->setTabButton(index, QTabBar::RightSide, close);
    tab_bar_->setCurrentIndex(index);
    UpdateTabTitle(file_name);

    file_panel_->setOpen(file_name, true);
    ShowFile(file_name);
    UpdateCenter();
    log_panel->logInfo(tr("Opened %1").arg(file_name));
    return true;
}

void MainWindow::ShowFile(const QString &file_name)
{
    if (!doc_store.contains(file_name))
        return;

    if (!current_file_.isEmpty())
        expanded_paths_[current_file_] = ExpandedPaths();

    current_file_ = file_name;

    applying_snapshot_ = true;
    tree_model->SetRoot(doc_store.get(file_name));
    applying_snapshot_ = false;
    last_committed_tree_ = doc_store.get(file_name);

    if (expanded_paths_.contains(file_name))
        RestoreExpanded(expanded_paths_.value(file_name));
    else
        tree_view->expandToDepth(0);

    RefreshKeyFilter();
    undo_group_->setActiveStack(StackFor(file_name));
    file_panel_->setCurrent(file_name);

    if (find_bar_->isVisible() && !find_bar_->text().isEmpty())
        search_controller->find(find_bar_->text(), find_bar_->caseSensitive(),
                                find_bar_->useRegex(), true);
    else
        find_bar_->setResult(-1, -1);

    UpdateActions();
    UpdateStatus();
}

void MainWindow::ShowNoFile()
{
    current_file_.clear();
    applying_snapshot_ = true;
    tree_model->SetRoot(YamlNode());
    applying_snapshot_ = false;
    last_committed_tree_ = YamlNode();
    filter_panel_->setKeys({}, {});
    undo_group_->setActiveStack(nullptr);
    file_panel_->setCurrent(QString());
    find_bar_->hide();
    UpdateCenter();
    UpdateActions();
    UpdateStatus();
}

void MainWindow::OnTabChanged(int index)
{
    if (index < 0 || index >= tab_bar_->count()) {
        ShowNoFile();
        return;
    }
    const QString file_name = FileAt(index);
    if (file_name != current_file_)
        ShowFile(file_name);
}

void MainWindow::CloseTab(int index)
{
    if (index < 0 || index >= tab_bar_->count())
        return;

    const QString file_name = FileAt(index);

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
    doc_store.remove(file_name);
    hidden_keys_.remove(file_name);
    expanded_paths_.remove(file_name);
    if (QUndoStack *stack = undo_stacks_.take(file_name)) {
        undo_group_->removeStack(stack);
        delete stack;
    }
    file_panel_->setOpen(file_name, false);
    file_panel_->setDirty(file_name, false);

    if (file_name == current_file_)
        current_file_.clear();

    tab_bar_->removeTab(index); // switches to a neighbour tab, or to "no file"
    if (tab_bar_->count() == 0)
        ShowNoFile();
    UpdateCenter();
    UpdateActions();
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
    for (const QUrl &url : mime_data->urls()) {
        const QFileInfo info(url.toLocalFile());
        if (info.isDir()) {
            const QDir dir(info.absoluteFilePath());
            for (const QString &name : dir.entryList({"*.yaml", "*.yml"}, QDir::Files))
                OpenFileByPath(dir.absoluteFilePath(name));
        } else {
            OpenFileByPath(info.absoluteFilePath());
        }
    }
}

void MainWindow::OpenFileByPath(const QString &local_path)
{
    const QFileInfo info(local_path);
    if (!info.exists() || !info.isFile())
        return;

    if (!IsYamlFile(local_path)) {
        log_panel->logWarning(tr("Not a YAML file: %1").arg(info.fileName()));
        statusBar()->showMessage(tr("Not a YAML file: %1").arg(info.fileName()), 4000);
        return;
    }

    const QString file_name = info.fileName();
    if (file_local_system->Contains(file_name)) {
        const QString known = QFileInfo(file_local_system->GetFilePath(file_name)).absoluteFilePath();
        if (known.compare(info.absoluteFilePath(), Qt::CaseInsensitive) != 0) {
            QMessageBox::information(
                this, tr("Open File"),
                tr("Another file named \"%1\" is already in the list:\n%2")
                    .arg(file_name, QDir::toNativeSeparators(known)));
            return;
        }
    } else {
        file_local_system->AddFile(info.absoluteFilePath());
    }

    AddKnownFile(file_name);
    ActivateFile(file_name);
}

void MainWindow::OpenFileDialog()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Select YAML file(s)"), YmlFolder(),
        tr("YAML files (*.yml *.yaml);;All files (*)"));

    for (const QString &path : paths)
        OpenFileByPath(path);
}

void MainWindow::OpenFolderDialog()
{
    const QString dir_path = QFileDialog::getExistingDirectory(
        this, tr("Select a folder with YAML files"), QDir::currentPath());
    if (dir_path.isEmpty())
        return;

    const QDir dir(dir_path);
    const QStringList files = dir.entryList({"*.yaml", "*.yml"}, QDir::Files);
    if (files.isEmpty()) {
        QMessageBox::information(this, tr("Open Folder"), tr("No YAML files in the folder"));
        return;
    }

    for (const QString &name : files)
        OpenFileByPath(dir.absoluteFilePath(name));
}

void MainWindow::OnFolderChanged(const QString &path)
{
    const QDir dir(path);
    const QString folder = dir.absolutePath();
    const QStringList files = dir.entryList({"*.yaml", "*.yml"}, QDir::Files);

    for (const QString &file : files) {
        if (!file_local_system->Contains(file))
            file_local_system->AddFile(dir.absoluteFilePath(file));
        if (!file_panel_->contains(file))
            AddKnownFile(file);
    }

    // Drop entries that lived in this folder and are gone. Files opened from other
    // folders are not affected, and an open tab is kept so its content can be saved.
    const auto known_files = file_local_system->GetFiles();
    for (auto it = known_files.cbegin(); it != known_files.cend(); ++it) {
        const QFileInfo known(it.value());
        if (known.absolutePath().compare(folder, Qt::CaseInsensitive) != 0 || known.exists())
            continue;
        if (FindTabByName(it.key()) != -1)
            continue;
        file_panel_->removeFile(it.key());
        file_local_system->RemoveFile(it.key());
    }
    file_panel_->setCurrent(current_file_);
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
        expanded_paths_[file_name] = ExpandedPaths();
        current_file_.clear(); // make ShowFile reload the model
        ShowFile(file_name);
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
    statusBar()->showMessage(tr("Saved %1").arg(file_name), 3000);

    yandex_api->UploadFile(full_path, [this, file_name](bool ok) {
        if (ok)
            log_panel->logInfo(tr("Uploaded %1 to Yandex Disk").arg(file_name));
    });
    return true;
}

void MainWindow::SaveCurrent()
{
    if (!current_file_.isEmpty() && IsFileDirty(current_file_))
        WriteFile(current_file_);
}

void MainWindow::SaveAll()
{
    for (const QString &file_name : UnsavedFiles()) {
        if (!WriteFile(file_name))
            return;
    }
}

bool MainWindow::IsFileDirty(const QString &file_name) const
{
    const QUndoStack *stack = undo_stacks_.value(file_name);
    return stack && !stack->isClean();
}

QStringList MainWindow::UnsavedFiles() const
{
    QStringList dirty;
    for (int i = 0; i < tab_bar_->count(); ++i) {
        const QString file_name = FileAt(i);
        if (IsFileDirty(file_name))
            dirty << file_name;
    }
    return dirty;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    const QStringList dirty = UnsavedFiles();
    if (!dirty.isEmpty()) {
        const QString list = dirty.join("\n  • ");
        QMessageBox box(this);
        box.setWindowTitle(tr("Unsaved changes"));
        box.setText(tr("The following files have unsaved changes:") + "\n\n  • " + list);
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
    }

    SaveSettings();
    event->accept();
}

// ---------------------------------------------------------------------------
// Tree editing
// ---------------------------------------------------------------------------

void MainWindow::AddChildAt(const QModelIndex &index)
{
    if (current_file_.isEmpty())
        return;
    const QModelIndex node = index.isValid() ? index.siblingAtColumn(0) : QModelIndex();
    if (node.isValid() && !tree_model->CanHaveChildren(node))
        return;

    const YamlNode before = last_committed_tree_;
    const QModelIndex added = tree_model->AddChild(node, QStringLiteral("new_key"));
    if (!added.isValid())
        return;
    if (node.isValid())
        tree_view->expand(node);
    CommitEdit(tr("Add"), before);

    const bool list_item = added.data(YamlTreeModel::IsSequenceItemRole).toBool();
    const QModelIndex to_edit =
        added.siblingAtColumn(list_item ? YamlTreeModel::ValueColumn : YamlTreeModel::KeyColumn);
    tree_view->setCurrentIndex(to_edit);
    tree_view->scrollTo(to_edit);
    tree_view->edit(to_edit);
}

void MainWindow::DeleteAt(const QModelIndex &index)
{
    if (current_file_.isEmpty() || !index.isValid())
        return;
    const YamlNode before = last_committed_tree_;
    if (tree_model->RemoveNode(index.siblingAtColumn(0)))
        CommitEdit(tr("Delete"), before);
}

void MainWindow::ShowTreeContextMenu(const QPoint &pos)
{
    if (current_file_.isEmpty())
        return;

    const QModelIndex index = tree_view->indexAt(pos);
    tree_view->setCurrentIndex(index);
    UpdateActions();

    QMenu menu(this);
    menu.addAction(act_add_child_);
    if (index.isValid())
        menu.addAction(act_delete_);
    menu.addSeparator();
    menu.addAction(act_expand_all_);
    menu.addAction(act_collapse_all_);
    menu.exec(tree_view->viewport()->mapToGlobal(pos));
}

// ---------------------------------------------------------------------------
// Key filter
// ---------------------------------------------------------------------------

void MainWindow::RefreshKeyFilter()
{
    QSet<QString> keys;
    ForEachRow(tree_model, QModelIndex(), [&keys](const QModelIndex &idx) {
        if (idx.data(YamlTreeModel::IsLeafKeyRole).toBool())
            keys.insert(idx.data(Qt::EditRole).toString());
    });

    QStringList sorted = keys.values();
    std::sort(sorted.begin(), sorted.end(), [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });

    // Forget hidden keys that no longer exist.
    QSet<QString> &hidden = hidden_keys_[current_file_];
    hidden.intersect(keys);

    filter_panel_->setKeys(sorted, hidden);
    ApplyKeyFilter();
}

void MainWindow::ApplyKeyFilter()
{
    const QSet<QString> hidden = hidden_keys_.value(current_file_);
    ForEachRow(tree_model, QModelIndex(), [&](const QModelIndex &idx) {
        const bool hide = idx.data(YamlTreeModel::IsLeafKeyRole).toBool()
                          && hidden.contains(idx.data(Qt::EditRole).toString());
        tree_view->setRowHidden(idx.row(), idx.parent(), hide);
    });
}

// ---------------------------------------------------------------------------
// Find
// ---------------------------------------------------------------------------

void MainWindow::ShowFind(bool replace)
{
    if (current_file_.isEmpty())
        return;
    if (replace)
        find_bar_->showReplace();
    else
        find_bar_->showFind();
}
