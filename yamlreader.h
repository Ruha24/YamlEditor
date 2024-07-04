#ifndef YAMLREADER_H
#define YAMLREADER_H

#include <QFileInfo>
#include <QObject>
#include <QString>
#include "yaml-cpp/yaml.h"
#include <fstream>

#include "yamlnode.h"

class YamlReader
{
public:
    YamlReader();

    bool readFile(const QString &fileName);

    YamlNode getRootNode() const;

    void saveValues(const YamlNode &rootNode, const QString &filePath);

private:
    void collectKeys(const YAML::Node &node, YamlNode &yamlNode);
    void buildNode(YAML::Emitter &out, const YamlNode &node);

    YamlNode root;
};

#endif // YAMLREADER_H
