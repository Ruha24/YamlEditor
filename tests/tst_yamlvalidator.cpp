#include <QtTest>

#include "files/yaml/yamlvalidator.h"

class TestYamlValidator : public QObject
{
    Q_OBJECT

private slots:

    void value_plain_ok();
    void value_empty_ok();
    void value_unbalancedQuote_fails();
    void value_unbalancedBracket_fails();
    void value_trailingBackslash_fails();
    void value_balancedQuotes_ok();

    void key_plain_ok();
    void key_empty_fails();
    void key_whitespaceOnly_fails();
    void key_colon_fails();
    void key_bracket_fails();
    void key_equalsSign_ok();
    void key_midSpecialChars_ok();
    void key_leadingIndicator_fails();
};

void TestYamlValidator::value_plain_ok()
{
    QVERIFY(YamlValidator::validateValue("hello world").ok);
    QVERIFY(YamlValidator::validateValue("123").ok);
}

void TestYamlValidator::value_empty_ok()
{
    QVERIFY(YamlValidator::validateValue("").ok);
}

void TestYamlValidator::value_unbalancedQuote_fails()
{
    const auto r = YamlValidator::validateValue("he said \"hi");
    QVERIFY(!r.ok);
    QVERIFY(!r.message.isEmpty());
}

void TestYamlValidator::value_unbalancedBracket_fails()
{
    QVERIFY(!YamlValidator::validateValue("[1, 2, 3").ok);
    QVERIFY(!YamlValidator::validateValue("a}").ok);
}

void TestYamlValidator::value_trailingBackslash_fails()
{
    QVERIFY(!YamlValidator::validateValue("path\\").ok);
}

void TestYamlValidator::value_balancedQuotes_ok()
{
    QVERIFY(YamlValidator::validateValue("\"quoted\"").ok);
    QVERIFY(YamlValidator::validateValue("[1, 2, 3]").ok);
}

void TestYamlValidator::key_plain_ok()
{
    QVERIFY(YamlValidator::validateKey("database").ok);
    QVERIFY(YamlValidator::validateKey("my_key-1").ok);
}

void TestYamlValidator::key_empty_fails()
{
    QVERIFY(!YamlValidator::validateKey("").ok);
}

void TestYamlValidator::key_whitespaceOnly_fails()
{
    QVERIFY(!YamlValidator::validateKey("   ").ok);
}

void TestYamlValidator::key_colon_fails()
{
    QVERIFY(!YamlValidator::validateKey("a: b").ok);
    QVERIFY(!YamlValidator::validateKey("key:").ok);
}

void TestYamlValidator::key_bracket_fails()
{
    QVERIFY(!YamlValidator::validateKey("arr[0").ok);
}

void TestYamlValidator::key_equalsSign_ok()
{
    QVERIFY(YamlValidator::validateKey("==").ok);
    QVERIFY(YamlValidator::validateKey("a=b").ok);
}

void TestYamlValidator::key_midSpecialChars_ok()
{
    QVERIFY(YamlValidator::validateKey("a?b").ok);
    QVERIFY(YamlValidator::validateKey("x>y").ok);
    QVERIFY(YamlValidator::validateKey("100%").ok);
}

void TestYamlValidator::key_leadingIndicator_fails()
{
    QVERIFY(!YamlValidator::validateKey("- item").ok);
    QVERIFY(!YamlValidator::validateKey("#comment").ok);
}

QTEST_APPLESS_MAIN(TestYamlValidator)

#include "tst_yamlvalidator.moc"
