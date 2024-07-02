#include "yamlreader.h"
#include "qdebug.h"

YamlReader::YamlReader() {}

void YamlReader::collectKeys(const YAML::Node &node,
                             QStringList &keys,
                             QMap<QString, QString> &values,
                             const QString &prefix)
{
    if (node.IsMap()) {
        for (const auto &kv : node) {
            QString key = QString::fromStdString(kv.first.as<std::string>());
            QString fullKey = prefix.isEmpty() ? key : prefix + "." + key;
            keys.append(fullKey);
            if (kv.second.IsScalar()) {
                values[fullKey] = QString::fromStdString(kv.second.as<std::string>());
            } else {
                collectKeys(kv.second, keys, values, fullKey);
            }
        }
    } else if (node.IsSequence()) {
        for (std::size_t i = 0; i < node.size(); ++i) {
            collectKeys(node[i], keys, values, prefix + "[" + QString::number(i) + "]");
        }
    }
}

QString YamlReader::findKey(const QStringList &keys,
                            const QString &key,
                            QMap<QString, QString> values)
{
    for (const QString &fullKey : keys) {
        if (fullKey.endsWith(key)) {
            return values.value(fullKey, "");
        }
    }
    return QString();
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

    QStringList keys;

    values.clear();

    collectKeys(config, keys, values);

    return true;
}

QMap<QString, QMap<QString, QString>> YamlReader::getGroupedKeysAndValues() const
{
    QMap<QString, QMap<QString, QString>> groupedValues;
    for (auto it = values.begin(); it != values.end(); ++it) {
        QStringList parts = it.key().split('.');
        if (parts.size() > 2) {
            QString rootKey = parts[0] + "." + parts[1] + "." + parts[2];
            QString subKey = it.key().mid(rootKey.length() + 1);
            groupedValues[rootKey][subKey] = it.value();
        } else if (parts.size() == 2) {
            QString rootKey = parts[0] + "." + parts[1];
            groupedValues[rootKey][""] = it.value();
        } else {
            groupedValues[parts[0]][""] = it.value();
        }
    }
    return groupedValues;
}

void YamlReader::saveValues(const QMap<QString, QMap<QString, QString>> &groupedValues,
                            const QString &filePath)
{
    YAML::Emitter out;

    out << YAML::BeginMap;

    for (auto it = groupedValues.begin(); it != groupedValues.end(); ++it) {
        out << YAML::Key << it.key().toStdString();
        out << YAML::Value << YAML::BeginMap;

        for (auto subIt = it.value().begin(); subIt != it.value().end(); ++subIt) {
            out << YAML::Key << subIt.key().toStdString();
            out << YAML::Value << subIt.value().toStdString();
        }

        out << YAML::EndMap;
    }

    out << YAML::EndMap;

    std::ofstream fout(filePath.toStdString());
    fout << out.c_str();
    fout.close();
}

QMap<QString, QString> YamlReader::getAllKeysAndValues() const
{
    return values;
}
