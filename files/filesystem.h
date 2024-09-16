#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QList>
#include <QMessageBox>
#include <QString>

class FileSystem
{
public:
    FileSystem(const QString &path);

    bool checkFolder();
    bool instanceFolder();

    QString calculateFileCheckSum(
        const QString &filePath,
        QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256);

    QList<QString> getFiles();

private:
    void getAllFiles();

    const QString &path;

    QList<QString> files;
};

#endif // FILESYSTEM_H
