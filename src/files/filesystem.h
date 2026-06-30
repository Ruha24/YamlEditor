#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QMap>
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

    QMap<QString, QString> GetFiles();
    QString GetFilePath(QString fileName);
    void AddFile(QString path);

private:
    void GetFilesDirectory(const QString &path_dir);

    QMap<QString, QString> files;
};

#endif // FILESYSTEM_H
