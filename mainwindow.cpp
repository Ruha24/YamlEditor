#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPalette>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mainWidget = nullptr;
    mainLayout = nullptr;
    treeWidget = nullptr;

    ui->stackedWidget->setCurrentIndex(0);

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

    yandexApi->getFiles([&](bool success) {
        FileSystem *fileSystem = new FileSystem(QDir::currentPath() + "/ymlFiles");

        if (success) {
            QList<QString> localFiles = fileSystem->getFiles();
            QList<QString> yandexFiles = yandexApi->getListFileName();

            if (localFiles.isEmpty()) {
                for (const QString &yandexFile : yandexFiles) {
                    ui->fileNamecmb->addItem(yandexFile);
                }
            } else {
                for (const QString &localFile : localFiles) {
                    QString localFilePath = QDir::currentPath() + "/ymlFiles/" + localFile;
                    QString localHash = fileSystem->calculateFileCheckSum(localFilePath);

                    bool foundMatch = false;

                    for (const QString &yandexFile : yandexFiles) {
                        if (localFile == yandexFile) {
                            QString yandexFilePath = QDir::currentPath() + "/ymlFiles/"
                                                     + yandexFile;
                            QString yandexHash = fileSystem->calculateFileCheckSum(yandexFilePath);

                            if (localHash == yandexHash) {
                                ui->fileNamecmb->addItem(localFile);
                            } else {
                                ui->fileNamecmb->addItem(yandexFile);
                            }

                            foundMatch = true;
                            break;
                        }
                    }

                    if (!foundMatch) {
                        ui->fileNamecmb->addItem(localFile);
                    }
                }

                for (const QString &yandexFile : yandexFiles) {
                    if (!localFiles.contains(yandexFile)) {
                        ui->fileNamecmb->addItem(yandexFile);
                    }
                }
            }
        }

        delete fileSystem;
    });

    previousTextCmb = ui->fileNamecmb->currentText();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_fileNamecmb_currentIndexChanged(int index)
{
    ui->fileNamecmb->setCurrentIndex(index);

    if (!root.children.isEmpty()) {
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
    QString fullPath = QDir::currentPath() + "/ymlFiles/" + fileName;

    yamlReader->saveValues(root, fullPath);
}

void MainWindow::displayYamlData()
{
    clearScrollArea();

    root = yamlReader->getRootNode();
    displayedKeys.clear();

    if (ui->treechb->isChecked()) {
        for (const auto &node : root.children) {
            displayTreeNode(node, "", "", nullptr, treeWidget, false);
        }

        ui->verticalLayout_3->addWidget(treeWidget);
    } else if (ui->prettychb->isChecked()) {
        for (const auto &node : root.children) {
            displayNode(node, "", "", false);
        }

        ui->verticalLayout_2->addWidget(mainWidget);
    }
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
    if (yamlReader->readFile(ui->fileNamecmb->currentText())) {
        displayYamlData();
        displaykeys();
    }
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

        connect(replaceWnd, &ReplaceWindow::searchReplaceText, this, &MainWindow::searchReplaceText);

        connect(replaceWnd, &ReplaceWindow::replaceText, this, &MainWindow::replaceText);

        replaceWnd->show();
    }
}

void MainWindow::displaykeys()
{
    clearKeysArea();

    int row_lvl = 0;
    int col_lvl = 0;

    keys.clear();

    for (const auto &node : root.children) {
        collectKeys(node, keys);
    }

    for (const QString &key : qAsConst(keys)) {
        createCheckBox(key, row_lvl, col_lvl);

        col_lvl++;
        if (col_lvl > 1) {
            col_lvl = 0;
            row_lvl++;
        }
    }

    displayYamlData();
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
}

void MainWindow::handleAddKeyValue(QString path, QString newValue, bool isKey)
{
    if (isKey) {
        root.addValueToKey(path, newValue);
    } else {
        root.addKeyWithValue(path, newValue);
    }

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

    saveData(previousTextCmb);

    readFile();
}

