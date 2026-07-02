#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QCryptographicHash>
#include <QHash>
#include <QString>

class FileSystem
{
public:
    explicit FileSystem(const QString &path);

    bool CheckFolder(const QString &path) const;
    bool InstanceFolder(const QString &path) const;

    QString CalculateFileCheckSum(
        const QString &file_path,
        QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256) const;

    const QHash<QString, QString> &GetFiles() const;
    QString GetFilePath(const QString &fileName) const;
    bool Contains(const QString &fileName) const;
    void AddFile(const QString &path);

private:
    void GetFilesDirectory(const QString &path_dir);

    QHash<QString, QString> files;
};

#endif // FILESYSTEM_H
