#ifndef YAMLNODE_H
#define YAMLNODE_H

#include <QList>
#include <QString>

class YamlNode
{
public:
    // What the node held in the source document. Only matters for nodes without
    // children: it decides whether an empty node is written as "", ~, {} or [].
    enum class Type { Scalar, Null, Map, Sequence };

    QList<YamlNode> children;
    QString value;
    QString key;
    bool is_sequence = false; // true for an element of a sequence (written as "- ...")
    Type type = Type::Scalar;
    bool quoted = false;      // scalar was quoted in the source, so it stays a string

    bool IsSequenceContainer() const;

    YamlNode() = default;
    explicit YamlNode(const QString &key, const QString &value = QString());

    const YamlNode *FindNodeByKey(const QString &searchKey) const;
    YamlNode *FindNodeByKey(const QString &searchKey);

    void AddKeyWithValue(const QString &key, const QString &value);
    void AddValueToKey(const QString &key, const QString &value);

    void RemoveKey(const QString &key);
    void RemoveValue(const QString &value);

    bool operator==(const YamlNode &other) const;
    bool operator!=(const YamlNode &other) const { return !(*this == other); }

private:
    YamlNode *NavigateToParent(const QStringList &path_parts);
    YamlNode *FindOrCreateChild(const QString &part);
};

#endif // YAMLNODE_H
