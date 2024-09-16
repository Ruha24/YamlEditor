#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    yandexApi = new YandexApi();
    yamlReader = new YamlReader();

    connect(yamlReader, &YamlReader::fileUploaded, this, [&](bool success) {
        if (success) {
            QMessageBox::information(this, "Saving", "Your file is saved");
        }
    });

    keyF11 = new QShortcut(this);
    keyF11->setKey(Qt::Key_F11);

    connect(keyF11, &QShortcut::activated, this, &MainWindow::slotShortcutF11);

    keyCtrlF = new QShortcut(this);
    keyCtrlF->setKey(Qt::CTRL | Qt::Key_F);

    connect(keyCtrlF, &QShortcut::activated, this, &MainWindow::slotShortcutCtrlF);

    keyCtrlS = new QShortcut(this);
    keyCtrlS->setKey(Qt::CTRL | Qt::Key_S);

    connect(keyCtrlS, &QShortcut::activated, this, &MainWindow::slotShortcutCtrlS);

    keyCtrlR = new QShortcut(this);
    keyCtrlR->setKey(Qt::CTRL | Qt::Key_R);

    connect(keyCtrlR, &QShortcut::activated, this, &MainWindow::slotShortcutCtrlR);

    fileLocalSystem = new FileSystem(QDir::currentPath() + "/ymlFiles");

    localFiles = fileLocalSystem->getFiles();

    connect(yandexApi, &YandexApi::newFile, this, &MainWindow::uploadFileOnCmb);

    yandexApi->getFiles();

    previousTextCmb = ui->fileNamecmb->currentText();

    fileWatcher = new QFileSystemWatcher();

    fileWatcher->addPath(QDir::currentPath() + "/ymlFiles");

    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onFolderChanged);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
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
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();

        for (const QUrl &url : urlList) {
            QString fileName = url.fileName();

            QString suffixFile = QFileInfo(fileName).suffix();

            if (suffixFile == "yaml" || suffixFile == "yml")
                if (ui->fileNamecmb->findText(fileName) == -1)
                    ui->fileNamecmb->addItem(fileName);
        }
    }
}

void MainWindow::on_fileNamecmb_currentIndexChanged(int index)
{
    ui->fileNamecmb->setCurrentIndex(index);

    if (isUpdateFile && !root.children.isEmpty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  "Save File",
                                                                  "Do you want to save the file?",
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            saveData(previousTextCmb);
        }
    }

    previousTextCmb = ui->fileNamecmb->currentText();

    readFile();
}

void MainWindow::saveData(const QString &fileName)
{
    if (isUpdateFile) {
        QString fullPath = QDir::currentPath() + "/ymlFiles/" + fileName;

        yamlReader->saveValues(root, fullPath);

        isUpdateFile = false;
    }
}

void MainWindow::uploadFileOnCmb(const QString &file)
{
    QString filePath = QDir::currentPath() + "/ymlFiles/" + file;
    QString fileHash = fileLocalSystem->calculateFileCheckSum(filePath);

    bool fileExists = false;

    if (localFiles.isEmpty()) {
        ui->fileNamecmb->addItem(file);
        localFiles.append(file);
    } else {
        for (const QString &localFile : localFiles) {
            QString localFilePath = QDir::currentPath() + "/ymlFiles/" + localFile;
            QString localHash = fileLocalSystem->calculateFileCheckSum(localFilePath);

            if (localFile == file && localHash == fileHash) {
                ui->fileNamecmb->addItem(localFile);
                fileExists = true;
                break;
            }
        }

        if (!fileExists) {
            if (ui->fileNamecmb->findText(file) == -1) {
                localFiles.append(file);
                ui->fileNamecmb->addItem(file);
            }
        }
    }
}

void MainWindow::closeTab(int index)
{
    if (index >= 0 && index < ui->tabWidget->count()) {
        QString fileName = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

        saveData(fileName);

        nodes.remove(fileName);

        QWidget *tab = ui->tabWidget->widget(index);
        ui->tabWidget->removeTab(index);
        delete tab;
    }
}
void MainWindow::displayYamlData()
{
    root = yamlReader->getRootNode();

    displaykeys(root);

    QWidget *newTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(newTab);

    clearTreeWidget();

    for (const auto &node : root.children) {
        displayTreeNode(node, "", "", nullptr, treeWidget, false);
    }

    layout->addWidget(treeWidget);
    newTab->setLayout(layout);

    ui->tabWidget->addTab(newTab, ui->fileNamecmb->currentText());
    ui->tabWidget->setCurrentWidget(newTab);
}

