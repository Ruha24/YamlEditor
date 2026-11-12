#ifndef FINDBAR_H
#define FINDBAR_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

// Inline find / replace strip shown above the tree (Ctrl+F / Ctrl+H).
class FindBar : public QWidget
{
    Q_OBJECT

public:
    explicit FindBar(QWidget *parent = nullptr);

    void showFind();
    void showReplace();
    void setResult(int current, int total);

    QString text() const;
    bool caseSensitive() const;
    bool useRegex() const;

signals:
    void findRequested(const QString &text, bool caseSensitive, bool useRegex, bool forward);
    void replaceRequested(const QString &text, const QString &replacement, bool caseSensitive,
                          bool useRegex);
    void replaceAllRequested(const QString &text, const QString &replacement, bool caseSensitive,
                             bool useRegex);
    void closed();

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void retranslate();
    void refreshIcons();
    void setReplaceVisible(bool visible);
    void emitFind(bool forward);

    QLineEdit *find_edit_;
    QLineEdit *replace_edit_;
    QToolButton *toggle_replace_;
    QToolButton *case_btn_;
    QToolButton *regex_btn_;
    QToolButton *prev_btn_;
    QToolButton *next_btn_;
    QToolButton *close_btn_;
    QLabel *result_;
    QPushButton *replace_btn_;
    QPushButton *replace_all_btn_;
    QWidget *replace_row_;

    int last_current_ = -1;
    int last_total_ = -1;
};

#endif // FINDBAR_H
