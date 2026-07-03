#include "mainwindow.h"

#include <memory>
#include "./ui_mainwindow.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QListView>
#include <QMessageBox>
#include <QMimeData>
#include <QVBoxLayout>

#include <algorithm>

#include "api/yandex/yandexapi.h"
#include "customWidget/customlineedit.h"
#include "customWidget/flowlayout.h"
#include "files/filesystem.h"
#include "customWidget/logpanel.h"
#include "files/yaml/yamlreader.h"
#include "replacewindow.h"
#include "searchingwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    InitLogPanel();
    InitKeysLayout();

    file_local_system = std::make_unique<FileSystem>(QDir::currentPath() + "/ymlFiles");
    yandex_api = std::make_unique<YandexApi>();
    yaml_reader = std::make_unique<YamlReader>();

    ConnectServices();
    ConnectShortCut();
    InitLanguageCmb();

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
}

void MainWindow::InitLogPanel()
{
    log_panel = new LogPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, log_panel);
    log_panel->logInfo(tr("Application started"));
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

    file_watcher = std::make_unique<QFileSystemWatcher>();
    file_watcher->addPath(QDir::currentPath() + "/ymlFiles");
    connect(file_watcher.get(), &QFileSystemWatcher::directoryChanged,
            this, &MainWindow::OnFolderChanged);

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
}

void MainWindow::InitLanguageCmb()
{
    ui->langCombo->addItem("English", "en_GB");
    ui->langCombo->addItem("Русский", "ru_RU");
    connect(ui->langCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        switchLanguage(ui->langCombo->currentData().toString());
    });
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

    if (is_update_file && !root.children.isEmpty()) {
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
    if (!is_update_file)
        return;

    const QString full_path = file_local_system->GetFilePath(fileName);
    yaml_reader->SaveValues(root, full_path);
    yandex_api->UploadFile(full_path, [](bool) {});
    is_update_file = false;
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

    root = nodes.value(file);
    Displaykeys(root);
    ui->tabWidget->setCurrentIndex(existing_tab_index);
    return true;
}

void MainWindow::CloseTab(int index)
{
    if (index < 0 || index >= ui->tabWidget->count())
        return;

    const QString file_name = ui->tabWidget->tabText(index);
    SaveData(file_name);

    check_box_states_nodes.remove(file_name);
    nodes.remove(file_name);
    check_box_states.clear();

    QWidget *tab = ui->tabWidget->widget(index);
    ui->tabWidget->removeTab(index);
    delete tab;
}

void MainWindow::DisplayYamlData()
{
    root = yaml_reader->GetRootNode();
    Displaykeys(root);

    QWidget *new_tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(new_tab);

    ClearTreeWidget();

    static const QRegularExpression kNoRegex;
    for (const YamlNode &node : root.children)
        DisplayTreeNode(node, "", "", nullptr, tree_widget, kNoRegex, false);

    layout->addWidget(tree_widget);
    new_tab->setLayout(layout);

    ui->tabWidget->addTab(new_tab, ui->fileNamecmb->currentText());
    ui->tabWidget->setCurrentWidget(new_tab);
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
        if (yaml_reader->ReadFile(file_path))
            DisplayYamlData();
    }

    nodes.insert(file_name, root);
    check_box_states_nodes.insert(file_name, check_box_states);
}

void MainWindow::RefreshCurrentTree()
{
    SaveExpandedState();

    QWidget *current_tab = ui->tabWidget->currentWidget();
    if (!current_tab)
        return;

    if (QLayout *layout = current_tab->layout()) {
        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }

    Displaykeys(root);
    ClearTreeWidget();

    static const QRegularExpression kNoRegex;
    for (const YamlNode &node : root.children)
        DisplayTreeNode(node, "", "", nullptr, tree_widget, kNoRegex, false);

    RestoreExpandedState();
    current_tab->layout()->addWidget(tree_widget);

    nodes.insert(ui->fileNamecmb->currentText(), root);
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
    connect(search_wnd, &SearchingWindow::searchingText, this, &MainWindow::SearchingText);
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
    connect(replace_wnd, &ReplaceWindow::searchReplaceText, this, &MainWindow::SearchReplaceText);
    connect(replace_wnd, &ReplaceWindow::replaceText, this, &MainWindow::ReplaceText);
    replace_wnd->show();
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
    RefreshCurrentTree();
}

