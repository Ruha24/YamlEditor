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

void MainWindow::setSpecialValue()
{
    for (auto it = updatedValues.begin(); it != updatedValues.end(); ++it) {
        for (auto subIt = it.value().begin(); subIt != it.value().end(); ++subIt) {
            if (subIt.key().contains("==")) {
                ui->lineEdit->setText(subIt.value());
                break;
            }
        }
    }
}

void MainWindow::saveData()
{
    QString fullPath = "ymlFiles/" + ui->fileNamecmb->currentText();
    yamlReader->saveValues(updatedValues, fullPath);
}

void MainWindow::displayYamlData()
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

    updatedValues.clear();

    updatedValues = yamlReader->getGroupedKeysAndValues();

    setSpecialValue();

    QSet<QString> displayedKeys;
    for (auto it = updatedValues.begin(); it != updatedValues.end(); ++it) {
        QStringList parts = it.key().split('.');
        QString currentPath = "";
        for (int i = 0; i < parts.size(); ++i) {
            if (!currentPath.isEmpty()) {
                currentPath += ".";
            }
            currentPath += parts[i];
            if (!displayedKeys.contains(currentPath)) {
                displayedKeys.insert(currentPath);
                QLabel *titleLabel = new QLabel(parts[i], this);
                titleLabel->setStyleSheet("QLabel { font-size: 24px; color: rgb(98, 127, 255)}");
                mainLayout->addWidget(titleLabel);
                if (i == 0 || i == 1) {
                    QFrame *lineD = new QFrame;
                    titleLabel->setStyleSheet("QLabel { font-size: 24px; color: #36b578; }");
                    lineD->setFrameShape(QFrame::HLine);
                    lineD->setStyleSheet("QFrame { margin: 5px 0; }");
                    mainLayout->addWidget(lineD);
                }
            }
        }

        QGridLayout *gridLayout = new QGridLayout();

        int row = 0;
        int column = 0;
        for (auto subIt = it.value().begin(); subIt != it.value().end(); ++subIt) {
            bool isChecked = checkBoxStates.contains(subIt.key()) ? checkBoxStates[subIt.key()]
                                                                  : false;
            if (isChecked) {
                QLabel *keyLabel = new QLabel(subIt.key(), this);
                keyLabel->setStyleSheet("QLabel {font-size: 18px;}");
                QLineEdit *valueEdit = new QLineEdit(subIt.value(), this);
                valueEdit->setStyleSheet("QLabel {font-size: 16px;}");

                connect(valueEdit, &QLineEdit::textChanged, this, [=](const QString &newValue) {
                    updatedValues[it.key()][subIt.key()] = newValue;
                });

                gridLayout->addWidget(keyLabel, row, column);
                gridLayout->addWidget(valueEdit, row, column + 1);

                column += 2;
                if (column >= 4) {
                    column = 0;
                    row++;
                }
            }
        }
        mainLayout->addLayout(gridLayout);
    }

    ui->verticalLayout_2->addWidget(mainWidget);
}

void MainWindow::displaykeys()
{
    QLayoutItem *item;
    while ((item = ui->gridLayout_2->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    int row = 0;
    int col = 0;

    QSet<QString> keys;
    for (auto it = updatedValues.begin(); it != updatedValues.end(); ++it) {
        for (auto subIt = it.value().begin(); subIt != it.value().end(); ++subIt) {
            if (keys.contains(subIt.key()))
                continue;

            keys.insert(subIt.key());

            if (subIt.key().contains("=="))
                continue;

            QCheckBox *checkBox = new QCheckBox(subIt.key(), this);
            checkBox->setStyleSheet(
                "QCheckBox{ font-size: 24px;} QCheckBox::indicator { width: 20px; height: "
                "20px; "
                "background-color: #525252; "
                "color: #37A2C3; border: 1px solid #00AADF; border-radius: 10px; } "
                "QCheckBox::indicator:checked { border: 1px solid #3CC7F2; background-color: "
                "#51B4D2; } ");
            ui->gridLayout_2->addWidget(checkBox, row, col);

            checkBox->setChecked(true);
            checkBoxStates[subIt.key()] = true;

            connect(checkBox, &QCheckBox::stateChanged, this, &MainWindow::onCheckBoxStateChanged);

            col++;
            if (col > 1) {
                col = 0;
                row++;
            }
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
