#ifndef YAMLNODE_H
#define YAMLNODE_H

#include <QList>

class YamlNode
{
public:
    QList<YamlNode> children;
    QString value;
    QString key;
    bool is_sequence = false;

    YamlNode();
    YamlNode(const QString &key, const QString &value = "");

    YamlNode *FindNodeByKey(const QString &searchKey);

    void AddKeyWithValue(const QString &key, const QString &value);

    void AddValueToKey(const QString &key, const QString &value);

    void RemoveKey(const QString &key);

    void RemoveValue(const QString &value);
};

#endif // YAMLNODE_H