int MainWindow::findTabByName(const QString &fileName)
{
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == fileName) {
            return i;
        }
    }
    return -1;
}

void MainWindow::collectKeys(const YamlNode &node, QSet<QString> &keys)
{
    QString key = node.key;
    QString value = node.value;

    if (key.isEmpty())
        key = "-";

    if (node.children.isEmpty()) {
        keys.insert(key);
    }

    for (const auto &subNode : node.children) {
        collectKeys(subNode, keys);
    }
}

void MainWindow::readFile()
{
    QString fileName = ui->fileNamecmb->currentText();

    int existingTabIndex = findTabByName(fileName);

    if (existingTabIndex != -1) {
        root = nodes.value(fileName);
        displaykeys(root);
        ui->tabWidget->setCurrentIndex(existingTabIndex);
    } else {
        if (yamlReader->readFile(fileName)) {
            displayYamlData();
        }
    }

    nodes.insert(fileName, root);
}

void MainWindow::slotShortcutCtrlF()
{
    if (replaceWnd) {
        replaceWnd->close();
        replaceWnd = nullptr;
    }

    if (searchWnd) {
        searchWnd->activateWindow();
    } else {
        searchWnd = new searchingWindow();

        searchWnd->setAttribute(Qt::WA_DeleteOnClose);

        connect(searchWnd, &QObject::destroyed, this, [=]() { searchWnd = nullptr; });

        connect(searchWnd, &searchingWindow::searchingText, this, &MainWindow::searchingText);

        searchWnd->show();
    }
}

void MainWindow::slotShortcutF11()
{
    if (this->isFullScreen()) {
        this->showNormal();
    } else {
        this->showFullScreen();
    }
}

void MainWindow::slotShortcutCtrlS()
{
    saveData(ui->fileNamecmb->currentText());
}

void MainWindow::slotShortcutCtrlR()
{
    if (searchWnd) {
        searchWnd->close();
        searchWnd = nullptr;
    }

    if (replaceWnd)
        replaceWnd->activateWindow();
    else {
        replaceWnd = new ReplaceWindow();

        replaceWnd->setAttribute(Qt::WA_DeleteOnClose);

        connect(replaceWnd, &QObject::destroyed, this, [=]() { replaceWnd = nullptr; });

        connect(replaceWnd, &ReplaceWindow::searchReplaceText, this, &MainWindow::searchReplaceText);

        connect(replaceWnd, &ReplaceWindow::replaceText, this, &MainWindow::replaceText);

        replaceWnd->show();
    }
}

void MainWindow::displaykeys(YamlNode root)
{
    clearKeysArea();

    int row_lvl = 0;
    int col_lvl = 0;

    keys.clear();

    for (const auto &node : root.children) {
        collectKeys(node, keys);
    }

    QList<QString> sortedKeys = keys.values();
    std::sort(sortedKeys.begin(), sortedKeys.end());

    for (const QString &key : sortedKeys) {
        createCheckBox(key, row_lvl, col_lvl);

        col_lvl++;
        if (col_lvl > 1) {
            col_lvl = 0;
            row_lvl++;
        }
    }
}

void MainWindow::on_pushButton_clicked()
{
    saveData(ui->fileNamecmb->currentText());
}

void MainWindow::on_pushButton_2_clicked()
{
    displayYamlData();
}

void MainWindow::onCheckBoxStateChanged(int state)
{
    QCheckBox *checkBox = qobject_cast<QCheckBox *>(sender());
    if (checkBox) {
        QString key = checkBox->text();
        checkBoxStates[key] = (state == Qt::Checked);
    }
}

