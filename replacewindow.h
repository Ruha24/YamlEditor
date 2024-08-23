#ifndef REPLACEWINDOW_H
#define REPLACEWINDOW_H

#include <QWidget>

namespace Ui {
class ReplaceWindow;
}

class ReplaceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ReplaceWindow(QWidget *parent = nullptr);
    ~ReplaceWindow();

signals:
    void searchReplaceText(const QString &text, bool isSensitive);

    void replaceText(const QString &findText, const QString &replaceText, bool allText);

private slots:
    void on_searchbtn_clicked();

    void on_replacebtn_clicked();

    void on_replaceAllbtn_clicked();

    void on_cancelbtn_clicked();

private:
    Ui::ReplaceWindow *ui;
};

#endif // REPLACEWINDOW_H