void MainWindow::onCheckBoxStateChanged(int state)
{
    if (QCheckBox *check_box = qobject_cast<QCheckBox *>(sender()))
        check_box_states[check_box->text()] = (state == Qt::Checked);
}

void MainWindow::UpdateValue(const QString &path, const QString &newValue, bool isKey)
{
    const QStringList parts = path.split('.');
    YamlNode *current_node = &root;

    for (const QString &key : parts) {
        YamlNode *next = nullptr;
        for (YamlNode &child : current_node->children) {
            if (child.key == key) {
                next = &child;
                break;
            }
        }
        if (!next)
            return;
        current_node = next;
    }

    if (isKey)
        current_node->key = newValue;
    else
        current_node->value = newValue;

    is_update_file = true;
}

void MainWindow::HandleAddKeyValue(const QString &path, const QString &newValue, bool isKey)
{
    if (isKey)
        root.AddValueToKey(path, newValue);
    else
        root.AddKeyWithValue(path, newValue);

    is_update_file = true;
    SaveData(previous_text_cmb);
    RefreshCurrentTree();
}

void MainWindow::HandleDeleteElement(const QString &path, bool isKey)
{
    if (isKey)
        root.RemoveKey(path);
    else
        root.RemoveValue(path);

    is_update_file = true;
    SaveData(previous_text_cmb);
    RefreshCurrentTree();
}

void MainWindow::DisplayTreeNode(const YamlNode &node,
                                 const QString &parentPath,
                                 const QString &searchText,
                                 QTreeWidgetItem *parentItem,
                                 QTreeWidget *treeWidget,
                                 const QRegularExpression &regex,
                                 bool useRegex)
{
    if (check_box_states.contains(node.key) && !check_box_states[node.key])
        return;

    const QString currentPath = parentPath.isEmpty() ? node.key : parentPath + "." + node.key;

    QTreeWidgetItem *tree_item = new QTreeWidgetItem();
    tree_item->setData(0, Qt::UserRole, currentPath);

    if (expanded_paths.contains(currentPath))
        tree_item->setExpanded(true);

    if (parentItem)
        parentItem->addChild(tree_item);
    else
        treeWidget->addTopLevelItem(tree_item);

    CustomLineEdit *key_txt = new CustomLineEdit(this, currentPath, true);
    if (node.key.isEmpty()) {
        key_txt->setText("-");
        key_txt->setReadOnly(true);
    } else {
        key_txt->setText(node.key);
    }
    key_txt->setStyleSheet("QLineEdit { border: none; background: transparent; "
                           "font-size: 16px; color: #7aa2ff; }");
    treeWidget->setItemWidget(tree_item, 0, key_txt);

    connect(key_txt, &CustomLineEdit::AddKeyValue, this, &MainWindow::HandleAddKeyValue);
    connect(key_txt, &CustomLineEdit::DeleteElement, this, &MainWindow::HandleDeleteElement);
    connect(key_txt, &QLineEdit::textChanged, this, [this, currentPath](const QString &newValue) {
        UpdateValue(currentPath, newValue, true);
    });

    bool key_matches = false;
    bool value_matches = false;
    if (useRegex) {
        key_matches = regex.match(node.key).hasMatch();
        value_matches = regex.match(node.value).hasMatch();
    } else {
        key_matches = node.key.contains(searchText, cs);
        value_matches = node.value.contains(searchText, cs);
    }

    if (!searchText.isEmpty() && key_matches)
        found_widgets.append(key_txt);

    if (!node.value.isEmpty() || node.children.isEmpty()) {
        CustomLineEdit *value_txt = new CustomLineEdit(this, currentPath, false);
        value_txt->setText(node.value);
        value_txt->setStyleSheet("QLineEdit { border: none; background: transparent; "
                                 "font-size: 14px; color: #e6e6e6; }");
        treeWidget->setItemWidget(tree_item, 1, value_txt);

        connect(value_txt, &CustomLineEdit::AddKeyValue, this, &MainWindow::HandleAddKeyValue);
        connect(value_txt, &CustomLineEdit::DeleteElement, this, &MainWindow::HandleDeleteElement);
        connect(value_txt, &QLineEdit::textChanged, this,
                [this, currentPath](const QString &newValue) {
                    UpdateValue(currentPath, newValue, false);
                });

        if (!searchText.isEmpty() && value_matches)
            found_widgets.append(value_txt);
    }

    for (const YamlNode &child : node.children)
        DisplayTreeNode(child, currentPath, searchText, tree_item, treeWidget, regex, useRegex);
}

