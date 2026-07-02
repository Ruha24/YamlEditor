#include <QtTest>

#include "files/yaml/yamlnode.h"

class TestYamlNode : public QObject
{
    Q_OBJECT

private slots:
    void defaultConstructed_isEmpty();
    void keyValueConstructor_setsFields();

    void findNodeByKey_findsSelf();
    void findNodeByKey_findsNestedChild();
    void findNodeByKey_missingReturnsNull();
    void findNodeByKey_constOverload();

    void addKeyWithValue_onFreshLeaf_setsValue();
    void addKeyWithValue_createsNestedPath();
    void addKeyWithValue_onExistingLeaf_nestsPreviousContent();

    void addValueToKey_appendsChildAndClearsValue();
    void addValueToKey_missingKey_isNoOp();

    void removeKey_removesTopLevel();
    void removeKey_removesNested();
    void removeKey_missing_isNoOp();
    void removeValue_clearsValueAndChildren();
};

void TestYamlNode::defaultConstructed_isEmpty()
{
    YamlNode node;
    QVERIFY(node.key.isEmpty());
    QVERIFY(node.value.isEmpty());
    QVERIFY(node.children.isEmpty());
    QCOMPARE(node.is_sequence, false);
}

void TestYamlNode::keyValueConstructor_setsFields()
{
    YamlNode node("host", "localhost");
    QCOMPARE(node.key, QString("host"));
    QCOMPARE(node.value, QString("localhost"));
    QVERIFY(node.children.isEmpty());
}

void TestYamlNode::findNodeByKey_findsSelf()
{
    YamlNode root("root");
    QCOMPARE(root.FindNodeByKey("root"), &root);
}

void TestYamlNode::findNodeByKey_findsNestedChild()
{
    YamlNode root("root");
    root.children.append(YamlNode("a"));
    root.children[0].children.append(YamlNode("b"));

    YamlNode *found = root.FindNodeByKey("b");
    QVERIFY(found != nullptr);
    QCOMPARE(found->key, QString("b"));
}

void TestYamlNode::findNodeByKey_missingReturnsNull()
{
    YamlNode root("root");
    root.children.append(YamlNode("a"));
    QCOMPARE(root.FindNodeByKey("zzz"), nullptr);
}

void TestYamlNode::findNodeByKey_constOverload()
{
    YamlNode root("root");
    root.children.append(YamlNode("child"));

    const YamlNode &const_ref = root;
    const YamlNode *found = const_ref.FindNodeByKey("child");
    QVERIFY(found != nullptr);
    QCOMPARE(found->key, QString("child"));
}

void TestYamlNode::addKeyWithValue_onFreshLeaf_setsValue()
{
    YamlNode root;
    root.AddKeyWithValue("name", "editor");

    YamlNode *node = root.FindNodeByKey("name");
    QVERIFY(node != nullptr);
    QCOMPARE(node->value, QString("editor"));
    QVERIFY(node->children.isEmpty());
}

void TestYamlNode::addKeyWithValue_createsNestedPath()
{
    YamlNode root;
    root.AddKeyWithValue("db.host", "127.0.0.1");

    YamlNode *db = root.FindNodeByKey("db");
    QVERIFY(db != nullptr);
    QCOMPARE(db->children.size(), 1);

    YamlNode *host = root.FindNodeByKey("host");
    QVERIFY(host != nullptr);
    QCOMPARE(host->value, QString("127.0.0.1"));
}

void TestYamlNode::addKeyWithValue_onExistingLeaf_nestsPreviousContent()
{
    YamlNode root;
    root.AddKeyWithValue("port", "8080");
    root.AddKeyWithValue("port", "9090");

    YamlNode *port = root.FindNodeByKey("port");
    QVERIFY(port != nullptr);
    QVERIFY(port->value.isEmpty());
    QCOMPARE(port->children.size(), 1);
    QCOMPARE(port->children[0].key, QString("8080"));
    QCOMPARE(port->children[0].value, QString("9090"));
}

void TestYamlNode::addValueToKey_appendsChildAndClearsValue()
{
    YamlNode root;
    root.AddKeyWithValue("color", "red");

    root.AddValueToKey("color", "blue");

    YamlNode *color = root.FindNodeByKey("color");
    QVERIFY(color != nullptr);
    QVERIFY(color->value.isEmpty());
    QCOMPARE(color->children.size(), 1);
    QCOMPARE(color->children[0].key, QString("blue"));
    QCOMPARE(color->children[0].value, QString("red"));
}

void TestYamlNode::addValueToKey_missingKey_isNoOp()
{
    YamlNode root;
    root.AddKeyWithValue("a", "1");

    root.AddValueToKey("does_not_exist", "x");

    YamlNode *a = root.FindNodeByKey("a");
    QVERIFY(a != nullptr);
    QCOMPARE(a->value, QString("1"));
    QCOMPARE(root.children.size(), 1);
}

void TestYamlNode::removeKey_removesTopLevel()
{
    YamlNode root;
    root.AddKeyWithValue("a", "1");
    root.AddKeyWithValue("b", "2");

    root.RemoveKey("a");

    QCOMPARE(root.FindNodeByKey("a"), nullptr);
    QVERIFY(root.FindNodeByKey("b") != nullptr);
}

void TestYamlNode::removeKey_removesNested()
{
    YamlNode root;
    root.AddKeyWithValue("db.host", "h");
    root.AddKeyWithValue("db.port", "p");

    root.RemoveKey("db.host");

    QCOMPARE(root.FindNodeByKey("host"), nullptr);
    QVERIFY(root.FindNodeByKey("port") != nullptr);
}

void TestYamlNode::removeKey_missing_isNoOp()
{
    YamlNode root;
    root.AddKeyWithValue("a", "1");

    root.RemoveKey("nope");

    QVERIFY(root.FindNodeByKey("a") != nullptr);
    QCOMPARE(root.children.size(), 1);
}

void TestYamlNode::removeValue_clearsValueAndChildren()
{
    YamlNode root;
    root.AddKeyWithValue("k", "v");

    root.RemoveValue("k");

    YamlNode *k = root.FindNodeByKey("k");
    QVERIFY(k != nullptr);
    QVERIFY(k->value.isEmpty());
    QVERIFY(k->children.isEmpty());
}

QTEST_APPLESS_MAIN(TestYamlNode)
#include "tst_yamlnode.moc"
