#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

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

QTEST_APPLESS_MAIN(TestYamlReader)
#include "tst_yamlreader.moc"