void MainWindow::updateValue(const QString &path, const QString &newValue, bool isKey)
{
    QStringList keys = path.split('.');
    YamlNode *currentNode = &root;

    for (const QString &key : keys) {
        bool found = false;
        for (YamlNode &child : currentNode->children) {
            if (child.key == key) {
                currentNode = &child;
                found = true;
                break;
            }
        }
        if (!found) {
            return;
        }
    }

    if (isKey)
        currentNode->key = newValue;
    else
        currentNode->value = newValue;

    isUpdateFile = true;
}

void MainWindow::handleAddKeyValue(QString path, QString newValue, bool isKey)
{
    if (isKey) {
        root.addValueToKey(path, newValue);
    } else {
        root.addKeyWithValue(path, newValue);
    }

    isUpdateFile = true;

    saveData(previousTextCmb);

    readFile();
}

void MainWindow::handleDeleteElement(QString path, bool isKey)
{
    if (isKey) {
        root.removeKey(path);
    } else {
        root.removeValue(path);
    }

    isUpdateFile = true;

    saveData(previousTextCmb);

    readFile();
}

void MainWindow::displayTreeNode(const YamlNode &node,
                                 const QString &parentPath,
                                 const QString &searchText,
                                 QTreeWidgetItem *parentItem,
                                 QTreeWidget *treeWidget,
                                 bool useRegex)
{
    if (checkBoxStates.contains(node.key) && !checkBoxStates[node.key]) {
        return;
    }

    QString currentPath = parentPath.isEmpty() ? node.key : parentPath + "." + node.key;

    QTreeWidgetItem *treeItem = new QTreeWidgetItem();
    treeItem->setText(0, node.key);

    if (parentItem) {
        parentItem->addChild(treeItem);
    } else {
        treeWidget->addTopLevelItem(treeItem);
    }

    CustomLineEdit *keytxt = new CustomLineEdit(this, currentPath, true);

    if (node.key.isEmpty()) {
        keytxt->setText("-");
        keytxt->setReadOnly(true);
    } else {
        keytxt->setText(node.key);
    }
    keytxt->setStyleSheet("QLineEdit { border: none; font-size: 16px; color: rgb(98, 127, 255); }");
    treeWidget->setItemWidget(treeItem, 0, keytxt);

    connect(keytxt, &CustomLineEdit::addKeyValue, this, &MainWindow::handleAddKeyValue);
    connect(keytxt, &CustomLineEdit::deleteElement, this, &MainWindow::handleDeleteElement);

    connect(keytxt, &QLineEdit::textChanged, this, [=](const QString &newValue) {
        updateValue(currentPath, newValue, true);
    });

    bool keyMatches = false;
    bool valueMatches = false;

    if (useRegex) {
        QRegularExpression regex(searchText,
                                 cs ? QRegularExpression::NoPatternOption
                                    : QRegularExpression::CaseInsensitiveOption);

        keyMatches = regex.match(node.key).hasMatch();
        valueMatches = regex.match(node.value).hasMatch();
    } else {
        keyMatches = node.key.contains(searchText, cs);
        valueMatches = node.value.contains(searchText, cs);
    }

    if (!searchText.isEmpty() && keyMatches) {
        foundWidgets.append(keytxt);
    }

    if (!node.value.isEmpty() || node.children.isEmpty()) {
        CustomLineEdit *valuetxt = new CustomLineEdit(this, currentPath, false);
        valuetxt->setText(node.value);
        valuetxt->setStyleSheet("QLineEdit { border: none; font-size: 14px; color: white; }");
        treeWidget->setItemWidget(treeItem, 1, valuetxt);

        connect(valuetxt, &CustomLineEdit::addKeyValue, this, &MainWindow::handleAddKeyValue);
        connect(valuetxt, &CustomLineEdit::deleteElement, this, &MainWindow::handleDeleteElement);

        connect(valuetxt, &QLineEdit::textChanged, this, [=](const QString &newValue) {
            updateValue(currentPath, newValue, false);
        });

        if (!searchText.isEmpty() && valueMatches) {
            foundWidgets.append(valuetxt);
        }
    }

    for (const auto &child : node.children) {
        displayTreeNode(child, currentPath, searchText, treeItem, treeWidget, useRegex);
    }
}

