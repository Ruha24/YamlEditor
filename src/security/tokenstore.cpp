#include "tokenstore.h"

#include <QDir>
#include <QFile>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

TokenStore::TokenStore(const QString &fileName)
    : file_name_(fileName)
{}

QString TokenStore::filePath() const
{
    return QDir(QDir::currentPath()).filePath(file_name_);
}

bool TokenStore::isEncrypted()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

#ifdef Q_OS_WIN

bool TokenStore::save(const QString &token)
{
    const QByteArray plain = token.toUtf8();

    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(plain.constData()));
    in.cbData = static_cast<DWORD>(plain.size());

    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"YamlEditor Yandex token", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &out))
        return false;

    QByteArray cipher(reinterpret_cast<char *>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);

    QFile file(filePath());
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const bool ok = file.write(cipher) == cipher.size();
    file.close();
    return ok;
}

QString TokenStore::load() const
{
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    const QByteArray cipher = file.readAll();
    file.close();
    if (cipher.isEmpty())
        return QString();

    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(cipher.constData()));
    in.cbData = static_cast<DWORD>(cipher.size());

    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &out))
        return QString();

    QByteArray plain(reinterpret_cast<char *>(out.pbData), static_cast<int>(out.cbData));
    LocalFree(out.pbData);
    return QString::fromUtf8(plain);
}

#else

bool TokenStore::save(const QString &token)
{
    QFile file(filePath());
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QByteArray data = token.toUtf8();
    const bool ok = file.write(data) == data.size();
    file.close();
    return ok;
}

QString TokenStore::load() const
{
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    const QString token = QString::fromUtf8(file.readAll());
    file.close();
    return token;
}

#endif

bool TokenStore::hasToken() const
{
    return !load().isEmpty();
}

void TokenStore::clear()
{
    QFile::remove(filePath());
}
