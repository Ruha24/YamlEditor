#include "filelistpanel.h"

#include <QDir>
#include <QEvent>
#include <QLabel>
#include <QListWidget>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "../theme/theme.h"

namespace {
constexpr int kNameRole = Qt::UserRole + 1;
constexpr int kOpenRole = Qt::UserRole + 2;
constexpr int kDirtyRole = Qt::UserRole + 3;
} // namespace

FileListPanel::FileListPanel(QWidget *parent)
    : QWidget(parent)
{
    list_ = new QListWidget(this);
    list_->setSortingEnabled(true);
    list_->setIconSize(QSize(16, 16));
    list_->setUniformItemSizes(true);
    list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_->setTextElideMode(Qt::ElideMiddle);

    empty_ = new QLabel(this);
    empty_->setProperty("role", "muted");
    empty_->setWordWrap(true);
    empty_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    empty_->setContentsMargins(12, 10, 12, 10);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(list_);
    layout->addWidget(empty_);

    connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        emit fileActivated(item->data(kNameRole).toString());
    });
    connect(list_, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit fileActivated(item->data(kNameRole).toString());
    });
    connect(&Theme::instance(), &Theme::changed, this, [this]() {
        for (int i = 0; i < list_->count(); ++i)
            refresh(list_->item(i));
    });

    retranslate();
    updateEmptyState();
}

QListWidgetItem *FileListPanel::find(const QString &name) const
{
    for (int i = 0; i < list_->count(); ++i) {
        if (list_->item(i)->data(kNameRole).toString() == name)
            return list_->item(i);
    }
    return nullptr;
}

bool FileListPanel::contains(const QString &name) const
{
    return find(name) != nullptr;
}

void FileListPanel::addFile(const QString &name, const QString &path)
{
    QListWidgetItem *item = find(name);
    if (!item) {
        item = new QListWidgetItem(list_);
        item->setData(kNameRole, name);
    }
    item->setToolTip(QDir::toNativeSeparators(path));
    refresh(item);
    updateEmptyState();
}

void FileListPanel::removeFile(const QString &name)
{
    delete find(name);
    updateEmptyState();
}

void FileListPanel::setOpen(const QString &name, bool open)
{
    if (QListWidgetItem *item = find(name)) {
        item->setData(kOpenRole, open);
        refresh(item);
    }
}

void FileListPanel::setDirty(const QString &name, bool dirty)
{
    if (QListWidgetItem *item = find(name)) {
        item->setData(kDirtyRole, dirty);
        refresh(item);
    }
}

void FileListPanel::setCurrent(const QString &name)
{
    const QSignalBlocker blocker(list_);
    QListWidgetItem *item = find(name);
    list_->setCurrentItem(item);
    if (!item)
        list_->clearSelection();
}

void FileListPanel::refresh(QListWidgetItem *item)
{
    const QString name = item->data(kNameRole).toString();
    const bool open = item->data(kOpenRole).toBool();
    const bool dirty = item->data(kDirtyRole).toBool();

    item->setText(dirty ? name + QStringLiteral("  ●") : name);
    QFont font = list_->font();
    font.setBold(open);
    item->setFont(font);

    const Theme &theme = Theme::instance();
    item->setIcon(theme.icon("file", open ? theme.colors().accent : theme.colors().text_muted));
    item->setForeground(open ? theme.colors().text : theme.colors().text_muted);
}

void FileListPanel::updateEmptyState()
{
    const bool empty = list_->count() == 0;
    list_->setVisible(!empty);
    empty_->setVisible(empty);
}

void FileListPanel::retranslate()
{
    empty_->setText(tr("No files yet.\nOpen a file or a folder, or drop .yml files here."));
}

void FileListPanel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QWidget::changeEvent(event);
}
