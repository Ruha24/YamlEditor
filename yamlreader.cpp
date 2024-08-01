#include "yamlreader.h"
#include "qdebug.h"

YamlReader::YamlReader() {}

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
            collectKeys(node[i], child);
            yamlNode.children.append(child);
        }
    }
}

bool YamlReader::readFile(const QString &fileName)
{
    QFileInfo file(fileName);

    QString fileExtension = file.suffix();

    if (fileExtension != "yml" && fileExtension != "yaml")
        return false;

    YAML::Node config = YAML::LoadFile("ymlFiles/" + fileName.toStdString());

    if (config.IsNull()) {
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
            if (!child.value.isEmpty()) {
                qDebug() << child.value;

                out << child.value.toStdString();
            } else {
                out << YAML::BeginMap;
                buildNode(out, child);
                out << YAML::EndMap;
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

    root = rootNode;
}
