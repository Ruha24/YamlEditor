#include "yamltreemodel.h"

#include <QRegularExpression>
#include <QStringList>

#include <algorithm>

YamlTreeModel::YamlTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , root_(std::make_unique<TreeItem>())
{}

YamlTreeModel::~YamlTreeModel() = default;

void YamlTreeModel::buildFrom(const YamlNode &node, TreeItem *item)
{
    for (const YamlNode &child : node.children) {
        auto ti = std::make_unique<TreeItem>();
        ti->key = child.key;
        ti->value = child.value;
        ti->is_sequence = child.is_sequence;
        ti->type = child.type;
        ti->quoted = child.quoted;
        ti->parent = item;
        buildFrom(child, ti.get());
        item->children.push_back(std::move(ti));
    }
}

void YamlTreeModel::SetRoot(const YamlNode &root)
{
    beginResetModel();
    root_ = std::make_unique<TreeItem>();
    root_->type = root.type;
    buildFrom(root, root_.get());
    endResetModel();
}

void YamlTreeModel::exportTo(const TreeItem *item, YamlNode &node) const
{
    for (const auto &child : item->children) {
        YamlNode child_node;
        child_node.key = child->key;
        child_node.value = child->value;
        child_node.is_sequence = child->is_sequence;
        child_node.type = child->type;
        child_node.quoted = child->quoted;
        exportTo(child.get(), child_node);
        node.children.append(child_node);
    }
}

YamlNode YamlTreeModel::ToYamlNode() const
{
    YamlNode root;
    root.type = root_->type;
    exportTo(root_.get(), root);
    return root;
}

YamlTreeModel::TreeItem *YamlTreeModel::itemFor(const QModelIndex &index) const
{
    if (!index.isValid())
        return root_.get();
    return static_cast<TreeItem *>(index.internalPointer());
}

int YamlTreeModel::rowOf(TreeItem *item) const
{
    TreeItem *parent = item->parent;
    if (!parent)
        return 0;
    for (std::size_t i = 0; i < parent->children.size(); ++i) {
        if (parent->children[i].get() == item)
            return static_cast<int>(i);
    }
    return 0;
}

QModelIndex YamlTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parent_item = itemFor(parent);
    if (row < 0 || row >= static_cast<int>(parent_item->children.size()))
        return QModelIndex();

    TreeItem *child = parent_item->children[static_cast<std::size_t>(row)].get();
    return createIndex(row, column, child);
}

QModelIndex YamlTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    TreeItem *child_item = itemFor(child);
    TreeItem *parent_item = child_item ? child_item->parent : nullptr;

    if (!parent_item || parent_item == root_.get())
        return QModelIndex();

    return createIndex(rowOf(parent_item), 0, parent_item);
}

int YamlTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    TreeItem *item = itemFor(parent);
    return item ? static_cast<int>(item->children.size()) : 0;
}

int YamlTreeModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

QVariant YamlTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TreeItem *item = itemFor(index);
    if (!item)
        return QVariant();

    const bool is_seq_item = item->is_sequence && item->key.isEmpty();

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == KeyColumn)
            return is_seq_item ? QStringLiteral("[%1]").arg(index.row()) : item->key;
        if (!item->children.empty()) {
            const bool is_list = item->children.front()->is_sequence;
            return QString::fromLatin1(is_list ? "[%1]" : "{%1}").arg(item->children.size());
        }
        switch (item->type) {
        case YamlNode::Type::Null:
            return QStringLiteral("null");
        case YamlNode::Type::Map:
            return QStringLiteral("{}");
        case YamlNode::Type::Sequence:
            return QStringLiteral("[]");
        case YamlNode::Type::Scalar:
            break;
        }
        return item->value;
    case Qt::EditRole:
        if (index.column() == KeyColumn)
            return item->key;
        return item->value;
    case Qt::ToolTipRole:
        if (index.column() == ValueColumn && item->children.empty() && !item->value.isEmpty())
            return item->value;
        return PathFor(index);
    case PathRole:
        return PathFor(index);
    case IsSequenceItemRole:
        return is_seq_item;
    case HasChildrenRole:
        return !item->children.empty();
    case IsLeafKeyRole:
        return !is_seq_item && item->children.empty();
    case ValueKindRole: {
        ValueKind kind = ValueKind::String;
        if (!item->children.empty())
            kind = item->children.front()->is_sequence ? ValueKind::Sequence : ValueKind::Map;
        else if (item->type == YamlNode::Type::Null)
            kind = ValueKind::Null;
        else if (item->type == YamlNode::Type::Map)
            kind = ValueKind::Map;
        else if (item->type == YamlNode::Type::Sequence)
            kind = ValueKind::Sequence;
        else
            kind = ScalarKind(item->value, item->quoted);
        return static_cast<int>(kind);
    }
    default:
        return QVariant();
    }
}

