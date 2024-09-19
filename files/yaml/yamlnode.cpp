#include "yamlnode.h"
#include "qdebug.h"

YamlNode::YamlNode()
{
    
}

YamlNode::YamlNode(const QString &key, const QString &value)
    : key(key)
    , value(value)
{}

YamlNode *YamlNode::FindNodeByKey(const QString &searchKey)
{
    if (this->key == searchKey)
        return this;

    for (auto &child : children) {
        YamlNode *result = child.FindNodeByKey(searchKey);
        if (result)
            return result;
    }

    return nullptr;
}

void YamlNode::AddKeyWithValue(const QString &key, const QString &value)
{
    QStringList path_parts = key.split(".");
    YamlNode *current_node = this;

    for (const QString &part : path_parts) {
        bool found = false;
        for (YamlNode &child : current_node->children) {
            if (child.key == part) {
                current_node = &child;
                found = true;
                break;
            }
        }

        if (!found) {
            YamlNode new_node;
            new_node.key = part;
            current_node->children.append(new_node);
            current_node = &current_node->children.last();
        }
    }

    if (!current_node->children.isEmpty() || !current_node->value.isEmpty()) {
        YamlNode new_child;
        new_child.key = current_node->value;
        new_child.value = value;
        new_child.children = current_node->children;

        current_node->children.clear();
        current_node->children.append(new_child);
        current_node->value.clear();
    } else {
        current_node->value = value;
    }
}

void YamlNode::AddValueToKey(const QString &key, const QString &value)
{
    QStringList path_parts = key.split(".");
    YamlNode *current_node = this;

    for (const QString &part : path_parts) {
        bool found = false;
        for (YamlNode &child : current_node->children) {
            if (child.key == part) {
                current_node = &child;
                found = true;
                break;
            }
        }

        if (!found) {
            qDebug() << "Key not found: " << key;
            return;
        }
    }

    YamlNode new_child;
    new_child.key = value;
    new_child.value = current_node->value;
    current_node->children.append(new_child);
    current_node->value.clear();
}

void YamlNode::RemoveKey(const QString &path)
{
    QStringList path_parts = path.split(".");
    YamlNode *current_node = this;

    for (int i = 0; i < path_parts.size() - 1; ++i) {
        bool found = false;
        for (YamlNode &child : current_node->children) {
            if (child.key == path_parts[i]) {
                current_node = &child;
                found = true;
                break;
            }
        }

        if (!found) {
            qDebug() << "Key not found in path: " << path_parts[i];
            return;
        }
    }

    QString keyToRemove = path_parts.last();
    for (int i = 0; i < current_node->children.size(); ++i) {
        if (current_node->children[i].key == keyToRemove) {
            QList<YamlNode> children_to_move = current_node->children[i].children;

            current_node->children.removeAt(i);

            current_node->children.append(children_to_move);

            return;
        }
    }
}

void YamlNode::RemoveValue(const QString &path)
{
    QStringList path_parts = path.split(".");
    YamlNode *current_node = this;

    for (int i = 0; i < path_parts.size() - 1; ++i) {
        bool found = false;
        for (YamlNode &child : current_node->children) {
            if (child.key == path_parts[i]) {
                current_node = &child;
                found = true;
                break;
            }
        }

        if (!found) {
            qDebug() << "Key not found in path: " << path_parts[i];
            return;
        }
    }

    QString key = path_parts.last();
    for (int i = 0; i < current_node->children.size(); ++i) {
        if (current_node->children[i].key == key) {
            QList<YamlNode> children_to_move = current_node->children[i].children;

            current_node->children.removeAt(i);

            current_node->children.append(children_to_move);

            return;
        }
    }
}
