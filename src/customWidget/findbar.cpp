#include "findbar.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "../theme/theme.h"

namespace {

QToolButton *makeIconButton(QWidget *parent)
{
    auto *btn = new QToolButton(parent);
    btn->setAutoRaise(true);
    btn->setIconSize(QSize(16, 16));
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QToolButton *makeToggle(const QString &text, QWidget *parent)
{
    auto *btn = new QToolButton(parent);
    btn->setText(text);
    btn->setCheckable(true);
    btn->setProperty("textButton", true);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

} // namespace

FindBar::FindBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("findBar");
    setAttribute(Qt::WA_StyledBackground);

    toggle_replace_ = makeIconButton(this);
    toggle_replace_->setCheckable(true);

    find_edit_ = new QLineEdit(this);
    find_edit_->setClearButtonEnabled(true);
    find_edit_->setMinimumWidth(220);
    find_edit_->installEventFilter(this);

    case_btn_ = makeToggle(QStringLiteral("Aa"), this);
    regex_btn_ = makeToggle(QStringLiteral(".*"), this);

    result_ = new QLabel(this);
    result_->setProperty("role", "muted");
    result_->setMinimumWidth(90);

    prev_btn_ = makeIconButton(this);
    next_btn_ = makeIconButton(this);
    close_btn_ = makeIconButton(this);

    auto *find_row = new QHBoxLayout();
    find_row->setSpacing(4);
    find_row->addWidget(toggle_replace_);
    find_row->addWidget(find_edit_, 1);
    find_row->addWidget(case_btn_);
    find_row->addWidget(regex_btn_);
    find_row->addSpacing(6);
    find_row->addWidget(result_);
    find_row->addWidget(prev_btn_);
    find_row->addWidget(next_btn_);
    find_row->addWidget(close_btn_);

    replace_row_ = new QWidget(this);
    replace_edit_ = new QLineEdit(replace_row_);
    replace_edit_->installEventFilter(this);
    replace_btn_ = new QPushButton(replace_row_);
    replace_all_btn_ = new QPushButton(replace_row_);
    auto *replace_layout = new QHBoxLayout(replace_row_);
    replace_layout->setContentsMargins(0, 0, 0, 0);
    replace_layout->setSpacing(4);
    // Line the replace field up with the find field.
    replace_layout->addSpacing(toggle_replace_->sizeHint().width() + 4);
    replace_layout->addWidget(replace_edit_, 1);
    replace_layout->addWidget(replace_btn_);
    replace_layout->addWidget(replace_all_btn_);
    replace_layout->addStretch(0);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 6, 8, 6);
    root->setSpacing(6);
    root->addLayout(find_row);
    root->addWidget(replace_row_);

    connect(find_edit_, &QLineEdit::textChanged, this, [this]() { emitFind(true); });
    connect(case_btn_, &QToolButton::toggled, this, [this]() { emitFind(true); });
    connect(regex_btn_, &QToolButton::toggled, this, [this]() { emitFind(true); });
    connect(next_btn_, &QToolButton::clicked, this, [this]() { emitFind(true); });
    connect(prev_btn_, &QToolButton::clicked, this, [this]() { emitFind(false); });
    connect(close_btn_, &QToolButton::clicked, this, [this]() {
        hide();
        emit closed();
    });
    connect(toggle_replace_, &QToolButton::toggled, this, &FindBar::setReplaceVisible);
    connect(replace_btn_, &QPushButton::clicked, this, [this]() {
        emit replaceRequested(text(), replace_edit_->text(), caseSensitive(), useRegex());
    });
    connect(replace_all_btn_, &QPushButton::clicked, this, [this]() {
        emit replaceAllRequested(text(), replace_edit_->text(), caseSensitive(), useRegex());
    });
    connect(&Theme::instance(), &Theme::changed, this, &FindBar::refreshIcons);

    setReplaceVisible(false);
    refreshIcons();
    retranslate();
    setResult(-1, -1);
}

void FindBar::refreshIcons()
{
    const Theme &theme = Theme::instance();
    toggle_replace_->setIcon(theme.icon(toggle_replace_->isChecked() ? "chevron-down" : "replace"));
    prev_btn_->setIcon(theme.icon("chevron-up"));
    next_btn_->setIcon(theme.icon("chevron-down"));
    close_btn_->setIcon(theme.icon("close"));
}

void FindBar::retranslate()
{
    find_edit_->setPlaceholderText(tr("Find in keys and values"));
    replace_edit_->setPlaceholderText(tr("Replace with"));
    toggle_replace_->setToolTip(tr("Toggle replace (Ctrl+H)"));
    case_btn_->setToolTip(tr("Match case"));
    regex_btn_->setToolTip(tr("Use regular expression"));
    prev_btn_->setToolTip(tr("Previous match (Shift+Enter)"));
    next_btn_->setToolTip(tr("Next match (Enter)"));
    close_btn_->setToolTip(tr("Close (Esc)"));
    replace_btn_->setText(tr("Replace"));
    replace_all_btn_->setText(tr("Replace all"));
    setResult(last_current_, last_total_);
}

void FindBar::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QWidget::changeEvent(event);
}

bool FindBar::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Escape) {
            hide();
            emit closed();
            return true;
        }
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            if (watched == replace_edit_)
                replace_btn_->click();
            else
                emitFind(!(key->modifiers() & Qt::ShiftModifier));
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FindBar::setReplaceVisible(bool visible)
{
    replace_row_->setVisible(visible);
    if (toggle_replace_->isChecked() != visible)
        toggle_replace_->setChecked(visible);
    refreshIcons();
}

void FindBar::showFind()
{
    show();
    find_edit_->setFocus();
    find_edit_->selectAll();
    if (!find_edit_->text().isEmpty())
        emitFind(true);
}

void FindBar::showReplace()
{
    setReplaceVisible(true);
    showFind();
}

void FindBar::emitFind(bool forward)
{
    emit findRequested(text(), caseSensitive(), useRegex(), forward);
}

void FindBar::setResult(int current, int total)
{
    last_current_ = current;
    last_total_ = total;

    QString text;
    const bool none = total == 0;
    if (total < 0)
        text.clear();
    else if (none)
        text = tr("No results");
    else if (current < 0)
        text = tr("%n result(s)", nullptr, total);
    else
        text = tr("%1 of %2").arg(current + 1).arg(total);

    result_->setText(text);
    result_->setProperty("role", none ? "warning" : "muted");
    result_->style()->unpolish(result_);
    result_->style()->polish(result_);

    find_edit_->setProperty("invalid", none && !find_edit_->text().isEmpty());
    find_edit_->style()->unpolish(find_edit_);
    find_edit_->style()->polish(find_edit_);

    const bool has = total > 0;
    prev_btn_->setEnabled(has);
    next_btn_->setEnabled(has);
    replace_btn_->setEnabled(has);
    replace_all_btn_->setEnabled(has);
}

QString FindBar::text() const
{
    return find_edit_->text();
}

bool FindBar::caseSensitive() const
{
    return case_btn_->isChecked();
}

bool FindBar::useRegex() const
{
    return regex_btn_->isChecked();
}
