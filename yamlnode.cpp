#include "yamlnode.h"

YamlNode::YamlNode()
{
    
}

YamlNode::YamlNode(const QString &key, const QString &value)
    : key(key)
    , value(value)
{}
