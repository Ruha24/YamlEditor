#ifndef SEARCHINGWINDOW_H
#define SEARCHINGWINDOW_H

#include <QWidget>

namespace Ui {
class searchingWindow;
}

class SearchingWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SearchingWindow(QWidget *parent = nullptr);
    ~SearchingWindow();

signals:
    void searchingText(const QString &text, bool isSensitive, bool is_downward, bool useRegex);

private slots:
    void on_searchingBtn_clicked();

    void on_cancelBtn_clicked();

    void on_upchb_toggled(bool checked);

    void on_downchb_toggled(bool checked);

private:
    Ui::searchingWindow *ui;

    bool is_downward;
};

#endif // SEARCHINGWINDOW_H
