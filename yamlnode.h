#ifndef YAMLNODE_H
#define YAMLNODE_H

#include <QList>

class YamlNode
{
public:
    QList<YamlNode> children;
    QString value;
    QString key;

    YamlNode();
    YamlNode(const QString &key, const QString &value = "");
};

#endif // YAMLNODE_H
