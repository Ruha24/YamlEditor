#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
    yamlReader->saveValues(updatedValues, fullPath);
}

void MainWindow::displayYamlData()
{
    clearScrollArea();

    updatedValues = yamlReader->getRootNode();
    displayedKeys.clear();

    for (const auto &node : updatedValues.children) {
        displayNode(node, "");
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

void MainWindow::displaykeys()
{
    clearKeysArea();

    int rowTopLevel = 0;
    int colTopLevel = 0;

    int rowSubLevel = 0;
    int colSubLevel = 0;

    QSet<QString> topLevelKeys;
    QSet<QString> subKeys;

    for (const auto &node : updatedValues.children) {
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
    YamlNode *currentNode = &updatedValues;

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

void MainWindow::displayNode(const YamlNode &node, const QString &parentPath)
{
    QString currentPath = parentPath.isEmpty() ? node.key : parentPath + "." + node.key;

    if (!displayedKeys.contains(currentPath)) {
        displayedKeys.insert(currentPath);

        QLabel *titleLabel = new QLabel(node.key, this);
        titleLabel->setStyleSheet("QLabel { font-size: 24px; color: rgb(98, 127, 255)}");
        mainLayout->addWidget(titleLabel);

        if (parentPath.isEmpty()) {
            QFrame *lineD = new QFrame;
            lineD->setFrameShape(QFrame::HLine);
            lineD->setStyleSheet("QFrame { margin: 5px 0; background-color: #36b578; }");
            mainLayout->addWidget(lineD);
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

        QLabel *keyLabel = new QLabel(key, this);
        keyLabel->setStyleSheet("QLabel {font-size: 18px;}");
        QLineEdit *valueEdit = new QLineEdit(child.value, this);
        valueEdit->setStyleSheet("QLineEdit {font-size: 16px; max-width: 200px;}");

        QString fullPath = currentPath.isEmpty() ? key : currentPath + "." + key;

        connect(valueEdit, &QLineEdit::textChanged, this, [=](const QString &newValue) {
            updateValue(fullPath, newValue);
        });

        gridLayout->addWidget(keyLabel, row, column);
        gridLayout->addWidget(valueEdit, row, column + 1);

        column += 2;
        if (column >= 4) {
            column = 0;
            row++;
        }

        if (!child.children.isEmpty()) {
            displayNode(child, currentPath);
        }
    }

    mainLayout->addLayout(gridLayout);
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

    checkBox->setChecked(true);
    checkBoxStates[name] = true;

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
