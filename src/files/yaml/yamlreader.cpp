#include "yamlreader.h"

#include <QFileInfo>

#include <fstream>

YamlReader::YamlReader(QObject *parent)
    : QObject(parent)
{
}

void YamlReader::CollectKeys(const YAML::Node &node, YamlNode &yamlNode, int depth)
{
    if (depth > kMaxDepth) {
        emit ErrorOccurred(tr("YAML nesting too deep; aborting parse"));
        return;
    }

    if (node.IsMap()) {
        for (const auto &kv : node) {
            const QString key = QString::fromStdString(kv.first.as<std::string>());
            YamlNode child(key);

            if (kv.second.IsScalar()) {
                child.value = QString::fromStdString(kv.second.as<std::string>());
            } else if (kv.second.IsSequence()) {
                for (std::size_t i = 0; i < kv.second.size(); ++i) {
                    YamlNode seq_child;
                    if (kv.second[i].IsScalar())
                        seq_child.value = QString::fromStdString(kv.second[i].as<std::string>());
                    child.children.append(seq_child);
                }
            } else {
                CollectKeys(kv.second, child, depth + 1);
            }

            yamlNode.children.append(child);
        }
    } else if (node.IsSequence()) {
        for (std::size_t i = 0; i < node.size(); ++i) {
            YamlNode child;
            if (node[i].IsScalar())
                child.value = QString::fromStdString(node[i].as<std::string>());
            else
                CollectKeys(node[i], child, depth + 1);
            yamlNode.children.append(child);
        }
    }
}

bool YamlReader::ReadFile(const QString &file)
{
    YAML::Node config;
    try {
        config = YAML::LoadFile(file.toStdString());
    } catch (const YAML::Exception &e) {
        emit ErrorOccurred(tr("Failed to parse YAML: %1").arg(QString::fromStdString(e.what())));
        return false;
    }

    if (config.IsNull() || !config.IsDefined())
        return false;

    root = YamlNode();
    CollectKeys(config, root, 0);
    return true;
}

const YamlNode &YamlReader::GetRootNode() const
{
    return root;
}

void YamlReader::BuildNode(YAML::Emitter &out, const YamlNode &node)
{
    if (node.children.isEmpty())
        return;

    for (const YamlNode &child : node.children) {
        out << YAML::Key << child.key.toStdString();
        out << YAML::Value;

        if (!child.children.isEmpty()) {
            if (!child.children.first().key.isEmpty()) {
                out << YAML::BeginMap;
                BuildNode(out, child);
                out << YAML::EndMap;
            } else {
                out << YAML::BeginSeq;
                for (const YamlNode &element : child.children)
                    out << YAML::DoubleQuoted << element.value.toStdString();
                out << YAML::EndSeq;
            }
        } else if (!child.value.isEmpty()) {
            if (child.value.contains(':') || child.value.contains('{') ||
                child.value.contains('}') || child.value.contains('[') ||
                child.value.contains(']')) {
                out << YAML::DoubleQuoted << child.value.toStdString();
            } else {
                out << child.value.toStdString();
            }
        } else {
            out << YAML::DoubleQuoted << "";
        }
    }
}

void YamlReader::SaveValues(const YamlNode &rootNode, const QString &filePath)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    BuildNode(out, rootNode);
    out << YAML::EndMap;

    std::ofstream ofstream(filePath.toStdString());
    if (!ofstream.is_open()) {
        emit ErrorOccurred(tr("Cannot open file for writing: %1").arg(filePath));
        emit FileUploaded(false);
        return;
    }

    ofstream << out.c_str();
    ofstream.close();
    if (ofstream.fail()) {
        emit ErrorOccurred(tr("Failed writing file: %1").arg(filePath));
        emit FileUploaded(false);
        return;
    }

    emit FileUploaded(true);
}
