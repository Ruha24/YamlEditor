#ifndef YAMLREADER_H
#define YAMLREADER_H

#include <QObject>
#include <QString>

#include "yaml-cpp/yaml.h"

#include "yamlnode.h"

class YamlReader : public QObject
{
    Q_OBJECT

public:
    explicit YamlReader(QObject *parent = nullptr);

    bool ReadFile(const QString &file);

    const YamlNode &GetRootNode() const;

    void SaveValues(const YamlNode &rootNode, const QString &filePath);

signals:
    void FileUploaded(bool success);
    void ErrorOccurred(const QString &message);

private:
    void CollectKeys(const YAML::Node &node, YamlNode &yamlNode, int depth);
    void BuildNode(YAML::Emitter &out, const YamlNode &node);

    YamlNode root;

    static constexpr int kMaxDepth = 100;
};

#endif // YAMLREADER_H
