#include "customlineedit.h"

#include <QContextMenuEvent>

#include "../editwidget.h"

CustomLineEdit::CustomLineEdit(QWidget *parent, const QString &path, bool isKey)
    : QLineEdit(parent)
    , current_path(path)
    , is_key(isKey)
{
}

void CustomLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event);

    EditWidget *edit_wnd = new EditWidget(this, this);
    edit_wnd->setWindowFlags(Qt::Dialog);
    edit_wnd->setAttribute(Qt::WA_DeleteOnClose);

    connect(edit_wnd, &EditWidget::AddKeyValue, this, &CustomLineEdit::HandleAddKeyValue);
    connect(edit_wnd, &EditWidget::DeleteElement, this, &CustomLineEdit::HandleDeleteElement);

    edit_wnd->show();
}

void CustomLineEdit::HandleAddKeyValue(const QString &value)
{
    emit AddKeyValue(current_path, value, is_key);
}

void CustomLineEdit::HandleDeleteElement()
{
    emit DeleteElement(current_path, is_key);
}

QString CustomLineEdit::GetCurrentPath() const
{
    return current_path;
}

void CustomLineEdit::SetCurrentPath(const QString &path)
{
    current_path = path;
}

bool CustomLineEdit::GetIsKey() const
{
    return is_key;
}
