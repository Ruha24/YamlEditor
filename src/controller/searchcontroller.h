#ifndef SEARCHCONTROLLER_H
#define SEARCHCONTROLLER_H

#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QRegularExpression>

class QAbstractItemModel;
class QTreeView;

class SearchController : public QObject
{
    Q_OBJECT

public:
    explicit SearchController(QObject *parent = nullptr);

    void setTargets(QAbstractItemModel *model, QTreeView *view);

           // Reset transient state (e.g. when the active document changes).
    void reset();

public slots:
    void search(const QString &text, bool caseSensitive, bool downward, bool useRegex);
    void searchForReplace(const QString &text, bool caseSensitive, bool useRegex);
    void replace(const QString &findText, const QString &replaceText, bool allText, bool useRegex);

signals:
    void info(const QString &message);
    void warning(const QString &message);

private:
    bool buildRegex(const QString &text, bool caseSensitive, QRegularExpression &out) const;
    void runSearch(const QString &text, bool useRegex, bool resetSelection);
    void collectMatches(const QModelIndex &parent, const QString &text, bool useRegex);
    void selectMatch(int index);
    void replaceInIndex(const QModelIndex &index, const QString &findText,
                        const QString &replaceText, bool useRegex);

    QAbstractItemModel *model_ = nullptr;
    QTreeView *view_ = nullptr;

    QList<QModelIndex> found_;
    int current_ = -1;
    int starting_ = -1;
    QRegularExpression regex_;
    QString text_;
    Qt::CaseSensitivity cs_ = Qt::CaseInsensitive;
};

#endif // SEARCHCONTROLLER_H