YamlTreeModel::ValueKind YamlTreeModel::ScalarKind(const QString &value, bool quoted)
{
    if (quoted)
        return ValueKind::String;

    static const QRegularExpression null_re(QStringLiteral("^(~|null|Null|NULL)$"));
    static const QRegularExpression bool_re(QStringLiteral("^(true|True|TRUE|false|False|FALSE)$"));
    static const QRegularExpression number_re(QStringLiteral(
        R"(^([-+]?(\.[0-9]+|[0-9]+(\.[0-9]*)?)([eE][-+]?[0-9]+)?)"
        R"(|0x[0-9a-fA-F]+|0o[0-7]+|[-+]?\.(inf|Inf|INF)|\.(nan|NaN|NAN))$)"));

    if (null_re.match(value).hasMatch())
        return ValueKind::Null;
    if (bool_re.match(value).hasMatch())
        return ValueKind::Bool;
    if (number_re.match(value).hasMatch())
        return ValueKind::Number;
    return ValueKind::String;
}

bool YamlTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    TreeItem *item = itemFor(index);
    if (!item)
        return false;

    const QString text = value.toString();
    if (index.column() == KeyColumn) {
        if (item->is_sequence && item->key.isEmpty())
            return false;
        if (item->key == text)
            return true;
        if (HasSiblingWithKey(index, text))
            return false;
        item->key = text;
    } else {
        if (!item->children.empty())
            return false;
        if (item->value == text)
            return true;
        item->value = text;
        if (!text.isEmpty())
            item->type = YamlNode::Type::Scalar;
    }

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    return true;
}

QVariant YamlTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();
    if (section == KeyColumn)
        return tr("Key");
    if (section == ValueColumn)
        return tr("Value");
    return QVariant();
}

Qt::ItemFlags YamlTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    TreeItem *item = itemFor(index);
    const bool is_seq_marker = item && item->is_sequence && item->key.isEmpty();

    const bool is_container = item && !item->children.empty();
    if (index.column() == KeyColumn ? !is_seq_marker : !is_container)
        f |= Qt::ItemIsEditable;

    return f;
}

bool YamlTreeModel::CanHaveChildren(const QModelIndex &index) const
{
    const TreeItem *item = itemFor(index);
    return item && (!item->children.empty() || item->value.isEmpty());
}

bool YamlTreeModel::HasSiblingWithKey(const QModelIndex &index, const QString &key) const
{
    const TreeItem *item = itemFor(index);
    if (!item || !item->parent)
        return false;
    for (const auto &sibling : item->parent->children) {
        if (sibling.get() != item && !sibling->is_sequence && sibling->key == key)
            return true;
    }
    return false;
}

QModelIndex YamlTreeModel::AddChild(const QModelIndex &parent, const QString &baseKey)
{
    const QModelIndex parent0 = parent.isValid() ? parent.siblingAtColumn(0) : QModelIndex();
    if (parent0.isValid() && !CanHaveChildren(parent0))
        return QModelIndex();

    TreeItem *parent_item = itemFor(parent0);
    if (!parent_item)
        return QModelIndex();

    const bool as_list_item = parent_item->children.empty()
                                  ? parent_item->type == YamlNode::Type::Sequence
                                  : parent_item->children.front()->is_sequence;

    auto ti = std::make_unique<TreeItem>();
    ti->parent = parent_item;
    if (as_list_item) {
        ti->is_sequence = true;
    } else {
        QString key = baseKey;
        for (int n = 2;; ++n) {
            const bool taken = std::any_of(parent_item->children.begin(),
                                           parent_item->children.end(),
                                           [&](const auto &c) { return c->key == key; });
            if (!taken)
                break;
            key = QStringLiteral("%1_%2").arg(baseKey).arg(n);
        }
        ti->key = key;
    }

    const int row = static_cast<int>(parent_item->children.size());
    beginInsertRows(parent0, row, row);
    parent_item->children.push_back(std::move(ti));
    if (parent_item->type != YamlNode::Type::Sequence && parent_item->type != YamlNode::Type::Map)
        parent_item->type = as_list_item ? YamlNode::Type::Sequence : YamlNode::Type::Map;
    endInsertRows();
    return index(row, 0, parent0);
}

bool YamlTreeModel::RemoveNode(const QModelIndex &index)
{
    if (!index.isValid())
        return false;

    TreeItem *item = itemFor(index);
    TreeItem *parent_item = item ? item->parent : nullptr;
    if (!parent_item)
        return false;

    const int row = rowOf(item);
    beginRemoveRows(parent(index), row, row);
    parent_item->children.erase(parent_item->children.begin() + row);
    endRemoveRows();
    return true;
}

QString YamlTreeModel::PathFor(const QModelIndex &index) const
{
    QStringList parts;
    TreeItem *item = itemFor(index);
    while (item && item != root_.get()) {
        parts.prepend(item->key);
        item = item->parent;
    }
    return parts.join('.');
}
