#include "yamlnode.h"

#include <QDebug>
#include <QStringList>

YamlNode::YamlNode(const QString &key, const QString &value)
    : value(value)
    , key(key)
{
}

const YamlNode *YamlNode::FindNodeByKey(const QString &searchKey) const
{
    if (key == searchKey)
        return this;

    for (const YamlNode &child : children) {
        if (const YamlNode *result = child.FindNodeByKey(searchKey))
            return result;
    }
    return nullptr;
}

YamlNode *YamlNode::FindNodeByKey(const QString &searchKey)
{
    // Reuse the const implementation without duplicating the traversal.
    return const_cast<YamlNode *>(std::as_const(*this).FindNodeByKey(searchKey));
}

YamlNode *YamlNode::FindOrCreateChild(const QString &part)
{
    for (YamlNode &child : children) {
        if (child.key == part)
            return &child;
    }
    children.append(YamlNode(part));
    return &children.last();
}

void YamlNode::AddKeyWithValue(const QString &key, const QString &value)
{
    const QStringList path_parts = key.split('.');
    YamlNode *current_node = this;

    for (const QString &part : path_parts)
        current_node = current_node->FindOrCreateChild(part);

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
    const QStringList path_parts = key.split('.');
    YamlNode *current_node = this;

    for (const QString &part : path_parts) {
        YamlNode *next = nullptr;
        for (YamlNode &child : current_node->children) {
            if (child.key == part) {
                next = &child;
                break;
            }
        }
        if (!next) {
            qWarning() << "Key not found:" << key;
            return;
        }
        current_node = next;
    }

    YamlNode new_child;
    new_child.key = value;
    new_child.value = current_node->value;
    current_node->children.append(new_child);
    current_node->value.clear();
}

void YamlNode::RemoveKey(const QString &path)
{
    const QStringList path_parts = path.split('.');
    YamlNode *parent = NavigateToParent(path_parts);
    if (!parent)
        return;

    const QString key_to_remove = path_parts.last();
    for (qsizetype i = 0; i < parent->children.size(); ++i) {
        if (parent->children[i].key == key_to_remove) {
            parent->children.removeAt(i);
            return;
        }
    }
}

void YamlNode::RemoveValue(const QString &path)
{
    const QStringList path_parts = path.split('.');
    YamlNode *parent = NavigateToParent(path_parts);
    if (!parent)
        return;

    const QString key = path_parts.last();
    for (YamlNode &child : parent->children) {
        if (child.key == key) {
            child.value.clear();
            child.children.clear();
            return;
        }
    }
}

YamlNode *YamlNode::NavigateToParent(const QStringList &path_parts)
{
    YamlNode *current_node = this;

    for (qsizetype i = 0; i < path_parts.size() - 1; ++i) {
        YamlNode *next = nullptr;
        for (YamlNode &child : current_node->children) {
            if (child.key == path_parts[i]) {
                next = &child;
                break;
            }
        }
        if (!next) {
            qWarning() << "Key not found in path:" << path_parts[i];
            return nullptr;
        }
        current_node = next;
    }
    return current_node;
}
