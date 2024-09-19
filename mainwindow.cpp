#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);
    is_update_file = false;
    tree_widget = nullptr;
    search_wnd = nullptr;
    replace_wnd = nullptr;
    previous_widget = nullptr;

    yandex_api = new YandexApi();
    yaml_reader = new YamlReader();

    connect(yaml_reader, &YamlReader::FileUploaded, this, [&](bool success) {
        if (success) {
            QMessageBox::information(this, "Saving", "Your file is saved");
        }
    });

    key_F11 = new QShortcut(this);
    key_F11->setKey(Qt::Key_F11);

    connect(key_F11, &QShortcut::activated, this, &::MainWindow::SlotShortcutF11);

    key_ctrl_f = new QShortcut(this);
    key_ctrl_f->setKey(Qt::CTRL | Qt::Key_F);

    connect(key_ctrl_f, &QShortcut::activated, this, &MainWindow::SlotShortcutCtrlF);

    key_ctrl_s = new QShortcut(this);
    key_ctrl_s->setKey(Qt::CTRL | Qt::Key_S);

    connect(key_ctrl_s, &QShortcut::activated, this, &MainWindow::SlotShortcutCtrlS);

    key_ctrl_r = new QShortcut(this);
    key_ctrl_r->setKey(Qt::CTRL | Qt::Key_R);

    connect(key_ctrl_r, &QShortcut::activated, this, &MainWindow::SlotShortcutCtrlR);

    file_local_system = new FileSystem(QDir::currentPath() + "/ymlFiles");

    local_files = file_local_system->getFiles();

    connect(yandex_api, &YandexApi::NewFile, this, &MainWindow::UploadFileOnCmb);

    yandex_api->GetFiles();

    previous_text_cmb = ui->fileNamecmb->currentText();

    file_watcher = new QFileSystemWatcher();

    file_watcher->addPath(QDir::currentPath() + "/ymlFiles");

    connect(file_watcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::OnFolderChanged);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::CloseTab);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mime_data = event->mimeData();

    if (mime_data->hasUrls()) {
        QList<QUrl> urlList = mime_data->urls();

        for (const QUrl &url : urlList) {
            QString file_name = url.fileName();

            QString suffix_file = QFileInfo(file_name).suffix();

            if (suffix_file == "yaml" || suffix_file == "yml")
                if (ui->fileNamecmb->findText(file_name) == -1)
                    ui->fileNamecmb->addItem(file_name);
        }
    }
}

void MainWindow::on_fileNamecmb_currentIndexChanged(int index)
{
    ui->fileNamecmb->setCurrentIndex(index);

    if (is_update_file && !root.children.isEmpty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  "Save File",
                                                                  "Do you want to save the file?",
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            SaveData(previous_text_cmb);
        }
    }

    previous_text_cmb = ui->fileNamecmb->currentText();

    ReadFile();
}

void MainWindow::SaveData(const QString &fileName)
{
    if (is_update_file) {
        QString full_path = QDir::currentPath() + "/ymlFiles/" + fileName;

        yaml_reader->SaveValues(root, full_path);

        is_update_file = false;
    }
}

void MainWindow::UploadFileOnCmb(const QString &file)
{
    QString file_path = QDir::currentPath() + "/ymlFiles/" + file;
    QString file_hash = file_local_system->CalculateFileCheckSum(file_path);

    bool file_exists = false;

    if (local_files.isEmpty()) {
        ui->fileNamecmb->addItem(file);
        local_files.append(file);
    } else {
        for (const QString &local_file : local_files) {
            QString local_file_path = QDir::currentPath() + "/ymlFiles/" + local_file;
            QString local_hash = file_local_system->CalculateFileCheckSum(local_file_path);

            if (local_file == file && local_hash == file_hash) {
                ui->fileNamecmb->addItem(local_file);
                file_exists = true;
                break;
            }
        }

        if (!file_exists) {
            if (ui->fileNamecmb->findText(file) == -1) {
                local_files.append(file);
                ui->fileNamecmb->addItem(file);
            }
        }
    }
}

