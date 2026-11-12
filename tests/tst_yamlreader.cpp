#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QDir>

#include "files/yaml/yamlreader.h"
#include "files/yaml/yamlnode.h"

class TestYamlReader : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void readFile_parsesFlatMap();
    void readFile_parsesNestedMap();
    void readFile_missingFile_returnsFalse();
    void readFile_invalidYaml_emitsError();

    void roundTrip_flatMap_preservesValues();
    void roundTrip_nestedMap_preservesStructure();

    void saveValues_unwritablePath_emitsError();

    void roundTrip_preservesDocument_data();
    void roundTrip_preservesDocument();
    void roundTrip_trickyStrings();
    void readFile_nonAsciiPath();
    void readFile_emptyFile_isEmptyMap();
    void readFile_sequenceItems_areMarked();

private:
    QString writeTemp(const QString &name, const QString &content);

    QTemporaryDir temp_dir;
};

void TestYamlReader::initTestCase()
{
    QVERIFY(temp_dir.isValid());
}

QString TestYamlReader::writeTemp(const QString &name, const QString &content)
{
    const QString path = temp_dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return QString();
    QTextStream out(&file);
    out << content;
    file.close();
    return path;
}

void TestYamlReader::readFile_parsesFlatMap()
{
    const QString path = writeTemp("flat.yaml", "name: editor\nversion: 1\n");
    QVERIFY(!path.isEmpty());

    YamlReader reader;
    QVERIFY(reader.ReadFile(path));

    const YamlNode &root = reader.GetRootNode();
    QCOMPARE(root.children.size(), 2);

    const YamlNode *name = root.FindNodeByKey("name");
    QVERIFY(name != nullptr);
    QCOMPARE(name->value, QString("editor"));
}

void TestYamlReader::readFile_parsesNestedMap()
{
    const QString path = writeTemp("nested.yaml", "db:\n  host: localhost\n  port: 5432\n");
    QVERIFY(!path.isEmpty());

    YamlReader reader;
    QVERIFY(reader.ReadFile(path));

    const YamlNode &root = reader.GetRootNode();
    const YamlNode *db = root.FindNodeByKey("db");
    QVERIFY(db != nullptr);
    QCOMPARE(db->children.size(), 2);

    const YamlNode *host = root.FindNodeByKey("host");
    QVERIFY(host != nullptr);
    QCOMPARE(host->value, QString("localhost"));
}

void TestYamlReader::readFile_missingFile_returnsFalse()
{
    YamlReader reader;
    QSignalSpy error_spy(&reader, &YamlReader::ErrorOccurred);

    const bool ok = reader.ReadFile(temp_dir.filePath("no_such_file.yaml"));

    QVERIFY(!ok);
    QVERIFY(error_spy.count() >= 1);
}

void TestYamlReader::readFile_invalidYaml_emitsError()
{
    const QString path = writeTemp("bad.yaml", "key: [unclosed\n  : : :\n");
    QVERIFY(!path.isEmpty());

    YamlReader reader;
    QSignalSpy error_spy(&reader, &YamlReader::ErrorOccurred);

    const bool ok = reader.ReadFile(path);

    QVERIFY(!ok);
    QCOMPARE(error_spy.count(), 1);
}

void TestYamlReader::roundTrip_flatMap_preservesValues()
{
    const QString src = writeTemp("rt_flat_in.yaml", "alpha: one\nbeta: two\n");
    QVERIFY(!src.isEmpty());

    YamlReader writer;
    QVERIFY(writer.ReadFile(src));
    const YamlNode root = writer.GetRootNode();

    const QString out = temp_dir.filePath("rt_flat_out.yaml");
    QSignalSpy uploaded_spy(&writer, &YamlReader::FileUploaded);
    writer.SaveValues(root, out);

    QCOMPARE(uploaded_spy.count(), 1);
    QCOMPARE(uploaded_spy.takeFirst().at(0).toBool(), true);

    YamlReader rereader;
    QVERIFY(rereader.ReadFile(out));
    const YamlNode &reread = rereader.GetRootNode();

    const YamlNode *alpha = reread.FindNodeByKey("alpha");
    const YamlNode *beta = reread.FindNodeByKey("beta");
    QVERIFY(alpha != nullptr);
    QVERIFY(beta != nullptr);
    QCOMPARE(alpha->value, QString("one"));
    QCOMPARE(beta->value, QString("two"));
}

void TestYamlReader::roundTrip_nestedMap_preservesStructure()
{
    const QString src =
        writeTemp("rt_nested_in.yaml", "server:\n  host: example.com\n  port: 443\n");
    QVERIFY(!src.isEmpty());

    YamlReader writer;
    QVERIFY(writer.ReadFile(src));
    const YamlNode root = writer.GetRootNode();

    const QString out = temp_dir.filePath("rt_nested_out.yaml");
    writer.SaveValues(root, out);

    YamlReader rereader;
    QVERIFY(rereader.ReadFile(out));
    const YamlNode &reread = rereader.GetRootNode();

    const YamlNode *server = reread.FindNodeByKey("server");
    QVERIFY(server != nullptr);
    QCOMPARE(server->children.size(), 2);

    const YamlNode *host = reread.FindNodeByKey("host");
    QVERIFY(host != nullptr);
    QCOMPARE(host->value, QString("example.com"));
}

