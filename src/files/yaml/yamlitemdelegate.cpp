#include "yamlitemdelegate.h"

#include <QLineEdit>

#include "yamltreemodel.h"
#include "yamlvalidator.h"

YamlItemDelegate::YamlItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

QWidget *YamlItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
                                        const QModelIndex &index) const
{
    QLineEdit *editor = new QLineEdit(parent);
    const bool is_key = index.column() == YamlTreeModel::KeyColumn;

    connect(editor, &QLineEdit::textChanged, editor, [editor, is_key]() {
        const QString text = editor->text();
        const auto result = is_key ? YamlValidator::validateKey(text)
                                   : YamlValidator::validateValue(text);
        if (result.ok) {
            editor->setStyleSheet(QString());
            editor->setToolTip(QString());
        } else {
            editor->setStyleSheet("border: 1px solid #e06c6c; background: #3a2b2b;");
            editor->setToolTip(result.message);
        }
    });

    return editor;
}

void YamlItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QLineEdit *line = qobject_cast<QLineEdit *>(editor);
    if (line)
        line->setText(index.data(Qt::EditRole).toString());
}

void YamlItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    QLineEdit *line = qobject_cast<QLineEdit *>(editor);
    if (!line)
        return;

    const QString text = line->text();
    const bool is_key = index.column() == YamlTreeModel::KeyColumn;
    const auto result = is_key ? YamlValidator::validateKey(text)
                               : YamlValidator::validateValue(text);

    if (!result.ok)
        return;

    model->setData(index, text, Qt::EditRole);
}
