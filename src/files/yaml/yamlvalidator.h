#ifndef YAMLVALIDATOR_H
#define YAMLVALIDATOR_H

#include <QString>

class YamlValidator
{
public:
    struct Result
    {
        bool ok = true;
        QString message;

        static Result valid() { return {true, QString()}; }
        static Result invalid(const QString &msg) { return {false, msg}; }
    };

    static Result validateValue(const QString &value);

    static Result validateKey(const QString &key);

private:
    static bool hasBalancedPairs(const QString &text);
    static bool hasBalancedQuotes(const QString &text);
};

#endif // YAMLVALIDATOR_H
