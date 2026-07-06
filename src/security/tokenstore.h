#ifndef TOKENSTORE_H
#define TOKENSTORE_H

#include <QString>

class TokenStore
{
public:
    explicit TokenStore(const QString &fileName = QStringLiteral("yandex_token.dat"));

    bool save(const QString &token);
    QString load() const;
    bool hasToken() const;
    void clear();

    static bool isEncrypted();

private:
    QString filePath() const;

    QString file_name_;
};

#endif // TOKENSTORE_H
