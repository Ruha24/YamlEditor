#include "customlineedit.h"

CustomLineEdit::CustomLineEdit(QWidget *parent, QString path, bool isKey)
    : QLineEdit(parent)
    , current_path(path)
    , is_key(isKey)
{}

void CustomLineEdit::HandleAddKeyValue(QString key, QString value)
{
    emit AddKeyValue(current_path, value, is_key);
}

void CustomLineEdit::HandleDeleteElement(QString element)
{
    emit DeleteElement(current_path, is_key);
}

QString CustomLineEdit::GetKey() const
{
    return key;
}

void CustomLineEdit::SetKey(const QString &newKey)
{
    key = newKey;
}

QString CustomLineEdit::GetCurrentPath() const
{
    return current_path;
}

bool CustomLineEdit::GetIsKey() const
{
    return is_key;
}
