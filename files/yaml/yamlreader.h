#ifndef YAMLREADER_H
#define YAMLREADER_H

#include <QFileInfo>
#include <QObject>
#include <QString>
#include "yaml-cpp/yaml.h"
#include <fstream>

#include "../../api/yandex/yandexapi.h"
#include "yamlnode.h"

class YamlReader : public QObject
{
    Q_OBJECT

public:
    YamlReader();

    bool ReadFile(const QString &fileName);

    YamlNode GetRootNode() const;

    void SaveValues(const YamlNode &rootNode, const QString &filePath);

signals:
    void FileUploaded(bool success);

private:
    void CollectKeys(const YAML::Node &node, YamlNode &yamlNode);
    void BuildNode(YAML::Emitter &out, const YamlNode &node);

    YamlNode root;

    YandexApi *yandex_api;
};

#endif // YAMLREADER_H