void TestYamlReader::saveValues_unwritablePath_emitsError()
{
    YamlReader reader;
    QSignalSpy error_spy(&reader, &YamlReader::ErrorOccurred);
    QSignalSpy uploaded_spy(&reader, &YamlReader::FileUploaded);

    YamlNode root;
    root.AddKeyWithValue("k", "v");

    const QString bad_path = temp_dir.filePath("no_such_dir/out.yaml");
    reader.SaveValues(root, bad_path);

    QCOMPARE(error_spy.count(), 1);
    QCOMPARE(uploaded_spy.count(), 1);
    QCOMPARE(uploaded_spy.takeFirst().at(0).toBool(), false);
}

// Reads `content`, writes it back and reads the result again: the trees must match.
void TestYamlReader::roundTrip_preservesDocument_data()
{
    QTest::addColumn<QString>("content");
    QTest::newRow("list of maps") << "users:\n  - name: Ann\n    age: 30\n  - name: Bob\n";
    QTest::newRow("scalar list keeps types") << "ports: [80, 443]\nflags: [true, false]\n";
    QTest::newRow("quoted numbers stay strings") << "id: \"007\"\nlist: [\"1\", 2]\n";
    QTest::newRow("null") << "nothing: ~\nalso: null\n";
    QTest::newRow("empty collections") << "a: []\nb: {}\n";
    QTest::newRow("nested lists") << "m:\n  - [1, 2]\n  - []\n";
    QTest::newRow("top-level list") << "- a\n- b: 1\n";
    QTest::newRow("unicode") << QString::fromUtf8("display: \"§7Шлем\"\n");
}

void TestYamlReader::roundTrip_preservesDocument()
{
    QFETCH(QString, content);
    const QString src = writeTemp("rt_doc_in.yaml", content);

    YamlReader reader;
    QVERIFY(reader.ReadFile(src));
    const YamlNode original = reader.GetRootNode();

    const QString out = temp_dir.filePath("rt_doc_out.yaml");
    QVERIFY(reader.SaveValues(original, out));

    YamlReader rereader;
    QVERIFY(rereader.ReadFile(out));
    if (rereader.GetRootNode() != original) {
        QFAIL(qPrintable("Round trip changed the document:\n"
                         + QString::fromUtf8(YamlReader::Serialize(original))));
    }
}

void TestYamlReader::roundTrip_trickyStrings()
{
    YamlNode root;
    root.type = YamlNode::Type::Map;
    const QStringList values{"It's", "a: b", "[1, 2", "#not a comment", "*alias", "- dash",
                             "  padded  ", "true", "", "multi\nline"};
    // Edited text has no "quoted" flag; it must still come back as the same text.
    for (int i = 0; i < values.size(); ++i)
        root.children.append(YamlNode(QStringLiteral("k%1").arg(i), values[i]));

    const QString out = temp_dir.filePath("tricky.yaml");
    YamlReader writer;
    QVERIFY(writer.SaveValues(root, out));

    YamlReader reader;
    QVERIFY(reader.ReadFile(out));
    const YamlNode &back = reader.GetRootNode();
    QCOMPARE(back.children.size(), values.size());
    for (int i = 0; i < values.size(); ++i)
        QCOMPARE(back.children[i].value, values[i]);
}

void TestYamlReader::readFile_nonAsciiPath()
{
    QDir(temp_dir.path()).mkdir(QString::fromUtf8("Тест"));
    const QString path = temp_dir.filePath(QString::fromUtf8("Тест/конфиг.yml"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("key: value\n");
    file.close();

    YamlReader reader;
    QVERIFY(reader.ReadFile(path));
    QVERIFY(reader.SaveValues(reader.GetRootNode(), path));
    QVERIFY(reader.ReadFile(path));
    QCOMPARE(reader.GetRootNode().children.first().value, QString("value"));
}

void TestYamlReader::readFile_emptyFile_isEmptyMap()
{
    const QString path = writeTemp("empty.yaml", "");
    YamlReader reader;
    QVERIFY(reader.ReadFile(path));
    QVERIFY(reader.GetRootNode().children.isEmpty());
}

void TestYamlReader::readFile_sequenceItems_areMarked()
{
    const QString path = writeTemp("seq.yaml", "list:\n  - a\n  - b\n");
    YamlReader reader;
    QVERIFY(reader.ReadFile(path));
    const YamlNode *list = reader.GetRootNode().FindNodeByKey("list");
    QVERIFY(list);
    QCOMPARE(list->children.size(), 2);
    QVERIFY(list->children[0].is_sequence);
    QVERIFY(list->IsSequenceContainer());
}

QTEST_APPLESS_MAIN(TestYamlReader)
#include "tst_yamlreader.moc"
