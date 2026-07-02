#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

#include "files/filesystem.h"

class TestFileSystem : public QObject
{
    Q_OBJECT

private slots:
    void scansYamlFilesOnConstruction();
    void addFile_acceptsYamlExtensions();
    void addFile_rejectsNonYaml();
    void getFilePath_returnsStoredPath();
    void getFilePath_missing_returnsEmpty();
    void checksum_isStableForSameContent();
    void checksum_differsForDifferentContent();

private:
    QString writeFile(const QDir &dir, const QString &name, const QString &content);
};

QString TestFileSystem::writeFile(const QDir &dir, const QString &name, const QString &content)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return QString();
    QTextStream out(&file);
    out << content;
    file.close();
    return path;
}

void TestFileSystem::scansYamlFilesOnConstruction()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir d(dir.path());

    writeFile(d, "a.yaml", "x: 1\n");
    writeFile(d, "b.yml", "y: 2\n");
    writeFile(d, "note.txt", "ignore me\n");

    FileSystem fs(dir.path());

    QVERIFY(fs.Contains("a.yaml"));
    QVERIFY(fs.Contains("b.yml"));
    QVERIFY(!fs.Contains("note.txt"));
}

void TestFileSystem::addFile_acceptsYamlExtensions()
{
    QTemporaryDir dir;
    QDir d(dir.path());
    const QString p = writeFile(d, "cfg.yaml", "k: v\n");

    FileSystem fs(dir.path());
    fs.AddFile(p);

    QVERIFY(fs.Contains("cfg.yaml"));
    QCOMPARE(fs.GetFilePath("cfg.yaml"), p);
}

void TestFileSystem::addFile_rejectsNonYaml()
{
    QTemporaryDir dir;
    QDir d(dir.path());
    const QString p = writeFile(d, "data.json", "{}\n");

    FileSystem fs(dir.path());
    fs.AddFile(p);

    QVERIFY(!fs.Contains("data.json"));
}

void TestFileSystem::getFilePath_returnsStoredPath()
{
    QTemporaryDir dir;
    QDir d(dir.path());
    const QString p = writeFile(d, "one.yaml", "a: b\n");

    FileSystem fs(dir.path());
    QCOMPARE(fs.GetFilePath("one.yaml"), p);
}

void TestFileSystem::getFilePath_missing_returnsEmpty()
{
    QTemporaryDir dir;
    FileSystem fs(dir.path());
    QVERIFY(fs.GetFilePath("ghost.yaml").isEmpty());
}

void TestFileSystem::checksum_isStableForSameContent()
{
    QTemporaryDir dir;
    QDir d(dir.path());
    const QString p = writeFile(d, "c.yaml", "same: content\n");

    FileSystem fs(dir.path());
    const QString h1 = fs.CalculateFileCheckSum(p);
    const QString h2 = fs.CalculateFileCheckSum(p);

    QVERIFY(!h1.isEmpty());
    QCOMPARE(h1, h2);
}

void TestFileSystem::checksum_differsForDifferentContent()
{
    QTemporaryDir dir;
    QDir d(dir.path());
    const QString p1 = writeFile(d, "x.yaml", "content: one\n");
    const QString p2 = writeFile(d, "y.yaml", "content: two\n");

    FileSystem fs(dir.path());
    QVERIFY(fs.CalculateFileCheckSum(p1) != fs.CalculateFileCheckSum(p2));
}

QTEST_MAIN(TestFileSystem)
#include "tst_filesystem.moc"