void MainWindow::displayNode(const YamlNode &node,
                             const QString &parentPath,
                             const QString &searchText,
                             bool useRegex,
                             int depth)
{
    QString currentPath = parentPath.isEmpty() ? node.key : parentPath + "." + node.key;

    QRegularExpression regex;
    if (useRegex && !searchText.isEmpty()) {
        QRegularExpression::PatternOption option = cs == Qt::CaseSensitive
                                                       ? QRegularExpression::NoPatternOption
                                                       : QRegularExpression::CaseInsensitiveOption;
        regex = QRegularExpression(searchText, option);
    }

    if (!displayedKeys.contains(currentPath)
        && (!node.children.isEmpty() || !node.value.trimmed().isEmpty())) {
        displayedKeys.insert(currentPath);

        CustomLineEdit *title = new CustomLineEdit(this, currentPath, true);
        title->setText(node.key);

        QMargins margins = title->contentsMargins();
        margins.setLeft(depth * 20);
        title->setContentsMargins(margins);

        bool matchFound = useRegex ? regex.match(node.key).hasMatch()
                                   : node.key.contains(searchText, cs);

        connect(title, &CustomLineEdit::addKeyValue, this, &MainWindow::handleAddKeyValue);
        connect(title, &CustomLineEdit::deleteElement, this, &MainWindow::handleDeleteElement);

        if (!searchText.isEmpty() && matchFound) {
            foundWidgets.append(title);
        }
        title->setStyleSheet(
            "QLineEdit { border: none; font-size: 24px; color: rgb(98, 127, 255); }");

        connect(title, &QLineEdit::textChanged, this, [=](const QString &newValue) {
            updateValue(currentPath, newValue, true);
        });

        mainLayout->addWidget(title);
    }

    QGridLayout *gridLayout = new QGridLayout();
    int row = 0;
    int column = 0;

    for (const auto &child : node.children) {
        QString key = child.key;
        QString value = child.value;

        if (checkBoxStates.contains(key) && !checkBoxStates[key]) {
            continue;
        }

        bool hasChildren = !child.children.isEmpty();

        if (!hasChildren) {
            QString keyPath = currentPath.isEmpty() ? key : currentPath + "." + key;

            CustomLineEdit *keyEdit = new CustomLineEdit(this, keyPath, true);

            if (key.isEmpty()) {
                keyEdit->setText("-");
                keyEdit->setReadOnly(true);
            } else
                keyEdit->setText(key);

            QMargins keyMargins = keyEdit->contentsMargins();
            keyMargins.setLeft(depth * 20);
            keyEdit->setContentsMargins(keyMargins);

            bool matchFound = useRegex ? regex.match(key).hasMatch() : key.contains(searchText, cs);
            if (!searchText.isEmpty() && matchFound) {
                foundWidgets.append(keyEdit);
            }

            keyEdit->setStyleSheet("QLineEdit { border: none; font-size: 18px; color: white; }");

            gridLayout->addWidget(keyEdit, row, column);

            connect(keyEdit, &QLineEdit::textChanged, this, [=](const QString &newValue) {
                updateValue(keyPath, newValue, true);
            });

            connect(keyEdit, &CustomLineEdit::addKeyValue, this, &MainWindow::handleAddKeyValue);
            connect(keyEdit, &CustomLineEdit::deleteElement, this, &MainWindow::handleDeleteElement);

            CustomLineEdit *valueEdit = new CustomLineEdit(this, keyPath);
            valueEdit->setText(value);

            matchFound = useRegex ? regex.match(value).hasMatch() : value.contains(searchText, cs);
            if (!searchText.isEmpty() && matchFound) {
                foundWidgets.append(valueEdit);
            }

            QMargins valueMargins = valueEdit->contentsMargins();
            valueMargins.setLeft(depth * 20);
            valueEdit->setContentsMargins(valueMargins);

            valueEdit->setStyleSheet(
                "QLineEdit {  font-size: 16px; color: white; max-width: 200px; }");

            connect(valueEdit, &CustomLineEdit::addKeyValue, this, &MainWindow::handleAddKeyValue);
            connect(valueEdit,
                    &CustomLineEdit::deleteElement,
                    this,
                    &MainWindow::handleDeleteElement);

            gridLayout->addWidget(valueEdit, row, column + 1);

            connect(valueEdit, &QLineEdit::textChanged, this, [=](const QString &newValue) {
                updateValue(keyPath, newValue, false);
            });

            column += 2;
            if (column >= 4) {
                column = 0;
                row++;
            }
        }

        if (hasChildren) {
            displayNode(child, currentPath, searchText, useRegex, depth + 1);
        }
    }

    if (gridLayout->count() > 0) {
        mainLayout->addLayout(gridLayout);
    }
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

    qDebug() << name;

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

void MainWindow::clearScrollArea()
{
    if (mainWidget != nullptr) {
        mainWidget->deleteLater();
    }

    if (treeWidget != nullptr) {
        treeWidget->deleteLater();
    }

    treeWidget = new QTreeWidget(this);

    treeWidget->setColumnCount(2);
    treeWidget->setHeaderLabels(QStringList() << "Key"
                                              << "Value");
    treeWidget->setColumnWidth(0, 200);
    treeWidget->setStyleSheet("QHeaderView {background-color: rgb(48, "
                              "48, 48); color: black;} QWidget {background-color: rgb(48, "
                              "48, 48); color: black;}");

    mainWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(mainWidget);

    QLayoutItem *item;
    while ((item = ui->verticalLayout_2->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
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

        clearScrollArea();
        displayedKeys.clear();
        foundWidgets.clear();
        currentFoundIndex = -1;
        startingIndex = -1;
        previousWidget = nullptr;

        if (ui->prettychb->isChecked()) {
            for (const auto &node : root.children) {
                displayNode(node, "", text, useRegex);
            }
            ui->verticalLayout_2->addWidget(mainWidget);
        } else if (ui->treechb->isChecked()) {
            for (const auto &node : root.children) {
                displayTreeNode(node, "", text, nullptr, treeWidget, useRegex);
            }
            ui->verticalLayout_3->addWidget(treeWidget);
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

        clearScrollArea();
        displayedKeys.clear();
        foundWidgets.clear();
        currentFoundIndex = -1;
        startingIndex = -1;
        previousWidget = nullptr;

        if (ui->prettychb->isChecked()) {
            for (const auto &node : root.children) {
                displayNode(node, "", text, useRegex);
            }
            ui->verticalLayout_2->addWidget(mainWidget);
        } else if (ui->treechb->isChecked()) {
            for (const auto &node : root.children) {
                displayTreeNode(node, "", text, nullptr, treeWidget, useRegex);
            }
            ui->gridLayout->addWidget(treeWidget);
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
    if (ui->treechb->isChecked() && treeWidget) {
        QTreeWidgetItemIterator it(treeWidget);
        while (*it) {
            QTreeWidgetItem *item = *it;

            if (treeWidget->itemWidget(item, 0) == widget
                || treeWidget->itemWidget(item, 1) == widget) {
                treeWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
                return;
            }
            ++it;
        }
    } else if (ui->scrollArea && ui->prettychb->isChecked()) {
        ui->scrollArea->ensureWidgetVisible(widget);
    }
}

void MainWindow::on_prettychb_toggled(bool checked)
{
    if (checked) {
        ui->treechb->setChecked(false);
    }

    ui->stackedWidget->setCurrentIndex(0);

    searching_text = "";

    displayYamlData();
}

void MainWindow::on_treechb_toggled(bool checked)
{
    if (checked) {
        ui->prettychb->setChecked(false);
    }

    ui->stackedWidget->setCurrentIndex(1);

    searching_text = "";

    displayYamlData();
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

void MainWindow::on_OpenFolderYmlFilebtn_clicked()
{
    QDesktopServices::openUrl(QDir::currentPath() + "/ymlFiles");
}
