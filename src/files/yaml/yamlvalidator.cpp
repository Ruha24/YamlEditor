#include "yamlvalidator.h"

#include <QObject>

bool YamlValidator::hasBalancedPairs(const QString &text)
{
    int curly = 0;
    int square = 0;
    for (const QChar &ch : text) {
        if (ch == '{')
            ++curly;
        else if (ch == '}')
            --curly;
        else if (ch == '[')
            ++square;
        else if (ch == ']')
            --square;
        if (curly < 0 || square < 0)
            return false;
    }
    return curly == 0 && square == 0;
}

bool YamlValidator::hasBalancedQuotes(const QString &text)
{
    int singles = 0;
    int doubles = 0;
    for (const QChar &ch : text) {
        if (ch == '\'')
            ++singles;
        else if (ch == '"')
            ++doubles;
    }
    return (singles % 2 == 0) && (doubles % 2 == 0);
}

YamlValidator::Result YamlValidator::validateValue(const QString &value)
{

    if (value.isEmpty())
        return Result::valid();

    if (!hasBalancedQuotes(value))
        return Result::invalid(QObject::tr("Unbalanced quotes"));

    if (!hasBalancedPairs(value))
        return Result::invalid(QObject::tr("Unbalanced brackets { } or [ ]"));

    if (value.endsWith('\\'))
        return Result::invalid(QObject::tr("Value ends with a backslash"));

    return Result::valid();
}

YamlValidator::Result YamlValidator::validateKey(const QString &key)
{
    if (key.trimmed().isEmpty())
        return Result::invalid(QObject::tr("Key cannot be empty"));

    if (!hasBalancedQuotes(key))
        return Result::invalid(QObject::tr("Unbalanced quotes"));

    if (!hasBalancedPairs(key))
        return Result::invalid(QObject::tr("Unbalanced brackets { } or [ ]"));

    const bool quoted = (key.startsWith('"') && key.endsWith('"'))
                        || (key.startsWith('\'') && key.endsWith('\''));
    if (quoted)
        return Result::valid();

    if (key.contains(": ") || key.endsWith(':'))
        return Result::invalid(QObject::tr("Key contains a colon"));

    static const QString kLeadingIndicators = "-?#&*!|>@`%";
    if (!key.isEmpty() && kLeadingIndicators.contains(key.at(0)))
        return Result::invalid(
            QObject::tr("Key starts with a reserved character: %1").arg(key.at(0)));

    return Result::valid();
}
