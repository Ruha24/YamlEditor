#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#pragma once

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QObject>
#include "qlineedit.h"

#include "../editwidget.h"

class CustomLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit CustomLineEdit(QWidget *parent = nullptr, QString path = "", bool isKey = false);

    QString GetCurrentPath() const;

    bool GetIsKey() const;

    QString GetKey() const;
    void SetKey(const QString &newKey);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override
    {
        editwidget *edit_wnd = new editwidget(this);
        connect(edit_wnd, &editwidget::AddKeyValue, this, &CustomLineEdit::HandleAddKeyValue);
        connect(edit_wnd, &editwidget::DeleteElement, this, &CustomLineEdit::HandleDeleteElement);

        edit_wnd->show();
    }

signals:
    void AddKeyValue(QString path, QString newValue, bool isKey);
    void DeleteElement(QString path, bool isKey);

private:
    void HandleAddKeyValue(QString key, QString value);
    void HandleDeleteElement(QString element);

    QString current_path;
    bool is_key;
    QString key;
};

#endif // CUSTOMLINEEDIT_H
