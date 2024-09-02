#include "yamlreader.h"

YamlReader::YamlReader()
{
    yndApi = new YandexApi();
}

void YamlReader::collectKeys(const YAML::Node &node, YamlNode &yamlNode)
{
    if (node.IsMap()) {
        for (const auto &kv : node) {
            QString key = QString::fromStdString(kv.first.as<std::string>());
            YamlNode child(key);

            if (kv.second.IsScalar()) {
                child.value = QString::fromStdString(kv.second.as<std::string>());
            } else {
                collectKeys(kv.second, child);
            }

            yamlNode.children.append(child);
        }
    } else if (node.IsSequence()) {
        for (std::size_t i = 0; i < node.size(); ++i) {
            YamlNode child;

            if (node[i].IsScalar()) {
                child.value = QString::fromStdString(node[i].as<std::string>());
            } else {
                collectKeys(node[i], child);
            }

            yamlNode.children.append(child);
        }
    }
}

bool YamlReader::readFile(const QString &fileName)
{
    QFileInfo file(fileName);

    YAML::Node config = YAML::LoadFile(QDir::currentPath().toStdString() + "/ymlFiles/"
                                       + fileName.toStdString());

    if (config.IsNull() || !config.IsDefined()) {
        return false;
    }

    root = YamlNode();

    collectKeys(config, root);

    return true;
}

YamlNode YamlReader::getRootNode() const
{
    return root;
}

void YamlReader::buildNode(YAML::Emitter &out, const YamlNode &node)
{
    if (!node.children.isEmpty()) {
        for (const auto &child : node.children) {
            out << YAML::Key << child.key.toStdString();
            out << YAML::Value;

            if (!child.children.isEmpty()) {
                out << YAML::BeginMap;
                buildNode(out, child);
                out << YAML::EndMap;
            } else if (!child.value.isEmpty()) {
                if (child.value.contains('\n') || child.value.contains(':')
                    || child.value.contains('{') || child.value.contains('}')
                    || child.value.contains('[') || child.value.contains(']')
                    || child.value.contains('"')) {
                    out << YAML::DoubleQuoted << child.value.toStdString();
                } else {
                    out << child.value.toStdString();
                }
            } else {
                out << YAML::Flow << YAML::BeginSeq;
                for (const auto &element : child.children) {
                    out << YAML::DoubleQuoted << element.value.toStdString();
                }
                out << YAML::EndSeq;
            }
        }
    }
}

void YamlReader::saveValues(const YamlNode &rootNode, const QString &filePath)
{
    YAML::Emitter out;

    out << YAML::BeginMap;
    buildNode(out, rootNode);
    out << YAML::EndMap;

    std::ofstream ofstream(filePath.toStdString());
    ofstream << out.c_str();
    ofstream.close();

    yndApi->uploadFile(filePath, [&](bool success) {
        if (success)
            emit fileUploaded(true);
        else
            emit fileUploaded(true);
    });
}
