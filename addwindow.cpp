#include "addwindow.h"
#include "ui_addwindow.h"

AddWindow::AddWindow(QWidget *parent, QSet<QString> keys)
    : QMainWindow(parent)
    , ui(new Ui::AddWindow)
{
    ui->setupUi(this);

    pixMapAdd = QPixmap(":/assets/images/plus.png");
    pixMapDel = QPixmap(":/assets/images/minus.png");

    this->keys = keys;

    createWidget();
    createWidget();
}

AddWindow::~AddWindow()
{
    delete ui;
}

void AddWindow::createWidget()
{
    QWidget *widgetContainer = new QWidget(this);

    QPushButton *addBtn = new QPushButton("");
    QPushButton *deleteBtn = new QPushButton("");
    QLineEdit *valuetxt = new QLineEdit();
    QComboBox *keysCmb = new QComboBox();

    addBtn->setStyleSheet("QPushButton { background-color: white; width: 25px; height: 25px; } ");
    deleteBtn->setStyleSheet(
        "QPushButton { background-color: white; width: 25px; height: 25px; } ");
    addBtn->setIcon(QIcon(pixMapAdd));
    deleteBtn->setIcon(QIcon(pixMapDel));
    valuetxt->setStyleSheet("QLineEdit { font-size: 20px; }");

    for (const auto &key : keys) {
        keysCmb->addItem(key);
    }

    QHBoxLayout *layout = new QHBoxLayout(widgetContainer);
    layout->addWidget(keysCmb);
    layout->addWidget(valuetxt);
    layout->addWidget(addBtn);
    layout->addWidget(deleteBtn);

    int id = nextId++;
    widgetDataList.append(WidgetData(keysCmb, valuetxt, id));

    connect(addBtn, &QPushButton::clicked, this, &AddWindow::createWidget);
    connect(deleteBtn, &QPushButton::clicked, this, [this, widgetContainer, id] {
        deleteWidget(widgetContainer);
        widgetDataList.removeIf([id](const WidgetData &data) { return data.id == id; });
        emit dataChanged(id, "", "");
    });
    connect(keysCmb, &QComboBox::currentTextChanged, this, [=](const QString &value) {
        emit dataChanged(id, value, valuetxt->text());
    });
    connect(valuetxt, &QLineEdit::textChanged, this, [=](const QString &value) {
        emit dataChanged(id, keysCmb->currentText(), value);
    });

    ui->verticalLayout->addWidget(widgetContainer);
    countWidget++;
}

void AddWindow::deleteWidget(QWidget *widgetContainer)
{
    if (countWidget <= 1)
        return;
    if (!widgetContainer)
        return;

    ui->verticalLayout->removeWidget(widgetContainer);
    delete widgetContainer;

    countWidget--;
}

void AddWindow::clearLayout(QLayout *layout)
{
    if (!layout)
        return;

    while (QLayoutItem *item = layout->takeAt(0)) {
        if (item->layout()) {
            clearLayout(item->layout());
        }
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void AddWindow::on_clearBtn_clicked()
{
    widgetDataList.clear();

    clearLayout(ui->verticalLayout);

    countWidget = 0;

    createWidget();
}

void AddWindow::on_savebtn_clicked()
{
    emit(widgetDataList);
}