void MainWindow::RestoreExpandedState()
{
    if (!tree_widget)
        return;

    QTreeWidgetItemIterator it(tree_widget);
    while (*it) {
        QTreeWidgetItem *item = *it;
        const QString path = item->data(0, Qt::UserRole).toString();
        if (expanded_paths.contains(path))
            item->setExpanded(true);
        ++it;
    }
}

void MainWindow::SaveExpandedState()
{
    expanded_paths.clear();
    if (!tree_widget)
        return;

    QTreeWidgetItemIterator it(tree_widget);
    while (*it) {
        QTreeWidgetItem *item = *it;
        if (item->isExpanded()) {
            const QString path = item->data(0, Qt::UserRole).toString();
            if (!path.isEmpty())
                expanded_paths.insert(path);
        }
        ++it;
    }
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

void MainWindow::ClearTreeWidget()
{
    tree_widget = new QTreeWidget(this);
    tree_widget->setColumnCount(2);
    tree_widget->setHeaderLabels(QStringList{tr("Key"), tr("Value")});
    tree_widget->setColumnWidth(0, 280);
    tree_widget->setMinimumHeight(200);
    tree_widget->setRootIsDecorated(true);
    tree_widget->setIndentation(20);
    tree_widget->setAlternatingRowColors(true);

    tree_widget->setStyleSheet(R"(
        QTreeWidget {
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
        QTreeView { show-decoration-selected: 1; }
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

void MainWindow::ClearTabWidget(QWidget *tab)
{
    if (!tab)
        return;

    if (QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(tab->layout())) {
        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        layout->addWidget(tree_widget);
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

bool MainWindow::BuildRegex(const QString &text, bool isSensitive, QRegularExpression &out) const
{
    const auto option = isSensitive ? QRegularExpression::NoPatternOption
                                    : QRegularExpression::CaseInsensitiveOption;
    out = QRegularExpression(text, option);
    return out.isValid();
}

void MainWindow::RunSearch(const QString &text, bool useRegex, bool resetSelection)
{
    ClearTreeWidget();
    found_widgets.clear();
    current_found_index = -1;
    starting_index = -1;
    previous_widget = nullptr;

    for (const YamlNode &node : root.children)
        DisplayTreeNode(node, "", text, nullptr, tree_widget, searching_regex, useRegex);

    ClearTabWidget(ui->tabWidget->currentWidget());

    if (!found_widgets.isEmpty()) {
        current_found_index = resetSelection ? 0 : found_widgets.size() - 1;
        HighlightCurrentFound();
    }
}

void MainWindow::SearchingText(const QString &text, bool isSensitive, bool isDownward, bool useRegex)
{
    const Qt::CaseSensitivity new_cs = isSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QRegularExpression regex;
    if (useRegex && !BuildRegex(text, isSensitive, regex)) {
        QMessageBox::warning(this, tr("Search"), tr("Invalid regular expression"));
        return;
    }

    const bool query_changed = (useRegex && regex.pattern() != searching_regex.pattern())
                               || (!useRegex && searching_text != text) || new_cs != cs;

    if (query_changed) {
        cs = new_cs;
        searching_text = text;
        searching_regex = regex;
        RunSearch(text, useRegex, isDownward);
        return;
    }

    if (starting_index == -1)
        starting_index = current_found_index;

    if (found_widgets.isEmpty())
        return;

    if (isDownward)
        current_found_index = (current_found_index + 1) % found_widgets.size();
    else
        current_found_index = (current_found_index - 1 + found_widgets.size()) % found_widgets.size();

    if (current_found_index == starting_index) {
        QMessageBox::information(this, tr("Search"), tr("Reached the end of the search results."));
        starting_index = -1;
        return;
    }

    HighlightCurrentFound();
}

void MainWindow::SearchReplaceText(const QString &text, bool isSensitive, bool useRegex)
{
    const Qt::CaseSensitivity new_cs = isSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QRegularExpression regex;
    if (useRegex && !BuildRegex(text, isSensitive, regex)) {
        QMessageBox::warning(this, tr("Replace"), tr("Invalid regular expression"));
        return;
    }

    const bool query_changed = (useRegex && regex.pattern() != searching_regex.pattern())
                               || (!useRegex && searching_text != text) || new_cs != cs;

    if (query_changed) {
        cs = new_cs;
        searching_text = text;
        searching_regex = regex;
        RunSearch(text, useRegex, true);
        return;
    }

    if (starting_index == -1)
        starting_index = current_found_index;

    if (found_widgets.isEmpty())
        return;

    current_found_index = (current_found_index + 1) % found_widgets.size();
    if (current_found_index == starting_index) {
        starting_index = -1;
        return;
    }

    HighlightCurrentFound();
}

void MainWindow::ReplaceText(const QString &findText,
                             const QString &replaceText,
                             bool allText,
                             bool useRegex)
{
    SearchReplaceText(findText, cs == Qt::CaseSensitive, useRegex);

    if (allText) {
        for (QWidget *widget : std::as_const(found_widgets))
            ReplaceInWidget(widget, findText, replaceText, useRegex);
        searching_text.clear();
    } else if (current_found_index >= 0 && current_found_index < found_widgets.size()) {
        ReplaceInWidget(found_widgets[current_found_index], findText, replaceText, useRegex);
    }
}

void MainWindow::HighlightCurrentFound()
{
    if (current_found_index < 0 || current_found_index >= found_widgets.size()) {
        QMessageBox::information(this, tr("Editor"), tr("Can't find it ") + searching_text + "\n");
        return;
    }

    QWidget *current_widget = found_widgets[current_found_index];
    if (!current_widget) {
        qWarning() << "Current widget is null!";
        return;
    }

    if (previous_widget)
        previous_widget->setStyleSheet(previous_widget_original_style_sheet);

    QString current_style_sheet = current_widget->styleSheet();
    previous_widget = current_widget;
    previous_widget_original_style_sheet = current_style_sheet;

    static const QRegularExpression color_regex("color:\\s*[^;]+;");
    const QString newColor = "color: blue;";
    if (color_regex.match(current_style_sheet).hasMatch())
        current_style_sheet.replace(color_regex, newColor);
    else
        current_style_sheet += " " + newColor;

    current_widget->setStyleSheet(current_style_sheet);
    ScrollIntoView(current_widget);
}

void MainWindow::ScrollIntoView(QWidget *widget)
{
    QTreeWidgetItemIterator it(tree_widget);
    while (*it) {
        QTreeWidgetItem *item = *it;
        if (tree_widget->itemWidget(item, 0) == widget
            || tree_widget->itemWidget(item, 1) == widget) {
            tree_widget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            return;
        }
        ++it;
    }
}

void MainWindow::ReplaceInWidget(QWidget *widget,
                                 const QString &findText,
                                 const QString &replaceText,
                                 bool useRegex)
{
    if (!widget)
        return;

    if (QLineEdit *line_edit = qobject_cast<QLineEdit *>(widget)) {
        QString text = line_edit->text();
        if (useRegex)
            text.replace(searching_regex, replaceText);
        else
            text.replace(findText, replaceText, cs);
        line_edit->setText(text);
    }
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

    if (nodes.contains(file_name)) {
        root = nodes.value(file_name);
        check_box_states = check_box_states_nodes.value(file_name);
        Displaykeys(root);
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