void MainWindow::CloseTab(int index)
{
    if (index >= 0 && index < ui->tabWidget->count()) {
        QString file_name = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

        SaveData(file_name);

        nodes.remove(file_name);

        QWidget *tab = ui->tabWidget->widget(index);
        ui->tabWidget->removeTab(index);
        delete tab;
    }
}

void MainWindow::DisplayYamlData()
{
    root = yaml_reader->GetRootNode();

    Displaykeys(root);

    QWidget *new_tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(new_tab);

    ClearTreeWidget();

    for (const auto &node : root.children) {
        DisplayTreeNode(node, "", "", nullptr, tree_widget, false);
    }

    layout->addWidget(tree_widget);
    new_tab->setLayout(layout);

    ui->tabWidget->addTab(new_tab, ui->fileNamecmb->currentText());
    ui->tabWidget->setCurrentWidget(new_tab);
}

int MainWindow::FindTabByName(const QString &fileName)
{
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == fileName) {
            return i;
        }
    }
    return -1;
}

void MainWindow::CollectKeys(const YamlNode &node, QSet<QString> &keys)
{
    QString key = node.key;
    QString value = node.value;

    if (key.isEmpty())
        key = "-";

    if (node.children.isEmpty()) {
        keys.insert(key);
    }

    for (const auto &sub_node : node.children) {
        CollectKeys(sub_node, keys);
    }
}

void MainWindow::ReadFile()
{
    QString file_name = ui->fileNamecmb->currentText();

    int existing_tab_index = FindTabByName(file_name);

    if (existing_tab_index != -1) {
        root = nodes.value(file_name);
        Displaykeys(root);
        ui->tabWidget->setCurrentIndex(existing_tab_index);
    } else {
        if (yaml_reader->ReadFile(file_name)) {
            DisplayYamlData();
        }
    }

    nodes.insert(file_name, root);
}

void MainWindow::SlotShortcutCtrlF()
{
    if (replace_wnd) {
        replace_wnd->close();
        replace_wnd = nullptr;
    }

    if (search_wnd) {
        search_wnd->activateWindow();
    } else {
        search_wnd = new searchingWindow();

        search_wnd->setAttribute(Qt::WA_DeleteOnClose);

        connect(search_wnd, &QObject::destroyed, this, [=]() { search_wnd = nullptr; });

        connect(search_wnd, &searchingWindow::searchingText, this, &MainWindow::SearchingText);

        search_wnd->show();
    }
}

void MainWindow::SlotShortcutF11()
{
    if (this->isFullScreen()) {
        this->showNormal();
    } else {
        this->showFullScreen();
    }
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

    if (replace_wnd)
        replace_wnd->activateWindow();
    else {
        replace_wnd = new ReplaceWindow();

        replace_wnd->setAttribute(Qt::WA_DeleteOnClose);

        connect(replace_wnd, &QObject::destroyed, this, [=]() { replace_wnd = nullptr; });

        connect(replace_wnd,
                &ReplaceWindow::searchReplaceText,
                this,
                &MainWindow::SearchReplaceText);

        connect(replace_wnd, &ReplaceWindow::replaceText, this, &MainWindow::ReplaceText);

        replace_wnd->show();
    }
}

void MainWindow::Displaykeys(YamlNode root)
{
    ClearKeysArea();

    int row_lvl = 0;
    int col_lvl = 0;

    keys.clear();

    for (const auto &node : root.children) {
        CollectKeys(node, keys);
    }

    QList<QString> sorted_keys = keys.values();
    std::sort(sorted_keys.begin(), sorted_keys.end());

    for (const QString &key : sorted_keys) {
        CreateCheckBox(key, row_lvl, col_lvl);

        col_lvl++;
        if (col_lvl > 1) {
            col_lvl = 0;
            row_lvl++;
        }
    }
}

void MainWindow::on_pushButton_clicked()
{
    SaveData(ui->fileNamecmb->currentText());
}

void MainWindow::on_pushButton_2_clicked()
{
    DisplayYamlData();
}

void MainWindow::onCheckBoxStateChanged(int state)
{
    QCheckBox *check_box = qobject_cast<QCheckBox *>(sender());
    if (check_box) {
        QString key = check_box->text();
        check_box_states[key] = (state == Qt::Checked);
    }
}

