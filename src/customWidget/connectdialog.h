#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>

class QLineEdit;
class QLabel;
class TokenStore;

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectDialog(TokenStore *store, QWidget *parent = nullptr);

signals:
    void tokenSaved();

private slots:
    void onSave();
    void onClear();

private:
    TokenStore *store_;
    QLineEdit *token_edit_;
    QLabel *status_;
};

#endif // CONNECTDIALOG_H
