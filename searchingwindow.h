#ifndef SEARCHINGWINDOW_H
#define SEARCHINGWINDOW_H

#include <QWidget>

namespace Ui {
class searchingWindow;
}

class searchingWindow : public QWidget
{
    Q_OBJECT

public:
    explicit searchingWindow(QWidget *parent = nullptr);
    ~searchingWindow();

signals:
    void searchingText(const QString &text, bool isSensitive, bool is_downward);

private slots:
    void on_searchingBtn_clicked();

    void on_cancelBtn_clicked();

    void on_upchb_toggled(bool checked);

    void on_downchb_toggled(bool checked);

private:
    Ui::searchingWindow *ui;

    bool is_downward = false;
};

#endif // SEARCHINGWINDOW_H