void MainWindow::UpdateValue(const QString &path, const QString &newValue, bool isKey)
{
    QStringList keys = path.split('.');
    YamlNode *current_node = &root;

    for (const QString &key : keys) {
        bool found = false;
        for (YamlNode &child : current_node->children) {
            if (child.key == key) {
                current_node = &child;
                found = true;
                break;
            }
        }
        if (!found) {
            return;
        }
    }

    if (isKey)
        current_node->key = newValue;
    else
        current_node->value = newValue;

    is_update_file = true;
}

void MainWindow::HandleAddKeyValue(QString path, QString newValue, bool isKey)
{
    if (isKey) {
        root.AddValueToKey(path, newValue);
    } else {
        root.AddKeyWithValue(path, newValue);
    }

    is_update_file = true;

    SaveData(previous_text_cmb);

    ReadFile();
}

void MainWindow::HandleDeleteElement(QString path, bool isKey)
{
    if (isKey) {
        root.RemoveKey(path);
    } else {
        root.RemoveValue(path);
    }

    is_update_file = true;

    SaveData(previous_text_cmb);

    ReadFile();
}

void MainWindow::DisplayTreeNode(const YamlNode &node,
                                 const QString &parentPath,
                                 const QString &searchText,
                                 QTreeWidgetItem *parentItem,
                                 QTreeWidget *treeWidget,
                                 bool useRegex)
{
    if (check_box_states.contains(node.key) && !check_box_states[node.key]) {
        return;
    }

    QString currentPath = parentPath.isEmpty() ? node.key : parentPath + "." + node.key;

    QTreeWidgetItem *tree_item = new QTreeWidgetItem();
    tree_item->setText(0, node.key);

    if (parentItem) {
        parentItem->addChild(tree_item);
    } else {
        treeWidget->addTopLevelItem(tree_item);
    }

    CustomLineEdit *key_txt = new CustomLineEdit(this, currentPath, true);

    if (node.key.isEmpty()) {
        key_txt->setText("-");
        key_txt->setReadOnly(true);
    } else {
        key_txt->setText(node.key);
    }
    key_txt->setStyleSheet(
        "QLineEdit { border: none; font-size: 16px; color: rgb(98, 127, 255); }");
    treeWidget->setItemWidget(tree_item, 0, key_txt);

    connect(key_txt, &CustomLineEdit::AddKeyValue, this, &MainWindow::HandleAddKeyValue);
    connect(key_txt, &CustomLineEdit::DeleteElement, this, &MainWindow::HandleDeleteElement);

    connect(key_txt, &QLineEdit::textChanged, this, [=](const QString &newValue) {
        UpdateValue(currentPath, newValue, true);
    });

    bool key_matches = false;
    bool value_matches = false;

    if (useRegex) {
        QRegularExpression regex(searchText,
                                 cs ? QRegularExpression::NoPatternOption
                                    : QRegularExpression::CaseInsensitiveOption);

        key_matches = regex.match(node.key).hasMatch();
        value_matches = regex.match(node.value).hasMatch();
    } else {
        key_matches = node.key.contains(searchText, cs);
        value_matches = node.value.contains(searchText, cs);
    }

    if (!searchText.isEmpty() && key_matches) {
        found_widgets.append(key_txt);
    }

    if (!node.value.isEmpty() || node.children.isEmpty()) {
        CustomLineEdit *value_txt = new CustomLineEdit(this, currentPath, false);
        value_txt->setText(node.value);
        value_txt->setStyleSheet("QLineEdit { border: none; font-size: 14px; color: white; }");
        treeWidget->setItemWidget(tree_item, 1, value_txt);

        connect(value_txt, &CustomLineEdit::AddKeyValue, this, &MainWindow::HandleAddKeyValue);
        connect(value_txt, &CustomLineEdit::DeleteElement, this, &MainWindow::HandleDeleteElement);

        connect(value_txt, &QLineEdit::textChanged, this, [=](const QString &newValue) {
            UpdateValue(currentPath, newValue, false);
        });

        if (!searchText.isEmpty() && value_matches) {
            found_widgets.append(value_txt);
        }
    }

    for (const auto &child : node.children) {
        DisplayTreeNode(child, currentPath, searchText, tree_item, treeWidget, useRegex);
    }
}

