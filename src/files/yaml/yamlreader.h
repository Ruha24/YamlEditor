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

    // Returns true when the file was written completely.
    bool SaveValues(const YamlNode &rootNode, const QString &filePath);

    static QByteArray Serialize(const YamlNode &rootNode);

signals:
    void FileUploaded(bool success);
    void ErrorOccurred(const QString &message);

private:
    bool Fill(const YAML::Node &source, YamlNode &target, int depth);
    static void EmitValue(YAML::Emitter &out, const YamlNode &node);

    YamlNode root;

    static constexpr int kMaxDepth = 100;
};

#endif // YAMLREADER_H
