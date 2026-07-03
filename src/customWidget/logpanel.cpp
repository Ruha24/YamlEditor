#include "logpanel.h"

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {
    constexpr int kLevelRole = Qt::UserRole + 1;

    QString levelName(LogPanel::Level level)
    {
        switch (level) {
        case LogPanel::Level::Info:
            return QStringLiteral("INFO");
        case LogPanel::Level::Warning:
            return QStringLiteral("WARN");
        case LogPanel::Level::Error:
            return QStringLiteral("ERROR");
        }
        return {};
    }

    QColor levelColor(LogPanel::Level level)
    {
        switch (level) {
        case LogPanel::Level::Info:
            return {0xcfeefa};
        case LogPanel::Level::Warning:
            return {0xe0b341};
        case LogPanel::Level::Error:
            return {0xe06c6c};
        }
        return {0xe6e6e6};
    }
}

LogPanel::LogPanel(QWidget *parent)
    : QDockWidget(tr("Log"), parent)
{
    setObjectName("logPanelDock");
    setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                | QDockWidget::DockWidgetClosable);

    setStyleSheet(R"(
        QDockWidget {
            color: #eaf6fb;
            font-weight: 600;
        }
        QDockWidget::title {
            background-color: #2f3a42;
            color: #eaf6fb;
            padding: 4px 8px;
        }
    )");

    QWidget *container = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(container);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    QHBoxLayout *toolbar = new QHBoxLayout();

    QLabel *filter_label = new QLabel(tr("Show:"), container);
    filter_label->setStyleSheet("color: #9a9a93;");

    filter_ = new QComboBox(container);
    filter_->addItem(tr("All"), -1);
    filter_->addItem(tr("Info"), static_cast<int>(Level::Info));
    filter_->addItem(tr("Warnings"), static_cast<int>(Level::Warning));
    filter_->addItem(tr("Errors"), static_cast<int>(Level::Error));
    connect(filter_, &QComboBox::currentIndexChanged, this, [this]() { applyFilter(); });

    QPushButton *clear_btn = new QPushButton(tr("Clear"), container);
    clear_btn->setCursor(Qt::PointingHandCursor);
    connect(clear_btn, &QPushButton::clicked, this, &LogPanel::clear);

    toolbar->addWidget(filter_label);
    toolbar->addWidget(filter_);
    toolbar->addStretch();
    toolbar->addWidget(clear_btn);

    list_ = new QListWidget(container);
    list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list_->setStyleSheet(R"(
        QListWidget {
            background-color: #262626;
            border: 1px solid #333333;
            border-radius: 6px;
            font-family: Consolas, "Courier New", monospace;
            font-size: 12px;
            padding: 4px;
        }
        QListWidget::item { padding: 2px 4px; }
        QListWidget::item:selected { background-color: #33405c; }
    )");

    root->addLayout(toolbar);
    root->addWidget(list_);

    container->setStyleSheet(R"(
        QComboBox, QPushButton {
            background-color: #33414a;
            color: #cfeefa;
            border: 1px solid #51b4d2;
            border-radius: 6px;
            padding: 3px 10px;
        }
        QPushButton:hover, QComboBox:hover { background-color: #3a4b55; }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #2f3a42;
            color: #cfeefa;
            border: 1px solid #51b4d2;
            border-radius: 0px;
            padding: 2px;
            outline: none;
            selection-background-color: #51b4d2;
            selection-color: #1c2429;
        }
        QComboBox QAbstractItemView::item {
            min-height: 22px;
            padding: 2px 8px;
        }
        QComboBox QAbstractItemView::item:hover {
            background-color: #3a4b55;
            color: #ffffff;
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

    filter_->view()->setStyleSheet(
        "background-color: #2f3a42; color: #cfeefa;"
        "selection-background-color: #51b4d2; selection-color: #1c2429;");

    QPalette popup_palette = filter_->view()->palette();
    popup_palette.setColor(QPalette::Base, QColor(0x2f3a42));
    popup_palette.setColor(QPalette::Text, QColor(0xcfeefa));
    popup_palette.setColor(QPalette::Highlight, QColor(0x51b4d2));
    popup_palette.setColor(QPalette::HighlightedText, QColor(0x1c2429));
    filter_->view()->setPalette(popup_palette);

    setWidget(container);
}

void LogPanel::logInfo(const QString &message)
{
    append(Level::Info, message);
}

void LogPanel::logWarning(const QString &message)
{
    append(Level::Warning, message);
}

void LogPanel::logError(const QString &message)
{
    append(Level::Error, message);
}

void LogPanel::append(Level level, const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString text = QStringLiteral("[%1] %2  %3")
                             .arg(timestamp, levelName(level).leftJustified(5), message);

    QListWidgetItem *item = new QListWidgetItem(text, list_);
    item->setForeground(levelColor(level));
    item->setData(kLevelRole, static_cast<int>(level));

    list_->scrollToItem(item);

    const int wanted = filter_->currentData().toInt();
    if (wanted != -1 && wanted != static_cast<int>(level))
        item->setHidden(true);
}

void LogPanel::applyFilter()
{
    const int wanted = filter_->currentData().toInt();
    for (int i = 0; i < list_->count(); ++i) {
        QListWidgetItem *item = list_->item(i);
        const int level = item->data(kLevelRole).toInt();
        item->setHidden(wanted != -1 && level != wanted);
    }
}

void LogPanel::clear()
{
    list_->clear();
}