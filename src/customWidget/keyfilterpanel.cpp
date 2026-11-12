#include "keyfilterpanel.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

KeyFilterPanel::KeyFilterPanel(QWidget *parent)
    : QWidget(parent)
{
    search_ = new QLineEdit(this);
    search_->setClearButtonEnabled(true);

    list_ = new QListWidget(this);
    list_->setUniformItemSizes(true);
    list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    empty_ = new QLabel(this);
    empty_->setProperty("role", "muted");
    empty_->setWordWrap(true);
    empty_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    all_btn_ = new QPushButton(this);
    none_btn_ = new QPushButton(this);
    for (QPushButton *btn : {all_btn_, none_btn_}) {
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
    }
    summary_ = new QLabel(this);
    summary_->setProperty("role", "muted");

    auto *buttons = new QHBoxLayout();
    buttons->setSpacing(8);
    buttons->addWidget(all_btn_);
    buttons->addWidget(none_btn_);
    buttons->addStretch();
    buttons->addWidget(summary_);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 8);
    layout->setSpacing(6);
    layout->addWidget(search_);
    layout->addLayout(buttons);
    layout->addWidget(list_, 1);
    layout->addWidget(empty_, 1);

    connect(search_, &QLineEdit::textChanged, this, &KeyFilterPanel::applySearch);
    connect(all_btn_, &QPushButton::clicked, this, [this]() { setAllVisibleChecked(true); });
    connect(none_btn_, &QPushButton::clicked, this, [this]() { setAllVisibleChecked(false); });
    connect(list_, &QListWidget::itemChanged, this, [this]() {
        if (updating_)
            return;
        updateSummary();
        emit hiddenKeysChanged(hiddenKeys());
    });

    retranslate();
    setKeys({}, {});
}

void KeyFilterPanel::setKeys(const QStringList &keys, const QSet<QString> &hidden)
{
    updating_ = true;
    list_->clear();
    for (const QString &key : keys) {
        auto *item = new QListWidgetItem(key, list_);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(hidden.contains(key) ? Qt::Unchecked : Qt::Checked);
    }
    updating_ = false;

    const bool empty = keys.isEmpty();
    list_->setVisible(!empty);
    empty_->setVisible(empty);
    search_->setVisible(!empty);
    all_btn_->setVisible(!empty);
    none_btn_->setVisible(!empty);
    applySearch();
    updateSummary();
}

QSet<QString> KeyFilterPanel::hiddenKeys() const
{
    QSet<QString> hidden;
    for (int i = 0; i < list_->count(); ++i) {
        if (list_->item(i)->checkState() != Qt::Checked)
            hidden.insert(list_->item(i)->text());
    }
    return hidden;
}

void KeyFilterPanel::setAllVisibleChecked(bool checked)
{
    updating_ = true;
    for (int i = 0; i < list_->count(); ++i) {
        QListWidgetItem *item = list_->item(i);
        if (!item->isHidden())
            item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
    updating_ = false;
    updateSummary();
    emit hiddenKeysChanged(hiddenKeys());
}

void KeyFilterPanel::applySearch()
{
    const QString needle = search_->text().trimmed();
    for (int i = 0; i < list_->count(); ++i) {
        QListWidgetItem *item = list_->item(i);
        item->setHidden(!needle.isEmpty() && !item->text().contains(needle, Qt::CaseInsensitive));
    }
}

void KeyFilterPanel::updateSummary()
{
    const int hidden = hiddenKeys().size();
    summary_->setText(hidden ? tr("%n hidden", nullptr, hidden) : QString());
}

void KeyFilterPanel::retranslate()
{
    search_->setPlaceholderText(tr("Filter keys"));
    all_btn_->setText(tr("Show all"));
    none_btn_->setText(tr("Hide all"));
    empty_->setText(tr("Keys of the open file appear here. Uncheck a key to hide it in the tree."));
    updateSummary();
}

void KeyFilterPanel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QWidget::changeEvent(event);
}
