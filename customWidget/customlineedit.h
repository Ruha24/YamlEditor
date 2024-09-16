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

    QString getCurrentPath() const;

    bool getIsKey() const;

    QString getKey() const;
    void setKey(const QString &newKey);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override
    {
        editwidget *editWnd = new editwidget(this);
        connect(editWnd, &editwidget::addKeyValue, this, &CustomLineEdit::handleAddKeyValue);
        connect(editWnd, &editwidget::deleteElement, this, &CustomLineEdit::handleDeleteElement);

        editWnd->show();
    }

signals:
    void addKeyValue(QString path, QString newValue, bool isKey);
    void deleteElement(QString path, bool isKey);

private:
    void handleAddKeyValue(QString key, QString value);
    void handleDeleteElement(QString element);

    QString currentPath;
    bool isKey;
    QString key;
};

#endif // CUSTOMLINEEDIT_H
