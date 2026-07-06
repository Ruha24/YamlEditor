#ifndef DOCUMENTSTORE_H
#define DOCUMENTSTORE_H

#include <QHash>
#include <QString>
#include <QStringList>

#include "../files/yaml/yamlnode.h"

class DocumentStore
{
public:
    void set(const QString &fileName, const YamlNode &tree);
    YamlNode get(const QString &fileName) const;
    bool contains(const QString &fileName) const;
    void remove(const QString &fileName);
    void clear();

    QStringList fileNames() const;

    static bool differsFromDisk(const YamlNode &tree, const QString &path);

private:
    QHash<QString, YamlNode> trees_;
};

#endif // DOCUMENTSTORE_H
