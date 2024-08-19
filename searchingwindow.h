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
    void searchingText(const QString &text, bool isSensitive);

private slots:
    void on_searchingBtn_clicked();

    void on_cancelBtn_clicked();

private:
    Ui::searchingWindow *ui;
};

#endif // SEARCHINGWINDOW_H
