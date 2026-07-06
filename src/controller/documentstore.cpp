#include "documentstore.h"

#include <QFileInfo>

#include "../files/yaml/yamlreader.h"

void DocumentStore::set(const QString &fileName, const YamlNode &tree)
{
    trees_.insert(fileName, tree);
}

YamlNode DocumentStore::get(const QString &fileName) const
{
    return trees_.value(fileName);
}

bool DocumentStore::contains(const QString &fileName) const
{
    return trees_.contains(fileName);
}

void DocumentStore::remove(const QString &fileName)
{
    trees_.remove(fileName);
}

void DocumentStore::clear()
{
    trees_.clear();
}

QStringList DocumentStore::fileNames() const
{
    return trees_.keys();
}

bool DocumentStore::differsFromDisk(const YamlNode &tree, const QString &path)
{
    if (path.isEmpty() || !QFileInfo::exists(path))
        return true;

    YamlReader disk_reader;
    if (!disk_reader.ReadFile(path))
        return true;

    return tree != disk_reader.GetRootNode();
}
