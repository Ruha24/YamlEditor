#include "yamltreemodel.h"

#include <QStringList>

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
        ti->parent = item;
        buildFrom(child, ti.get());
        item->children.push_back(std::move(ti));
    }
}

void YamlTreeModel::SetRoot(const YamlNode &root)
{
    beginResetModel();
    root_ = std::make_unique<TreeItem>();
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
        exportTo(child.get(), child_node);
        node.children.append(child_node);
    }
}

YamlNode YamlTreeModel::ToYamlNode() const
{
    YamlNode root;
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

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        if (index.column() == KeyColumn)
            return item->is_sequence && item->key.isEmpty() ? QStringLiteral("-") : item->key;
        return item->value;
    case PathRole:
        return PathFor(index);
    case IsSequenceItemRole:
        return item->is_sequence && item->key.isEmpty();
    case HasChildrenRole:
        return !item->children.empty();
    default:
        return QVariant();
    }
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
        item->key = text;
    } else {
        if (item->value == text)
            return true;
        item->value = text;
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

    if (!(index.column() == KeyColumn && is_seq_marker))
        f |= Qt::ItemIsEditable;

    return f;
}

bool YamlTreeModel::AddChild(const QModelIndex &parent, const QString &key, const QString &value)
{
    TreeItem *parent_item = itemFor(parent);
    if (!parent_item)
        return false;

    const int row = static_cast<int>(parent_item->children.size());
    beginInsertRows(parent, row, row);
    auto ti = std::make_unique<TreeItem>();
    ti->key = key;
    ti->value = value;
    ti->parent = parent_item;
    parent_item->children.push_back(std::move(ti));
    endInsertRows();
    return true;
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
