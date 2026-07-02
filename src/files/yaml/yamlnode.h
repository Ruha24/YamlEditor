#ifndef YAMLNODE_H
#define YAMLNODE_H

#include <QList>
#include <QString>

class YamlNode
{
public:
    QList<YamlNode> children;
    QString value;
    QString key;
    bool is_sequence = false;

    YamlNode() = default;
    explicit YamlNode(const QString &key, const QString &value = QString());

    const YamlNode *FindNodeByKey(const QString &searchKey) const;
    YamlNode *FindNodeByKey(const QString &searchKey);

    void AddKeyWithValue(const QString &key, const QString &value);
    void AddValueToKey(const QString &key, const QString &value);

    void RemoveKey(const QString &key);
    void RemoveValue(const QString &value);

private:
    YamlNode *NavigateToParent(const QStringList &path_parts);
    YamlNode *FindOrCreateChild(const QString &part);
};

#endif // YAMLNODE_H
