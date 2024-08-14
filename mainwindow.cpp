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

    yandexApi = new YandexApi();
    yamlReader = new YamlReader();

    keyF11 = new QShortcut(this);
    keyF11->setKey(Qt::Key_F11);

    connect(keyF11, &QShortcut::activated, this, &MainWindow::slotShortcutF11);

    keyCtrlF = new QShortcut(this);
    keyCtrlF->setKey(Qt::CTRL | Qt::Key_F);

    connect(keyCtrlF, &QShortcut::activated, this, &MainWindow::slotShortcutCtrlF);

    yandexApi->getFiles([&](bool success) {
        if (success) {
            for (const QString &file : yandexApi->getListFileName()) {
                ui->fileNamecmb->addItem(file);
            }
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_fileNamecmb_currentIndexChanged(int index)
{
    ui->fileNamecmb->setCurrentIndex(index);

    if (yamlReader->readFile(ui->fileNamecmb->currentText())) {
        displayYamlData();
        displaykeys();
    }
}

void MainWindow::saveData()
{
    QString fullPath = "ymlFiles/" + ui->fileNamecmb->currentText();
    yamlReader->saveValues(root, fullPath);
}

void MainWindow::displayYamlData()
{
    clearScrollArea();

    root = yamlReader->getRootNode();
    displayedKeys.clear();

    for (const auto &node : root.children) {
        displayNode(node, "", "");
    }

    ui->verticalLayout_2->addWidget(mainWidget);
}

void MainWindow::collectKeys(const YamlNode &node,
                             QSet<QString> &topLevelKeys,
                             QSet<QString> &subKeys)
{
    QString key = node.key;

    if (node.value.isEmpty()) {
        topLevelKeys.insert(key);
    } else {
        subKeys.insert(key);
    }

    for (const auto &subNode : node.children) {
        collectKeys(subNode, topLevelKeys, subKeys);
    }
}

void MainWindow::slotShortcutCtrlF()
{
    searchingWindow *searchWnd = new searchingWindow();

    connect(searchWnd, &searchingWindow::searchingText, this, &MainWindow::searchingText);

    searchWnd->show();
}

void MainWindow::slotShortcutF11()
{
    if (this->isFullScreen()) {
        this->showNormal();
    } else {
        this->showFullScreen();
    }
}

void MainWindow::displaykeys()
{
    clearKeysArea();

    int rowTopLevel = 0;
    int colTopLevel = 0;

    int rowSubLevel = 0;
    int colSubLevel = 0;

    topLevelKeys.clear();
    subKeys.clear();

    for (const auto &node : root.children) {
        collectKeys(node, topLevelKeys, subKeys);
    }

    for (const QString &key : topLevelKeys) {
        createCheckBox(key, rowTopLevel, colTopLevel, true);

        colTopLevel++;
        if (colTopLevel > 1) {
            colTopLevel = 0;
            rowTopLevel++;
        }
    }

    for (const QString &key : subKeys) {
        createCheckBox(key, rowSubLevel, colSubLevel, false);

        colSubLevel++;
        if (colSubLevel > 1) {
            colSubLevel = 0;
            rowSubLevel++;
        }
    }

    displayYamlData();
}

void MainWindow::on_pushButton_clicked()
{
    saveData();
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

void MainWindow::updateValue(const QString &path, const QString &newValue)
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

    currentNode->value = newValue;
}

void MainWindow::displayNode(const YamlNode &node,
                             const QString &parentPath,
                             const QString &searchText)
{
    QString currentPath = parentPath.isEmpty() ? node.key : parentPath + "." + node.key;

    // Проверка на уникальность и наличие данных
    if (!displayedKeys.contains(currentPath)
        && (!node.children.isEmpty() || !node.value.trimmed().isEmpty())) {
        displayedKeys.insert(currentPath);

        // Добавление заголовка узла, если он не пустой
        if (!node.key.trimmed().isEmpty()) {
            QLabel *titleLabel = new QLabel(node.key, this);
            if (!searchText.isEmpty() && node.key.contains(searchText, Qt::CaseInsensitive)) {
                foundWidgets.append(titleLabel);
            }
            titleLabel->setStyleSheet("QLabel { font-size: 24px; color: rgb(98, 127, 255); }");

            mainLayout->addWidget(titleLabel);

            if (parentPath.isEmpty()) {
                QFrame *lineD = new QFrame;
                lineD->setFrameShape(QFrame::HLine);
                lineD->setStyleSheet("QFrame { margin: 5px 0; background-color: #36b578; }");
                mainLayout->addWidget(lineD);
            }
        }
    }

    QGridLayout *gridLayout = new QGridLayout();
    int row = 0;
    int column = 0;

    for (const auto &child : node.children) {
        QString key = child.key;
        bool isChecked = checkBoxStates.contains(key) ? checkBoxStates[key] : false;

        if (!isChecked)
            continue;

        bool hasChildren = !child.children.isEmpty();
        bool hasValue = !child.value.trimmed().isEmpty();

        if (!hasChildren && hasValue) {
            QLabel *keyLabel = new QLabel(key, this);
            if (!searchText.isEmpty() && key.contains(searchText, Qt::CaseInsensitive)) {
                foundWidgets.append(keyLabel);
            }
            keyLabel->setStyleSheet("QLabel { font-size: 18px; color: white; }");

            gridLayout->addWidget(keyLabel, row, column);

            QLineEdit *valueEdit = new QLineEdit(child.value, this);
            if (!searchText.isEmpty() && child.value.contains(searchText, Qt::CaseInsensitive)) {
                foundWidgets.append(valueEdit);
            }
            valueEdit->setStyleSheet(
                "QLineEdit { font-size: 16px; color: white; max-width: 200px; }");

            QString fullPath = currentPath.isEmpty() ? key : currentPath + "." + key;

            connect(valueEdit, &QLineEdit::textChanged, this, [=](const QString &newValue) {
                updateValue(fullPath, newValue);
            });

            gridLayout->addWidget(valueEdit, row, column + 1);

            column += 2;
            if (column >= 4) {
                column = 0;
                row++;
            }
        }

        if (hasChildren) {
            displayNode(child, currentPath, searchText);
        }
    }

    if (gridLayout->count() > 0) {
        mainLayout->addLayout(gridLayout);
    }
}

void MainWindow::createCheckBox(const QString &name, int row, int col, bool topLevelKey)
{
    QCheckBox *checkBox = new QCheckBox(name, this);
    checkBox->setStyleSheet(
        "QCheckBox{ font-size: 24px;} QCheckBox::indicator { width: 20px; height: "
        "20px; "
        "background-color: #525252; "
        "color: #37A2C3; border: 1px solid #00AADF; border-radius: 10px; } "
        "QCheckBox::indicator:checked { border: 1px solid #3CC7F2; background-color: "
        "#51B4D2; } ");

    if (topLevelKey)
        ui->gridLayout_4->addWidget(checkBox, row, col);
    else
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

    while ((item = ui->gridLayout_4->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void MainWindow::clearScrollArea()
{
    if (mainWidget != nullptr) {
        mainWidget->deleteLater();
    }

    mainWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(mainWidget);

    QLayoutItem *item;
    while ((item = ui->verticalLayout_2->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void MainWindow::on_pushButton_3_clicked()
{
    if (topLevelKeys.isEmpty()) {
        QMessageBox::information(this, "Error", "Select a config");
        return;
    }

    AddWindow *addWindow = new AddWindow(this, topLevelKeys);

    connect(addWindow, &AddWindow::saveData, this, &MainWindow::handleSaveData);
    connect(addWindow, &AddWindow::dataAdded, this, &MainWindow::handleDataAdded);
    connect(addWindow, &AddWindow::dataChanged, this, &MainWindow::handleDataChanged);

    addWindow->show();
}

void MainWindow::handleDataAdded(int id, const QString &key, const QString &value)
{
    YamlNode newNode(key, value);
    root.children.append(newNode);
}

void MainWindow::handleDataChanged(int id, const QString &key, const QString &value)
{
    for (YamlNode &child : root.children) {
        if (child.key == key) {
            child.value = value;
            return;
        }
    }
    root.children.append(YamlNode(key, value));
}

void MainWindow::handleSaveData(const QList<WidgetData> &widgetDataList)
{
    for (const WidgetData &data : widgetDataList) {
        root.children.append(YamlNode(data.comboBox->currentText(), data.lineEdit->text()));
    }
}

void MainWindow::searchingText(const QString &text)
{
    if (searching_text == text) {
        currentFoundIndex++;
        highlightCurrentFound();
        return;
    }

    searching_text = text;

    clearScrollArea();

    displayedKeys.clear();
    foundWidgets.clear();
    currentFoundIndex = -1;

    for (const auto &node : root.children) {
        displayNode(node, "", text);
    }

    if (!foundWidgets.isEmpty()) {
        currentFoundIndex = 0;
        highlightCurrentFound();
    }

    ui->verticalLayout_2->addWidget(mainWidget);
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
    if (ui->scrollArea) {
        ui->scrollArea->ensureWidgetVisible(widget);
    }
}
