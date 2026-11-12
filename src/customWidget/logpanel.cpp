#include "logpanel.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "../theme/theme.h"

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
    const Theme::Colors &c = Theme::instance().colors();
    switch (level) {
    case LogPanel::Level::Info:
        return c.text_muted;
    case LogPanel::Level::Warning:
        return c.warning;
    case LogPanel::Level::Error:
        return c.danger;
    }
    return c.text;
}
} // namespace

LogPanel::LogPanel(QWidget *parent)
    : QDockWidget(parent)
{
    setObjectName("logPanelDock");
    setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                | QDockWidget::DockWidgetClosable);

    QWidget *container = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(container);
    root->setContentsMargins(8, 0, 8, 8);
    root->setSpacing(6);

    QHBoxLayout *toolbar = new QHBoxLayout();

    filter_label_ = new QLabel(container);
    filter_label_->setProperty("role", "muted");

    filter_ = new QComboBox(container);
    filter_->addItem(QString(), -1);
    filter_->addItem(QString(), static_cast<int>(Level::Info));
    filter_->addItem(QString(), static_cast<int>(Level::Warning));
    filter_->addItem(QString(), static_cast<int>(Level::Error));
    connect(filter_, &QComboBox::currentIndexChanged, this, [this]() { applyFilter(); });

    clear_btn_ = new QPushButton(container);
    clear_btn_->setCursor(Qt::PointingHandCursor);
    connect(clear_btn_, &QPushButton::clicked, this, &LogPanel::clear);

    toolbar->addWidget(filter_label_);
    toolbar->addWidget(filter_);
    toolbar->addStretch();
    toolbar->addWidget(clear_btn_);

    list_ = new QListWidget(container);
    list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list_->setFont(Theme::monoFont(list_->font()));
    list_->setWordWrap(true);

    root->addLayout(toolbar);
    root->addWidget(list_);
    setWidget(container);

    connect(&Theme::instance(), &Theme::changed, this, [this]() {
        for (int i = 0; i < list_->count(); ++i) {
            QListWidgetItem *item = list_->item(i);
            item->setForeground(levelColor(static_cast<Level>(item->data(kLevelRole).toInt())));
        }
    });

    retranslate();
}

void LogPanel::retranslate()
{
    setWindowTitle(tr("Log"));
    filter_label_->setText(tr("Show:"));
    filter_->setItemText(0, tr("All"));
    filter_->setItemText(1, tr("Info"));
    filter_->setItemText(2, tr("Warnings"));
    filter_->setItemText(3, tr("Errors"));
    clear_btn_->setText(tr("Clear"));
}

void LogPanel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QDockWidget::changeEvent(event);
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
    const QString text = QStringLiteral("%1  %2  %3")
                             .arg(timestamp, levelName(level).leftJustified(5), message);

    QListWidgetItem *item = new QListWidgetItem(text, list_);
    item->setForeground(levelColor(level));
    item->setData(kLevelRole, static_cast<int>(level));

    const int wanted = filter_->currentData().toInt();
    if (wanted != -1 && wanted != static_cast<int>(level))
        item->setHidden(true);

    list_->scrollToItem(item);
    emit entryAdded(level, message);
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
