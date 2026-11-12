#include "searchcontroller.h"

#include <QAbstractItemModel>
#include <QTreeView>

SearchController::SearchController(QObject *parent)
    : QObject(parent)
{}

void SearchController::setTargets(QAbstractItemModel *model, QTreeView *view)
{
    if (model_)
        disconnect(model_, nullptr, this, nullptr);
    model_ = model;
    view_ = view;
    if (model_) {
        // Any structural change invalidates the result list.
        connect(model_, &QAbstractItemModel::modelReset, this, &SearchController::reset);
        connect(model_, &QAbstractItemModel::rowsInserted, this, &SearchController::reset);
        connect(model_, &QAbstractItemModel::rowsRemoved, this, &SearchController::reset);
    }
}

void SearchController::reset()
{
    found_.clear();
    current_ = -1;
    valid_ = false;
}

bool SearchController::prepare(const QString &text, bool caseSensitive, bool useRegex)
{
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    if (valid_ && text == text_ && cs == cs_ && useRegex == use_regex_)
        return true;

    reset();
    text_ = text;
    cs_ = cs;
    use_regex_ = useRegex;

    if (text.isEmpty() || !model_) {
        emit resultChanged(-1, -1);
        return false;
    }

    if (useRegex) {
        regex_ = QRegularExpression(text, caseSensitive ? QRegularExpression::NoPatternOption
                                                        : QRegularExpression::CaseInsensitiveOption);
        if (!regex_.isValid()) {
            emit warning(tr("Invalid regular expression"));
            emit resultChanged(-1, 0);
            return false;
        }
    }

    // Matches are collected in tree order; anchor_ is the number of matches that come
    // before the current row, so a fresh search starts where the user is.
    anchor_ = -1;
    anchor_row_ = view_ ? view_->currentIndex().siblingAtColumn(0) : QModelIndex();
    collect(QModelIndex());
    valid_ = true;
    current_ = -1;
    return true;
}

bool SearchController::matches(const QString &cell) const
{
    if (cell.isEmpty())
        return false;
    return use_regex_ ? regex_.match(cell).hasMatch() : cell.contains(text_, cs_);
}

void SearchController::collect(const QModelIndex &parent)
{
    const int rows = model_->rowCount(parent);
    const int cols = model_->columnCount(parent);
    for (int r = 0; r < rows; ++r) {
        if (view_ && view_->isRowHidden(r, parent))
            continue;

        const QModelIndex row = model_->index(r, 0, parent);
        if (row == anchor_row_)
            anchor_ = found_.size();

        for (int c = 0; c < cols; ++c) {
            const QModelIndex idx = model_->index(r, c, parent);
            if (matches(model_->data(idx, Qt::EditRole).toString()))
                found_.append(idx);
        }
        collect(row);
    }
}

void SearchController::select(int index)
{
    current_ = index;
    emit resultChanged(current_, found_.size());

    if (!view_ || index < 0 || index >= found_.size())
        return;
    const QModelIndex idx = found_[index];
    if (!idx.isValid() || idx.model() != model_)
        return;

    for (QModelIndex p = idx.parent(); p.isValid(); p = p.parent())
        view_->expand(p);
    view_->setCurrentIndex(idx);
    view_->scrollTo(idx, QAbstractItemView::PositionAtCenter);
}

void SearchController::find(const QString &text, bool caseSensitive, bool useRegex, bool forward)
{
    const bool fresh = !(valid_ && text == text_ && useRegex == use_regex_
                         && (caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive) == cs_);
    if (!prepare(text, caseSensitive, useRegex))
        return;

    if (found_.isEmpty()) {
        select(-1);
        return;
    }

    const int n = found_.size();
    if (fresh || current_ < 0) {
        int start = anchor_ < 0 ? 0 : anchor_;
        if (!forward)
            start -= 1;
        select(((start % n) + n) % n);
        return;
    }
    select(forward ? (current_ + 1) % n : (current_ - 1 + n) % n);
}

QString SearchController::replaced(const QString &cell, const QString &replacement) const
{
    QString out = cell;
    if (use_regex_)
        out.replace(regex_, replacement);
    else
        out.replace(text_, replacement, cs_);
    return out;
}

void SearchController::replaceCurrent(const QString &text, const QString &replacement,
                                      bool caseSensitive, bool useRegex)
{
    if (!prepare(text, caseSensitive, useRegex) || found_.isEmpty())
        return;

    if (current_ < 0) {
        find(text, caseSensitive, useRegex, true);
        return;
    }

    const int position = current_;
    const QPersistentModelIndex idx = found_[position];
    if (idx.isValid())
        model_->setData(idx, replaced(idx.data(Qt::EditRole).toString(), replacement), Qt::EditRole);

    // Rebuild the list; the next match now sits at the same position.
    reset();
    if (!prepare(text, caseSensitive, useRegex))
        return;
    if (found_.isEmpty())
        select(-1);
    else
        select(qMin(position, int(found_.size()) - 1));
}

int SearchController::replaceAll(const QString &text, const QString &replacement,
                                 bool caseSensitive, bool useRegex)
{
    if (!prepare(text, caseSensitive, useRegex))
        return 0;

    int changed = 0;
    const QList<QPersistentModelIndex> targets = found_;
    for (const QPersistentModelIndex &idx : targets) {
        if (!idx.isValid())
            continue;
        const QString before = idx.data(Qt::EditRole).toString();
        const QString after = replaced(before, replacement);
        if (after != before && model_->setData(idx, after, Qt::EditRole))
            ++changed;
    }

    reset();
    prepare(text, caseSensitive, useRegex);
    select(found_.isEmpty() ? -1 : 0);
    return changed;
}
