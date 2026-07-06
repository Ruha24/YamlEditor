#include "searchcontroller.h"

#include <QAbstractItemModel>
#include <QTreeView>

SearchController::SearchController(QObject *parent)
    : QObject(parent)
{}

void SearchController::setTargets(QAbstractItemModel *model, QTreeView *view)
{
    model_ = model;
    view_ = view;
}

void SearchController::reset()
{
    found_.clear();
    current_ = -1;
    starting_ = -1;
    text_.clear();
    regex_ = QRegularExpression();
}

bool SearchController::buildRegex(const QString &text, bool caseSensitive,
                                  QRegularExpression &out) const
{
    const auto option = caseSensitive ? QRegularExpression::NoPatternOption
                                      : QRegularExpression::CaseInsensitiveOption;
    out = QRegularExpression(text, option);
    return out.isValid();
}

void SearchController::collectMatches(const QModelIndex &parent, const QString &text, bool useRegex)
{
    if (!model_)
        return;

    const int rows = model_->rowCount(parent);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < model_->columnCount(parent); ++c) {
            const QModelIndex idx = model_->index(r, c, parent);
            const QString cell = model_->data(idx, Qt::DisplayRole).toString();

            bool match = false;
            if (useRegex)
                match = regex_.match(cell).hasMatch();
            else
                match = !cell.isEmpty() && cell.contains(text, cs_);

            if (match)
                found_.append(idx);
        }
        const QModelIndex child_parent = model_->index(r, 0, parent);
        collectMatches(child_parent, text, useRegex);
    }
}

void SearchController::runSearch(const QString &text, bool useRegex, bool resetSelection)
{
    found_.clear();
    current_ = -1;
    starting_ = -1;

    if (!text.isEmpty())
        collectMatches(QModelIndex(), text, useRegex);

    if (!found_.isEmpty()) {
        current_ = resetSelection ? 0 : found_.size() - 1;
        selectMatch(current_);
    } else {
        emit info(tr("Can't find it ") + text + "\n");
    }
}

void SearchController::selectMatch(int index)
{
    if (!view_ || index < 0 || index >= found_.size())
        return;

    const QModelIndex idx = found_[index];
    if (!idx.isValid())
        return;

    QModelIndex p = idx.parent();
    while (p.isValid()) {
        view_->expand(p);
        p = p.parent();
    }
    view_->setCurrentIndex(idx);
    view_->scrollTo(idx, QAbstractItemView::PositionAtCenter);
}

void SearchController::search(const QString &text, bool caseSensitive, bool downward, bool useRegex)
{
    const Qt::CaseSensitivity new_cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QRegularExpression regex;
    if (useRegex && !buildRegex(text, caseSensitive, regex)) {
        emit warning(tr("Invalid regular expression"));
        return;
    }

    const bool query_changed = (useRegex && regex.pattern() != regex_.pattern())
                               || (!useRegex && text_ != text) || new_cs != cs_;

    if (query_changed) {
        cs_ = new_cs;
        text_ = text;
        regex_ = regex;
        runSearch(text, useRegex, downward);
        return;
    }

    if (starting_ == -1)
        starting_ = current_;

    if (found_.isEmpty())
        return;

    if (downward)
        current_ = (current_ + 1) % found_.size();
    else
        current_ = (current_ - 1 + found_.size()) % found_.size();

    if (current_ == starting_) {
        emit info(tr("Reached the end of the search results."));
        starting_ = -1;
        return;
    }

    selectMatch(current_);
}

void SearchController::searchForReplace(const QString &text, bool caseSensitive, bool useRegex)
{
    const Qt::CaseSensitivity new_cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QRegularExpression regex;
    if (useRegex && !buildRegex(text, caseSensitive, regex)) {
        emit warning(tr("Invalid regular expression"));
        return;
    }

    const bool query_changed = (useRegex && regex.pattern() != regex_.pattern())
                               || (!useRegex && text_ != text) || new_cs != cs_;

    if (query_changed) {
        cs_ = new_cs;
        text_ = text;
        regex_ = regex;
        runSearch(text, useRegex, true);
        return;
    }

    if (starting_ == -1)
        starting_ = current_;

    if (found_.isEmpty())
        return;

    current_ = (current_ + 1) % found_.size();
    if (current_ == starting_) {
        starting_ = -1;
        return;
    }

    selectMatch(current_);
}

void SearchController::replaceInIndex(const QModelIndex &index, const QString &findText,
                                      const QString &replaceText, bool useRegex)
{
    if (!model_ || !index.isValid())
        return;

    QString text = model_->data(index, Qt::EditRole).toString();
    if (useRegex)
        text.replace(regex_, replaceText);
    else
        text.replace(findText, replaceText, cs_);
    model_->setData(index, text, Qt::EditRole);
}

void SearchController::replace(const QString &findText, const QString &replaceText,
                               bool allText, bool useRegex)
{
    searchForReplace(findText, cs_ == Qt::CaseSensitive, useRegex);

    if (allText) {
        for (const QModelIndex &idx : std::as_const(found_))
            replaceInIndex(idx, findText, replaceText, useRegex);
        text_.clear();
    } else if (current_ >= 0 && current_ < found_.size()) {
        replaceInIndex(found_[current_], findText, replaceText, useRegex);
    }
}
