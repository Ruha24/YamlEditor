#ifndef YAMLTREEMODEL_H
#define YAMLTREEMODEL_H

#include <QAbstractItemModel>
#include <vector>
#include <memory>

#include "yamlnode.h"

class YamlTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Column { KeyColumn = 0, ValueColumn = 1, ColumnCount = 2 };

    enum Roles {
        PathRole = Qt::UserRole + 1,
        IsSequenceItemRole,
        HasChildrenRole,
        ValueKindRole, // ValueKind of the node (value column styling)
        IsLeafKeyRole  // map key without children (used by the key filter)
    };

    enum class ValueKind { String, Number, Bool, Null, Map, Sequence };

    // How a plain (unquoted) scalar is read by YAML 1.2 core schema.
    static ValueKind ScalarKind(const QString &value, bool quoted);

    explicit YamlTreeModel(QObject *parent = nullptr);
    ~YamlTreeModel() override;

    void SetRoot(const YamlNode &root);

    YamlNode ToYamlNode() const;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Appends a child to parent and returns its index (invalid if the parent holds a
    // scalar value and therefore cannot have children). In a list the child becomes a
    // list item; in a map it gets a unique key based on baseKey.
    QModelIndex AddChild(const QModelIndex &parent, const QString &baseKey);
    bool RemoveNode(const QModelIndex &index);

    bool CanHaveChildren(const QModelIndex &index) const;
    bool HasSiblingWithKey(const QModelIndex &index, const QString &key) const;

    QString PathFor(const QModelIndex &index) const;

private:
    struct TreeItem
    {
        QString key;
        QString value;
        bool is_sequence = false;
        YamlNode::Type type = YamlNode::Type::Scalar;
        bool quoted = false;
        TreeItem *parent = nullptr;
        std::vector<std::unique_ptr<TreeItem>> children;
    };

    TreeItem *itemFor(const QModelIndex &index) const;
    int rowOf(TreeItem *item) const;
    void buildFrom(const YamlNode &node, TreeItem *item);
    void exportTo(const TreeItem *item, YamlNode &node) const;

    std::unique_ptr<TreeItem> root_;
};

#endif // YAMLTREEMODEL_H
