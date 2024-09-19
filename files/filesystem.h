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

    bool CheckFolder(const QString &path);
    bool InstanceFolder(const QString &path);

    QString CalculateFileCheckSum(
        const QString &file_path,
        QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256);

    QList<QString> getFiles();

private:
    void GetFilesDirectory(const QString &path_dir);

    QList<QString> list_files_info;
};

#endif // FILESYSTEM_H
