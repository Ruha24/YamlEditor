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

private:
    Ui::ReplaceWindow *ui;
};

#endif // REPLACEWINDOW_H