void MainWindow::CreateCheckBox(const QString &name, int row, int col)
{
    QCheckBox *check_box = new QCheckBox(name, this);
    check_box->setStyleSheet(
        "QCheckBox{ font-size: 24px;} QCheckBox::indicator { width: 20px; height: "
        "20px; "
        "background-color: #525252; "
        "color: #37A2C3; border: 1px solid #00AADF; border-radius: 10px; } "
        "QCheckBox::indicator:checked { border: 1px solid #3CC7F2; background-color: "
        "#51B4D2; } ");

    ui->gridLayout_2->addWidget(check_box, row, col);

    if (name == "==") {
        check_box->setChecked(false);
        check_box_states[name] = false;
    } else {
        check_box->setChecked(true);
        check_box_states[name] = true;
    }

    connect(check_box, &QCheckBox::stateChanged, this, &MainWindow::onCheckBoxStateChanged);
}

void MainWindow::ClearKeysArea()
{
    QLayoutItem *item;
    while ((item = ui->gridLayout_2->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void MainWindow::ClearTreeWidget()
{
    tree_widget = new QTreeWidget(this);

    tree_widget->setColumnCount(2);
    tree_widget->setHeaderLabels(QStringList() << "Key"
                                               << "Value");
    tree_widget->setColumnWidth(0, 200);
    tree_widget->setStyleSheet("QHeaderView {background-color: rgb(48, "
                               "48, 48); color: black;} QWidget {background-color: rgb(48, "
                               "48, 48); color: black;}");
}

void MainWindow::ClearTabWidget(QWidget *tab)
{
    if (tab) {
        QVBoxLayout *layout = dynamic_cast<QVBoxLayout *>(tab->layout());
        if (layout) {
            QLayoutItem *item;
            while ((item = layout->takeAt(0)) != nullptr) {
                delete item->widget();
                delete item;
            }

            layout->addWidget(tree_widget);
        }
    }
}

void MainWindow::RemoveTab(const QString &tabText)
{
    int index = -1;

    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == tabText) {
            index = i;
            break;
        }
    }

    if (index != -1) {
        ui->tabWidget->removeTab(index);
    } else {
        qDebug() << "Tab with text" << tabText << "not found!";
    }
}

void MainWindow::SearchingText(const QString &text, bool isSensitive, bool isDownward, bool useRegex)
{
    Qt::CaseSensitivity new_cs = isSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QRegularExpression regex;
    if (useRegex) {
        QRegularExpression::PatternOption option = isSensitive
                                                       ? QRegularExpression::NoPatternOption
                                                       : QRegularExpression::CaseInsensitiveOption;
        regex = QRegularExpression(text, option);
    }

    if ((useRegex && regex.pattern() != searching_regex.pattern())
        || (!useRegex && searching_text != text) || new_cs != cs) {
        cs = new_cs;
        searching_text = text;
        searching_regex = regex;

        ClearTreeWidget();
        found_widgets.clear();
        current_found_index = -1;
        starting_index = -1;
        previous_widget = nullptr;

        for (const auto &node : root.children) {
            DisplayTreeNode(node, "", text, nullptr, tree_widget, useRegex);
        }

        QWidget *current_tab = ui->tabWidget->currentWidget();

        ClearTabWidget(current_tab);

        if (!found_widgets.isEmpty()) {
            current_found_index = isDownward ? 0 : found_widgets.size() - 1;
            HighlightCurrentFound();
        }
        return;
    }

    if (starting_index == -1) {
        starting_index = current_found_index;
    }

    if (isDownward) {
        current_found_index++;
        if (current_found_index >= found_widgets.size()) {
            current_found_index = 0;
        }
    } else {
        current_found_index--;
        if (current_found_index < 0) {
            current_found_index = found_widgets.size() - 1;
        }
    }

    if (current_found_index == starting_index) {
        QMessageBox::information(this, "Search", "Reached the end of the search results.");
        starting_index = -1;
        return;
    }

    HighlightCurrentFound();
}

void MainWindow::SearchReplaceText(const QString &text, bool isSensitive, bool useRegex)
{
    Qt::CaseSensitivity new_cs = isSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QRegularExpression regex;
    if (useRegex) {
        QRegularExpression::PatternOption option = isSensitive
                                                       ? QRegularExpression::NoPatternOption
                                                       : QRegularExpression::CaseInsensitiveOption;
        regex = QRegularExpression(text, option);
    }

    if ((useRegex && regex.pattern() != searching_regex.pattern())
        || (!useRegex && searching_text != text) || new_cs != cs) {
        cs = new_cs;
        searching_text = text;
        searching_regex = regex;

        ClearTreeWidget();
        found_widgets.clear();
        current_found_index = -1;
        starting_index = -1;
        previous_widget = nullptr;

        for (const auto &node : root.children) {
            DisplayTreeNode(node, "", text, nullptr, tree_widget, useRegex);
        }

        QWidget *current_tab = ui->tabWidget->currentWidget();

        ClearTabWidget(current_tab);

        if (!found_widgets.isEmpty()) {
            current_found_index = 0;
            HighlightCurrentFound();
        }
        return;
    }

    if (starting_index == -1) {
        starting_index = current_found_index;
    }

    current_found_index++;

    if (current_found_index >= found_widgets.size()) {
        current_found_index = 0;
    }

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
    SearchReplaceText(findText, cs, useRegex);

    if (allText) {
        for (QWidget *widget : found_widgets) {
            ReplaceInWidget(widget, findText, replaceText, useRegex);
        }

        searching_text = "";
    } else {
        if (current_found_index >= 0 && current_found_index < found_widgets.size()) {
            QWidget *current_widget = found_widgets[current_found_index];
            ReplaceInWidget(current_widget, findText, replaceText, useRegex);
        }
    }
}

void MainWindow::HighlightCurrentFound()
{
    if (current_found_index >= 0 && current_found_index < found_widgets.size()) {
        QWidget *current_widget = found_widgets[current_found_index];

        if (!current_widget) {
            qWarning() << "Current widget is null!";
            return;
        }

        if (previous_widget) {
            previous_widget->setStyleSheet(previous_widget_original_style_sheet);
        }

        QString current_style_sheet = current_widget->styleSheet();

        previous_widget = current_widget;
        previous_widget_original_style_sheet = current_style_sheet;

        static QRegularExpression regex("color:\\s*[^;]+;");
        QString newColor = "color: blue;";

        if (regex.match(current_style_sheet).hasMatch()) {
            current_style_sheet.replace(regex, newColor);
        } else {
            current_style_sheet += " " + newColor;
        }

        current_widget->setStyleSheet(current_style_sheet);

        ScrollIntoView(current_widget);

    } else {
        QMessageBox::information(this, "Editor", "Can't find it \"" + searching_text + "\"");
    }
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

        if (useRegex) {
            text.replace(searching_regex, replaceText);
        } else {
            text.replace(findText, replaceText, cs);
        }
        line_edit->setText(text);
    }
}

void MainWindow::OnFolderChanged(const QString &path)
{
    QDir dir(path);
    QStringList files = dir.entryList(QStringList() << "*.yaml"
                                                    << "*.yml",
                                      QDir::Files);

    for (const QString &file : files) {
        if (ui->fileNamecmb->findText(file) == -1)
            ui->fileNamecmb->addItem(file);
    }

    for (int i = 0; i < ui->fileNamecmb->count(); ++i) {
        QString combo_file = ui->fileNamecmb->itemText(i);
        if (!files.contains(combo_file)) {
            ui->fileNamecmb->removeItem(i);
            RemoveTab(combo_file);
            --i;
        }
    }
}

void MainWindow::on_OpenFolderYmlFilebtn_clicked()
{
    QDesktopServices::openUrl(QDir::currentPath() + "/ymlFiles");
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    QString file_name = ui->tabWidget->tabText(index);

    ui->fileNamecmb->setCurrentIndex(ui->fileNamecmb->findText(file_name));

    if (nodes.contains(file_name)) {
        root = nodes.value(file_name);
        Displaykeys(root);
    }
}
