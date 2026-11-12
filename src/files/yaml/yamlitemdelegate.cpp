#include "yamlitemdelegate.h"

#include <QLineEdit>

#include "../../theme/theme.h"

#include "yamltreemodel.h"
#include "yamlvalidator.h"

#include <QStyle>

YamlItemDelegate::YamlItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

QWidget *YamlItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    QLineEdit *editor = new QLineEdit(parent);
    const bool is_key = index.column() == YamlTreeModel::KeyColumn;
    if (!is_key)
        editor->setFont(Theme::monoFont(option.font));

    connect(editor, &QLineEdit::textChanged, editor, [editor, is_key]() {
        const QString text = editor->text();
        const auto result = is_key ? YamlValidator::validateKey(text)
                                   : YamlValidator::validateValue(text);
        editor->setProperty("invalid", !result.ok);
        editor->setToolTip(result.ok ? QString() : result.message);
        editor->style()->unpolish(editor);
        editor->style()->polish(editor);
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

    if (!result.ok) {
        emit editRejected(result.message);
        return;
    }

    if (is_key) {
        if (const auto *yaml_model = qobject_cast<const YamlTreeModel *>(model);
            yaml_model && yaml_model->HasSiblingWithKey(index, text)) {
            emit editRejected(tr("Key \"%1\" already exists at this level").arg(text));
            return;
        }
    }

    if (!model->setData(index, text, Qt::EditRole))
        emit editRejected(tr("This cell cannot be changed"));
}

void YamlItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                            const QModelIndex &) const
{
    editor->setGeometry(option.rect.adjusted(0, 1, 0, -1));
}

void YamlItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    const Theme::Colors &c = Theme::instance().colors();
    const bool has_children = index.data(YamlTreeModel::HasChildrenRole).toBool();

    if (index.column() == YamlTreeModel::KeyColumn) {
        if (index.data(YamlTreeModel::IsSequenceItemRole).toBool()) {
            option->palette.setColor(QPalette::Text, c.yaml_meta);
            option->palette.setColor(QPalette::HighlightedText, c.yaml_meta);
        } else if (has_children) {
            option->font.setWeight(QFont::DemiBold);
        }
        return;
    }

    option->font = Theme::monoFont(option->font);

    QColor color = c.yaml_string;
    switch (static_cast<YamlTreeModel::ValueKind>(index.data(YamlTreeModel::ValueKindRole).toInt())) {
    case YamlTreeModel::ValueKind::String:
        color = c.yaml_string;
        break;
    case YamlTreeModel::ValueKind::Number:
        color = c.yaml_number;
        break;
    case YamlTreeModel::ValueKind::Bool:
        color = c.yaml_bool;
        break;
    case YamlTreeModel::ValueKind::Null:
        color = c.yaml_null;
        option->font.setItalic(true);
        break;
    case YamlTreeModel::ValueKind::Map:
    case YamlTreeModel::ValueKind::Sequence:
        color = c.yaml_meta;
        break;
    }
    option->palette.setColor(QPalette::Text, color);
    option->palette.setColor(QPalette::HighlightedText, color);
}
