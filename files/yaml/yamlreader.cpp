#include "yamlreader.h"

YamlReader::YamlReader()
{
    yandex_api = new YandexApi();
}

void YamlReader::CollectKeys(const YAML::Node &node, YamlNode &yamlNode)
{
    if (node.IsMap()) {
        for (const auto &kv : node) {
            QString key = QString::fromStdString(kv.first.as<std::string>());
            YamlNode child(key);

            if (kv.second.IsScalar()) {
                QString value = QString::fromStdString(kv.second.as<std::string>());
                child.value = value.isEmpty() ? "" : value;
            } else if (kv.second.IsSequence()) {
                for (std::size_t i = 0; i < kv.second.size(); ++i) {
                    YamlNode seq_child;
                    if (kv.second[i].IsScalar()) {
                        QString value = QString::fromStdString(kv.second[i].as<std::string>());
                        seq_child.value = value == "" ? "" : value;
                    }
                    child.children.append(seq_child);
                }
            } else {
                CollectKeys(kv.second, child);
            }

            yamlNode.children.append(child);
        }
    } else if (node.IsSequence()) {
        for (std::size_t i = 0; i < node.size(); ++i) {
            YamlNode child;

            if (node[i].IsScalar()) {
                QString value = QString::fromStdString(node[i].as<std::string>());
                child.value = value.isEmpty() ? "" : value;
            } else {
                CollectKeys(node[i], child);
            }

            yamlNode.children.append(child);
        }
    }
}

bool YamlReader::ReadFile(const QString &file)
{
    YAML::Node config = YAML::LoadFile(file.toStdString());

    if (config.IsNull() || !config.IsDefined()) {
        return false;
    }

    root = YamlNode();

    CollectKeys(config, root);

    return true;
}

YamlNode YamlReader::GetRootNode() const
{
    return root;
}

void YamlReader::BuildNode(YAML::Emitter &out, const YamlNode &node)
{
    if (!node.children.isEmpty()) {
        for (const auto &child : node.children) {
            out << YAML::Key << child.key.toStdString();
            out << YAML::Value;

            if (!child.children.isEmpty()) {
                if (!child.children[0].key.isEmpty()) {
                    out << YAML::BeginMap;
                    BuildNode(out, child);
                    out << YAML::EndMap;
                } else {
                    out << YAML::BeginSeq;
                    for (const auto &element : child.children) {
                        out << YAML::DoubleQuoted << element.value.toStdString();
                    }
                    out << YAML::EndSeq;
                }
            } else if (!child.value.isEmpty()) {
                if (child.value.contains(':') || child.value.contains('{')
                    || child.value.contains('}') || child.value.contains('[')
                    || child.value.contains(']')) {
                    out << YAML::DoubleQuoted << child.value.toStdString();
                } else {
                    out << child.value.toStdString();
                }
            } else {
                out << YAML::DoubleQuoted << "";
            }
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
    ofstream << out.c_str();
    ofstream.close();

    yandex_api->UploadFile(filePath, [&](bool success) {
        if (success)
            emit FileUploaded(true);
        else
            emit FileUploaded(true);
    });
}
