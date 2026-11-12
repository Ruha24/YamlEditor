#include "yamlreader.h"

#include <QFile>
#include <QSaveFile>

YamlReader::YamlReader(QObject *parent)
    : QObject(parent)
{
}

bool YamlReader::Fill(const YAML::Node &source, YamlNode &target, int depth)
{
    if (depth > kMaxDepth) {
        emit ErrorOccurred(tr("YAML nesting too deep; aborting parse"));
        return false;
    }

    switch (source.Type()) {
    case YAML::NodeType::Null:
    case YAML::NodeType::Undefined:
        target.type = YamlNode::Type::Null;
        return true;

    case YAML::NodeType::Scalar:
        target.type = YamlNode::Type::Scalar;
        target.value = QString::fromStdString(source.Scalar());
        // yaml-cpp tags plain scalars with "?" and quoted ones with "!".
        target.quoted = source.Tag() == "!";
        return true;

    case YAML::NodeType::Sequence:
        target.type = YamlNode::Type::Sequence;
        for (const YAML::Node &element : source) {
            YamlNode child;
            child.is_sequence = true;
            if (!Fill(element, child, depth + 1))
                return false;
            target.children.append(child);
        }
        return true;

    case YAML::NodeType::Map:
        target.type = YamlNode::Type::Map;
        for (const auto &kv : source) {
            if (!kv.first.IsScalar()) {
                emit ErrorOccurred(tr("Complex (non-scalar) map keys are not supported"));
                return false;
            }
            YamlNode child(QString::fromStdString(kv.first.Scalar()));
            if (!Fill(kv.second, child, depth + 1))
                return false;
            target.children.append(child);
        }
        return true;
    }
    return true;
}

bool YamlReader::ReadFile(const QString &file)
{
    // Read through QFile: yaml-cpp's LoadFile cannot open non-ASCII paths on Windows.
    QFile in(file);
    if (!in.open(QIODevice::ReadOnly)) {
        emit ErrorOccurred(tr("Cannot open file: %1").arg(file));
        return false;
    }
    const QByteArray bytes = in.readAll();
    in.close();

    YAML::Node config;
    try {
        config = YAML::Load(bytes.toStdString());
    } catch (const YAML::Exception &e) {
        emit ErrorOccurred(tr("Failed to parse YAML: %1").arg(QString::fromStdString(e.what())));
        return false;
    }

    YamlNode parsed;
    try {
        if (!config.IsDefined() || config.IsNull()) {
            // Empty document: treat as an empty map so the user can start adding keys.
            parsed.type = YamlNode::Type::Map;
        } else if (config.IsScalar()) {
            emit ErrorOccurred(tr("The document root must be a map or a list"));
            return false;
        } else if (!Fill(config, parsed, 0)) {
            return false;
        }
    } catch (const YAML::Exception &e) {
        emit ErrorOccurred(tr("Failed to parse YAML: %1").arg(QString::fromStdString(e.what())));
        return false;
    }

    root = parsed;
    return true;
}

const YamlNode &YamlReader::GetRootNode() const
{
    return root;
}

void YamlReader::EmitValue(YAML::Emitter &out, const YamlNode &node)
{
    if (!node.children.isEmpty()) {
        if (node.IsSequenceContainer()) {
            out << YAML::BeginSeq;
            for (const YamlNode &child : node.children)
                EmitValue(out, child);
            out << YAML::EndSeq;
        } else {
            out << YAML::BeginMap;
            for (const YamlNode &child : node.children) {
                out << YAML::Key << child.key.toStdString() << YAML::Value;
                EmitValue(out, child);
            }
            out << YAML::EndMap;
        }
        return;
    }

    switch (node.type) {
    case YamlNode::Type::Null:
        out << YAML::Null;
        break;
    case YamlNode::Type::Map:
        out << YAML::Flow << YAML::BeginMap << YAML::EndMap;
        break;
    case YamlNode::Type::Sequence:
        out << YAML::Flow << YAML::BeginSeq << YAML::EndSeq;
        break;
    case YamlNode::Type::Scalar:
        // Plain output lets yaml-cpp quote only when needed, so numbers and bools keep
        // their type; values that were quoted in the source stay strings.
        if (node.quoted)
            out << YAML::DoubleQuoted;
        out << node.value.toStdString();
        break;
    }
}

QByteArray YamlReader::Serialize(const YamlNode &rootNode)
{
    YAML::Emitter out;
    out.SetIndent(2);
    YamlNode top = rootNode;
    if (top.children.isEmpty() && top.type != YamlNode::Type::Sequence)
        top.type = YamlNode::Type::Map;
    EmitValue(out, top);
    QByteArray bytes(out.c_str());
    if (!bytes.endsWith('\n'))
        bytes.append('\n');
    return bytes;
}

bool YamlReader::SaveValues(const YamlNode &rootNode, const QString &filePath)
{
    const QByteArray bytes = Serialize(rootNode);

    // QSaveFile writes to a temp file and renames it, so a failed save never
    // leaves a truncated document behind.
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit ErrorOccurred(tr("Cannot open file for writing: %1").arg(filePath));
        emit FileUploaded(false);
        return false;
    }
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        emit ErrorOccurred(tr("Failed writing file: %1").arg(filePath));
        emit FileUploaded(false);
        return false;
    }

    emit FileUploaded(true);
    return true;
}
