#include "customlineedit.h"

CustomLineEdit::CustomLineEdit(QWidget *parent, QString path, bool isKey)
    : QLineEdit(parent)
    , currentPath(path)
    , isKey(isKey)
{}

void CustomLineEdit::handleAddKeyValue(QString key, QString value)
{
    emit addKeyValue(currentPath, value, isKey);
}

void CustomLineEdit::handleDeleteElement(QString element)
{
    emit deleteElement(currentPath, isKey);
}

QString CustomLineEdit::getKey() const
{
    return key;
}

void CustomLineEdit::setKey(const QString &newKey)
{
    key = newKey;
}

QString CustomLineEdit::getCurrentPath() const
{
    return currentPath;
}

bool CustomLineEdit::getIsKey() const
{
    return isKey;
}
