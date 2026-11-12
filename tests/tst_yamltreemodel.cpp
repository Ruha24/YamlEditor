#include <QtTest>

#include "files/yaml/yamltreemodel.h"

class TestYamlTreeModel : public QObject
{
    Q_OBJECT

private slots:
    void roundTrip_keepsTypes();
    void addChild_toMap_usesUniqueKey();
    void addChild_toList_addsListItem();
    void addChild_toScalarWithValue_isRefused();
    void setData_duplicateKey_isRefused();
    void setData_containerValue_isRefused();

private:
    static YamlNode sample();
};

YamlNode TestYamlTreeModel::sample()
{
    YamlNode root;
    root.type = YamlNode::Type::Map;

    YamlNode name("name", "editor");
    YamlNode tags("tags");
    tags.type = YamlNode::Type::Sequence;
    YamlNode tag;
    tag.is_sequence = true;
    tag.value = "a";
    tags.children.append(tag);
    YamlNode nothing("nothing");
    nothing.type = YamlNode::Type::Null;
    YamlNode id("id", "007");
    id.quoted = true;

    root.children = {name, tags, nothing, id};
    return root;
}

void TestYamlTreeModel::roundTrip_keepsTypes()
{
    YamlTreeModel model;
    model.SetRoot(sample());
    QVERIFY(model.ToYamlNode() == sample());
}

void TestYamlTreeModel::addChild_toMap_usesUniqueKey()
{
    YamlTreeModel model;
    model.SetRoot(sample());

    const QModelIndex first = model.AddChild(QModelIndex(), "name");
    QVERIFY(first.isValid());
    QCOMPARE(first.data().toString(), QString("name_2"));
    const QModelIndex second = model.AddChild(QModelIndex(), "name");
    QCOMPARE(second.data().toString(), QString("name_3"));
}

void TestYamlTreeModel::addChild_toList_addsListItem()
{
    YamlTreeModel model;
    model.SetRoot(sample());

    const QModelIndex tags = model.index(1, 0);
    const QModelIndex added = model.AddChild(tags, "new_key");
    QVERIFY(added.isValid());
    QVERIFY(added.data(YamlTreeModel::IsSequenceItemRole).toBool());
    QVERIFY(model.ToYamlNode().children[1].IsSequenceContainer());
}

void TestYamlTreeModel::addChild_toScalarWithValue_isRefused()
{
    YamlTreeModel model;
    model.SetRoot(sample());

    const QModelIndex name = model.index(0, 0);
    QVERIFY(!model.CanHaveChildren(name));
    QVERIFY(!model.AddChild(name, "x").isValid());
    QCOMPARE(model.ToYamlNode().children[0].value, QString("editor"));
}

void TestYamlTreeModel::setData_duplicateKey_isRefused()
{
    YamlTreeModel model;
    model.SetRoot(sample());

    const QModelIndex name = model.index(0, YamlTreeModel::KeyColumn);
    QVERIFY(!model.setData(name, "tags"));
    QCOMPARE(name.data().toString(), QString("name"));
}

void TestYamlTreeModel::setData_containerValue_isRefused()
{
    YamlTreeModel model;
    model.SetRoot(sample());

    const QModelIndex tags_value = model.index(1, YamlTreeModel::ValueColumn);
    QVERIFY(!(model.flags(tags_value) & Qt::ItemIsEditable));
    QVERIFY(!model.setData(tags_value, "oops"));
}

QTEST_APPLESS_MAIN(TestYamlTreeModel)
#include "tst_yamltreemodel.moc"
