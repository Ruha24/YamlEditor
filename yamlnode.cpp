#include "yamlnode.h"
#include "qdebug.h"

YamlNode::YamlNode()
{
    
}

YamlNode::YamlNode(const QString &key, const QString &value)
    : key(key)
    , value(value)
{}

YamlNode *YamlNode::findNodeByKey(const QString &searchKey)
{
    if (this->key == searchKey)
        return this;

    for (auto &child : children) {
        YamlNode *result = child.findNodeByKey(searchKey);
        if (result)
            return result;
    }

    return nullptr;
}

void YamlNode::addKeyWithValue(const QString &key, const QString &value)
{
    QStringList pathParts = key.split(".");
    YamlNode *currentNode = this;

    for (const QString &part : pathParts) {
        bool found = false;
        for (YamlNode &child : currentNode->children) {
            if (child.key == part) {
                currentNode = &child;
                found = true;
                break;
            }
        }

        if (!found) {
            YamlNode newNode;
            newNode.key = part;
            currentNode->children.append(newNode);
            currentNode = &currentNode->children.last();
        }
    }

    if (!currentNode->children.isEmpty() || !currentNode->value.isEmpty()) {
        YamlNode newChild;
        newChild.key = currentNode->value;
        newChild.value = value;
        newChild.children = currentNode->children;

        currentNode->children.clear();
        currentNode->children.append(newChild);
        currentNode->value.clear();
    } else {
        currentNode->value = value;
    }
}

void YamlNode::addValueToKey(const QString &key, const QString &value)
{
    QStringList pathParts = key.split(".");
    YamlNode *currentNode = this;

    for (const QString &part : pathParts) {
        bool found = false;
        for (YamlNode &child : currentNode->children) {
            if (child.key == part) {
                currentNode = &child;
                found = true;
                break;
            }
        }

        if (!found) {
            qDebug() << "Key not found: " << key;
            return;
        }
    }

    YamlNode newChild;
    newChild.key = value;
    newChild.value = currentNode->value;
    currentNode->children.append(newChild);
    currentNode->value.clear();
}

void YamlNode::removeKey(const QString &path)
{
    QStringList pathParts = path.split(".");
    YamlNode *currentNode = this;

    for (int i = 0; i < pathParts.size() - 1; ++i) {
        bool found = false;
        for (YamlNode &child : currentNode->children) {
            if (child.key == pathParts[i]) {
                currentNode = &child;
                found = true;
                break;
            }
        }

        if (!found) {
            qDebug() << "Key not found in path: " << pathParts[i];
            return;
        }
    }

    QString keyToRemove = pathParts.last();
    for (int i = 0; i < currentNode->children.size(); ++i) {
        if (currentNode->children[i].key == keyToRemove) {
            QList<YamlNode> childrenToMove = currentNode->children[i].children;

            currentNode->children.removeAt(i);

            currentNode->children.append(childrenToMove);

            return;
        }
    }
}

void YamlNode::removeValue(const QString &path)
{
    QStringList pathParts = path.split(".");
    YamlNode *currentNode = this;

    for (int i = 0; i < pathParts.size() - 1; ++i) {
        bool found = false;
        for (YamlNode &child : currentNode->children) {
            if (child.key == pathParts[i]) {
                currentNode = &child;
                found = true;
                break;
            }
        }

        if (!found) {
            qDebug() << "Key not found in path: " << pathParts[i];
            return;
        }
    }

    QString key = pathParts.last();
    for (int i = 0; i < currentNode->children.size(); ++i) {
        if (currentNode->children[i].key == key) {
            QList<YamlNode> childrenToMove = currentNode->children[i].children;

            currentNode->children.removeAt(i);

            currentNode->children.append(childrenToMove);

            return;
        }
    }
}
