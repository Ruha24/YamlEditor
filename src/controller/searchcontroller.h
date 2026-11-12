#ifndef SEARCHCONTROLLER_H
#define SEARCHCONTROLLER_H

#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QPersistentModelIndex>
#include <QRegularExpression>

class QAbstractItemModel;
class QTreeView;

// Finds text in keys and values of the tree and moves the view's current index
// between matches. Hidden (filtered out) rows are skipped.
class SearchController : public QObject
{
    Q_OBJECT

public:
    explicit SearchController(QObject *parent = nullptr);

    void setTargets(QAbstractItemModel *model, QTreeView *view);

    // Drop cached results (they are rebuilt on the next request).
    void reset();

public slots:
    // A new query selects the first match at/after the current row; repeating the
    // same query steps to the next/previous match, wrapping around.
    void find(const QString &text, bool caseSensitive, bool useRegex, bool forward);
    void replaceCurrent(const QString &text, const QString &replacement, bool caseSensitive,
                        bool useRegex);
    // Returns the number of cells changed.
    int replaceAll(const QString &text, const QString &replacement, bool caseSensitive,
                   bool useRegex);

signals:
    // current is 0-based (-1 when nothing is selected); total is -1 for "no query".
    void resultChanged(int current, int total);
    void warning(const QString &message);

private:
    bool prepare(const QString &text, bool caseSensitive, bool useRegex);
    bool matches(const QString &cell) const;
    void collect(const QModelIndex &parent);
    void select(int index);
    QString replaced(const QString &cell, const QString &replacement) const;

    QAbstractItemModel *model_ = nullptr;
    QTreeView *view_ = nullptr;

    QList<QPersistentModelIndex> found_;
    int current_ = -1;
    int anchor_ = -1;
    QModelIndex anchor_row_;

    bool valid_ = false;
    QString text_;
    bool use_regex_ = false;
    Qt::CaseSensitivity cs_ = Qt::CaseInsensitive;
    QRegularExpression regex_;
};

#endif // SEARCHCONTROLLER_H
