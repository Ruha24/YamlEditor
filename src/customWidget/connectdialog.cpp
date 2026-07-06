#include "connectdialog.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "../security/tokenstore.h"

ConnectDialog::ConnectDialog(TokenStore *store, QWidget *parent)
    : QDialog(parent)
    , store_(store)
{
    setWindowTitle(tr("Connect Yandex Disk"));
    setModal(true);
    setMinimumWidth(420);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    QLabel *title = new QLabel(tr("Link your Yandex Disk account"), this);
    title->setStyleSheet("font-size: 16px; font-weight: 600; color: #cfeefa;");

    QLabel *hint = new QLabel(
        tr("Paste your OAuth token below. It is stored encrypted on this "
           "computer and never leaves it except to authenticate with Yandex."),
        this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #9a9a93;");

    QPushButton *get_token = new QPushButton(tr("How to get a token"), this);
    get_token->setCursor(Qt::PointingHandCursor);
    connect(get_token, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://yandex.ru/dev/disk/poligon/"));
    });

    token_edit_ = new QLineEdit(this);
    token_edit_->setPlaceholderText(tr("OAuth token"));
    token_edit_->setEchoMode(QLineEdit::Password);
    token_edit_->setText(store_ ? store_->load() : QString());

    status_ = new QLabel(this);
    status_->setWordWrap(true);
    if (store_ && store_->hasToken())
        status_->setText(tr("A token is already saved."));
    if (store_ && !TokenStore::isEncrypted())
        status_->setText(status_->text() + "\n"
                         + tr("Warning: encryption is unavailable on this platform; "
                              "the token is stored as plain text."));
    status_->setStyleSheet("color: #e0b341;");

    QPushButton *save_btn = new QPushButton(tr("Save"), this);
    QPushButton *clear_btn = new QPushButton(tr("Remove token"), this);
    QPushButton *close_btn = new QPushButton(tr("Close"), this);
    save_btn->setDefault(true);
    connect(save_btn, &QPushButton::clicked, this, &ConnectDialog::onSave);
    connect(clear_btn, &QPushButton::clicked, this, &ConnectDialog::onClear);
    connect(close_btn, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addWidget(clear_btn);
    buttons->addStretch();
    buttons->addWidget(close_btn);
    buttons->addWidget(save_btn);

    root->addWidget(title);
    root->addWidget(hint);
    root->addWidget(get_token, 0, Qt::AlignLeft);
    root->addWidget(token_edit_);
    root->addWidget(status_);
    root->addLayout(buttons);

    setStyleSheet(R"(
        QDialog { background-color: #2b2b2b; }
        QLineEdit {
            background-color: #33414a; color: #eaf6fb;
            border: 1px solid #51b4d2; border-radius: 6px; padding: 6px 10px;
        }
        QPushButton {
            background-color: #33414a; color: #cfeefa;
            border: 1px solid #51b4d2; border-radius: 6px; padding: 6px 14px;
        }
        QPushButton:hover { background-color: #3a4b55; }
    )");
}

void ConnectDialog::onSave()
{
    if (!store_)
        return;

    const QString token = token_edit_->text().trimmed();
    if (token.isEmpty()) {
        status_->setText(tr("Please enter a token."));
        return;
    }

    if (store_->save(token)) {
        status_->setText(tr("Token saved."));
        emit tokenSaved();
    } else {
        status_->setText(tr("Failed to save the token."));
    }
}

void ConnectDialog::onClear()
{
    if (!store_)
        return;
    store_->clear();
    token_edit_->clear();
    status_->setText(tr("Token removed."));
    emit tokenSaved();
}
