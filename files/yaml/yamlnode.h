#ifndef YAMLNODE_H
#define YAMLNODE_H

#include <QList>

class YamlNode
{
public:
    QList<YamlNode> children;
    QString value;
    QString key;
    bool isSequence = false;

    YamlNode();
    YamlNode(const QString &key, const QString &value = "");

    YamlNode *findNodeByKey(const QString &searchKey);

    void addKeyWithValue(const QString &key, const QString &value);

    void addValueToKey(const QString &key, const QString &value);

    void removeKey(const QString &key);

    void removeValue(const QString &value);
};

#endif // YAMLNODE_H