void MainWindow::createCheckBox(const QString &name, int row, int col)
{
    QCheckBox *checkBox = new QCheckBox(name, this);
    checkBox->setStyleSheet(
        "QCheckBox{ font-size: 24px;} QCheckBox::indicator { width: 20px; height: "
        "20px; "
        "background-color: #525252; "
        "color: #37A2C3; border: 1px solid #00AADF; border-radius: 10px; } "
        "QCheckBox::indicator:checked { border: 1px solid #3CC7F2; background-color: "
        "#51B4D2; } ");

    ui->gridLayout_2->addWidget(checkBox, row, col);

    if (name == "==") {
        checkBox->setChecked(false);
        checkBoxStates[name] = false;
    } else {
        checkBox->setChecked(true);
        checkBoxStates[name] = true;
    }

    connect(checkBox, &QCheckBox::stateChanged, this, &MainWindow::onCheckBoxStateChanged);
}

void MainWindow::clearKeysArea()
{
    QLayoutItem *item;
    while ((item = ui->gridLayout_2->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void MainWindow::clearTreeWidget()
{

    treeWidget = new QTreeWidget(this);

    treeWidget->setColumnCount(2);
    treeWidget->setHeaderLabels(QStringList() << "Key"
                                              << "Value");
    treeWidget->setColumnWidth(0, 200);
    treeWidget->setStyleSheet("QHeaderView {background-color: rgb(48, "
                              "48, 48); color: black;} QWidget {background-color: rgb(48, "
                              "48, 48); color: black;}");
}

void MainWindow::removeTab(const QString &tabText)
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

void MainWindow::searchingText(const QString &text,
                               bool isSensitive,
                               bool is_downward,
                               bool useRegex)
{
    Qt::CaseSensitivity newCs = isSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QRegularExpression regex;
    if (useRegex) {
        QRegularExpression::PatternOption option = isSensitive
                                                       ? QRegularExpression::NoPatternOption
                                                       : QRegularExpression::CaseInsensitiveOption;
        regex = QRegularExpression(text, option);
    }

    if ((useRegex && regex.pattern() != searchingRegex.pattern())
        || (!useRegex && searching_text != text) || newCs != cs) {
        cs = newCs;
        searching_text = text;
        searchingRegex = regex;

        clearTreeWidget();
        foundWidgets.clear();
        currentFoundIndex = -1;
        startingIndex = -1;
        previousWidget = nullptr;

        for (const auto &node : root.children) {
            displayTreeNode(node, "", text, nullptr, treeWidget, useRegex);
        }

        QWidget *currentTab = ui->tabWidget->currentWidget();

        if (currentTab) {
            QVBoxLayout *layout = dynamic_cast<QVBoxLayout *>(currentTab->layout());
            if (layout) {
                QLayoutItem *item;
                while ((item = layout->takeAt(0)) != nullptr) {
                    delete item->widget();
                    delete item;
                }

                layout->addWidget(treeWidget);
            }
        }

        if (!foundWidgets.isEmpty()) {
            currentFoundIndex = is_downward ? 0 : foundWidgets.size() - 1;
            highlightCurrentFound();
        }
        return;
    }

    if (startingIndex == -1) {
        startingIndex = currentFoundIndex;
    }

    if (is_downward) {
        currentFoundIndex++;
        if (currentFoundIndex >= foundWidgets.size()) {
            currentFoundIndex = 0;
        }
    } else {
        currentFoundIndex--;
        if (currentFoundIndex < 0) {
            currentFoundIndex = foundWidgets.size() - 1;
        }
    }

    if (currentFoundIndex == startingIndex) {
        QMessageBox::information(this, "Search", "Reached the end of the search results.");
        startingIndex = -1;
        return;
    }

    highlightCurrentFound();
}

void MainWindow::searchReplaceText(const QString &text, bool isSensitive, bool useRegex)
{
    Qt::CaseSensitivity newCs = isSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QRegularExpression regex;
    if (useRegex) {
        QRegularExpression::PatternOption option = isSensitive
                                                       ? QRegularExpression::NoPatternOption
                                                       : QRegularExpression::CaseInsensitiveOption;
        regex = QRegularExpression(text, option);
    }

    if ((useRegex && regex.pattern() != searchingRegex.pattern())
        || (!useRegex && searching_text != text) || newCs != cs) {
        cs = newCs;
        searching_text = text;
        searchingRegex = regex;

        clearTreeWidget();
        foundWidgets.clear();
        currentFoundIndex = -1;
        startingIndex = -1;
        previousWidget = nullptr;

        for (const auto &node : root.children) {
            displayTreeNode(node, "", text, nullptr, treeWidget, useRegex);
        }

        QWidget *currentTab = ui->tabWidget->currentWidget();

        if (currentTab) {
            QVBoxLayout *layout = dynamic_cast<QVBoxLayout *>(currentTab->layout());
            if (layout) {
                QLayoutItem *item;
                while ((item = layout->takeAt(0)) != nullptr) {
                    delete item->widget();
                    delete item;
                }

                layout->addWidget(treeWidget);
            }
        }

        if (!foundWidgets.isEmpty()) {
            currentFoundIndex = 0;
            highlightCurrentFound();
        }
        return;
    }

    if (startingIndex == -1) {
        startingIndex = currentFoundIndex;
    }

    currentFoundIndex++;

    if (currentFoundIndex >= foundWidgets.size()) {
        currentFoundIndex = 0;
    }

    if (currentFoundIndex == startingIndex) {
        startingIndex = -1;
        return;
    }

    highlightCurrentFound();
}

void MainWindow::replaceText(const QString &findText,
                             const QString &replaceText,
                             bool allText,
                             bool useRegex)
{
    searchReplaceText(findText, cs, useRegex);

    if (allText) {
        for (QWidget *widget : foundWidgets) {
            replaceInWidget(widget, findText, replaceText, useRegex);
        }

        searching_text = "";
    } else {
        if (currentFoundIndex >= 0 && currentFoundIndex < foundWidgets.size()) {
            QWidget *currentWidget = foundWidgets[currentFoundIndex];
            replaceInWidget(currentWidget, findText, replaceText, useRegex);
        }
    }
}

void MainWindow::highlightCurrentFound()
{
    if (currentFoundIndex >= 0 && currentFoundIndex < foundWidgets.size()) {
        QWidget *currentWidget = foundWidgets[currentFoundIndex];

        if (!currentWidget) {
            qWarning() << "Current widget is null!";
            return;
        }

        if (previousWidget) {
            previousWidget->setStyleSheet(previousWidgetOriginalStyleSheet);
        }

        QString currentStyleSheet = currentWidget->styleSheet();

        previousWidget = currentWidget;
        previousWidgetOriginalStyleSheet = currentStyleSheet;

        static QRegularExpression regex("color:\\s*[^;]+;");
        QString newColor = "color: blue;";

        if (regex.match(currentStyleSheet).hasMatch()) {
            currentStyleSheet.replace(regex, newColor);
        } else {
            currentStyleSheet += " " + newColor;
        }

        currentWidget->setStyleSheet(currentStyleSheet);

        scrollIntoView(currentWidget);

    } else {
        QMessageBox::information(this, "Editor", "Can't find it \"" + searching_text + "\"");
    }
}

void MainWindow::scrollIntoView(QWidget *widget)
{
    QTreeWidgetItemIterator it(treeWidget);
    while (*it) {
        QTreeWidgetItem *item = *it;

        if (treeWidget->itemWidget(item, 0) == widget || treeWidget->itemWidget(item, 1) == widget) {
            treeWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            return;
        }
        ++it;
    }
}

void MainWindow::replaceInWidget(QWidget *widget,
                                 const QString &findText,
                                 const QString &replaceText,
                                 bool useRegex)
{
    if (!widget)
        return;

    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
        QString text = lineEdit->text();

        if (useRegex) {
            text.replace(searchingRegex, replaceText);
        } else {
            text.replace(findText, replaceText, cs);
        }
        lineEdit->setText(text);
    }
}

void MainWindow::onFolderChanged(const QString &path)
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
        QString comboFile = ui->fileNamecmb->itemText(i);
        if (!files.contains(comboFile)) {
            ui->fileNamecmb->removeItem(i);
            removeTab(comboFile);
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
    QString fileName = ui->tabWidget->tabText(index);

    ui->fileNamecmb->setCurrentIndex(ui->fileNamecmb->findText(fileName));

    if (nodes.contains(fileName)) {
        root = nodes.value(fileName);
        displaykeys(root);
    }
}
