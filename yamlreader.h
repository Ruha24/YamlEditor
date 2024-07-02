#ifndef YAMLREADER_H
#define YAMLREADER_H

#include <QFileInfo>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QString>
#include "yaml-cpp/yaml.h"
#include <fstream>

class YamlReader
{
public:
    YamlReader();

    bool readFile(const QString &fileName);

    QMap<QString, QString> getAllKeysAndValues() const;
    QMap<QString, QMap<QString, QString>> getGroupedKeysAndValues() const;

    void saveValues(const QMap<QString, QMap<QString, QString>> &groupedValues,
                    const QString &filePath);

private:
    QString findKey(const QStringList &keys, const QString &key, QMap<QString, QString> values);
    void collectKeys(const YAML::Node &node,
                     QStringList &keys,
                     QMap<QString, QString> &values,
                     const QString &prefix = "");

    QMap<QString, QString> values;
};

#endif // YAMLREADER_H
